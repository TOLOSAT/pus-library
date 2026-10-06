/**
 * @file    pus15.c
 * @author  Matteo Planchet, Arthur Morain
 * @brief   Source file for PUS 15 functions (On-board storage and retrieval)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "services/pus15.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

returnCode_t InitS15(pus15Env_t *pus15_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Initialise the devices
        return_value = DeviceOpen(&pus15_env->dev_pus15_index, DEVICE_TYPE_FILE, pus15_env->fil_pus15_index);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceOpen(&pus15_env->dev_pus15_data, DEVICE_TYPE_FILE, pus15_env->fil_pus15_data);
            if (return_value == RET_SUCCESSFUL)
            {
                // TODO: WIP, check files and reset them if needed
            }
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              ResetIndexAndData(pus15Env_t *pus15_env)
 * @brief           This function reset index and data file (filling them with zeros)
 * @param[in,out]   pus15_env PUS15 environment
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t ResetIndexAndData(pus15Env_t *pus15_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Zero fill index file
        return_value = ZeroFillDevice(pus15_env->dev_pus15_index, INDEX_SIZE);
        if (return_value == RET_SUCCESSFUL)
        {
            // Zero fill data file
            return_value = ZeroFillDevice(pus15_env->dev_pus15_data, PUS15_DATA_TABLE_SIZE);
        }
    }

    return return_value;
}

// TODO: when PUS11 is updated, make this function generic
/**
 * @fn          ZeroFillDevice(deviceNo_t device, length_t total_size)
 * @brief       Fill a device file with zeros from the beginning
 * @param[in]   device Device to zero fill
 * @param[in]   total_size Total number of bytes to write
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ZeroFillDevice(deviceNo_t device, length_t total_size)
{
    returnCode_t return_value                      = RET_SUCCESSFUL;
    data_t zero_filled_data[ZERO_FILLED_DATA_SIZE] = { 0 };
    length_t origin                                = 0u;

    // Set read/write pointer to the beginning of the file
    return_value = DeviceIoctl(device, IOCTL_FS_SEEK, &origin, sizeof(origin));
    if (return_value == RET_SUCCESSFUL)
    {
        // Write 0s in the file
        length_t remaining_bytes = total_size;
        while ((return_value == RET_SUCCESSFUL) && (remaining_bytes > 0u))
        {
            if (remaining_bytes >= ZERO_FILLED_DATA_SIZE)
            {
                return_value     = DeviceWrite(device, (data_t)&zero_filled_data, ZERO_FILLED_DATA_SIZE);
                remaining_bytes -= ZERO_FILLED_DATA_SIZE;
            }
            else
            {
                return_value    = DeviceWrite(device, (data_t)&zero_filled_data, remaining_bytes);
                remaining_bytes = 0u;
            }
        }
    }

    return return_value;
}
