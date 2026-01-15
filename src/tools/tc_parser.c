/**
 * @file    tc_parser.c
 * @author  Matteo Planchet
 * @brief   Source file for TC parsing
 *
 * @copyright Copyright (c) TOLOSAT 2025
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tc_parser.h"
#include "tools/tables_management.h"
#include "tools/schedule_management.h"
#include "tools/crc_computation.h"
#include "services/pus1.h"
#include "pus.h"

/***************************** Macros Definitions ****************************/

// WIP WIP WIP WIP WIP WIP WIP WIP WIPOWI PIPWIWPWIPIWIWPIWPW OIOWIWOPIOOWIOOWO

#define RX_BUFFER_SIZE 2048 /**< Size of the RX buffer in bytes */

typedef struct
{
    uint8_t data[RX_BUFFER_SIZE]; /**< @brief Data buffer */
    size_t write_index;           /**< @brief Write index, updated by DMA */
    size_t read_index;            /**< @brief Read index, updated by parser */
} rxBuffer_t;

typedef enum
{
    TC_STATE_VALID   = 0u, /**< TC is valid */
    TC_STATE_PARTIAL = 1u, /**< TC is partial */
    TC_STATE_INVALID = 2u, /**< TC is invalid */
} tcState_t;

typedef struct
{
    size_t start;    /**< @brief Start index of current TC */
    size_t length;   /**< @brief Total declared TC length (from header) */
    tcState_t state; /**< @brief State of the current TC being parsed */
} tcFrame_t;

/*************************** Functions Declarations **************************/

static returnCode_t CheckCRC(pusTC_t *tc);
static returnCode_t FindTCHeader(const rxBuffer_t *rx, size_t *header_offset);
static returnCode_t ExtractTCLength(rxBuffer_t *rx, size_t start, uint16_t *length);
static returnCode_t HasFullTC(rxBuffer_t *rx, size_t start, size_t length);
static returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state, pusAcceptanceError_t *error);
static returnCode_t ParseBuffer(rxBuffer_t *rx, tcFrame_t *tc, pusAcceptanceError_t *error);
static returnCode_t CheckTCPacketIdValidity(sppPacketId_t tc_packet_id, pusAcceptanceError_t *error);
static returnCode_t CheckTCValidity(pusTC_t *tc, pusAcceptanceError_t *error);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          CheckCRC(pusTC_t *tc)
 * @brief       Function that verifies a received TC has not been corrupted
 * @param[in]   tc TC from which the CRC will be checked
 * @retval      #RET_ERROR if the computed CRC is different than the received CRC
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t CheckCRC(pusTC_t *tc)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    uint16_t data_size        = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length) + 1u;
    pusCRC_t reiceved_crc     = (pusCRC_t)(tc->data[data_size - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 0u] << 8u)
                            + (pusCRC_t)(tc->data[data_size - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 1u]);
    pusCRC_t computed_crc = 0u;

    // Compute the TC's CRC
    computed_crc = computeCRC((uint8_t *)tc, data_size + SPP_HEADER_SIZE - CRC_TRAILER_SIZE);
    if (computed_crc != reiceved_crc)
    {
        return_value = RET_ERROR;
    }

    return return_value;
}

/**
 * @fn          CheckTCPacketIdValidity(sppPacketId_t tc_packet_id, pusAcceptanceError_t *error)
 * @brief       Function that verifies if TC Packet ID is valid (right version, type, secondary header presence)
 * @param[in]   tc_packet_id TC Packet ID to check validity
 * @param[out]  error Pointer to pass error type to TM(1,2)
 * @retval      #RET_INVALID_PARAM if the TC Packet ID is not well formated
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t CheckTCPacketIdValidity(sppPacketId_t tc_packet_id, pusAcceptanceError_t *error)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    uint16_t packet_id        = HALF_WORD_BYTE_SWAP(tc_packet_id);

    if (error != NULL)
    {
        // Check Packet Version Number
        if (((packet_id & PACKET_VERSION_NUMBER_MASK) >> PACKET_VERSION_NUMBER_OFFSET) == PACKET_VERSION_NUMBER)
        {
            // Check Packet Type
            if (((packet_id & PACKET_TYPE_MASK) >> PACKET_TYPE_OFFSET) == TC_TYPE)
            {
                // Check Secondary Header Presence
                if (((packet_id & HEADER_PRESENCE_MASK) >> HEADER_PRESENCE_OFFSET) == HEADER_PRESENT)
                {
                    // All checks passed
                    return_value = RET_SUCCESSFUL;
                    *error       = PUS_ACCEPTANCE_NO_ERROR;
                }
                else
                {
                    // Secondary Header not present
                    return_value = RET_ERROR;
                    *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
                }
            }
            else
            {
                // Wrong Packet Type
                return_value = RET_ERROR;
                *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
            }
        }
        else
        {
            // Wrong Packet Version
            return_value = RET_ERROR;
            *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          CheckTCValidity(pusTC_t *tc, pusAcceptanceError_t *error)
 * @brief       Function that verifies if TC is valid (right version, type, size)
 * @param[in]   tc TC to check validity
 * @param[out]  error Pointer to pass error type to TM(1,2)
 * @retval      #RET_INVALID_PARAM if the TC is not well formated or CRC is invalid
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t CheckTCValidity(pusTC_t *tc, pusAcceptanceError_t *error)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    if (tc != NULL && error != NULL)
    {
        uint16_t packet_id  = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        uint16_t data_size  = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length) + 1u;
        uint8_t pus_version = tc->tc_header.version_flags;

        return_value = CheckTCPacketIdValidity(tc->spp_header.packet_id, error);

        if (return_value == RET_SUCCESSFUL)
        {
            // Check Size
            if (data_size > (SPP_HEADER_SIZE + TC_HEADER_SIZE + CRC_TRAILER_SIZE) && data_size <= TC_MAX_SIZE)
            {
                // Check PUS version number
                if (((pus_version & PUS_VERSION_NUMBER_MASK) >> PUS_VERSION_NUMBER_OFFSET) == PUS_VERSION_NUMBER)
                {
                    // Check CRC
                    if (CheckCRC(tc) == RET_SUCCESSFUL)
                    {
                        // All checks passed
                        return_value = RET_SUCCESSFUL;
                        *error       = PUS_ACCEPTANCE_NO_ERROR;
                    }
                    else
                    {
                        // CRC invalid
                        return_value = RET_ERROR;
                        *error       = PUS_ACCEPTANCE_INVALID_CRC;
                    }
                }
                else
                {
                    // Wrong PUS version
                    return_value = RET_ERROR;
                    *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
                }
            }
            else
            {
                // Size too small or too large
                return_value = RET_ERROR;
                *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
            }
        }
        else
        {
            // Packet ID invalid
            return_value = RET_ERROR;
            // error already set in CheckTCPacketIdValidity
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @brief Finds the offset of a TC header in the RX buffer
 *
 * @param rx the RX buffer
 * @param header_offset the offset of the found header, starting from read_index, -1 if not found
 * @return returnCode_t
 */
static returnCode_t FindTCHeader(const rxBuffer_t *rx, size_t *header_offset)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    size_t i                  = rx->read_index;
    size_t header_offset_tmp  = (size_t)-1;

    // Try to find an header until we reach the write index
    while (i != rx->write_index && header_offset_tmp == (size_t)-1)
    {
        sppPacketId_t packet_id = HALF_WORD_BYTE_SWAP(((uint16_t)rx->data[i] << 8) | rx->data[(i + 1) % RX_BUFFER_SIZE]);

        pusAcceptanceError_t error; // Dummy variable, we don't need the error here
        if (CheckTCPacketIdValidity(packet_id, &error) == RET_SUCCESSFUL)
        {
            header_offset_tmp = (i - rx->read_index + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
        }
        i = (i + 1) % RX_BUFFER_SIZE;
    }

    *header_offset = header_offset_tmp;

    if (header_offset_tmp == (size_t)-1)
    {
        return_value = RET_ERROR;
    }

    return return_value;
}

/**
 * @brief Extracts the total length of a TC from the RX buffer
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length pointer to store the extracted length
 * @return returnCode_t
 */
static returnCode_t ExtractTCLength(rxBuffer_t *rx, size_t start, uint16_t *length)
{
    uint8_t byte4 = rx->data[(start + 4) % RX_BUFFER_SIZE];
    uint8_t byte5 = rx->data[(start + 5) % RX_BUFFER_SIZE];

    uint16_t length_field = HALF_WORD_BYTE_SWAP(((uint16_t)byte4 << 8) | byte5);
    *length               = length_field + SPP_HEADER_SIZE; // header not included in length field
    return RET_SUCCESSFUL;
}

/**
 * @brief Checks if the RX buffer contains a full TC frame starting at a given index
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length total declared TC length
 * @return returnCode_t
 */
static returnCode_t HasFullTC(rxBuffer_t *rx, size_t start, size_t length)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    size_t available = (rx->write_index - start + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;

    if (available < length)
    {
        return_value = RET_ERROR;
    }
    return return_value;
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param tc the TC frame structure to fill
 * @param state pointer to store the TC state
 * @return returnCode_t
 */
static returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state, pusAcceptanceError_t *error)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    if (rx != NULL && tc != NULL && state != NULL && error != NULL)
    {
        tc->start = rx->read_index;
        ExtractTCLength(rx, tc->start, &tc->length);

        returnCode_t has_full_tc = HasFullTC(rx, tc->start, tc->length);

        // Linearize the circular buffer
        uint8_t tmp[MAX_TC_SIZE];
        for (i = 0; i < tc->length; i++)
        {
            tmp[i] = rx->data[(tc->start + i) % RX_BUFFER_SIZE];
        }

        returnCode_t is_tc_valid = CheckTCValidity((pusTC_t *)tmp, error);

        if (has_full_tc == RET_SUCCESSFUL && is_tc_valid == RET_SUCCESSFUL)
        {
            *state = TC_STATE_VALID;
        }
        else if (has_full_tc == RET_ERROR)
        {
            *state = TC_STATE_PARTIAL;
        }
        else
        {
            *state = TC_STATE_INVALID;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          ParseBuffer(rxBuffer_t *rx_buffer)
 * @brief       Function that parse the RX buffer to extract TCs
 * @param[in]   rx_buffer Pointer to the RX buffer structure
 * @param[out]  tc Pointer to the TC frame structure to fill
 * @param[out]  error Pointer to store the TC acceptance error code
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if parsing failed
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ParseBuffer(rxBuffer_t *rx, tcFrame_t *tc, pusAcceptanceError_t *error)
{
    returnCode_t ret = RET_SUCCESSFUL;
    *error           = PUS_ACCEPTANCE_NO_ERROR;

    if ((rx != NULL) && (tc != NULL) && (error != NULL))
    {
        size_t header_offset = 0u;
        ret                  = FindTCHeader(rx, &header_offset);

        if (ret == RET_SUCCESSFUL)
        {
            // Skip header
            rx->read_index = (rx->read_index + header_offset) % RX_BUFFER_SIZE;

            // Try to parse a TC from here
            tcState_t state = TC_STATE_INVALID;
            ret             = ParseOneTC(rx, tc, &state, error);

            if (ret == RET_SUCCESSFUL)
            {
                if ((state == TC_STATE_VALID) || (state == TC_STATE_INVALID))
                {
                    // Advance read pointer if TC is full (valid or invalid)
                    rx->read_index = (rx->read_index + tc->length) % RX_BUFFER_SIZE;
                }
                else
                {
                    // Partial TC, header OK but not complete
                    *error = PUS_ACCEPTANCE_INVALID_FORMAT;
                    ret    = RET_NOT_AVAILABLE;
                }
            }
            else
            {
                // The parsing is not successful
                // error is already set in ParseOneTC
            }
        }
        else
        {
            // No header found, so no TC at all, advance to the write index
            // because there is nothing usable between the read index and the write index.
            rx->read_index = rx->write_index;
            *error         = PUS_ACCEPTANCE_CANT_FORMAT;
        }
    }
    else
    {
        ret = RET_INVALID_PARAM;
    }

    return ret;
}
