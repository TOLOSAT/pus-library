/**
 * @file    tc_management.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for TC management
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup tc_management TC Management
 * @brief PUS telecommand (TC) management layer.
 * @{
 */

#ifndef TC_MANAGEMENT_H
#define TC_MANAGEMENT_H

/******************************* Include Files *******************************/

#include "pus_types.h"
#include "tools/tables_management.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/**
 * @struct  pusReceiveContext_t
 * @brief   Struct type for receive context
 */
typedef struct
{
    pusStatus_t status;              /**< @brief Receive context status */
    pusRoutingTable_t routing_table; /**< @brief Pointer to the routing table */
    uint32_t ref_rx;                 /**< @brief Resource where the TC come from */
    deviceType_t rx_type;            /**< @brief Type of resource used to receive the TC (e.g. buffer or peripheral) */
    deviceNo_t dev_rx;               /**< @brief Device bound to the RX resource */
    bufferNo_t buffer_ack;           /**< @brief Buffer where the ACK TM will be sent */
    deviceNo_t dev_ack;              /**< @brief Device bound to the ACK buffer */
    uint8_t *rx_buffer;              /**< @brief RX buffer structure */
    length_t rx_buffer_size;         /**< @brief Size of the RX buffer */
    length_t read_index;             /**< @brief Read index in the RX buffer */
} pusReceiveContext_t;

/**
 * @struct  pusExecutionContext_t
 * @brief   Struct type for execution context
 */
typedef struct
{
    pusStatus_t status;                  /**< @brief Execution context status */
    pusExecutionTable_t execution_table; /**< @brief Pointer to the execution table */
    bufferNo_t buffer_tc;                /**< @brief Buffer where the TC come from */
    bufferNo_t buffer_tm;                /**< @brief Buffer where the TM will be sent */
    bufferNo_t buffer_ack;               /**< @brief Buffer where the ACK TM will be sent */
    deviceNo_t dev_tc;                   /**< @brief Device bound to the TC buffer */
    deviceNo_t dev_tm;                   /**< @brief Device bound to the TM buffer */
    deviceNo_t dev_ack;                  /**< @brief Device bound to the ACK buffer */
} pusExecutionContext_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn          InitTCReceiveContext(pusReceiveContext_t *receive_context)
 * @brief       Function that initialise the receive context for TC handling
 * @param[in]   receive_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if a pointer is a null pointer
 * @retval      #RET_INVALID_PARAM if routing table size is zero
 * @retval      #RET_INVALID_PARAM if PUS receive buffer size is smaller than TC max size
 * @retval      #RET_INVALID_PARAM if buffer size is zero when rx_type is DEVICE_TYPE_PERIPHERAL
 * @retval      #RET_ERROR if initialisation failed because of device binding or execution table initialisation
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t InitTCReceiveContext(pusReceiveContext_t *receive_context);

/**
 * @fn          ReceiveTC(pusReceiveContext_t *receive_context)
 * @brief       Function that get a TC and and routes it toward it's corresponding task
 * @param[in]   receive_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if receive_context is not initialised
 * @retval      #RET_NOT_AVAILABLE if there is no TC available
 * @retval      #RET_ERROR if receiving the TC is not working
 * @retval      #RET_ERROR if cannot format TC
 * @retval      #RET_ERROR if cannot write TC into it's device
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t ReceiveTC(pusReceiveContext_t *receive_context);

/**
 * @fn          InitTCExecutionContext(pusExecutionContext_t *execution_context)
 * @brief       Function that initialise the execution context for TC handling
 * @param[in]   execution_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if a pointer is a null pointer or routing table size is null
 * @retval      #RET_ERROR if initialisation failed because of device binding or execution table initialisation
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t InitTCExecutionContext(pusExecutionContext_t *execution_context);

/**
 * @fn          ExecuteTC(pusExecutionContext_t *execution_context)
 * @brief       This function executes incoming TC.
 * @param[in]   execution_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if execution_context is empty or contains an empty field
 * @retval      #RET_ERROR if cannot recognize TC or has an error with device management
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteTC(pusExecutionContext_t *execution_context);

#endif /* TC_MANAGEMENT_H */

/**
 * @}
 * @}
 * @}
 */