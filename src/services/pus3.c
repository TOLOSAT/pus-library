/**
 * @file    pus3.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 3 functions (Housekeeping)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include "kernel.h"
#include "tm_management.h"
#include "services/pus3.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

static returnCode_t GetLinenoFromHKID(pus3HKTable_t *table, pus3HKID_t hkid, length_t *lineno);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

returnCode_t InitS3(pus3Env_t *pus3_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus3_env != NULL)
    {

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

            // Get HKID
            pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT32(tc->data);

            // Look for a specific HK or every HK
            if (hkid != 0)
            {
                length_t lineno = 0;
                // Search for HK Param
                return_value = GetLinenoFromHKID(&pus3_env->pus3_hk_table, hkid, &lineno);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Enable the HK
                    pus3_env->pus3_hk_table.entries[lineno].status = HK_REPORT_ENABLE;
                }
            }
            else
            {
                // Enable every HK
                for (uint32_t i = 0u; i < pus3_env->pus3_hk_table.size; i++)
                {
                    pus3_env->pus3_hk_table.entries[i].status = HK_REPORT_ENABLE;
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

            // Get HKID
            pus3HKID_t hkid = BIG_ENDIAN_ARRAY_TO_UINT32(tc->data);

            // Look for a specific HK or every HK
            if (hkid != 0)
            {
                length_t lineno = 0;
                // Search for HK Param
                return_value = GetLinenoFromHKID(&pus3_env->pus3_hk_table, hkid, &lineno);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Disable the HK
                    pus3_env->pus3_hk_table.entries[lineno].status = HK_REPORT_DISABLE;
                }
            }
            else
            {
                // Disable every HK
                for (uint32_t i = 0u; i < pus3_env->pus3_hk_table.size; i++)
                {
                    pus3_env->pus3_hk_table.entries[i].status = HK_REPORT_DISABLE;
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