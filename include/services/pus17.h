/**
 * @file    pus17.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 17 functions (Test)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus17 PUS Service 17
 * @brief PUS service 17 (Test) implementation
 * @{
 */

#ifndef PUS17_H
#define PUS17_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn              ExecuteS17SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S17SS2 TM (connexion report)
 * @param[in,out]   env PUS17 environment
 * @param[in]       tc S17SS1 TC (this parameter is unused for these service and subservice)
 * @param[out]      tm S17SS2 TM that we will send
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_ERROR if cannot build TM
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS17SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS17_H */

/**
 * @}
 * @}
 * @}
 */