/**
 * @file    pus9.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 9 functions (Time management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include "kernel.h"
#include "tm_management.h"
#include "services/pus9.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc ExecuteS9SS128
 */
returnCode_t ExecuteS9SS128(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(env);
    (void)(tm);

    // Check parameter(s)
    if ((tc != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        if ((tc->spp_header.packet_data_length + 1u) == (TC_HEADER_SIZE + CUC_TIME_SIZE + CRC_TRAILER_SIZE))
        {
            // Get upcoming time value from data field
            time_t upcoming_time = BIG_ENDIAN_ARRAY_TO_UINT64(tc->data);
            // Update upcoming_time value with data field
            returnCode_t set_time_status = SetTime(upcoming_time);
            if (set_time_status != RET_SUCCESSFUL)
            {
                if (set_time_status == RET_INVALID_PARAM)
                {
                    // Time is invalid (probably year < 2000)
                    return_value = RET_INVALID_PARAM;
                    *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                }
                else
                {
                    return_value = RET_ERROR;
                    *error_code  = PUS_EXECUTION_FAILED;
                }
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
            *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
