/**
 * @file    pus11.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 11 functions (Time-based scheduling)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tm_management.h"
#include "services/pus11.h"
#include "tools/schedule_management.h"

/***************************** Macros Definitions ****************************/

#define ZERO_FILLED_DATA_SIZE 512u /**< Size of zero filled data (used for reset purposes) */

/*************************** Functions Declarations **************************/

static returnCode_t SetScheduleStatus(pus11Env_t *pus11_env, pus11Status_t new_status);
static returnCode_t TryPopNextTC(pus11Env_t *pus11_env, pusTC_t *delayed_tc, time_t *next_tc_release_date);
static returnCode_t AddSingleTC(pus11Env_t *pus11_env, uint8_t *tc_data, uint32_t offset, uint32_t data_size, pusExecutionError_t *error_code);
static returnCode_t AllocateDataSlot(pus11Env_t *pus11_env, pus11DataIndex_t *data_index);
static returnCode_t ResetScheduleAndData(pus11Env_t *pus11_env);
static returnCode_t ReadTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info);
static returnCode_t WriteTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info);
static returnCode_t ReadDataFromTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data, pus11DataIndex_t data_index);
static returnCode_t WriteDataToTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data, pus11DataIndex_t data_index);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          InitS11(pus11Env_t *pus11_env)
 * @brief       This function initialises a pus11 environment
 * @param[in]   pus11_env PUS11 environment used for configuration
 * @retval      #RET_SUCCESSFUL always
 */
returnCode_t InitS11(pus11Env_t *pus11_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    length_t file_size        = 0;

    // Check parameter(s)
    if (pus11_env != NULL)
    {
        // Then initialises the devices
        return_value = DeviceOpen(&pus11_env->dev_pus11_schedule, DEVICE_TYPE_FILE, pus11_env->fil_pus11_schedule);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceOpen(&pus11_env->dev_pus11_data, DEVICE_TYPE_FILE, pus11_env->fil_pus11_data);
            if (return_value == RET_SUCCESSFUL)
            {
                // Check if pus11 files are complete
                return_value = DeviceIoctl(pus11_env->dev_pus11_schedule, IOCTL_FS_GET_SIZE, &file_size, sizeof(length_t));
                if (return_value == RET_SUCCESSFUL)
                {
                    if (file_size == SCHEDULE_SIZE)
                    {
                        return_value = DeviceIoctl(pus11_env->dev_pus11_data, IOCTL_FS_GET_SIZE, &file_size, sizeof(length_t));
                        if (return_value == RET_SUCCESSFUL)
                        {
                            if (file_size == PUS11_DATA_TABLE_SIZE)
                            {
                                // Pus11 files are complete
                                return_value = RET_SUCCESSFUL;
                            }
                            else
                            {
                                // Pus11 files are incomplete
                                return_value = ResetScheduleAndData(pus11_env);
                            }
                        }
                    }
                    else
                    {
                        // Pus11 files are incomplete
                        return_value = ResetScheduleAndData(pus11_env);
                    }
                }
            }
        }

        // Then initialise delayed TC buffer device
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceOpen(&pus11_env->dev_delayed_tc, DEVICE_TYPE_BUFFER, pus11_env->buffer_delayed_tc);
        }

        if (return_value == RET_SUCCESSFUL)
        {
            pus11_env->status = PUS_INITIALIZED;
        }
        else
        {
            pus11_env->status = PUS_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          TryReleaseDelayedTC(pus11Env_t *pus11_env, time_t *next_tc_release_date)
 * @brief       Function that tries to pop next tc from schedule into tc buffer if found to be on time
 * @param[in]   pus11_env           PUS11 context used for configuration
 * @param[out]  next_tc_release_date    Next TC release date
 * @retval      #RET_NOT_AVAILABLE if no delayed TC is available
 * @retval      #RET_ERROR if schedule encountered an error
 * @retval      #RET_ERROR if device writting failed
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t TryReleaseDelayedTC(pus11Env_t *pus11_env, time_t *next_tc_release_date)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    pusTC_t delayed_tc        = { 0 };

    // Check parameter(s)
    if ((pus11_env != NULL) && (next_tc_release_date != NULL) && (pus11_env->status == PUS_INITIALIZED))
    {
        // Check if pus11 is enabled
        if (pus11_env->pus11_status == PUS11_ENABLE)
        {
            // Get delayed TC if there is any
            return_value = TryPopNextTC(pus11_env, &delayed_tc, next_tc_release_date);
            if (return_value == RET_SUCCESSFUL)
            {
                // Delayed TC available, send it to TC receiver
                return_value = DeviceWrite(pus11_env->dev_delayed_tc, (data_t)&delayed_tc, TC_MAX_SIZE);
                if (return_value == RET_SUCCESSFUL)
                {
                    taskNo_t tc_receiver = NO_TASK;
                    return_value         = DeviceIoctl(pus11_env->dev_delayed_tc, IOCTL_BUFFER_GET_RECEIVER, &tc_receiver, sizeof(taskNo_t));
                    if ((return_value == RET_SUCCESSFUL) && (tc_receiver != NO_TASK))
                    {
                        return_value = SendSignal(tc_receiver, SIGNAL_NEW_TC);
                    }
                }
            }
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
 * @fn              ExecuteS11SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will enable time based schedule
 * @param[in,out]   env PUS11 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S11SS1 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS11SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
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

        // Set schedule status to enabled
        return_value = SetScheduleStatus((pus11Env_t *)env, PUS11_ENABLE);
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
 * @fn              ExecuteS11SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will disable time based schedule
 * @param[in,out]   env PUS11 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S11SS2 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS11SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
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

        // Set schedule status to disabled
        return_value = SetScheduleStatus((pus11Env_t *)env, PUS11_DISABLE);
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
 * @fn              ExecuteS11SS3(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will reset time-based schedule
 * @param[in,out]   env PUS11 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S11SS3 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if cannot reset the schedule
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS11SS3(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
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

        // Get environment
        pus11Env_t *pus11_env = (pus11Env_t *)env;

        // Check if pus11 is initialized
        if (pus11_env->status == PUS_INITIALIZED)
        {
            // Reset pus11 files
            returnCode_t test_reset = ResetScheduleAndData(pus11_env);
            if (test_reset != RET_SUCCESSFUL)
            {
                return_value = RET_NOT_AVAILABLE;
                *error_code  = PUS_EXECUTION_FAILED;
            }
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
 * @fn              ExecuteS11SS4(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that will add N activities to a time based schedule
 * @param[in,out]   env PUS11 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S11SS4 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if the PUS11 has been disabled
 * @retval          #RET_NOT_AVAILABLE if the tc timestamp is outdated
 * @retval          #RET_ERROR if an error has been encountered related to the schedule
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS11SS4(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
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
        pus11Env_t *pus11_env = (pus11Env_t *)env;

        // Check if PUS11 is initialized and enabled
        if ((pus11_env->status == PUS_INITIALIZED) && (pus11_env->pus11_status == PUS11_ENABLE))
        {
            // Get the N number of data
            pusNField_t N   = ((pusNField_t)(tc->data[0]) << 8) | ((pusNField_t)(tc->data[1]));
            uint32_t offset = sizeof(pusNField_t);

            // Get data from TC
            pusNField_t i = 0u;
            while ((return_value == RET_SUCCESSFUL) && (i < N))
            {
                // Check the offset is not out of the bound and CUC and SPPHEAD can be read
                if ((offset + SPP_HEADER_SIZE + sizeof(cucTime_t)) < TC_MAX_DATA_SIZE)
                {
                    // First get data i size
                    uint32_t data_i_tc_size =
                        SPP_HEADER_SIZE
                        + (((uint32_t)(tc->data[offset + sizeof(cucTime_t) + 4u]) << 8) | ((uint32_t)(tc->data[offset + sizeof(cucTime_t) + 5u])))
                        + 1u; // TO DO : get a proper getter for TC size
                    uint32_t data_i_size = sizeof(cucTime_t) + data_i_tc_size;

                    // Check the buffer won't be read or written out of the bound
                    if ((data_i_size <= PUS11_ACTIVITY_DATA_MAX_SIZE) && ((offset + data_i_size) <= TC_MAX_DATA_SIZE))
                    {
                        // Add TC into schedule and data table
                        return_value = AddSingleTC(pus11_env, tc->data, offset, data_i_size, error_code);

                        // Update index and offset
                        offset += data_i_size;
                        i++;
                    }
                    else
                    {
                        return_value = RET_INVALID_PARAM;
                        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                    }
                }
                else
                {
                    return_value = RET_INVALID_PARAM;
                    *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
                }
            }
        }
        else
        {
            return_value = RET_NOT_AVAILABLE;
            *error_code  = PUS_EXECUTION_UNAVAILABLE;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          SetScheduleStatus(pus11Env_t *pus11_env, pus11Status_t new_status)
 * @brief       Function that sets the PUS11 schedule status
 * @param[in,out]   pus11_env PUS11 environment
 * @param[in]       new_status New status to apply (PUS11_ENABLE or PUS11_DISABLE)
 * @retval          #RET_INVALID_PARAM if pus11_env is NULL
 * @retval          #RET_INVALID_PARAM if PUS11 is not initialized
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t SetScheduleStatus(pus11Env_t *pus11_env, pus11Status_t new_status)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus11_env != NULL)
    {
        // Check if pus11 is initialized
        if (pus11_env->status == PUS_INITIALIZED)
        {
            // Set PUS11 status
            pus11_env->pus11_status = new_status;
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
 * @fn              TryPopNextTC(pus11Env_t *pus11_env, pusTC_t *delayed_tc, time_t *next_tc_release_date)
 * @brief           Get delayed TC if there is any available
 * @param[in,out]   pus11_env PUS11 environment
 * @param[out]      delayed_tc              Delayed TC that was freed
 * @param[out]      next_tc_release_date    Next TC release date
 * @retval          #RET_INVALID_PARAM if delayed_tc is null pointer
 * @retval          #RET_NOT_AVAILABLE if there is not delayed tc available
 * @retval          #RET_ERROR if an error occured
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t TryPopNextTC(pus11Env_t *pus11_env, pusTC_t *delayed_tc, time_t *next_tc_release_date)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (delayed_tc != NULL))
    {
        pusActivity_t freed_activity = { 0 };
        // Get last activity in schedule
        return_value = PopActivityInSchedule(pus11_env->dev_pus11_schedule, &freed_activity, next_tc_release_date);
        if (return_value == RET_SUCCESSFUL)
        {
            pus11Data_t pus11_data = { 0 };
            // Get data from file
            return_value = ReadDataFromTable(pus11_env, &pus11_data, freed_activity.data);
            if (return_value == RET_SUCCESSFUL)
            {
                // Now we are getting data from the data table
                (void)memcpy((void *)delayed_tc, (void *)&pus11_data.raw_data, PUS11_ACTIVITY_DATA_MAX_SIZE);

                // Then we free data
                (void)memset((void *)&pus11_data.raw_data, 0u, PUS11_ACTIVITY_DATA_MAX_SIZE);
                pus11_data.status = PUS11_DATA_AVAILABLE;

                // Send this updated data to file
                return_value = WriteDataToTable(pus11_env, &pus11_data, freed_activity.data);
                if (return_value == RET_SUCCESSFUL)
                {
                    pus11DataTableInfo_t pus11_table_info = { 0 };
                    // Get current info before update
                    return_value = ReadTableInfo(pus11_env, &pus11_table_info);
                    if (return_value == RET_SUCCESSFUL)
                    {
                        // Update data number
                        pus11_table_info.nb_data--;
                        return_value = WriteTableInfo(pus11_env, &pus11_table_info);
                    }
                }
            }
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          AddSingleTC(pus11Env_t *pus11_env, uint8_t *tc_data, uint32_t offset,
 *                                uint32_t data_size, pusExecutionError_t *error_code)
 * @brief       Function that adds a single tc to the schedule if its timestamp lies in future
 * @param[in,out]   pus11_env PUS11 environment
 * @param[in]       tc_data Raw TC data buffer
 * @param[in]       offset Current offset in tc_data
 * @param[in]       data_size Total size of this activity (timestamp + TC)
 * @param[out]      error_code Indicates which error has been encountered
 * @retval          #RET_NOT_AVAILABLE if the timestamp is outdated
 * @retval          #RET_ERROR if schedule or data table encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t AddSingleTC(pus11Env_t *pus11_env, uint8_t *tc_data, uint32_t offset, uint32_t data_size, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Get Current time
    time_t current_time = GetTime();

    // Check if requested timestamp is in the future
    time_t tc_timestamp = BIG_ENDIAN_ARRAY_TO_UINT64(&tc_data[offset]);

    if (current_time <= tc_timestamp)
    {
        pus11DataTableInfo_t pus11_table_info = { 0 };
        // Check if there is still data available
        return_value = ReadTableInfo(pus11_env, &pus11_table_info);
        if ((return_value == RET_SUCCESSFUL) && (pus11_table_info.nb_data < PUS11_MAXIMUM_DATA))
        {
            pus11DataIndex_t new_data_index = 0u;
            // Get a data slot
            return_value = AllocateDataSlot(pus11_env, &new_data_index);
            if (return_value == RET_SUCCESSFUL)
            {
                // Put incomming data in data struct
                pus11Data_t pus11_data = { 0 };
                (void)memcpy((void *)&pus11_data.raw_data, &tc_data[offset + sizeof(time_t)], data_size - sizeof(time_t));
                pus11_data.status = PUS11_DATA_UNAVAILABLE;
                // Send data to file
                return_value = WriteDataToTable(pus11_env, &pus11_data, new_data_index);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Create Activity based on TC data
                    pusActivity_t activity = { 0 };
                    activity.timestamp     = tc_timestamp;
                    activity.data          = new_data_index;
                    // Insert activity in schedule
                    return_value = PushActivityInSchedule(pus11_env->dev_pus11_schedule, &activity);
                    if (return_value != RET_SUCCESSFUL)
                    {
                        *error_code = PUS_EXECUTION_FAILED;
                    }
                }
                else
                {
                    *error_code = PUS_EXECUTION_FAILED;
                }
            }
            else
            {
                *error_code = PUS_EXECUTION_FAILED;
            }
        }
        else
        {
            *error_code = PUS_EXECUTION_FAILED;
        }
    }
    else
    {
        return_value = RET_NOT_AVAILABLE;
        *error_code  = PUS_EXECUTION_UNEXPECTED_DATA;
    }
    return return_value;
}

/**
 * @fn              AllocateDataSlot(pus11DataTable_t *data_table, pus11DataIndex_t *data_index)
 * @brief           This function gets the closest available data from the writing pointer
 * @param[in,out]   pus11_env PUS11 environment
 * @param[out]      data_index New data index
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_NOT_AVAILABLE if no data is available
 * @retval          #RET_ERROR if FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t AllocateDataSlot(pus11Env_t *pus11_env, pus11DataIndex_t *data_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (data_index != NULL))
    {
        pus11DataTableInfo_t pus11_table_info = { 0 };
        // First get table info
        returnCode_t test_val = ReadTableInfo(pus11_env, &pus11_table_info);
        if (test_val == RET_SUCCESSFUL)
        {
            // Initialize data variable and current write index
            pus11Data_t pus11_data               = { 0 };
            pus11DataIndex_t current_write_index = pus11_table_info.write_index;

            // Get data at current write index
            test_val = ReadDataFromTable(pus11_env, &pus11_data, current_write_index);

            // Find a new slot if current slot is not available
            while ((test_val == RET_SUCCESSFUL) && (pus11_data.status == (pus11DataIndex_t)PUS11_DATA_UNAVAILABLE)
                   && (current_write_index != pus11_table_info.write_index))
            {
                if (current_write_index == MAXIMUM_ACTIVITIES_PER_SCHEDULE)
                {
                    current_write_index = 0u;
                }
                else
                {
                    current_write_index++;
                }

                // Get New data slot
                test_val = ReadDataFromTable(pus11_env, &pus11_data, current_write_index);
            }

            // Check if no error occured
            if (test_val == RET_SUCCESSFUL)
            {
                // Make sure you haven't gone full circle
                if ((current_write_index == pus11_table_info.write_index) && (pus11_data.status == (pus11DataIndex_t)PUS11_DATA_UNAVAILABLE))
                {
                    return_value = RET_NOT_AVAILABLE;
                }
                else
                {
                    // Data index receive current index
                    *data_index = current_write_index;

                    // Update available info
                    pus11_table_info.write_index = current_write_index + 1u;
                    pus11_table_info.nb_data++;
                    // Send it to file
                    test_val = WriteTableInfo(pus11_env, &pus11_table_info);
                    if (test_val != RET_SUCCESSFUL)
                    {
                        return_value = RET_ERROR;
                    }
                }
            }
            else
            {
                return_value = RET_ERROR;
            }
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

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
 * @fn              ResetScheduleAndData(pus11Env_t *pus11_env)
 * @brief           This function reset schedule and data file (filling them with zeros)
 * @param[in,out]   pus11_env PUS11 environment
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t ResetScheduleAndData(pus11Env_t *pus11_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (pus11_env != NULL)
    {
        // Zero fill schedule file
        return_value = ZeroFillDevice(pus11_env->dev_pus11_schedule, SCHEDULE_SIZE);
        if (return_value == RET_SUCCESSFUL)
        {
            // Zero fill data file
            return_value = ZeroFillDevice(pus11_env->dev_pus11_data, PUS11_DATA_TABLE_SIZE);
        }
    }

    return return_value;
}

/**
 * @fn              ReadTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info)
 * @brief           Get PUS11 info from data table
 * @param[in,out]   pus11_env PUS11 environment
 * @param[out]      pus11_table_info Infos from pus11 table
 * @retval          #RET_INVALID_PARAM if a pointer is null
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t ReadTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (pus11_table_info != NULL))
    {
        length_t origin = 0u;
        // Move the read/write pointer to the beginning (where the info table is located)
        returnCode_t test_fs = DeviceIoctl(pus11_env->dev_pus11_data, IOCTL_FS_SEEK, &origin, sizeof(origin));
        if (test_fs == RET_SUCCESSFUL)
        {
            // Then read the info table
            test_fs = DeviceRead(pus11_env->dev_pus11_data, (data_t)pus11_table_info, PUS11_DATA_TABLE_INFO_SIZE);
            if (test_fs != RET_SUCCESSFUL)
            {
                return_value = RET_ERROR;
            }
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              WriteTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info)
 * @brief           Set PUS11 info from data table
 * @param[in,out]   pus11_env PUS11 environment
 * @param[in]       pus11_table_info Infos for pus11 table
 * @retval          #RET_INVALID_PARAM if a pointer is null
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t WriteTableInfo(pus11Env_t *pus11_env, pus11DataTableInfo_t *pus11_table_info)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (pus11_table_info != NULL))
    {
        length_t origin = 0u;
        // Move the read/write pointer to the beginning (where the info table is located)
        returnCode_t test_fs = DeviceIoctl(pus11_env->dev_pus11_data, IOCTL_FS_SEEK, &origin, sizeof(origin));
        if (test_fs == RET_SUCCESSFUL)
        {
            // Then write the info table
            test_fs = DeviceWrite(pus11_env->dev_pus11_data, (data_t)pus11_table_info, PUS11_DATA_TABLE_INFO_SIZE);
            if (test_fs != RET_SUCCESSFUL)
            {
                return_value = RET_ERROR;
            }
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              ReadDataFromTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data, pus11DataIndex_t data_index)
 * @brief           Get PUS11 data from data table
 * @param[in,out]   pus11_env PUS11 environment
 * @param[out]      pus11_data Data from pus11 table
 * @param[in]       data_index Data index
 * @retval          #RET_INVALID_PARAM if a pointer is null
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t ReadDataFromTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data, pus11DataIndex_t data_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (pus11_data != NULL))
    {
        // Move the read/write pointer to the desired data field
        length_t offset = PUS11_DATA_TABLE_INFO_SIZE + (data_index * PUS11_MAXIMUM_DATA_SIZE); // cppcheck-suppress misra-c2012-10.7; False positive,
                                                                                               // there is no wider type arithmetic conversion,
                                                                                               // (data_index * PUS11_MAXIMUM_DATA_SIZE) is a uint32_t
        returnCode_t test_fs = DeviceIoctl(pus11_env->dev_pus11_data, IOCTL_FS_SEEK, &offset, sizeof(offset));
        if (test_fs == RET_SUCCESSFUL)
        {
            // Then read data in table
            test_fs = DeviceRead(pus11_env->dev_pus11_data, (data_t)pus11_data, PUS11_MAXIMUM_DATA_SIZE);
            if (test_fs != RET_SUCCESSFUL)
            {
                return_value = RET_ERROR;
            }
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              WriteDataToTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data)
 * @brief           Set PUS11 data from data table
 * @param[in,out]   pus11_env PUS11 environment
 * @param[in]       pus11_data Data for pus11 table
 * @param[in]       data_index Data index
 * @retval          #RET_INVALID_PARAM if a pointer is null
 * @retval          #RET_ERROR if write in FS has encountered an error
 * @retval          #RET_SUCCESSFUL else
 */
static returnCode_t WriteDataToTable(pus11Env_t *pus11_env, pus11Data_t *pus11_data, pus11DataIndex_t data_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((pus11_env != NULL) && (pus11_data != NULL))
    {
        // Move the read/write pointer to the desired data field
        length_t offset = PUS11_DATA_TABLE_INFO_SIZE + (data_index * PUS11_MAXIMUM_DATA_SIZE); // cppcheck-suppress misra-c2012-10.7; False positive,
                                                                                               // there is no wider type arithmetic conversion,
                                                                                               // (data_index * PUS11_MAXIMUM_DATA_SIZE) is a uint32_t
        returnCode_t test_fs = DeviceIoctl(pus11_env->dev_pus11_data, IOCTL_FS_SEEK, &offset, sizeof(offset));
        if (test_fs == RET_SUCCESSFUL)
        {
            // Then write data in table
            test_fs = DeviceWrite(pus11_env->dev_pus11_data, (data_t)pus11_data, PUS11_MAXIMUM_DATA_SIZE);
            if (test_fs != RET_SUCCESSFUL)
            {
                return_value = RET_ERROR;
            }
        }
        else
        {
            return_value = RET_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
