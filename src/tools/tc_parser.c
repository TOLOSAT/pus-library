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
#include "tools/tc_parser.h"
#include "tools/tables_management.h"
#include "tools/schedule_management.h"
#include "tools/crc_computation.h"
#include "services/pus1.h"
#include "pus.h"

/***************************** Macros Definitions ****************************/

#define EXTRACT_TC_LENGTH(buffer, start, buffer_size)                                                                   \
    ((((uint16_t)((buffer)[((start) + sizeof(sppPacketId_t) + sizeof(sppPacketSequenceCtrl_t)) % (buffer_size)] << 8))  \
      | (uint16_t)((buffer)[((start) + sizeof(sppPacketId_t) + sizeof(sppPacketSequenceCtrl_t) + 1u) % (buffer_size)])) \
     + SPP_HEADER_SIZE + 1) /**<                                                                                        \
Macro to extract TC data length from the RX buffer, given the start index of the TC */

/*************************** Functions Declarations **************************/

static returnCode_t CheckCRC(pusTC_t *tc);
static returnCode_t FindTCHeader(pusParsingContext_t *ctx, length_t *header_offset);
static returnCode_t HasFullTC(pusParsingContext_t *ctx, length_t start, length_t length);
static returnCode_t ParseOneTC(pusParsingContext_t *ctx, pusTC_t *tc, tcState_t *state, pusAcceptanceError_t *error);
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

    // Compute the TC's CRC
    pusCRC_t computed_crc = computeCRC((uint8_t *)tc, data_size + SPP_HEADER_SIZE - CRC_TRAILER_SIZE);
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
        uint16_t data_size  = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length) + 1u;
        uint8_t pus_version = tc->tc_header.version_flags;

        // Not needed to check the type and secondary header presence again, done in CheckTCPacketIdValidity before
        // Even if the header is invalid, we still check size and CRC

        if (return_value == RET_SUCCESSFUL)
        {
            // Check Size
            if (data_size > 0 && data_size <= TC_MAX_SIZE)
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
static returnCode_t FindTCHeader(pusParsingContext_t *ctx, length_t *header_offset)
{
    returnCode_t return_value  = RET_SUCCESSFUL;
    length_t i                 = ctx->read_index;
    length_t header_offset_tmp = (length_t)-1;

    // Try to find an header until we reach the write index
    while (i != ctx->write_index && header_offset_tmp == (length_t)-1)
    {
        sppPacketId_t packet_id = HALF_WORD_BYTE_SWAP(((uint16_t)ctx->p_buffer[i] << 8) | ctx->p_buffer[(i + 1) % ctx->buffer_size]);

        pusAcceptanceError_t error; // Dummy variable, we don't need the error here
        if (CheckTCPacketIdValidity(packet_id, &error) == RET_SUCCESSFUL)
        {
            header_offset_tmp = (i - ctx->read_index + ctx->buffer_size) % ctx->buffer_size;
        }
        i = (i + 1) % ctx->buffer_size;
    }

    *header_offset = header_offset_tmp;

    if (header_offset_tmp == (length_t)-1)
    {
        return_value = RET_ERROR;
    }

    return return_value;
}

/**
 * @brief Checks if the RX buffer contains a full TC frame starting at a given index
 *
 * @param[in] ctx the parsing context
 * @param[in] start start index of the TC
 * @param[in] length total declared TC length
 * @return returnCode_t
 */
static returnCode_t HasFullTC(pusParsingContext_t *ctx, length_t start, length_t length)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    length_t available = (ctx->write_index - start + ctx->buffer_size) % ctx->buffer_size;
    if (available < length)
    {
        return_value = RET_ERROR;
    }
    return return_value;
}

/**
 * @brief
 *
 * @param[in] ctx the parsing context
 * @param[out] tc the TC frame structure to fill
 * @param[out] state pointer to store the TC state
 * @return returnCode_t
 */
static returnCode_t ParseOneTC(pusParsingContext_t *ctx, pusTC_t *tc, tcState_t *state, pusAcceptanceError_t *error)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    if (ctx != NULL && tc != NULL && state != NULL && error != NULL)
    {
        length_t tc_start_index = ctx->read_index;
        length_t tc_length      = EXTRACT_TC_LENGTH(ctx->p_buffer, tc_start_index, ctx->buffer_size);

        returnCode_t has_full_tc = HasFullTC(ctx, tc_start_index, tc_length);

        // Linearize the circular buffer
        uint8_t tmp[TC_MAX_SIZE] = { 0 };
        for (length_t i = 0; i < tc_length; i++)
        {
            tmp[i] = ctx->p_buffer[(tc_start_index + i) % ctx->buffer_size];
        }

        // Check TC validity (header, size, CRC)
        returnCode_t is_tc_valid = CheckTCValidity((pusTC_t *)&tmp, error);

        if (has_full_tc == RET_SUCCESSFUL && is_tc_valid == RET_SUCCESSFUL)
        {
            // TC is valid and full, copy it to output variable
            memcpy(tc, tmp, tc_length);
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
 * @fn          ParseBuffer(pusParsingContext_t *ctx, pusTC_t *tc, pusAcceptanceError_t *error)
 * @brief       Function that parse the RX buffer to extract TCs
 * @param[in]   ctx Parsing context containing the RX buffer
 * @param[out]  tc Pointer to the TC frame structure to fill
 * @param[out]  error Pointer to store the TC acceptance error code
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if parsing failed
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ParseBuffer(pusParsingContext_t *ctx, pusTC_t *tc, pusAcceptanceError_t *error)
{
    returnCode_t ret = RET_SUCCESSFUL;
    *error           = PUS_ACCEPTANCE_NO_ERROR;

    if ((ctx != NULL) && (tc != NULL) && (error != NULL))
    {
        length_t header_offset = 0u;
        ret                    = FindTCHeader(ctx, &header_offset);

        if (ret == RET_SUCCESSFUL)
        {
            // Skip to header
            ctx->read_index = (ctx->read_index + header_offset) % ctx->buffer_size;

            // Try to parse a TC from here
            tcState_t state = TC_STATE_INVALID;
            ret             = ParseOneTC(ctx, tc, &state, error);

            if (ret == RET_SUCCESSFUL)
            {
                if ((state == TC_STATE_VALID) || (state == TC_STATE_INVALID))
                {
                    // Advance read pointer if TC is full (valid or invalid)
                    ctx->read_index =
                        (ctx->read_index + SPP_HEADER_SIZE + HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length) + 1) % ctx->buffer_size;
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
            ctx->read_index = ctx->write_index;
            *error          = PUS_ACCEPTANCE_CANT_FORMAT;
        }
    }
    else
    {
        ret = RET_INVALID_PARAM;
    }

    return ret;
}
