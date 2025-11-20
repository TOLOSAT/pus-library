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

#define RX_BUFFER_SIZE            2048 /**< Size of the RX buffer in bytes */
#define CCSDS_PRIMARY_HEADER_SIZE 6u   /**< Size of the CCSDS primary header in bytes */
#define MIN_TC_HEADER_SIZE        6u   /**< Minimum size of a TC header in bytes, which is the size of a CCSDS Space Packet's primary header */

typedef struct
{
    uint8_t data[RX_BUFFER_SIZE];
    size_t write_index; // updated by DMA
    size_t read_index;  // updated by parser
} RxBuffer;

typedef enum
{
    TC_STATE_VALID,
    TC_STATE_PARTIAL,
    TC_STATE_INVALID
} TcState;

typedef struct
{
    size_t start;  // start index of current TC
    size_t length; // total declared TC length (from header)
    TcState state;
} TcFrame;

/*************************** Functions Declarations **************************/

int FindTCHeader(const RxBuffer *rx);
uint16_t ExtractTCLength(RxBuffer *rx, size_t start);
bool HasFullTC(RxBuffer *rx, size_t start, size_t length);
bool VerifyCRC(RxBuffer *rx, size_t start, size_t length);
TcState ParseOneTC(RxBuffer *rx, TcFrame *tc);
returnCode_t ParseBuffer(RxBuffer *rx_buffer);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

int FindTCHeader(const RxBuffer *rx)
{
    size_t i = rx->read_index;

    while (i != rx->write_index)
    {
        uint16_t packet_id = ((uint16_t)rx->data[i] << 8) | rx->data[(i + 1) % RX_BUFFER_SIZE];
        if (CheckTCPacketIdValidity(packet_id, NULL) == RET_SUCCESSFUL)
        {
            return (i - rx->read_index + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
        }
        i = (i + 1) % RX_BUFFER_SIZE;
    }

    return -1; // no header found
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @return uint16_t
 */
uint16_t ExtractTCLength(RxBuffer *rx, size_t start)
{
    uint8_t b4 = rx->data[(start + 4) % RX_BUFFER_SIZE];
    uint8_t b5 = rx->data[(start + 5) % RX_BUFFER_SIZE];

    uint16_t length_field = ((uint16_t)b4 << 8) | b5;
    return length_field + CCSDS_PRIMARY_HEADER_SIZE; // header not included in length field
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length total declared TC length
 * @return if full TC is available in buffer
 */
bool HasFullTC(RxBuffer *rx, size_t start, size_t length)
{
    size_t available = (rx->write_index - start + RX_BUFFER_SIZE) % RX_BUFFER_SIZE;
    return available >= length;
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param start start index of the TC
 * @param length total declared TC length
 * @return if CRC is valid
 */
bool VerifyCRC(RxBuffer *rx, size_t start, size_t length)
{
    uint16_t crc_calc = computeCRC(&rx->data[start], length - 2);
    uint16_t crc_recv = ((uint16_t)rx->data[(start + length - 2) % RX_BUFFER_SIZE] << 8) | rx->data[(start + length - 1) % RX_BUFFER_SIZE];
    return crc_calc == crc_recv;
}

/**
 * @brief
 *
 * @param rx the RX buffer
 * @param tc the TC frame structure to fill
 * @return TcState
 */
TcState ParseOneTC(RxBuffer *rx, TcFrame *tc)
{
    tc->start  = rx->read_index;
    tc->length = ExtractTCLength(rx, tc->start);

    if (!HasFullTC(rx, tc->start, tc->length))
    {
        return TC_STATE_PARTIAL;
    }

    if (VerifyCRC(rx, tc->start, tc->length))
    {
        return TC_STATE_VALID;
    }

    return TC_STATE_INVALID;
}

/**
 * @fn          ParseBuffer(RxBuffer *rx_buffer)
 * @brief       Function that parse the RX buffer to extract TCs
 * @param[in]   rx_buffer Pointer to the RX buffer structure
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if parsing failed
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ParseBuffer(RxBuffer *rx)
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
        TcFrame tc;
        TcState state = ParseOneTC(rx, &tc);

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