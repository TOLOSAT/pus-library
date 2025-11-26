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

#define RX_BUFFER_SIZE     2048 /**< Size of the RX buffer in bytes */
#define MIN_TC_HEADER_SIZE 6u   /**< Minimum size of a TC header in bytes, which is the size of a CCSDS Space Packet's primary header */

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

returnCode_t FindTCHeader(const rxBuffer_t *rx, size_t *header_offset);
returnCode_t ExtractTCLength(rxBuffer_t *rx, size_t start, uint16_t *length);
returnCode_t HasFullTC(rxBuffer_t *rx, size_t start, size_t length, bool *has_full);
returnCode_t VerifyCRC(rxBuffer_t *rx, size_t start, size_t length, bool *is_valid);
returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state);
returnCode_t ParseBuffer(rxBuffer_t *rx_buffer);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @brief Finds the offset of a TC header in the RX buffer
 *
 * @param rx the RX buffer
 * @param header_offset the offset of the found header, starting from read_index, -1 if not found
 * @return returnCode_t
 */
returnCode_t FindTCHeader(const rxBuffer_t *rx, size_t *header_offset)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    size_t i                  = rx->read_index;
    size_t header_offset_tmp  = (size_t)-1;

    while (i != rx->write_index && header_offset_tmp == (size_t)-1)
    {
        uint16_t packet_id = ((uint16_t)rx->data[i] << 8) | rx->data[(i + 1) % RX_BUFFER_SIZE];
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
returnCode_t ExtractTCLength(rxBuffer_t *rx, size_t start, uint16_t *length)
{
    uint8_t byte4 = rx->data[(start + 4) % RX_BUFFER_SIZE];
    uint8_t byte5 = rx->data[(start + 5) % RX_BUFFER_SIZE];

    uint16_t length_field = HALF_WORD_BYTE_SWAP(((uint16_t)byte4 << 8) | byte5);
    *length               = length_field + MIN_TC_HEADER_SIZE; // header not included in length field
    return RET_SUCCESSFUL;
}

/**
 * @brief Checks if the RX buffer contains a full TC frame starting at a given index
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length total declared TC length
 * @param has_full pointer to store if full TC is available
 * @return returnCode_t
 */
returnCode_t HasFullTC(rxBuffer_t *rx, size_t start, size_t length, bool *has_full)
{
    size_t available = (rx->write_index - start + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
    *has_full        = available >= length;
    return RET_SUCCESSFUL;
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length total declared TC length
 * @param is_valid pointer to store if CRC is valid
 * @return returnCode_t
 */
returnCode_t VerifyCRC(rxBuffer_t *rx, size_t start, size_t length, bool *is_valid)
{
    uint16_t crc_calc = computeCRC(&rx->data[start], length - CRC_TRAILER_SIZE);
    uint16_t crc_recv =
        ((uint16_t)rx->data[(start + length - CRC_TRAILER_SIZE) % RX_BUFFER_SIZE] << 8) | rx->data[(start + length - 1) % RX_BUFFER_SIZE];
    *is_valid = (crc_calc == crc_recv);
    return RET_SUCCESSFUL;
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param tc the TC frame structure to fill
 * @param state pointer to store the TC state
 * @return returnCode_t
 */
returnCode_t ParseOneTC(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state)
{
    tc->start = rx->read_index;
    ExtractTCLength(rx, tc->start, &tc->length);

    bool has_full_tc;
    HasFullTC(rx, tc->start, tc->length, &has_full_tc);

    bool is_crc_valid;
    VerifyCRC(rx, tc->start, tc->length, &is_crc_valid);

    if (has_full_tc && is_crc_valid)
    {
        *state = TC_STATE_VALID;
    }
    else if (!has_full_tc)
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
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if parsing failed
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ParseBuffer(rxBuffer_t *rx)
{
    while (1)
    {
        size_t available = (rx->write_index - rx->read_index + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;

        // Not enough data to read even the minimum TC header, exit the loop
        if (available < MIN_TC_HEADER_SIZE)
        {
            break;
        }

        int header_offset = FindTCHeader(rx);
        // No header found, exit the loop
        if (header_offset < 0)
        {
            break;
        }

        rx->read_index = (rx->read_index + header_offset) % RX_BUFFER_SIZE;

        // Try to parse a TC from here
        tcFrame_t tc;
        tcState_t state = ParseOneTC(rx, &tc);

        if (state == TC_STATE_PARTIAL)
        {
            // Not enough data yet to parse the full TC, exit the loop
            break;
        }
        else if (state == TC_STATE_INVALID)
        {
            // Invalid TC, skip the first byte and try again
            rx->read_index = (rx->read_index + 1) % RX_BUFFER_SIZE;
            continue;
        }

        // 4. Advance read pointer appropriately
        rx->read_index = (rx->read_index + tc.length) % RX_BUFFER_SIZE;
    }
}
