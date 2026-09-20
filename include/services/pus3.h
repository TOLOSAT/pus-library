/**
 * @file    pus3.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 3 functions (Housekeeping)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus3 PUS Service 3
 * @brief PUS service 3 (Housekeeping) implementation
 * @{
 */

#ifndef PUS3_H
#define PUS3_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

#define HK_REPORT_DISABLE 0u /**< HK report disable */
#define HK_REPORT_ENABLE  1u /**< HK report enable */

/***************************** Types Definitions *****************************/

/** @brief PUS3 HK ID type definition */
typedef uint16_t pus3HKID_t;

/** @brief PUS3 HK status type definition */
typedef uint16_t pus3HKStatus_t;

/** @brief PUS3 HK collection rate type definition */
typedef uint16_t pus3HKRate_t;

/**
 * @struct  pus3HKParam_t
 * @brief   Struct type for a pus3 HK report parameter
 */
typedef struct
{
    pus3HKID_t hkid;              /**< @brief HK ID */
    pus3HKStatus_t status;        /**< @brief HK status */
    pus3HKRate_t collection_rate; /**< @brief Collection rate (every x period) */
    void *p_addr;                 /**< @brief Pointer to the HK data */
    length_t size;                /**< @brief Size of the HK data */
} ATTR_BYTE_ALIGNED pus3HKParam_t;

/**
 * @struct  pus3HKTable_t
 * @brief   Struct type for HK report parameter table
 */
typedef struct
{
    length_t size;          /**< @brief Number of rows (table size) */
    pus3HKParam_t *entries; /**< @brief Pointer to the array of table rows */
} pus3HKTable_t;

/**
 * @struct  pus3Env_t
 * @brief   Struct type for a pus3 environment
 */
typedef struct
{
    pusStatus_t status;     /**< @brief PUS3 environment status */
    pus3HKTable_t hk_table; /**< @brief HK report parameter table */
    bufferNo_t buffer_hktm; /**< @brief Buffer where the HKTM will be sent */
    deviceNo_t dev_hktm;    /**< @brief Device bound to the HKTM buffer */
    uint32_t cycle;         /**< @brief Current cycle */
} pus3Env_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

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
extern returnCode_t InitS3(pus3Env_t *pus3_env);

/**
 * @fn          EmitHKs(pus3Env_t *pus3_env)
 * @brief       Function that is call every period to emit housekeeping TMs
 * @param[in]   pus3_env pus3_env PUS3 environment
 * @retval      #RET_INVALID_PARAM if pus3_env is null or not initialized
 * @retval      #RET_ERROR if Building or sending the TM encounters an error
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t EmitHKs(pus3Env_t *pus3_env);

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
extern returnCode_t ExecuteS3SS5(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

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
extern returnCode_t ExecuteS3SS6(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

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
 */
extern returnCode_t ExecuteS3SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

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
 */
extern returnCode_t ExecuteS3SS31(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS3_H */

/**
 * @}
 * @}
 * @}
 */