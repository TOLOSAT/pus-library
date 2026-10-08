/**
 * @file    pus15.h
 * @author  Matteo Planchet, Arthur Morain
 * @brief   Header file for PUS 15 functions (On-board storage and retrieval)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus15 PUS Service 15
 * @brief PUS service 15 (On-board storage and retrieval) implementation
 * @{
 */

#ifndef PUS15_H
#define PUS15_H

/******************************* Include Files *******************************/

#include "pus_types.h"

/***************************** Macros Definitions ****************************/

#define PUS15_MAXIMUM_PACKET_STORE_LENGTH 100u /**< PUS15 maximum packet store length */
#define PUS15_INDEX_TABLE_SIZE \
    sizeof(pus15PacketStoreIndexInfo_t) + (PUS15_MAXIMUM_PACKET_STORE_LENGTH * sizeof(pus15PacketStoreIndexEntry_t)) /**< PUS15 index table size */
#define PUS15_DATA_TABLE_SIZE PUS15_MAXIMUM_PACKET_STORE_LENGTH * sizeof(TM_MAX_SIZE)                                /**< PUS15 data table size */

/***************************** Types Definitions *****************************/

/**
 * @enum    pus15Status_t
 * @brief   PUS 15 status enum
 */
typedef enum
{
    PUS15_DISABLED = 0u, /**< PUS15 is disabled */
    PUS15_ENABLED  = 1u, /**< PUS15 is enabled */
} pus15Status_t;

typedef enum
{
    PUS15_PACKET_STORE_DISABLED = 0u, /**< Packet store is disabled */
    PUS15_PACKET_STORE_ENABLED  = 1u, /**< Packet store is enabled */
} pus15PacketStoreStatus_t;

typedef enum
{
    PUS15_BY_TIME_RETRIEVAL_DISABLED = 0u, /**< By time retrieval is disabled */
    PUS15_BY_TIME_RETRIEVAL_ENABLED  = 1u, /**< By time retrieval is enabled */
} pus15PacketStoreByTimeRetrievalStatus_t;

typedef enum
{
    PUS15_PACKET_STORE_TYPE_CIRCULAR = 0u, /**< Packet store type is circular */
    PUS15_PACKET_STORE_TYPE_BOUNDED  = 1u, /**< Packet store type is bounded */
} pus15PacketStoreType_t;

typedef struct
{
    pus15PacketStoreType_t type;                                   /**< @brief Packet store type */
    pus15PacketStoreStatus_t status;                               /**< @brief Packet store status */
    pus15PacketStoreByTimeRetrievalStatus_t open_retrieval_status; /**< @brief Packet store open retrieval status */
    uint32_t length;                                               /**< @brief Packet store length */
    uint32_t nb_entries;                                           /**< @brief Packet store number of entries */
    uint32_t write_cursor;                                         /**< @brief Packet store write cursor */
    pus15PacketStoreIndexEntry_t *oldest_entry;                    /**< @brief Pointer to the oldest packet store index entry */
} ATTR_BYTE_ALIGNED pus15PacketStoreIndexInfo_t;

typedef uint8_t pus15Data_t[TM_MAX_SIZE];

typedef struct
{
    uint32_t timestamp;                 /**< @brief Packet store timestamp */
    pus15Data_t *data;                  /**< @brief Pointer to the packet store data */
    pus15PacketStoreIndexEntry_t *next; /**< @brief Pointer to the next packet store index entry */
} pus15PacketStoreIndexEntry_t;

typedef struct
{
    pus15PacketStoreIndexInfo_t packet_store_info;       /**< @brief Packet store information */
    pus15Data_t data[PUS15_MAXIMUM_PACKET_STORE_LENGTH]; /**< @brief Packet store data */
} ATTR_BYTE_ALIGNED pus15PacketStore_t;

/**
 * @struct  pus15Env_t
 * @brief   Struct type for pus15 environment
 */
typedef struct
{
    pusStatus_t status;              /**< @brief PUS15 environment status */
    pus15Status_t pus15_status;      /**< @brief PUS15 status */
    pus15PacketStore_t packet_store; /**< @brief Packet store */
    deviceNo_t dev_pus15_index;      /**< @brief Device bound to the pus15 index file */
    deviceNo_t dev_pus15_data;       /**< @brief Device bound to the pus15 data file */
    fileNo_t fil_pus15_index;        /**< @brief File where the pus15 index is stored */
    fileNo_t fil_pus15_data;         /**< @brief File where the pus15 data is stored */
} pus15Env_t;

/*************************** Variables Declarations **************************/

/**
 * @fn          InitS15(pus15Env_t *pus15_env)
 * @brief       This function initialises a pus15 environment
 * @param[in]   pus15_env PUS15 environment used for configuration
 * @retval      #RET_SUCCESSFUL always
 */
extern returnCode_t InitS15(pus15Env_t *pus15_env);

/**
 * @fn              ExecuteS15SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will enable the storage function of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS1 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will disable the storage function of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS2 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will start the by-time-range retrieval of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS9 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS11(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will delete the content of packet stores up to the specified time
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS11 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS11(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will abort the by-time-range retrieval of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS17 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS18(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will report the status of each packet store
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS18 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS18(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS26(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will change a packet store type to circular
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS26 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS26(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn             ExecuteS15SS27(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will change a packet store type to bounded
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS27 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS27(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

#endif /* PUS15_H */

/**
 * @}
 * @}
 * @}
 */
