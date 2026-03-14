/**
 * @file    pus3.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 3 functions (Housekeeping)
 *
 * @copyright Copyright (c) TOLOSAT 2026
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
 * @fn              InitS3(pus3Env_t *pus3_env)
 * @brief           This function initializes a PUS3 environment
 * @param[in,out]   pus3_env PUS3 environment
 * @retval          #RET_INVALID_PARAM if pus3_env is a null pointer
 * @retval          #RET_INVALID_PARAM if hk_table is empty or size is zero
 * @retval          #RET_INVALID_PARAM if a pus3 entry is invalid (null size, null pointer or null collection rate)
 * @retval          #RET_INVALID_PARAM if hk_table entries are not ordered by hkid
 * @retval          #RET_ERROR if DeviceOpen encountered an error
 * @retval          #RET_SUCCESSFUL else
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
 * @fn          EmitHKs(pus3Env_t *pus3_env)
 * @brief       Function that is call every period to emit housekeeping TMs
 * @param[in]   pus3_env pus3_env PUS3 environment
 * @retval      #RET_INVALID_PARAM if pus3_env is null or not initialized
 * @retval      #RET_ERROR if Building or sending the TM encounters an error
 * @retval      #RET_SUCCESSFUL else
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
                            return_value = SendSignal(tm_sender, SIGNAL_TC);
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
 * @fn              ExecuteS3SS5(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send enable HK report by HKID (if HKID = 0 enable all)
 * @param[in,out]   env PUS3 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_INVALID_PARAM if HKID does not exist
 * @retval          #RET_SUCCESSFUL else
 *
 * @todo Need to implements a N field (see pus6 or pus11)
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

        if ((tc->spp_header.packet_data_length + 1u) == (TC_HEADER_SIZE + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                // Get HKID
                pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

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
                }
                else
                {
                    // Enable every HK
                    for (uint32_t i = 0u; i < pus3_env->hk_table.size; i++)
                    {
                        pus3_env->hk_table.entries[i].status = HK_REPORT_ENABLE;
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
 * @fn              ExecuteS3SS6(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send disable HK report by HKID (if HKID = 0 disable all)
 * @param[in,out]   env PUS3 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if HKID does not exist
 * @retval          #RET_SUCCESSFUL else
 *
 * @todo Need to implements a N field (see pus6 or pus11)
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

        if ((tc->spp_header.packet_data_length + 1u) == (TC_HEADER_SIZE + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                // Get HKID
                pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

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
                }
                else
                {
                    // Disable every HK
                    for (uint32_t i = 0u; i < pus3_env->hk_table.size; i++)
                    {
                        pus3_env->hk_table.entries[i].status = HK_REPORT_DISABLE;
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
 * @fn              ExecuteS3SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that dumps HK report parameters
 * @param[in,out]   env PUS3 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if HKID does not exist
 * @retval          #RET_SUCCESSFUL else
 *
 * @todo Need to implements a N field (see pus6 or pus11)
 */
returnCode_t ExecuteS3SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (tm != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        if ((tc->spp_header.packet_data_length + 1u) == (TC_HEADER_SIZE + sizeof(pus3HKID_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                // Get HKID
                pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);

                // Look for a specific HK or every HK
                if (hkid != 0u)
                {
                    length_t lineno = 0;
                    // Search for HK Param
                    return_value = GetLinenoFromHKID(&pus3_env->hk_table, hkid, &lineno);
                    if (return_value == RET_SUCCESSFUL)
                    {
                        // Dumps into the TM the HK report parameter
                        return_value = BuildTM(tm, 3u, 10u, (pusData_t *)&pus3_env->hk_table.entries[lineno], sizeof(pus3HKParam_t));
                        if (return_value != RET_SUCCESSFUL)
                        {
                            *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                        }
                    }
                }
                else
                {
                    // Disable every HK
                    for (uint32_t i = 0u; i < pus3_env->hk_table.size; i++)
                    {
                        pus3_env->hk_table.entries[i].status = HK_REPORT_DISABLE;
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
 * @fn              ExecuteS3SS31(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that change an HK collection rate
 * @param[in,out]   env PUS3 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if HKID does not exist
 * @retval          #RET_SUCCESSFUL else
 *
 * @todo Need to implements a N field (see pus6 or pus11)
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

        if ((tc->spp_header.packet_data_length + 1u) == (TC_HEADER_SIZE + sizeof(pus3HKID_t) + sizeof(pus3HKRate_t) + CRC_TRAILER_SIZE))
        {
            // Get environment
            pus3Env_t *pus3_env = (pus3Env_t *)env;

            // Check if pus3 is initialized
            if (pus3_env->status == PUS_INITIALIZED)
            {
                // Get HKID and collection rate
                pus3HKID_t hkid              = BIG_ENDIAN_ARRAY_TO_UINT16(tc->data);
                pus3HKRate_t collection_rate = BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[sizeof(pus3HKID_t)]);

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