/**
 * @file    pus15.c
 * @author  Matteo Planchet, Arthur Morain
 * @brief   Source file for PUS 15 functions (On-board storage and retrieval)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "services/pus15.h"

/***************************** Macros Definitions ****************************/

#define ZERO_FILLED_DATA_SIZE 512u /**< Size of zero filled data (used for reset purposes) */

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc InitS15
 */
returnCode_t InitS15(pus15Env_t *pus15_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    length_t file_size        = 0;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Initialise the devices
        return_value = DeviceOpen(&pus15_env->dev_pus15_index, DEVICE_TYPE_FILE, pus15_env->fil_pus15_index);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceOpen(&pus15_env->dev_pus15_data, DEVICE_TYPE_FILE, pus15_env->fil_pus15_data);
            if (return_value == RET_SUCCESSFUL)
            {
                // Check if pus15 files are complete
                return_value = DeviceIoctl(pus15_env->dev_pus15_index, IOCTL_FS_GET_SIZE, &file_size, sizeof(length_t));
                if (return_value == RET_SUCCESSFUL)
                {
                    if (file_size == PUS15_INDEX_TABLE_SIZE)
                    {
                        return_value = DeviceIoctl(pus15_env->dev_pus15_data, IOCTL_FS_GET_SIZE, &file_size, sizeof(length_t));
                        if (return_value == RET_SUCCESSFUL)
                        {
                            if (file_size == PUS15_DATA_TABLE_SIZE)
                            {
                                // Pus15 files are complete
                                return_value = RET_SUCCESSFUL;

                                // Read packet store info from index file header
                                pus15PacketStoreIndexInfo_t packet_store_info = { 0 };
                                // TODO need to set read cursor at the beginning of the file, idk how to rn
                                return_value = FsRead(pus15_env->fil_pus15_index, (data_t)&packet_store_info, sizeof(pus15PacketStoreIndexInfo_t));

                                if (return_value == RET_SUCCESSFUL)
                                {
                                    pus15_env->packet_store.packet_store_info = packet_store_info;
                                }
                                else
                                {
                                    return_value = RET_ERROR;
                                }
                            }
                            else
                            {
                                // Pus15 files are incomplete
                                return_value = ResetIndexAndData(pus15_env);
                            }
                        }
                    }
                    else
                    {
                        // Pus15 files are incomplete
                        return_value = ResetIndexAndData(pus15_env);
                    }
                }
            }
        }

        if (return_value == RET_SUCCESSFUL)
        {
            pus15_env->status = PUS_INITIALIZED;
        }
        else
        {
            pus15_env->status = PUS_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn             SetPacketStoreStatus(pus15Env_t *pus15_env, pus15PacketStoreStatus_t new_status)
 * @brief          Function that sets the PUS15 packet store status
 * @param[in,out]  pus15_env PUS15 environment
 * @param[in]      new_status New status to apply (PUS15_PACKET_STORE_ENABLED or PUS15_PACKET_STORE_DISABLED)
 * @retval         #RET_INVALID_PARAM if pus15_env is NULL
 * @retval         #RET_INVALID_PARAM if PUS15 is not initialized
 * @retval         #RET_SUCCESSFUL else
 */
static returnCode_t SetPacketStoreStatus(pus15Env_t *pus15_env, pus15PacketStoreStatus_t new_status)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Check if pus15 is initialized
        if (pus15_env->status == PUS_INITIALIZED)
        {
            // Set PUS15 status
            pus15_env->packet_store.packet_store_info.status = new_status;
        }
        else
        {
            return_value = RET_INVALID_PARAM;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn             SetPacketStoreType(pus15Env_t *pus15_env, pus15PacketStoreType_t new_type)
 * @brief          Function that sets the PUS15 packet store type
 * @param[in,out]  pus15_env PUS15 environment
 * @param[in]      new_type New type to apply (PUS15_PACKET_STORE_TYPE_CIRCULAR or PUS15_PACKET_STORE_TYPE_BOUNDED)
 * @retval         #RET_INVALID_PARAM if pus15_env is NULL
 * @retval         #RET_INVALID_PARAM if PUS15 is not initialized
 * @retval         #RET_SUCCESSFUL else
 */
static returnCode_t SetPacketStoreType(pus15Env_t *pus15_env, pus15PacketStoreType_t new_type)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Check if pus15 is initialized
        if (pus15_env->status == PUS_INITIALIZED)
        {
            // Set PUS15 type
            pus15_env->packet_store.packet_store_info.type = new_type;
        }
        else
        {
            return_value = RET_INVALID_PARAM;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

// TODO: when PUS11 is updated, make this function generic https://github.com/TOLOSAT/flight-software/issues/138
/**
 * @fn          ZeroFillDevice(deviceNo_t device, length_t total_size)
 * @brief       Fill a device file with zeros from the beginning
 * @param[in]   device Device to zero fill
 * @param[in]   total_size Total number of bytes to write
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ZeroFillDevice(deviceNo_t device, length_t total_size)
{
    returnCode_t return_value                      = RET_SUCCESSFUL;
    data_t zero_filled_data[ZERO_FILLED_DATA_SIZE] = { 0 };
    length_t origin                                = 0u;

    // Set read/write pointer to the beginning of the file
    return_value = DeviceIoctl(device, IOCTL_FS_SEEK, &origin, sizeof(origin));
    if (return_value == RET_SUCCESSFUL)
    {
        // Write 0s in the file
        length_t remaining_bytes = total_size;
        while ((return_value == RET_SUCCESSFUL) && (remaining_bytes > 0u))
        {
            if (remaining_bytes >= ZERO_FILLED_DATA_SIZE)
            {
                return_value     = DeviceWrite(device, (data_t)&zero_filled_data, ZERO_FILLED_DATA_SIZE);
                remaining_bytes -= ZERO_FILLED_DATA_SIZE;
            }
            else
            {
                return_value    = DeviceWrite(device, (data_t)&zero_filled_data, remaining_bytes);
                remaining_bytes = 0u;
            }
        }
    }

    return return_value;
}

/**
 * @fn              ResetIndexAndData(pus15Env_t *pus15_env)
 * @brief           This function reset index and data file (filling them with zeros)
 * @param[in,out]   pus15_env PUS15 environment
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t ResetIndexAndData(pus15Env_t *pus15_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus15_env != NULL)
    {
        // Zero fill index file
        return_value = ZeroFillDevice(pus15_env->dev_pus15_index, PUS15_INDEX_TABLE_SIZE);

        // Write packet store info in index file header
        if (return_value == RET_SUCCESSFUL)
        {
            pus15PacketStoreIndexInfo_t packet_store_info = { id                       = "MAIN",
                                                              type                     = PUS15_PACKET_STORE_TYPE_CIRCULAR,
                                                              status                   = PUS15_PACKET_STORE_DISABLED,
                                                              by_time_retrieval_status = PUS15_BY_TIME_RETRIEVAL_DISABLED,
                                                              length                   = PUS15_MAXIMUM_PACKET_STORE_LENGTH,
                                                              nb_entries               = 0u,
                                                              write_cursor             = 0u,
                                                              oldest_entry             = NULL };

            pus15_env->packet_store.packet_store_info = packet_store_info;

            // TODO need to set write cursor at the beginning of the file, idk how to rn
            return_value = FsWrite(pus15_env->fil_pus15_index, (data_t)&packet_store_info, sizeof(pus15PacketStoreIndexInfo_t));

            if (return_value == RET_SUCCESSFUL)
            {
                // Zero fill data file
                return_value = ZeroFillDevice(pus15_env->dev_pus15_data, PUS15_DATA_TABLE_SIZE);
            }
        }
    }

    return return_value;
}

/**
 * @copydoc ExecuteS15SS1
 */
returnCode_t ExecuteS15SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tc);
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Set packet store status to enabled
        return_value = SetPacketStoreStatus((pus15Env_t *)env, PUS15_PACKET_STORE_ENABLED);
        if (return_value != RET_SUCCESSFUL)
        {
            *error_code = PUS_EXECUTION_UNAVAILABLE;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS15SS2
 */
returnCode_t ExecuteS15SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tc);
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Set packet store status to disabled
        return_value = SetPacketStoreStatus((pus15Env_t *)env, PUS15_PACKET_STORE_DISABLED);
        if (return_value != RET_SUCCESSFUL)
        {
            *error_code = PUS_EXECUTION_UNAVAILABLE;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS15SS9
 */
returnCode_t ExecuteS15SS9(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus15Env_t *pus15_env = (pus15Env_t *)env;

        // Check if pus15 is initialized and packet store is enabled
        if ((pus15_env->status == PUS_INITIALIZED) && (pus15_env->packet_store.packet_store_info.status == PUS15_PACKET_STORE_ENABLED))
        {
            //! For now there is only one packet store, so we assume there's only one data entry in the TC.
            //! We assume N=1, so we skip the first 2 bytes (N field).
            pusData_t *packet_store_data = &tc->data[sizeof(pusNField_t)];

            // Retrieve packet store ID from TC data (since we only have one packet store, we don't check the ID for now)
            char packet_store_id[PUS15_PACKET_STORE_ID_SIZE] = { 0 };
            memcpy(packet_store_id, packet_store_data, PUS15_PACKET_STORE_ID_SIZE);

            // Retrieval priority not implemented yet, it's optional so we don't check it for now.

            // Retrieve from time (in CUC format)
            cucTime_t from_time = { 0 };
            memcpy(&from_time, &packet_store_data[PUS15_PACKET_STORE_ID_SIZE], sizeof(cucTime_t));

            // Retrieve stop time (in CUC format)
            cucTime_t stop_time = { 0 };
            memcpy(&stop_time, &packet_store_data[PUS15_PACKET_STORE_ID_SIZE + sizeof(cucTime_t)], sizeof(cucTime_t));

            // TODO: Implement the retrieval logic based on the from_time and stop_time, and send the TMs to the TmSender
            // basically:
            // start from the oldest entry in the packet store index
            // check each entry's timestamp,
            // and if it falls within the from_time and stop_time range,
            // retrieve the corresponding data from the packet store data file and send it to TmSender
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS15SS18
 */
returnCode_t ExecuteS15SS18(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value         = RET_SUCCESSFUL;
    sppHeader_t spp_header_buffer     = { 0 };
    pusData_t data[S15SS18_DATA_SIZE] = { 0 };

    // Check parameter(s)
    if ((tc != NULL) && (env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus15Env_t *pus15_env = (pus15Env_t *)env;

        // Check if pus15 is initialized
        if (pus15_env->status == PUS_INITIALIZED)
        {
            // Set up headers
            spp_header_buffer.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
            spp_header_buffer.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);

            // Set up data
            // N | Packet Store ID | Packet Store Status |
            // Packet Store Open Retrieval Status | Packet Store By Time Range Retrieval Status
            uint32_t index = 0u;

            // N = 1
            data[index++] = 1u;

            // Packet Store ID (8 bytes)
            memcpy(&data[index], pus15_env->packet_store.packet_store_info.id, PUS15_PACKET_STORE_ID_SIZE);

            index += PUS15_PACKET_STORE_ID_SIZE;

            // Packet Store Status
            data[index++] = (uint8_t)pus15_env->packet_store.packet_store_info.status;

            // Packet Store Open Retrieval Status
            // Currently always disabled
            data[index++] = 0u;

            // Packet Store By-Time-Range Retrieval Status
            data[index++] = (uint8_t)pus15_env->packet_store.packet_store_info.by_time_retrieval_status;

            // Build TM
            return_value = BuildTM(tm, 1u, 1u, (pusData_t *)&data, S15SS18_DATA_SIZE);
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS15SS26
 */
returnCode_t ExecuteS15SS26(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tc);
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Set packet store status to disabled
        return_value = SetPacketStoreType((pus15Env_t *)env, PUS15_PACKET_STORE_TYPE_CIRCULAR);
        if (return_value != RET_SUCCESSFUL)
        {
            *error_code = PUS_EXECUTION_UNAVAILABLE;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @copydoc ExecuteS11SS27
 */
returnCode_t ExecuteS15SS27(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(tc);
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Set packet store status to disabled
        return_value = SetPacketStoreType((pus15Env_t *)env, PUS15_PACKET_STORE_TYPE_BOUNDED);
        if (return_value != RET_SUCCESSFUL)
        {
            *error_code = PUS_EXECUTION_UNAVAILABLE;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
