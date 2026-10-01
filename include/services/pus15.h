/**
 * @file    pus15.h
 * @author  Matteo Planchet
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

typedef enum {
    PUS15_OPEN_RETRIEVAL_DISABLED = 0u, /**< Open retrieval is disabled */
    PUS15_OPEN_RETRIEVAL_ENABLED  = 1u, /**< Open retrieval is enabled */
} pus15PacketStoreOpenRetrievalStatus_t;

typedef enum
{
    PUS15_PACKET_STORE_TYPE_CIRCULAR = 0u, /**< Packet store type is circular */
    PUS15_PACKET_STORE_TYPE_BOUNDED  = 1u, /**< Packet store type is bounded */
} pus15PacketStoreType_t;

typedef struct 
{
    pus15PacketStoreType_t type; /**< @brief Packet store type */
    pus15PacketStoreStatus_t status; /**< @brief Packet store status */
    uint32_t size;               /**< @brief Packet store size */
} ATTR_BYTE_ALIGNED pus15PacketStoreInfo_t;

typedef struct {
    uint32_t index;     /**< @brief Packet store index */
    uint32_t timestamp; /**< @brief Packet store timestamp */
} pus15PacketStoreIndexEntry_t;


/**
 * @struct  pus15Env_t
 * @brief   Struct type for pus15 environment
 */
typedef struct
{
    pusStatus_t status;            /**< @brief PUS15 environment status */
    pus15Status_t pus15_status;    /**< @brief PUS15 status */
    pus15PacketStoreInfo_t packet_store_info; /**< @brief Packet store information */
    deviceNo_t dev_pus15_index;     /**< @brief Device bound to the pus15 index file */
    deviceNo_t dev_pus15_data;     /**< @brief Device bound to the pus15 data file */
    fileNo_t fil_pus15_index;      /**< @brief File where the pus15 index is stored */
    fileNo_t fil_pus15_data;       /**< @brief File where the pus15 data is stored */
} pus15Env_t;

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

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
 * @fn              ExecuteS15SS14(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will change the open retrieval start time tag of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS14 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS14(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

/**
 * @fn              ExecuteS15SS15(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will resume the open retrieval of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS15 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS15(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

// TC[15,16] suspend the open retrieval of packet stores 
/**
 * @fn              ExecuteS15SS16(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will suspend the open retrieval of packet stores
 * @param[in,out]   env PUS15 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S15SS16 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
extern returnCode_t ExecuteS15SS16(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code);

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
