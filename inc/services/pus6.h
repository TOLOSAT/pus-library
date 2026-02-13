/**
 * @file    pus6.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 6 functions (Memory Management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus6 PUS Service 6
 * @brief PUS service 6 (Memory Management) implementation
 * @{
 */

#ifndef PUS6_H
#define PUS6_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

#define MEMORY_BASE_SIZE         2u                                        /**< Memory base size */
#define MEMORY_OFFSET_SIZE       4u                                        /**< Memory offset size */
#define MEMORY_LENGTH_SIZE       4u                                        /**< Memory data length size */
#define MEMORY_TC_DATA_DUMP_SIZE (MEMORY_OFFSET_SIZE + MEMORY_LENGTH_SIZE) /**< Memory TC data dump size */
#define MEMORY_TC_DATA_LOAD_MAX_SIZE \
    (TC_MAX_DATA_SIZE - PUS_N_FIELD_SIZE - MEMORY_OFFSET_SIZE - MEMORY_LENGTH_SIZE) /**< Memory maximum TC data load size */
#define MEMORY_TM_DATA_DUMP_MAX_SIZE \
    (TM_MAX_DATA_SIZE - PUS_N_FIELD_SIZE - MEMORY_OFFSET_SIZE - MEMORY_LENGTH_SIZE) /**< Memory maximum TM data dump size */

/***************************** Types Definitions *****************************/

/** @brief PUS6 base type (file no) */
typedef uint16_t pus6Base_t;

/** @brief PUS6 offset type */
typedef uint32_t pus6Offset_t;

/** @brief PUS6 length type */
typedef uint32_t pus6Length_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

extern returnCode_t ExecuteS6SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);
extern returnCode_t ExecuteS6SS3(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS6_H */

/**
 * @}
 * @}
 * @}
 */
