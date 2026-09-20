/**
 * @file    pus1.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 1 functions (Request verification)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tm_management.h"
#include "services/pus1.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc BuildS1SS1
 */
returnCode_t BuildS1SS1(const pusTC_t *tc, pusTM_t *acceptance_tm)
{
    returnCode_t return_value       = RET_SUCCESSFUL;
    pusData_t data[S1SS1_DATA_SIZE] = { 0 };
    sppHeader_t spp_header_buffer   = { 0 };

    // Check parameter(s)
    if ((tc != NULL) && (acceptance_tm != NULL))
    {
        // Set up headers
        spp_header_buffer.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        spp_header_buffer.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);

        // Set up data
        (void)memcpy((void *)&data, (void *)&spp_header_buffer, S1SS1_DATA_SIZE);

        // Build TM
        return_value = BuildTM(acceptance_tm, 1u, 1u, (pusData_t *)&data, S1SS1_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc BuildS1SS2
 */
returnCode_t BuildS1SS2(const pusTC_t *tc, pusTM_t *acceptance_tm, pusAcceptanceError_t acceptance_error)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    pusData_t data[S1SS2_DATA_SIZE];
    sppHeader_t spp_header_buffer = { 0 };

    // Check parameter(s)
    if ((tc != NULL) && (acceptance_tm != NULL) && (acceptance_error != 0u))
    {
        // Set up headers
        spp_header_buffer.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        spp_header_buffer.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);

        // Set up data
        (void)memcpy((void *)&data, (void *)&spp_header_buffer, S1SS2_DATA_SIZE - 1u);
        data[S1SS2_DATA_SIZE - 1u] = acceptance_error;

        // Build TM
        return_value = BuildTM(acceptance_tm, 1u, 2u, (pusData_t *)&data, S1SS2_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc BuildS1SS7
 */
returnCode_t BuildS1SS7(const pusTC_t *tc, pusTM_t *execution_tm)
{
    returnCode_t return_value       = RET_SUCCESSFUL;
    pusData_t data[S1SS7_DATA_SIZE] = { 0 };
    sppHeader_t spp_header_buffer   = { 0 };

    // Check parameter(s)
    if ((tc != NULL) && (execution_tm != NULL))
    {
        // Set up headers
        spp_header_buffer.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        spp_header_buffer.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);

        // Set up data
        (void)memcpy((void *)&data, (void *)&spp_header_buffer, S1SS7_DATA_SIZE);

        // Build TM
        return_value = BuildTM(execution_tm, 1u, 7u, (pusData_t *)&data, S1SS7_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc BuildS1SS8
 */
returnCode_t BuildS1SS8(const pusTC_t *tc, pusTM_t *execution_tm, pusExecutionError_t execution_error)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    pusData_t data[S1SS8_DATA_SIZE];
    sppHeader_t spp_header_buffer = { 0 };

    // Check parameter(s)
    if ((tc != NULL) && (execution_tm != NULL) && (execution_error != 0u))
    {
        // Set up headers
        spp_header_buffer.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        spp_header_buffer.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);

        // Set up data
        (void)memcpy((void *)&data, (void *)&spp_header_buffer, S1SS8_DATA_SIZE - 1u);
        data[S1SS8_DATA_SIZE - 1u] = execution_error;

        // Build TM
        return_value = BuildTM(execution_tm, 1u, 8u, (pusData_t *)&data, S1SS8_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
