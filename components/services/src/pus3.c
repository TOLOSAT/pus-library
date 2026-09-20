/**
 * @file    pus3.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 3 functions (Housekeeping)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tm_management.h"
#include "services/pus3.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

static returnCode_t GetLinenoFromHKID(pus3HKTable_t *table, pus3HKID_t hkid, length_t *lineno);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc InitS3
 */
returnCode_t InitS3(pus3Env_t *pus3_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus3_env != NULL) && (pus3_env->hk_table.size != 0u) && (pus3_env->hk_table.entries != NULL))
    {
        // Open device for hktm
        return_value = DeviceOpen(&pus3_env->dev_hktm, DEVICE_TYPE_BUFFER, pus3_env->buffer_hktm);
        if (return_value == RET_SUCCESSFUL)
        {
            // First check the first HK entry and then all of them (checking order requires to look at the first elements independently)
            if ((pus3_env->hk_table.entries[0].hkid != 0u)                                         // HKID must be non zero
                && (pus3_env->hk_table.entries[0].collection_rate != 0u)                           // Collection rate must be non zero
                && (pus3_env->hk_table.entries[0].size != 0u)                                      // Size must be non zero
                && (pus3_env->hk_table.entries[0].size <= (TM_MAX_DATA_SIZE - sizeof(pus3HKID_t))) // Size must be less than max data size
                && (pus3_env->hk_table.entries[0].p_addr != NULL))                                 // Pointer must be non null
            {
                uint32_t i = 1u; // First entry already has been checked
                // Check all remaining entries
                while ((return_value == RET_SUCCESSFUL) && (i < pus3_env->hk_table.size))
                {
                    // Check entry
                    if ((pus3_env->hk_table.entries[i].hkid > pus3_env->hk_table.entries[i - 1u].hkid)     // HKID should be increasing
                        && (pus3_env->hk_table.entries[i].collection_rate != 0u)                           // Collection rate must be non zero
                        && (pus3_env->hk_table.entries[i].size != 0u)                                      // Size must be non zero
                        && (pus3_env->hk_table.entries[i].size <= (TM_MAX_DATA_SIZE - sizeof(pus3HKID_t))) // Size must be less than max data size
                        && (pus3_env->hk_table.entries[i].p_addr != NULL))                                 // Pointer must be non null
                    {
                        // Go to next entry
                        i++;
                    }
                    else
                    {
                        return_value = RET_INVALID_PARAM;
                    }
                }

                // If everything went well, initialize the environment and reset the cycle counter
                if (return_value == RET_SUCCESSFUL)
                {
                    pus3_env->status = PUS_INITIALIZED;
                    pus3_env->cycle  = 0;
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
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
 * @copydoc EmitHKs
 */
returnCode_t EmitHKs(pus3Env_t *pus3_env)
{
    returnCode_t return_value        = RET_SUCCESSFUL;
    pusTM_t tm                       = { 0 };
    pusData_t data[TM_MAX_DATA_SIZE] = { 0 };

    if ((pus3_env != NULL) && (pus3_env->status == PUS_INITIALIZED))
    {
        uint32_t i = 0u;
        // For every HK param in the hk_table, verify if a HKTM needs to be emitted
        while ((return_value == RET_SUCCESSFUL) && (i < pus3_env->hk_table.size))
        {
            if (((pus3_env->cycle % pus3_env->hk_table.entries[i].collection_rate) == 0u)
                && (pus3_env->hk_table.entries[i].status == HK_REPORT_ENABLE))
            {
                // Retrieve data for the HKTM
                (void)memcpy(&data[0], (void *)&pus3_env->hk_table.entries[i].hkid, sizeof(pus3HKID_t));
                (void)memcpy(&data[sizeof(pus3HKID_t)], pus3_env->hk_table.entries[i].p_addr, pus3_env->hk_table.entries[i].size);
                return_value = BuildTM(&tm, 3u, 25u, (pusData_t *)&data, sizeof(pus3HKID_t) + pus3_env->hk_table.entries[i].size);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Emit the TM
                    return_value = DeviceWrite(pus3_env->dev_hktm, (data_t)&tm, TM_MAX_SIZE);
                    if (return_value == RET_SUCCESSFUL)
                    {
                        taskNo_t tm_sender = NO_TASK;
                        ConsolePrint("HKTM(3,25) has been sent\n");
                        return_value = DeviceIoctl(pus3_env->dev_hktm, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                        if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                        {
                            return_value = SendSignal(tm_sender, SIGNAL_TM);
                        }
                    }
                }
            }
            i++;
        }

        // Finaly increment cycle counter
        pus3_env->cycle++;
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS3SS5
 */
returnCode_t ExecuteS3SS5(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Check if the size of the TC could contain at least one HKID
        if ((tc->spp_header.packet_data_length + 1u) >= (TC_HEADER_SIZE + sizeof(pusNField_t) + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                uint32_t offset = sizeof(pusNField_t);
                pusNField_t N   = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

                // Get HKIDs from TC
                pusNField_t i = 0u;
                while ((return_value == RET_SUCCESSFUL) && (i < N))
                {
                    // Check that offset is not out of the bound
                    if ((offset + sizeof(pus3HKID_t)) <= TC_MAX_DATA_SIZE)
                    {
                        // Get HKID
                        pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[offset]);

                        // Look for a specific HK or every HK
                        if (hkid != 0u)
                        {
                            length_t lineno = 0;
                            // Search for HK Param
                            return_value = GetLinenoFromHKID(&pus3_env->hk_table, hkid, &lineno);
                            if (return_value == RET_SUCCESSFUL)
                            {
                                // Enable the HK
                                pus3_env->hk_table.entries[lineno].status = HK_REPORT_ENABLE;
                            }
                            else
                            {
                                *error_code = PUS_EXECUTION_UNEXPECTED_DATA;
                            }
                        }
                        else
                        {
                            // Enable every HK
                            for (uint32_t k = 0u; k < pus3_env->hk_table.size; k++)
                            {
                                pus3_env->hk_table.entries[k].status = HK_REPORT_ENABLE;
                            }
                        }

                        // Update index and offset
                        offset += sizeof(pus3HKID_t);
                        i++;
                    }
                    else
                    {
                        return_value = RET_INVALID_PARAM;
                        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                    }
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
                *error_code  = PUS_EXECUTION_UNAVAILABLE;
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

/**
 * @copydoc ExecuteS3SS6
 */
returnCode_t ExecuteS3SS6(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Check if the size of the TC could contain at least one HKID
        if ((tc->spp_header.packet_data_length + 1u) >= (TC_HEADER_SIZE + sizeof(pusNField_t) + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                uint32_t offset = sizeof(pusNField_t);
                pusNField_t N   = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

                // Get HKIDs from TC
                pusNField_t i = 0u;
                while ((return_value == RET_SUCCESSFUL) && (i < N))
                {
                    // Check that offset is not out of the bound
                    if ((offset + sizeof(pus3HKID_t)) <= TC_MAX_DATA_SIZE)
                    {
                        // Get HKID
                        pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[offset]);

                        // Look for a specific HK or every HK
                        if (hkid != 0u)
                        {
                            length_t lineno = 0;
                            // Search for HK Param
                            return_value = GetLinenoFromHKID(&pus3_env->hk_table, hkid, &lineno);
                            if (return_value == RET_SUCCESSFUL)
                            {
                                // Disable the HK
                                pus3_env->hk_table.entries[lineno].status = HK_REPORT_DISABLE;
                            }
                            else
                            {
                                *error_code = PUS_EXECUTION_UNEXPECTED_DATA;
                            }
                        }
                        else
                        {
                            // Disable every HK
                            for (uint32_t k = 0u; k < pus3_env->hk_table.size; k++)
                            {
                                pus3_env->hk_table.entries[k].status = HK_REPORT_DISABLE;
                            }
                        }

                        // Update index and offset
                        offset += sizeof(pus3HKID_t);
                        i++;
                    }
                    else
                    {
                        return_value = RET_INVALID_PARAM;
                        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                    }
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
                *error_code  = PUS_EXECUTION_UNAVAILABLE;
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

/**
 * @copydoc ExecuteS3SS9
 */
returnCode_t ExecuteS3SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value           = RET_SUCCESSFUL;
    pusData_t tm_data[TM_MAX_DATA_SIZE] = { 0 };

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (tm != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Check if the size of the TC could contain at least one HKID
        if ((tc->spp_header.packet_data_length + 1u) >= (TC_HEADER_SIZE + sizeof(pusNField_t) + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                uint32_t offset       = sizeof(pusNField_t);
                pusNField_t N         = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);
                length_t tm_data_size = 0u;

                // Get HKIDs from TC
                pusNField_t i = 0u;
                while ((return_value == RET_SUCCESSFUL) && (i < N))
                {
                    // Check that offset and tm data size are not out of the bound
                    if (((offset + sizeof(pus3HKID_t)) <= TC_MAX_DATA_SIZE) && ((tm_data_size + sizeof(pus3HKParam_t)) <= TM_MAX_DATA_SIZE))
                    {
                        // Get HKID
                        pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[offset]);

                        // Look for a specific HK or every HK
                        if (hkid != 0u)
                        {
                            length_t lineno = 0;
                            // Search for HK Param
                            return_value = GetLinenoFromHKID(&pus3_env->hk_table, hkid, &lineno);
                            if (return_value == RET_SUCCESSFUL)
                            {
                                // Copy into the TM data buffer the HK report parameter
                                (void)memcpy(&tm_data[tm_data_size], (void *)&pus3_env->hk_table.entries[lineno], sizeof(pus3HKParam_t));
                            }
                            else
                            {
                                *error_code = PUS_EXECUTION_UNEXPECTED_DATA;
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
                        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                    }

                    // Update index, offset and tm data size
                    offset       += sizeof(pus3HKID_t);
                    tm_data_size += sizeof(pus3HKParam_t);
                    i++;
                }

                // If retrieving paramters went successful, send the TM
                if (return_value == RET_SUCCESSFUL)
                {
                    return_value = BuildTM(tm, 3u, 10u, (pusData_t *)tm_data, tm_data_size);
                    if (return_value != RET_SUCCESSFUL)
                    {
                        *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                    }
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
                *error_code  = PUS_EXECUTION_UNAVAILABLE;
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

/**
 * @copydoc ExecuteS3SS31
 */
returnCode_t ExecuteS3SS31(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Check if the size of the TC could contain at least one HKID and rate
        if ((tc->spp_header.packet_data_length + 1u)
            >= (TC_HEADER_SIZE + sizeof(pusNField_t) + sizeof(pus3HKID_t) + sizeof(pus3HKRate_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                uint32_t offset = sizeof(pusNField_t);
                pusNField_t N   = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

                // Get HKIDs from TC
                pusNField_t i = 0u;
                while ((return_value == RET_SUCCESSFUL) && (i < N))
                {
                    // Check that offset is not out of the bound
                    if ((offset + sizeof(pus3HKID_t) + sizeof(pus3HKRate_t)) <= TC_MAX_DATA_SIZE)
                    {
                        // Get HKID and collection rate
                        pus3HKID_t hkid              = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[offset]);
                        pus3HKRate_t collection_rate = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[offset + sizeof(pus3HKID_t)]);

                        // Look for a specific HK or every HK
                        if ((hkid != 0u) && (collection_rate != 0u))
                        {
                            length_t lineno = 0;
                            // Search for HK Param
                            return_value = GetLinenoFromHKID(&pus3_env->hk_table, hkid, &lineno);
                            if (return_value == RET_SUCCESSFUL)
                            {
                                // Setup collection rate
                                pus3_env->hk_table.entries[lineno].collection_rate = collection_rate;
                            }
                            else
                            {
                                *error_code = PUS_EXECUTION_UNEXPECTED_DATA;
                            }
                        }
                        else
                        {
                            return_value = RET_INVALID_PARAM;
                            *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                        }

                        // Update index and offset
                        offset += sizeof(pus3HKID_t) + sizeof(pus3HKRate_t);
                        i++;
                    }
                    else
                    {
                        return_value = RET_INVALID_PARAM;
                        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                    }
                }
            }
            else
            {
                return_value = RET_INVALID_PARAM;
                *error_code  = PUS_EXECUTION_UNAVAILABLE;
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

/**
 * @fn          GetLinenoFromHKID(pus3HKTable_t *table, pus3HKID_t hkid, length_t *lineno)
 * @brief       This function search for an HK param report entry linenoo in hk table with a hkid
 * @param[in]   table HK table to search in
 * @param[in]   hkid HK ID to search for
 * @param[out]  lineno Lineno of the HK param report entry
 * @retval      #RET_NOT_AVAILABLE if hkid does not exist in table
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t GetLinenoFromHKID(pus3HKTable_t *table, pus3HKID_t hkid, length_t *lineno)
{
    returnCode_t return_value = RET_NOT_AVAILABLE;
    pusTableSize_t left       = 0u;
    pusTableSize_t right      = table->size - 1u;
    pusTableSize_t cursor     = left + (right - left) / 2u;

    // Perform a binary search
    while ((left <= right) && (right < table->size) && (return_value != RET_SUCCESSFUL))
    {
        if (table->entries[cursor].hkid == hkid)
        {
            *lineno      = cursor;
            return_value = RET_SUCCESSFUL;
        }
        else if (table->entries[cursor].hkid < hkid)
        {
            left   = cursor + 1u;
            cursor = left + (right - left) / 2u;
        }
        else
        {
            right  = cursor - 1u;
            cursor = left + (right - left) / 2u;
        }
    }

    return return_value;
}
