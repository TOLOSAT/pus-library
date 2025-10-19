/**
 * @file    tables_management.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for execution or routing tables
 *
 * @copyright Copyright (c) TOLOSAT 2025
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup tables_management Tables Management
 * @brief PUS specific tables management layer.
 * @{
 */

#ifndef TABLES_MANAGEMENT_H
#define TABLES_MANAGEMENT_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/**
 * @def  BUILD_ROUTING_KEY(apid, service, subservice)
 * @brief Preprocessor function that build routing key with APID, sevice and subservice
 */
#define BUILD_ROUTING_KEY(apid, service, subservice) ((uint32_t)(((uint32_t)(apid) << 16) | ((uint32_t)(service) << 8) | (uint32_t)(subservice)))

/***************************** Types Definitions *****************************/

/** @brief Pointer to execution function type */
typedef returnCode_t (*pusExecutionFunctionPtr_t)(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @enum    pusTMRequested_t
 * @brief   Type enum use to indicates if this TC needs a specific TM to be send
 */
typedef enum
{
    TM_NOT_REQUESTED = 0u, /**< No specific TM has to be send for this TC */
    TM_REQUESTED     = 1u, /**< A TM has to be send for this TC */
} pusTMRequested_t;

/**
 * @struct  pusExecutionTableEntry_t
 * @brief   Struct type for execution table entry
 */
typedef struct
{
    uint32_t key;                                 /**< @brief Key allowing to link to the execution function */
    pusExecutionFunctionPtr_t execution_function; /**< @brief Execution function */
    pusTMRequested_t tm_requested;                /**< @brief Indicates if this TC needs a specific TM to be send */
    void *env;                                    /**< @brief Pointer to the service environment */
} pusExecutionTableEntry_t;

/**
 * @struct pusExecutionTable_t
 * @brief  Represents the execution table
 */
typedef struct
{
    length_t size;                     /**< @brief Number of rows (table size) */
    pusExecutionTableEntry_t *entries; /**< @brief Pointer to the array of table rows */
} pusExecutionTable_t;

/**
 * @struct  pusRoutingTableEntry_t
 * @brief   Struct type for routing table entry
 */
typedef struct
{
    uint32_t key;         /**< @brief Key allowing to route */
    uint32_t route;       /**< @brief Route reference number */
    deviceNo_t dev_route; /**< @brief Route device (i.e. a buffer device) */
} pusRoutingTableEntry_t;

/**
 * @struct  pusRoutingTable_t
 * @brief   Struct type for routing table
 */
typedef struct
{
    length_t size;                     /**< @brief Number of rows (table size) */
    pusRoutingTableEntry_t *entries; /**< @brief Pointer to the array of table rows */
} pusRoutingTable_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

extern returnCode_t InitRoutingTable(pusRoutingTable_t *routing_table);
extern returnCode_t InitExecutionTable(pusExecutionTable_t *execution_table);
extern returnCode_t RouteSearch(uint32_t key, pusRoutingTable_t *routing_table, pusRoutingTableEntry_t **entry);
extern returnCode_t ExecutionSearch(uint32_t key, pusExecutionTable_t *execution_table, pusExecutionTableEntry_t **entry);

#endif /* TABLES_MANAGEMENT_H */

/**
 * @}
 * @}
 * @}
 */