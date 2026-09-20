/**
 * @file    pus9.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 9 functions (Time Management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus9 PUS Service 9
 * @brief PUS service 9 (Time Management) implementation
 * @{
 */

#ifndef PUS9_H
#define PUS9_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn              ExecuteS9SS128(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that receive S9SS128 TC and update OBT
 * @param[in,out]   env PUS9 environment
 * @param[in]       tc S9SS128 TC that contains upcoming time
 * @param[out]      tm None (this parameter is unused for these service and subservice)
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_ERROR if time cannot be set
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS9SS128(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS9_H */

/**
 * @}
 * @}
 * @}
 */