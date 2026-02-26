/**
 * @file    tc_parser.h
 * @author  Matteo Planchet
 * @brief   Header file for TC parsing
 *
 * @copyright Copyright (c) TOLOSAT 2025
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus_tc_parser TC Parser
 * @brief Telecommand (TC) parsing implementation.
 * @{
 */

#ifndef TC_PARSER_H
#define TC_PARSER_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/**
 * @enum tcState_t
 * @brief Enumeration representing the state of a TC during parsing process
 */
typedef enum
{
    TC_STATE_VALID   = 0u, /**< TC is valid */
    TC_STATE_PARTIAL = 1u, /**< TC is partial */
    TC_STATE_INVALID = 2u, /**< TC is invalid */
} tcState_t;

/**
 * @struct tcFrame_t
 * @brief Structure representing a TC frame during parsing process
 */
typedef struct
{
    length_t start;  /**< @brief Start index of current TC */
    length_t length; /**< @brief Total declared TC length (from header) */
    tcState_t state; /**< @brief State of the current TC being parsed */
} tcFrame_t;

/**
 * @struct pusParsingContext_t
 * @brief Context structure for TC parsing
 */
typedef struct
{
    uint8_t *p_buffer;    /**< @brief Pointer to data buffer */
    length_t buffer_size; /**< @brief Size of the buffer */
    length_t read_index;  /**< @brief Current read index in the RX buffer */
    length_t write_index; /**< @brief Current write index in the RX buffer */
} pusParsingContext_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

extern returnCode_t ParseBuffer(pusParsingContext_t *ctx, pusTC_t *tc, pusAcceptanceError_t *error);
extern returnCode_t CheckTCValidity(pusTC_t *tc, pusAcceptanceError_t *error);

#endif /* TC_PARSER_H */

/**
 * @}
 * @}
 * @}
 */