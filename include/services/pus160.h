/**
 * @file    pus160.h
 * @author  Théo Bessel
 * @brief   Header file for PUS 160 functions (System Management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus160 PUS Service 160
 * @brief PUS service 160 (System Management) implementation
 * @{
 */

#ifndef PUS160_H
#define PUS160_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/**
 * @struct  softwareSelection_t
 * @brief   Structure to store the mandatory information for a software selection.
 * @see     ExecuteS160SS2
 */
typedef struct
{
    softwareId_t software_id;       /**< @brief Software ID to reboot to */
    softwareState_t software_state; /**< @brief Software state to reboot to */
} ATTR_BYTE_ALIGNED softwareSelection_t;

/**
 * @struct  pus160Env_t
 * @brief   Struct type for pus160 environment
 */
typedef struct
{
    pusStatus_t status;          /**< @brief PUS160 environment status */
    uint32_t nb_tasks;           /**< @brief Number of task in the system */
    deviceNo_t dev_reboot;       /**< @brief Device for rebooting the system */
    deviceNo_t dev_context;      /**< @brief Device for reading system context */
    deviceNo_t dev_system_usage; /**< @brief Device for reading system usage */
    deviceNo_t dev_task_usages;  /**< @brief Device for reading task usages */
} pus160Env_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn          InitS160(pus160Env_t *pus160_env)
 * @brief       Function that initialises PUS 160
 * @param[in]   pus160_env PUS160 environment used for configuration
 * @retval      #RET_INVALID_PARAM if nb_task is not correct
 * @retval      #RET_ERROR if cannot bind the pus160_dev_reboot to the reboot
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t InitS160(pus160Env_t *pus160_env);

/**
 * @fn              ExecuteS160SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that achieve a reboot to a chosen software
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
extern returnCode_t ExecuteS160SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that selects the default rebooting software (soft_id, safe/nominal)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
extern returnCode_t ExecuteS160SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the system context
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS18 TM
 */
extern returnCode_t ExecuteS160SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS19(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the reduced system context (without debug info)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS20 TM
 */
extern returnCode_t ExecuteS160SS19(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS21(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the error context (only debug info)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS22 TM
 */
extern returnCode_t ExecuteS160SS21(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS23(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that resets the error context
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
extern returnCode_t ExecuteS160SS23(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS33(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS34 TM (idle time report) when requested by a S160SS33
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS34 TM
 */
extern returnCode_t ExecuteS160SS33(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS35(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS36 TM (stack usage report) when requested by a S160SS35
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS36 TM
 */
extern returnCode_t ExecuteS160SS35(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS160SS37(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS38 TM (system usage report) when requested by a S160SS37
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS38 TM
 */
extern returnCode_t ExecuteS160SS37(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS160_H */

/**
 * @}
 * @}
 * @}
 */