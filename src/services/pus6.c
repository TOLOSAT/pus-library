/**
 * @file    pus6.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 6 functions (Memory management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tm_management.h"
#include "services/pus6.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn              ExecuteS6SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that load data to memory
 * @param[in,out]   env PUS6 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_INVALID_PARAM if cannot open the file (file does not exists)
 * @retval          #RET_ERROR if writting in the file does not work
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS6SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused Parameters
    (void)(env);
    (void)(tm);

    // Check parameter(s)
    if ((tc != NULL) && (error_code != NULL))
    {
        deviceNo_t temp_dev  = 0u;
        pus6Base_t load_base = 0u;
        pusNField_t N        = 0u;
        uint32_t offset      = sizeof(pus6Base_t) + sizeof(pusNField_t);

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get base and N (number of data)
        BIG_ENDIAN_ARRAY_TO_UINT16(tc->data, load_base);
        BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[sizeof(pus6Base_t)], N);

        // First open a device for this file
        returnCode_t test_fs = DeviceOpen(&temp_dev, DEVICE_TYPE_FILE, load_base);
        if (test_fs == RET_SUCCESSFUL)
        {
            // Get data from TC
            pusNField_t i = 0u;
            while ((return_value == RET_SUCCESSFUL) && (i < N))
            {
                // Check the offset is not out of the bound and offset and length can be read
                if ((offset + sizeof(pus6Offset_t) + sizeof(pus6Length_t)) < TC_MAX_DATA_SIZE)
                {
                    pus6Length_t load_length = 0u;
                    pus6Offset_t load_offset = 0u;

                    // Get data dump size and offset
                    BIG_ENDIAN_ARRAY_TO_UINT32(&tc->data[offset], load_offset);
                    BIG_ENDIAN_ARRAY_TO_UINT32(&tc->data[offset + sizeof(pus6Offset_t)], load_length);

                    // Then get data i size
                    uint32_t data_i_size = sizeof(pus6Offset_t) + sizeof(pus6Length_t) + load_length;

                    // Check the buffer won't be read or written out of the bound
                    if ((data_i_size <= MEMORY_TC_DATA_LOAD_MAX_SIZE) && ((offset + data_i_size) <= TC_MAX_DATA_SIZE))
                    {
                        uint8_t pus6_data[MEMORY_TC_DATA_LOAD_MAX_SIZE] = { 0 };
                        // Get data from TC
                        (void)memcpy((void *)&pus6_data, (void *)&tc->data[offset + sizeof(pus6Offset_t) + sizeof(pus6Length_t)], load_length);

                        // Move read/write pointer
                        test_fs = DeviceIoctl(temp_dev, IOCTL_FS_SEEK, &load_offset, sizeof(load_offset));
                        if (test_fs == RET_SUCCESSFUL)
                        {
                            // Write data into FS
                            test_fs = DeviceWrite(temp_dev, pus6_data, load_length);
                            if (test_fs != RET_SUCCESSFUL)
                            {
                                return_value = RET_ERROR;
                                *error_code  = PUS_EXECUTION_FAILED;
                            }
                        }
                        else
                        {
                            return_value = RET_ERROR;
                            *error_code  = PUS_EXECUTION_FAILED;
                        }

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

            // Then close the device anyway (to avoid blocking the resource)
            (void)DeviceClose(temp_dev);
        }
        else
        {
            return_value = RET_INVALID_PARAM;
            *error_code  = PUS_EXECUTION_FAILED;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              ExecuteS6SS3(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that dump data from memory
 * @param[in,out]   env PUS6 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S1SS8 TM
 * @retval          #RET_INVALID_PARAM if a pointer is NULL
 * @retval          #RET_INVALID_PARAM if cannot open the file (file does not exists)
 * @retval          #RET_ERROR if reading the file does not work
 * @retval          #RET_ERROR if cannot build the TM
 * @retval          #RET_SUCCESSFUL else
 */
returnCode_t ExecuteS6SS3(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value               = RET_SUCCESSFUL;
    pusData_t dumped_data[TM_MAX_DATA_SIZE] = { 0 };

    // Unused
    (void)(env);

    // Check parameter(s)
    if ((tc != NULL) && (tm != NULL) && (error_code != NULL))
    {
        deviceNo_t temp_dev       = 0u;
        pus6Base_t dump_base      = 0u;
        pusNField_t N             = 0u;
        uint32_t offset           = sizeof(pus6Base_t) + sizeof(pusNField_t);
        length_t dumped_data_size = 0u;

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get base and N (number of data)
        BIG_ENDIAN_ARRAY_TO_UINT16(tc->data, dump_base);
        BIG_ENDIAN_ARRAY_TO_UINT16(&tc->data[sizeof(pus6Base_t)], N);

        // Copy them to the TM data
        (void)memcpy(&dumped_data[dumped_data_size], tc->data, sizeof(pus6Base_t) + sizeof(pusNField_t));
        dumped_data_size += sizeof(pus6Base_t) + sizeof(pusNField_t);

        // First open a device for this file
        returnCode_t test_fs = DeviceOpen(&temp_dev, DEVICE_TYPE_FILE, dump_base);
        if (test_fs == RET_SUCCESSFUL)
        {
            // Get data from TC
            pusNField_t i = 0u;
            while ((return_value == RET_SUCCESSFUL) && (i < N))
            {
                // Check if the data offset and size will fit in the TM at this point
                if ((dumped_data_size + sizeof(pus6Offset_t) + sizeof(pus6Length_t)) <= TM_MAX_DATA_SIZE)
                {
                    pus6Length_t dump_length = 0u;
                    pus6Offset_t dump_offset = 0u;

                    // Get data dump size and offset
                    BIG_ENDIAN_ARRAY_TO_UINT32(&tc->data[offset], dump_offset);
                    BIG_ENDIAN_ARRAY_TO_UINT32(&tc->data[offset + sizeof(pus6Offset_t)], dump_length);

                    // Copy them to the TM data
                    (void)memcpy(&dumped_data[dumped_data_size], &tc->data[offset], sizeof(pus6Offset_t) + sizeof(pus6Length_t));
                    dumped_data_size += sizeof(pus6Offset_t) + sizeof(pus6Length_t);

                    // Check the data will fit in the TM at this point
                    if ((dumped_data_size + dump_length) <= TM_MAX_DATA_SIZE)
                    {
                        // Move read/write pointer
                        test_fs = DeviceIoctl(temp_dev, IOCTL_FS_SEEK, &dump_offset, sizeof(dump_offset));
                        if (test_fs == RET_SUCCESSFUL)
                        {
                            // Read data from FS
                            test_fs = DeviceRead(temp_dev, &dumped_data[dumped_data_size], dump_length);
                            if (test_fs == RET_SUCCESSFUL)
                            {
                                // Update dumped data size
                                dumped_data_size += dump_length;

                                // Update index and offset
                                offset += sizeof(pus6Offset_t) + sizeof(pus6Length_t);
                                i++;
                            }
                            else if (test_fs == RET_NOT_AVAILABLE)
                            {
                                // Can't read the file because the section does not exist.
                                return_value = RET_NOT_AVAILABLE;
                                *error_code  = PUS_EXECUTION_FAILED;
                            }
                            else
                            {
                                return_value = RET_ERROR;
                                *error_code  = PUS_EXECUTION_FAILED;
                            }
                        }
                        else
                        {
                            return_value = RET_ERROR;
                            *error_code  = PUS_EXECUTION_FAILED;
                        }
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

            // If data dumping went successful, send the TM
            if (return_value == RET_SUCCESSFUL)
            {
                return_value = BuildTM(tm, 6u, 4u, (pusData_t *)dumped_data, dumped_data_size);
            }

            // Then close the device anyway (to avoid blocking the resource)
            (void)DeviceClose(temp_dev);
        }
        else
        {
            return_value = RET_INVALID_PARAM;
            *error_code  = PUS_EXECUTION_FAILED;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
