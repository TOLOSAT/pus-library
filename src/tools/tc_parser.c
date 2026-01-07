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
    uint8_t data[RX_BUFFER_SIZE];
    size_t write_index; // updated by DMA
    size_t read_index;  // updated by parser
} rxBuffer_t;

typedef enum
{
    TC_STATE_VALID,
    TC_STATE_PARTIAL,
    TC_STATE_INVALID
} tcState_t;

typedef struct
{
    size_t start;  // start index of current TC
    size_t length; // total declared TC length (from header)
    tcState_t state;
} tcFrame_t;

/*************************** Functions Declarations **************************/

static returnCode_t FindTCHeader(const rxBuffer_t *rx, size_t *header_offset);
static returnCode_t ExtractTCLength(rxBuffer_t *rx, size_t start, uint16_t *length);
static returnCode_t HasFullTC(rxBuffer_t *rx, size_t start, size_t length);
static returnCode_t VerifyCRC(rxBuffer_t *rx, size_t start, size_t length);
static returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state);
static returnCode_t ParseBuffer(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state);
static returnCode_t CheckTCPacketIdValidity(sppPacketId_t tc_packet_id, pusAcceptanceError_t *error);
static returnCode_t CheckTCValidity(pusTC_t *tc, pusAcceptanceError_t *error);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

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

    // Check Packet Version Number
    if (((packet_id & PACKET_VERSION_NUMBER_MASK) >> PACKET_VERSION_NUMBER_OFFSET) == PACKET_VERSION_NUMBER)
    {
        // Check Packet Type
        if (((packet_id & PACKET_TYPE_MASK) >> PACKET_TYPE_OFFSET) == TC_TYPE)
        {
            // Check Secondary Header Presence
            if (((packet_id & HEADER_PRESENCE_MASK) >> HEADER_PRESENCE_OFFSET) != HEADER_PRESENT)
            {
                return_value = RET_INVALID_PARAM;
                *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
            }

            else
            {
                return_value = RET_INVALID_PARAM;
                *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
            *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
        *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
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
    uint16_t packet_id        = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
    uint16_t data_size        = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length) + 1u;
    uint8_t pus_version       = tc->tc_header.version_flags;

    return_value = CheckTCPacketIdValidity(tc->spp_header.packet_id, error);

    if (return_value == RET_SUCCESSFUL)
    {
        // Check Size
        if (data_size >= (TC_HEADER_SIZE + CRC_TRAILER_SIZE))
        {
            // Check PUS version number
            if (((pus_version & PUS_VERSION_NUMBER_MASK) >> PUS_VERSION_NUMBER_OFFSET) == PUS_VERSION_NUMBER)
            {
                // Check CRC
                if (CheckCRC(tc) != RET_SUCCESSFUL)
                {
                    return_value = RET_INVALID_PARAM;
                    *error       = PUS_ACCEPTANCE_INVALID_CRC;
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
                *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
            *error       = PUS_ACCEPTANCE_INVALID_FORMAT;
        }
    }
    else
    {
        *error = PUS_ACCEPTANCE_INVALID_FORMAT;
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

    while (i != rx->write_index && header_offset_tmp == (size_t)-1)
    {
        sppPacketId_t packet_id = HALF_WORD_BYTE_SWAP(((uint16_t)rx->data[i] << 8) | rx->data[(i + 1) % RX_BUFFER_SIZE]);
        if (CheckTCPacketIdValidity(packet_id, NULL) == RET_SUCCESSFUL)
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
 * @param start start index of the TC
 * @param length total declared TC length
 * @return returnCode_t
 */
static returnCode_t VerifyCRC(rxBuffer_t *rx, size_t start, size_t length)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    uint16_t crc_calc = computeCRC(&rx->data[start], length - CRC_TRAILER_SIZE);
    uint16_t crc_recv =
        ((uint16_t)rx->data[(start + length - CRC_TRAILER_SIZE) % RX_BUFFER_SIZE] << 8) | rx->data[(start + length - 1) % RX_BUFFER_SIZE];

    if (crc_calc != crc_recv)
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
static returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state)
{
    tc->start = rx->read_index;
    ExtractTCLength(rx, tc->start, &tc->length);

    returnCode_t has_full_tc = HasFullTC(rx, tc->start, tc->length);

    // TODO: not sure about the pusTc_t cast here
    if (has_full_tc == RET_SUCCESSFUL && CheckTCValidity((pusTC_t *)&rx->data[tc->start], PUS_ACCEPTANCE_NO_ERROR) == RET_SUCCESSFUL)
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

    return RET_SUCCESSFUL;
}

/**
 * @fn          ParseBuffer(rxBuffer_t *rx_buffer)
 * @brief       Function that parse the RX buffer to extract TCs
 * @param[in]   rx_buffer Pointer to the RX buffer structure
 * @param[out]  tc Pointer to the TC frame structure to fill
 * @param[out]  state Pointer to store the TC state
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if parsing failed
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ParseBuffer(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state)
{
    size_t available          = (rx->write_index - rx->read_index + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check if there's enough data to read even the minimum TC header
    if (available >= SPP_HEADER_SIZE)
    {
        int header_offset = FindTCHeader(rx);
        // Header found
        if (header_offset >= 0)
        {
            rx->read_index = (rx->read_index + header_offset) % RX_BUFFER_SIZE;

            // Try to parse a TC from here
            ParseOneTC(rx, tc, state);

            if (*state == TC_STATE_PARTIAL)
            {
                // Not enough data yet to parse the full TC
                return_value = RET_NOT_AVAILABLE;
            }
            else if (*state == TC_STATE_INVALID)
            {
                // Invalid TC, skip the first byte
                rx->read_index = (rx->read_index + 1) % RX_BUFFER_SIZE;
                return_value   = RET_ERROR;
            }

            // Advance read pointer appropriately
            rx->read_index = (rx->read_index + tc->length) % RX_BUFFER_SIZE;
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_ERROR;
    }

    return return_value;
}
