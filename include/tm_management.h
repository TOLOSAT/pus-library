/**
 * @file    tm_management.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for TM management
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup tm_management TM management
 * @brief PUS telemetry (TM) management layer.
 * @{
 */

#ifndef TM_MANAGEMENT_H
#define TM_MANAGEMENT_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

/***************************** Types Definitions *****************************/

/*************************** Variables Declarations **************************/

/**
 * @struct  pusSendTable_t
 * @brief   Struct type for send table
 */
typedef struct
{
    bufferNo_t buffer;     /**< @brief Buffer reference number from where the TM arrives */
    deviceNo_t dev_buffer; /**< @brief Device bound to the TM buffer */
} pusSendTable_t;

/**
 * @struct  pusSendContext_t
 * @brief   Struct type for execution context
 */
typedef struct
{
    pusStatus_t status;             /**< @brief Execution context status */
    pusSendTable_t *send_table;     /**< @brief Pointer to the send table */
    pusTableSize_t send_table_size; /**< @brief Size of the execution table */
    uint32_t ref_tx;                /**< @brief resource where the TM will be sent */
    deviceType_t tx_type;           /**< @brief Type of resource used to send the TM (e.g. buffer or peripheral) */
    deviceNo_t dev_tx;              /**< @brief Device bound to the TX resource */
    pusTM_t *tm;                    /**< @brief Pointer to a TM data field in the case it needs to be allocated in the DMA section */
} pusSendContext_t;

/*************************** Functions Declarations **************************/

/**
 * @fn          InitTMSendContext(pusSendContext_t *send_context)
 * @brief       Function that initialise the send context for TM sending
 * @param[in]   send_context Execution context for the task dealing with TM sending
 * @retval      #RET_INVALID_PARAM if a pointer is a null pointer or send table size is null
 * @retval      #RET_ERROR if initialisation failed because of device binding
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t InitTMSendContext(pusSendContext_t *send_context);

/**
 * @fn          SendTM(pusSendContext_t *send_context)
 * @brief       Function that send reads incoming TM from the entry buffers and send them
 * @param[in]   send_context Send context for the task dealing with TM sending
 * @retval      #RET_INVALID_PARAM if send_context is not initialised
 * @retval      #RET_ERROR if cannot read entry buffers
 * @retval      #RET_ERROR if cannot send TM in the TX device
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t SendTM(pusSendContext_t *send_context);

/**
 * @fn          BuildTM(pusTM_t *tm, pusService_t service, pusSubService_t subservice, pusData_t *data, uint16_t data_size)
 * @brief       Function that build a TM.
 * @param[out]  tm Pointer to the TM we want to create
 * @param[in]   service PUS Service of TM.
 * @param[in]   subservice PUS Subservice of TM.
 * @param[in]   data Data Packet.
 * @param[in]   data_size Size of data packet.
 * @retval      #RET_INVALID_PARAM if tm is null pointer or service or subservice equal to 0
 * @retval      #RET_ERROR if cannot fill time field
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildTM(pusTM_t *tm, pusService_t service, pusSubService_t subservice, pusData_t *data, uint16_t data_size);

/**
 * @fn              FormatTM(pusTM_t *tm)
 * @brief           Function that format tm the right way
 * @param[in,out]   tm Pointer to the TM we want to format
 * @retval          #RET_INVALID_PARAM if tm is null pointer
 * @retval          #RET_SUCCESSFUL else
 *
 * As we work we little endian processors, but the TM and TM are big endian
 * formated, we need to swap to big endian before send the TM.
 *
 * @warning This function wont format TM data field, it has to be format before.
 */
extern returnCode_t FormatTM(pusTM_t *tm);

/**
 * @fn              EraseTM(pusTM_t *tm)
 * @brief           Function that erase a TM, it fills it with zeros
 * @param[in,out]   tm Pointer to the TM we want to erase
 * @return          Nothing
 */
extern void EraseTM(pusTM_t *tm);

#endif /* TM_MANAGEMENT_H */

/**
 * @}
 * @}
 * @}
 */