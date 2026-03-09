/**
 * @file    pus3.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 3 functions (Housekeeping)
 *
 * @copyright Copyright (c) TOLOSAT 2026
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

extern returnCode_t InitS3(pus3Env_t *pus3_env);
extern returnCode_t EmitHKs(pus3Env_t *pus3_env);
extern returnCode_t ExecuteS3SS5(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);
extern returnCode_t ExecuteS3SS6(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);
extern returnCode_t ExecuteS3SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);
extern returnCode_t ExecuteS3SS31(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS3_H */

/**
 * @}
 * @}
 * @}
 */