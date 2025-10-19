/**
 * @file    tables_management.h
 * @author  Merlin Kooshmanian
 * @brief   Source file for execution or routing tables
 *
 * @copyright Copyright (c) TOLOSAT 2025
 */

/******************************* Include Files *******************************/

#include "kernel.h"
#include "tools/tables_management.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          InitRoutingTable(pusRoutingTable_t *routing_table, pusTableSize_t table_size)
 * @brief       Check if the table is ordered from smallest to largest key
 * @param[in]   routing_table routing table we want to check
 * @param[in]   table_size Size of the routing table
 * @retval      #RET_ERROR if the table is not ordered from smallest to largest key or cannot open device
 * @retval      #RET_INVALID_PARAM if table size is 0 or if table is null pointer
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitRoutingTable(pusRoutingTable_t *routing_table, pusTableSize_t table_size)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((routing_table != NULL) && (table_size > 0u))
    {
        uint32_t i        = 0u;
        uint32_t last_key = 0u;
        return_value      = RET_SUCCESSFUL;
        while ((i < table_size) && (routing_table[i].key > last_key) && (return_value == RET_SUCCESSFUL))
        {
            // Open the device for the route
            return_value = DeviceOpen(&routing_table[i].dev_route, DEVICE_TYPE_BUFFER, routing_table[i].route);

            // Update last key
            last_key = routing_table[i].key;
            i++;
        }
        // Checks whether the entire table has been browsed
        // and if every device has been correctly opened.
        // If this is not the case, the table is not ordered.
        if ((i != table_size) && (return_value == RET_SUCCESSFUL))
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
 * @fn          InitExecutionTable(pusExecutionTable_t *execution_table, pusTableSize_t table_size)
 * @brief       Check if the table is ordered from smallest to largest key
 * @param[in]   execution_table Execution table we want to check
 * @param[in]   table_size Size of the execution table
 * @retval      #RET_ERROR if the table is not ordered from smallest to largest key
 * @retval      #RET_INVALID_PARAM if table size is 0 or if table is null pointer
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitExecutionTable(pusExecutionTable_t *execution_table, pusTableSize_t table_size)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((execution_table != NULL) && (table_size > 0u))
    {
        uint32_t i        = 0u;
        uint32_t last_key = 0u;
        while ((i < table_size) && (execution_table[i].key > last_key))
        {
            last_key = execution_table[i].key;
            i++;
        }
        // Checks whether the entire table has been browsed.
        // If this is not the case, the table is not ordered.
        if (i != table_size)
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
 * @fn          RouteSearch(pusRoutingTable_t *routing_table, pusTableSize_t table_size, uint32_t key, deviceNo_t *dev_route)
 * @brief       This function search for route in routing table with a key
 * @param[in]   routing_table Routing table where we search the route
 * @param[in]   table_size Size of the routing table
 * @param[in]   key Key that help us to find the route.
 * @param[out]  dev_route Device route we are looking for
 * @retval      #RET_NOT_AVAILABLE if key does not exist in routing table
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t RouteSearch(pusRoutingTable_t *routing_table, pusTableSize_t table_size, uint32_t key, deviceNo_t *dev_route) // TO DO : just return the
                                                                                                                           // line and not dev_route
{
    returnCode_t return_value = RET_NOT_AVAILABLE;
    pusTableSize_t left       = 0u;
    pusTableSize_t right      = table_size - 1u;
    pusTableSize_t cursor     = left + (right - left) / 2u;

    // Perform a binary search
    while ((left <= right) && (right < table_size) && (return_value != RET_SUCCESSFUL))
    {
        if (routing_table[cursor].key == key)
        {
            *dev_route   = routing_table[cursor].dev_route;
            return_value = RET_SUCCESSFUL;
        }
        else if (routing_table[cursor].key < key)
        {
            left   = cursor + 1u;
            cursor = left + (right - left) / 2u;
        }
        else
        {
            right  = cursor - 1u;
            cursor = left + (right - left) / 2u;
        }
    }

    return return_value;
}

/**
 * @fn          ExecutionSearch(pusExecutionTable_t *execution_table, pusTableSize_t table_size, uint32_t key, pusTMRequested_t *tm_requested,
 * pusExecutionFunctionPtr_t *execution_function_ptr, void** env)
 * @brief       This function search for execution function in execution table with a key
 * @param[in]   execution_table Execution table where we search the function to execute
 * @param[in]   table_size Size of the execution table
 * @param[in]   key Key that help us to find the route.
 * @param[out]  tm_requested Indicates if a specific TM has to be send for this TC
 * @param[out]  execution_function_ptr Pointer to the function we want to execute
 * @param[out]  env Pointer to the service environement
 * @retval      #RET_NOT_AVAILABLE if key does not exist in routing table
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ExecutionSearch(pusExecutionTable_t *execution_table, pusTableSize_t table_size, uint32_t key, pusTMRequested_t *tm_requested,
                             pusExecutionFunctionPtr_t *execution_function_ptr, void **env) // TO DO : just return the line and not every parameters
                                                                                            // (tm_requested, execution_function_ptr, env)
{
    returnCode_t return_value = RET_NOT_AVAILABLE;
    pusTableSize_t left       = 0u;
    pusTableSize_t right      = table_size - 1u;
    pusTableSize_t cursor     = left + (right - left) / 2u;

    // Perform a binary search
    while ((left <= right) && (right < table_size) && (return_value != RET_SUCCESSFUL))
    {
        if (execution_table[cursor].key == key)
        {
            *execution_function_ptr = execution_table[cursor].execution_function;
            *tm_requested           = execution_table[cursor].tm_requested;
            *env                    = execution_table[cursor].env;
            return_value            = RET_SUCCESSFUL;
        }
        else if (execution_table[cursor].key < key)
        {
            left   = cursor + 1u;
            cursor = left + (right - left) / 2u;
        }
        else
        {
            right  = cursor - 1u;
            cursor = left + (right - left) / 2u;
        }
    }

    return return_value;
}
