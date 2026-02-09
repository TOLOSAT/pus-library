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

static returnCode_t BuildS6SS4(pusTM_t *tm, pusTMDumpDataField_t *memory_dump);

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
    returnCode_t return_value           = RET_SUCCESSFUL;
    pusTCDumpDataField_t requested_data = { 0 };
    pusTMDumpDataField_t dumped_data    = { 0 };

    // Unused
    (void)(env);

    // Check parameter(s)
    if ((tc != NULL) && (tm != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get the N number of data
        pus6Base_t base;
        BIG_ENDIAN_ARRAY_TO_UINT16(tc->data, base);

        // First get data from TC
        (void)memcpy((void *)&requested_data, (void *)tc->data, MEMORY_TC_DATA_DUMP_SIZE);
        // Swip Endianness
        requested_data.offset = WORD_BYTE_SWAP(requested_data.offset);
        requested_data.length = WORD_BYTE_SWAP(requested_data.length);

        // First open a device for this file
        deviceNo_t temp_dev  = 0u;
        returnCode_t test_fs = DeviceOpen(&temp_dev, DEVICE_TYPE_FILE, base);
        if (test_fs == RET_SUCCESSFUL)
        {
            // Move read/write pointer
            test_fs = DeviceIoctl(temp_dev, IOCTL_FS_SEEK, &requested_data.offset, sizeof(requested_data.offset));
            if (test_fs == RET_SUCCESSFUL)
            {
                // Read data from FS
                test_fs = DeviceRead(temp_dev, dumped_data.data, requested_data.length);
                if (test_fs == RET_SUCCESSFUL)
                {
                    // Update data an build TM
                    dumped_data.offset      = requested_data.offset;
                    dumped_data.length      = requested_data.length;
                    returnCode_t test_build = BuildS6SS4(tm, &dumped_data);
                    if (test_build != RET_SUCCESSFUL)
                    {
                        return_value = RET_ERROR;
                        *error_code  = PUS_EXECUTION_TM_BUILDING_FAILED;
                    }
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
 * @fn          BuildS6SS4(pusTM_t *tm, pusTMDumpDataField_t *memory_dump)
 * @brief       Function that send S6SS4 TM (Memory Dump)
 * @param[out]  tm TM that will be sent
 * @param[in]   memory_dump Data dumped that will be send
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t BuildS6SS4(pusTM_t *tm, pusTMDumpDataField_t *memory_dump)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tm != NULL) && (memory_dump != NULL))
    {
        // Compute size
        uint16_t data_size = MEMORY_BASE_SIZE + MEMORY_OFFSET_SIZE + MEMORY_LENGTH_SIZE + memory_dump->length;

        // Swip Endianness
        memory_dump->offset = WORD_BYTE_SWAP(memory_dump->offset);
        memory_dump->length = WORD_BYTE_SWAP(memory_dump->length);

        // Build TM
        return_value = BuildTM(tm, 6u, 4u, (pusData_t *)memory_dump, data_size);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}