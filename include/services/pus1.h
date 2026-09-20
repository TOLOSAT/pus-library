/**
 * @file    pus1.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 1 functions (Request Verification)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus1 PUS Service 1
 * @brief PUS service 1 (Request Verification) implementation
 * @{
 */

#ifndef PUS1_H
#define PUS1_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

#define S1SS1_DATA_SIZE 4u /**< Size of PUS S1SS1 data field */
#define S1SS2_DATA_SIZE 5u /**< Size of PUS S1SS2 data field */
#define S1SS7_DATA_SIZE 4u /**< Size of PUS S1SS7 data field */
#define S1SS8_DATA_SIZE 5u /**< Size of PUS S1SS8 data field */

/***************************** Types Definitions *****************************/

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn          BuildS1SS1(const pusTC_t *tc, pusTM_t *acceptance_tm)
 * @brief       Function that send S1SS1 TM (acceptance acknowledgment)
 * @param[in]   tc TC we want to acknowledge
 * @param[out]  acceptance_tm Acceptance TM we will send
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildS1SS1(const pusTC_t *tc, pusTM_t *acceptance_tm);

/**
 * @fn          BuildS1SS2(const pusTC_t *tc, pusTM_t *acceptance_tm, pusAcceptanceError_t acceptance_error)
 * @brief       Function that send S1SS2 TM (acceptance non acknowledgment)
 * @param[in]   tc TC we want to non acknowledge
 * @param[out]  acceptance_tm Acceptance TM we will send
 * @param[in]   acceptance_error Error that explain why we non acknowledge
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildS1SS2(const pusTC_t *tc, pusTM_t *acceptance_tm, pusAcceptanceError_t acceptance_error);

/**
 * @fn          BuildS1SS7(const pusTC_t *tc, pusTM_t *execution_tm)
 * @brief       Function that send S1SS7 TM (execution acknowledgment)
 * @param[in]   tc TC we want to acknowledge
 * @param[out]  execution_tm Execution TM we will send
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildS1SS7(const pusTC_t *tc, pusTM_t *execution_tm);

/**
 * @fn          BuildS1SS8(const pusTC_t *tc, pusTM_t *execution_tm, pusExecutionError_t execution_error)
 * @brief       Function that send S1SS8 TM (execution non acknowledgment)
 * @param[in]   tc TC we want to non acknowledge
 * @param[out]  execution_tm Execution TM we will send
 * @param[in]   execution_error Error that explain why we non acknowledge
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildS1SS8(const pusTC_t *tc, pusTM_t *execution_tm, pusExecutionError_t execution_error);

#endif /* PUS1_H */

/**
 * @}
 * @}
 * @}
 */