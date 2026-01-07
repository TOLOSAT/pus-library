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

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

extern static returnCode_t ParseBuffer(rxBuffer_t *rx, tcFrame_t *tc, tcState_t *state);

#endif /* TC_PARSER_H */

/**
 * @}
 * @}
 * @}
 */