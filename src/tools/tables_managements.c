/**
 * @file    tables_management.h
 * @author  Merlin Kooshmanian
 * @brief   Source file for execution or routing tables
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include "kernel.h"
#include "tools/tables_management.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          InitRoutingTable(pusRoutingTable_t *routing_table)
 * @brief       Check if the table is ordered from smallest to largest key
 * @param[in]   routing_table routing table we want to check
 * @retval      #RET_ERROR if the table is not ordered from smallest to largest key or cannot open device
 * @retval      #RET_INVALID_PARAM if table size is 0 or if table is null pointer
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitRoutingTable(pusRoutingTable_t *routing_table)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((routing_table != NULL) && (routing_table->size > 0u))
    {
        uint32_t i        = 0u;
        uint32_t last_key = 0u;
        return_value      = RET_SUCCESSFUL;
        while ((i < routing_table->size) && (routing_table->entries[i].key > last_key) && (return_value == RET_SUCCESSFUL))
        {
            // Open the device for the route
            return_value = DeviceOpen(&routing_table->entries[i].dev_route, DEVICE_TYPE_BUFFER, routing_table->entries[i].route);

            // Update last key
            last_key = routing_table->entries[i].key;
            i++;
        }
        // Checks whether the entire table has been browsed
        // and if every device has been correctly opened.
        // If this is not the case, the table is not ordered.
        if ((i != routing_table->size) && (return_value == RET_SUCCESSFUL))
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
 * @fn          InitExecutionTable(pusExecutionTable_t *execution_table)
 * @brief       Check if the table is ordered from smallest to largest key
 * @param[in]   execution_table Execution table we want to check
 * @retval      #RET_ERROR if the table is not ordered from smallest to largest key
 * @retval      #RET_INVALID_PARAM if table size is 0 or if table is null pointer
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitExecutionTable(pusExecutionTable_t *execution_table)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((execution_table->entries != NULL) && (execution_table->size > 0u))
    {
        uint32_t i        = 0u;
        uint32_t last_key = 0u;
        while ((i < execution_table->size) && (execution_table->entries[i].key > last_key))
        {
            last_key = execution_table->entries[i].key;
            i++;
        }
        // Checks whether the entire table has been browsed.
        // If this is not the case, the table is not ordered.
        if (i != execution_table->size)
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
 * @fn          RouteSearch(uint32_t key, pusRoutingTable_t *routing_table, pusRoutingTableEntry_t **entry)
 * @brief       This function search for route in routing table with a key
 * @param[in]   key Key that help us to find the route.
 * @param[in]   routing_table Routing table where we search the route
 * @param[out]  entry Entry we are looking for
 * @retval      #RET_NOT_AVAILABLE if key does not exist in routing table
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t RouteSearch(uint32_t key, pusRoutingTable_t *routing_table, pusRoutingTableEntry_t **entry)
{
    returnCode_t return_value = RET_NOT_AVAILABLE;
    pusTableSize_t left       = 0u;
    pusTableSize_t right      = routing_table->size - 1u;
    pusTableSize_t cursor     = left + (right - left) / 2u;

    // Perform a binary search
    while ((left <= right) && (right < routing_table->size) && (return_value != RET_SUCCESSFUL))
    {
        if (routing_table->entries[cursor].key == key)
        {
            *entry = &routing_table->entries[cursor];
            return_value = RET_SUCCESSFUL;
        }
        else if (routing_table->entries[cursor].key < key)
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
 * @fn          ExecutionSearch(uint32_t key, pusExecutionTable_t *execution_table, pusExecutionTableEntry_t **entry)
 * @brief       This function search for execution function in execution table with a key
 * @param[in]   key Key that help us to find the route.
 * @param[in]   execution_table Execution table where we search the function to execute
 * @param[out]  entry Entry we are looking for
 * @retval      #RET_NOT_AVAILABLE if key does not exist in routing table
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ExecutionSearch(uint32_t key, pusExecutionTable_t *execution_table, pusExecutionTableEntry_t **entry)
{
    returnCode_t return_value = RET_NOT_AVAILABLE;
    pusTableSize_t left       = 0u;
    pusTableSize_t right      = execution_table->size - 1u;
    pusTableSize_t cursor     = left + (right - left) / 2u;

    // Perform a binary search
    while ((left <= right) && (right < execution_table->size) && (return_value != RET_SUCCESSFUL))
    {
        if (execution_table->entries[cursor].key == key)
        {
            *entry = &execution_table->entries[cursor];
            return_value            = RET_SUCCESSFUL;
        }
        else if (execution_table->entries[cursor].key < key)
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
