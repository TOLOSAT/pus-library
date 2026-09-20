/**
 * @file    schedule_management.h
 * @author  Merlin Kooshmanian
 * @brief   Source file for schedules
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "utils/schedule_management.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

static returnCode_t GetAvailableNode(deviceNo_t schedule_deviceno, pusNodeIndex_t *available_node_index);
static returnCode_t InsertNodeInSchedule(deviceNo_t schedule_deviceno, pusActivity_t *activity, pusNodeIndex_t new_node_index);
static returnCode_t ReleaseNextActivity(deviceNo_t schedule_deviceno, pusActivity_t *activity);
static returnCode_t InsertNodeInBetween(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info, pusActivity_t *activity,
                                        pusNodeIndex_t new_node_index, pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index);
static returnCode_t BuildAndWriteNewNode(deviceNo_t schedule_deviceno, pusNodeIndex_t new_node_index, pusActivity_t *activity,
                                         pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index);
static returnCode_t ReadScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info);
static returnCode_t WriteScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info);
static returnCode_t ReadNodeFromSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index);
static returnCode_t WriteNodeToSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc PushActivityInSchedule
 */
returnCode_t PushActivityInSchedule(deviceNo_t schedule_deviceno, pusActivity_t *activity)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity != NULL)
    {
        pusScheduleInfo_t schedule_info = { 0 };
        // Check if there is still room in schedule
        return_value = ReadScheduleInfo(schedule_deviceno, &schedule_info);
        if (return_value == RET_SUCCESSFUL)
        {
            if (schedule_info.nb_activities < MAXIMUM_ACTIVITIES_PER_SCHEDULE)
            {
                pusNodeIndex_t new_node_index = 0u;
                // Get a node
                return_value = GetAvailableNode(schedule_deviceno, &new_node_index);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Insert New node in schedule
                    return_value = InsertNodeInSchedule(schedule_deviceno, activity, new_node_index);
                }
            }
            else
            {
                return_value = RET_NOT_AVAILABLE;
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
 * @copydoc PopActivityInSchedule
 */
returnCode_t PopActivityInSchedule(deviceNo_t schedule_deviceno, pusActivity_t *activity, time_t *next_activity_date)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity != NULL)
    {
        pusScheduleInfo_t schedule_info = { 0 };
        // Get Schedule info
        return_value = ReadScheduleInfo(schedule_deviceno, &schedule_info);
        if (return_value == RET_SUCCESSFUL)
        {
            // Check if there is an activity in schedule
            if (schedule_info.nb_activities != 0u)
            {
                time_t current_time = GetTime();
                // Get current time
                pusActivityNode_t oldest_node = { 0 };
                // Get oldest node
                return_value = ReadNodeFromSchedule(schedule_deviceno, &oldest_node, schedule_info.oldest_activity_index);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Now check if oldest node can be released or not
                    if (oldest_node.activity.timestamp <= current_time)
                    {
                        // It means that oldest_node_time <= current_time so we can release activity
                        return_value = ReleaseNextActivity(schedule_deviceno, activity);
                        if (return_value == RET_SUCCESSFUL)
                        {
                            // Update next_activity_date if non null
                            if (next_activity_date != NULL)
                            {
                                // From the previous nb_activities (-1 because an activity has been pop) check if there is still an activity
                                if ((schedule_info.nb_activities - 1u) > 0u)
                                {
                                    return_value = ReadNodeFromSchedule(schedule_deviceno, &oldest_node, oldest_node.next_node_index);
                                    if (return_value == RET_SUCCESSFUL)
                                    {
                                        *next_activity_date = oldest_node.activity.timestamp;
                                    }
                                }
                                else
                                {
                                    // Update next_activity_date with INVALID TIME (because no next activity)
                                    *next_activity_date = INVALID_TIME;
                                }
                            }
                        }
                    }
                    else
                    {
                        // It means that oldest_node_time > current_time so we cannot release activity
                        return_value = RET_NOT_AVAILABLE;

                        // Update next_activity_date if non null
                        if (next_activity_date != NULL)
                        {
                            *next_activity_date = oldest_node.activity.timestamp;
                        }
                    }
                }
            }
            else
            {
                // No activities available in schedule
                return_value = RET_NOT_AVAILABLE;

                // Update next_activity_date if non null
                if (next_activity_date != NULL)
                {
                    // Update next_activity_date with INVALID TIME (because no next activity)
                    *next_activity_date = INVALID_TIME;
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
 * @fn          GetAvailableNode(deviceNo_t schedule_deviceno, pusNodeIndex_t *available_node_index)
 * @brief       This function gets the closest available node from the writing pointer
 * @param[in]   schedule_deviceno Schedule file number from which a new node is taken
 * @param[out]  available_node_index New node index
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if no node is available
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t GetAvailableNode(deviceNo_t schedule_deviceno, pusNodeIndex_t *available_node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (available_node_index != NULL)
    {
        pusScheduleInfo_t schedule_info = { 0 };
        // First get table info
        return_value = ReadScheduleInfo(schedule_deviceno, &schedule_info);
        if (return_value == RET_SUCCESSFUL)
        {
            // Initialize data variable and current write index
            pusActivityNode_t activity_node    = { 0 };
            pusNodeIndex_t current_write_index = schedule_info.write_index;

            do
            {
                // Get node at current write index
                return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, current_write_index);

                if ((return_value == RET_SUCCESSFUL) && (activity_node.status == (uint16_t)ACTIVITY_NODE_OCCUPIED))
                {
                    // Advance to the next slot, wrapping at the end of the buffer
                    if (current_write_index >= (MAXIMUM_ACTIVITIES_PER_SCHEDULE - 1u))
                    {
                        current_write_index = 0u;
                    }
                    else
                    {
                        current_write_index++;
                    }
                }
            } while ((return_value == RET_SUCCESSFUL) && (activity_node.status == (uint16_t)ACTIVITY_NODE_OCCUPIED)
                     && (current_write_index != schedule_info.write_index));

            // Check if no error occured
            if (return_value == RET_SUCCESSFUL)
            {
                // Make sure you haven't gone full circle
                if (activity_node.status == (uint16_t)ACTIVITY_NODE_OCCUPIED)
                {
                    return_value = RET_ERROR;
                }
                else
                {
                    // Available node index receive current index
                    *available_node_index = current_write_index;

                    // Update available info, wrapping at the end of the buffer
                    if (current_write_index >= (MAXIMUM_ACTIVITIES_PER_SCHEDULE - 1u))
                    {
                        schedule_info.write_index = 0u;
                    }
                    else
                    {
                        schedule_info.write_index = current_write_index + 1u;
                    }
                    // Send it to file
                    return_value = WriteScheduleInfo(schedule_deviceno, &schedule_info);
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
 * @fn          InsertNodeInSchedule(deviceNo_t schedule_deviceno, pusActivity_t *activity, pusNodeIndex_t new_node_index)
 * @brief       This function insert new activity in schedule
 * @param[in]   schedule_deviceno Schedule file number in which the activity is inserted
 * @param[in]   activity New activity content to insert
 * @param[in]   new_node_index New node index
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if the schedule was walked without finding an insertion point
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t InsertNodeInSchedule(deviceNo_t schedule_deviceno, pusActivity_t *activity, pusNodeIndex_t new_node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity != NULL)
    {
        pusScheduleInfo_t schedule_info = { 0 };
        // Get schedule info
        return_value = ReadScheduleInfo(schedule_deviceno, &schedule_info);
        if (return_value == RET_SUCCESSFUL)
        {
            // Check if there is at least one node in schedule
            if (schedule_info.nb_activities == 0u)
            {
                // Empty schedule, write the new node with no neighbours
                return_value = BuildAndWriteNewNode(schedule_deviceno, new_node_index, activity, UNEXISTING_NODE_INDEX, UNEXISTING_NODE_INDEX);
                if (return_value == RET_SUCCESSFUL)
                {
                    schedule_info.nb_activities++;
                    schedule_info.oldest_activity_index = new_node_index;
                    // Then update info
                    return_value = WriteScheduleInfo(schedule_deviceno, &schedule_info);
                }
            }
            else
            {
                // Walk the linked list from the oldest until we find the node
                pusActivityNode_t activity_node = { 0 };

                // We start with the oldest node.
                pusNodeIndex_t next_node_index = schedule_info.oldest_activity_index;
                pusNodeIndex_t nodes_visited   = 0u;

                // Start looking for the next node
                return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, next_node_index);
                while ((return_value == RET_SUCCESSFUL) && (activity_node.activity.timestamp <= activity->timestamp)
                       && (activity_node.next_node_index != UNEXISTING_NODE_INDEX) && (nodes_visited < MAXIMUM_ACTIVITIES_PER_SCHEDULE))
                {
                    // Move to the next node in the list
                    next_node_index = activity_node.next_node_index;
                    return_value    = ReadNodeFromSchedule(schedule_deviceno, &activity_node, next_node_index);

                    // Guards against an accidental full lap through the list
                    nodes_visited++;
                }

                // Check if no error occured
                if (return_value == RET_SUCCESSFUL)
                {
                    // While loop stopped: look at why
                    if (nodes_visited < MAXIMUM_ACTIVITIES_PER_SCHEDULE)
                    {
                        pusNodeIndex_t previous_node_index = UNEXISTING_NODE_INDEX;

                        // Either we reeached the end of the list, or it sits just before activity_node
                        if ((activity_node.next_node_index == UNEXISTING_NODE_INDEX) && (activity_node.activity.timestamp <= activity->timestamp))
                        {
                            // New node becomes the next_node_index
                            previous_node_index = next_node_index;
                            next_node_index     = UNEXISTING_NODE_INDEX;
                        }
                        else
                        {
                            // New node sits just before activity_node
                            previous_node_index = activity_node.previous_node_index;
                        }

                        // Insert the new node between its computed neighbours
                        return_value =
                            InsertNodeInBetween(schedule_deviceno, &schedule_info, activity, new_node_index, previous_node_index, next_node_index);
                    }
                    else
                    {
                        // We went around the schedule without finding any node
                        return_value = RET_ERROR;
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
 * @fn          ReleaseNextActivity(deviceNo_t schedule_deviceno, pusActivity_t *activity)
 * @brief       This function releases the oldest activity and update the schedule
 * @param[in]   schedule_deviceno Schedule file number from which the oldest activity is release
 * @param[out]  activity Oldest activity released content
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ReleaseNextActivity(deviceNo_t schedule_deviceno, pusActivity_t *activity)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity != NULL)
    {
        pusScheduleInfo_t schedule_info = { 0 };
        // First get schedule info
        return_value = ReadScheduleInfo(schedule_deviceno, &schedule_info);
        if (return_value == RET_SUCCESSFUL)
        {
            pusActivityNode_t activity_node         = { 0 };
            pusNodeIndex_t former_oldest_node_index = schedule_info.oldest_activity_index;
            // Get former oldest node (the one that will be released)
            return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, former_oldest_node_index);
            if (return_value == RET_SUCCESSFUL)
            {
                // Get new oldest node
                pusNodeIndex_t new_oldest_node_index = activity_node.next_node_index;

                // First, we save copy of the node content to exiting activity variable
                activity->timestamp = activity_node.activity.timestamp;
                activity->data      = activity_node.activity.data;

                // Then, we free the former oldest node
                (void)memset(&activity_node, 0u, ACTIVITY_NODE_SIZE);
                return_value = WriteNodeToSchedule(schedule_deviceno, &activity_node, former_oldest_node_index);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Then, we update the new oldest node if it exists
                    if (new_oldest_node_index != UNEXISTING_NODE_INDEX)
                    {
                        // Get new oldest node
                        return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, new_oldest_node_index);
                        if (return_value == RET_SUCCESSFUL)
                        {
                            activity_node.previous_node_index = UNEXISTING_NODE_INDEX;
                            // Update oldest node
                            return_value = WriteNodeToSchedule(schedule_deviceno, &activity_node, new_oldest_node_index);
                        }
                    }

                    // If everything went right then update schedule info
                    if (return_value == RET_SUCCESSFUL)
                    {
                        schedule_info.oldest_activity_index = new_oldest_node_index;
                        schedule_info.nb_activities--;
                        return_value = WriteScheduleInfo(schedule_deviceno, &schedule_info);
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
 * @fn          InsertNodeInBetween(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info, pusActivity_t *activity,
 *                                  pusNodeIndex_t new_node_index, pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index)
 * @brief       Inserts a new node in between its neighbours, updating the schedule info
 * @param[in]       schedule_deviceno Schedule file number
 * @param[in,out]   schedule_info Schedule info (oldest index / count are updated and written back)
 * @param[in]       activity Activity content to store in the new node
 * @param[in]       new_node_index Index at which the new node is written
 * @param[in]       previous_node_index Index of the previous node (UNEXISTING_NODE_INDEX if new node is head)
 * @param[in]       next_node_index Index of the next node (UNEXISTING_NODE_INDEX if new node is tail)
 * @retval      #RET_INVALID_PARAM if a pointer is NULL
 * @retval      #RET_ERROR if a write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t InsertNodeInBetween(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info, pusActivity_t *activity,
                                        pusNodeIndex_t new_node_index, pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((schedule_info != NULL) && (activity != NULL))
    {
        // Write the new node with its neighbours nodes
        return_value = BuildAndWriteNewNode(schedule_deviceno, new_node_index, activity, previous_node_index, next_node_index);
        if (return_value == RET_SUCCESSFUL)
        {
            pusActivityNode_t activity_node = { 0 };

            // Update the next node, if there is
            if (next_node_index != UNEXISTING_NODE_INDEX)
            {
                return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, next_node_index);
                if (return_value == RET_SUCCESSFUL)
                {
                    activity_node.previous_node_index = new_node_index;
                    return_value                      = WriteNodeToSchedule(schedule_deviceno, &activity_node, next_node_index);
                }
            }

            if (return_value == RET_SUCCESSFUL)
            {
                // Check if new node is not the oldest node
                if (previous_node_index != UNEXISTING_NODE_INDEX)
                {
                    // If there is a previous node, first we get it
                    return_value = ReadNodeFromSchedule(schedule_deviceno, &activity_node, previous_node_index);
                    if (return_value == RET_SUCCESSFUL)
                    {
                        activity_node.next_node_index = new_node_index;
                        // Then we update previous node
                        return_value = WriteNodeToSchedule(schedule_deviceno, &activity_node, previous_node_index);
                    }
                }
                else
                {
                    // No previous node so the new node is the head of the list
                    schedule_info->oldest_activity_index = new_node_index;
                }

                // Update info
                if (return_value == RET_SUCCESSFUL)
                {
                    schedule_info->nb_activities++;
                    return_value = WriteScheduleInfo(schedule_deviceno, schedule_info);
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
 * @fn          BuildAndWriteNewNode(deviceNo_t schedule_deviceno, pusNodeIndex_t new_node_index, pusActivity_t *activity,
 *                                   pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index)
 * @brief       Builds and writes a new activity node in schedule
 * @param[in]   schedule_deviceno Schedule file number
 * @param[in]   new_node_index New node index
 * @param[in]   activity Activity content to store in the new node
 * @param[in]   previous_node_index Previous node index
 * @param[in]   next_node_index Next node index
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t BuildAndWriteNewNode(deviceNo_t schedule_deviceno, pusNodeIndex_t new_node_index, pusActivity_t *activity,
                                         pusNodeIndex_t previous_node_index, pusNodeIndex_t next_node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity != NULL)
    {
        pusActivityNode_t new_activity_node   = { 0 };
        new_activity_node.status              = ACTIVITY_NODE_OCCUPIED;
        new_activity_node.activity.timestamp  = activity->timestamp;
        new_activity_node.activity.data       = activity->data;
        new_activity_node.previous_node_index = previous_node_index;
        new_activity_node.next_node_index     = next_node_index;

        // Write new node in schedule
        return_value = WriteNodeToSchedule(schedule_deviceno, &new_activity_node, new_node_index);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          ReadScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info)
 * @brief       Get schedule info from schedule
 * @param[in]   schedule_deviceno Schedule file number
 * @param[out]  schedule_info Infos from schedule
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ReadScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (schedule_info != NULL)
    {
        length_t offset = 0u;
        // Move the read/write pointer to the beginning (where the schedule info table is located)
        return_value = DeviceIoctl(schedule_deviceno, IOCTL_FS_SEEK, &offset, sizeof(offset));
        if (return_value == RET_SUCCESSFUL)
        {
            // Then read the schedule info table
            return_value = DeviceRead(schedule_deviceno, (data_t)schedule_info, SCHEDULE_INFO_SIZE);
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          WriteScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info)
 * @brief       Set schedule info toward schedule
 * @param[in]   schedule_deviceno Schedule file number
 * @param[out]  schedule_info Infos for schedule
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t WriteScheduleInfo(deviceNo_t schedule_deviceno, pusScheduleInfo_t *schedule_info)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (schedule_info != NULL)
    {
        length_t origin = 0u;
        // Move the read/write pointer to the beginning (where the schedule info table is located)
        return_value = DeviceIoctl(schedule_deviceno, IOCTL_FS_SEEK, &origin, sizeof(origin));
        if (return_value == RET_SUCCESSFUL)
        {
            // Then write the schedule info table
            return_value = DeviceWrite(schedule_deviceno, (data_t)schedule_info, SCHEDULE_INFO_SIZE);
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          ReadNodeFromSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index)
 * @brief       Get schedule node from schedule
 * @param[in]   schedule_deviceno Schedule file number
 * @param[out]  activity_node Node from schedule
 * @param[in]   node_index node index
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ReadNodeFromSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity_node != NULL)
    {
        length_t offset = SCHEDULE_INFO_SIZE + (node_index * ACTIVITY_NODE_SIZE);
        // Move the read/write pointer to the desired data field
        return_value = DeviceIoctl(schedule_deviceno, IOCTL_FS_SEEK, &offset, sizeof(offset));
        if (return_value == RET_SUCCESSFUL)
        {
            // Then read data in table
            return_value = DeviceRead(schedule_deviceno, (data_t)activity_node, ACTIVITY_NODE_SIZE);
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          WriteNodeToSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index)
 * @brief       Set schedule node toward schedule
 * @param[in]   schedule_deviceno Schedule file number
 * @param[out]  activity_node Node for schedule
 * @param[in]   node_index node index
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if write in FS has encountered an error
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t WriteNodeToSchedule(deviceNo_t schedule_deviceno, pusActivityNode_t *activity_node, pusNodeIndex_t node_index)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (activity_node != NULL)
    {
        length_t offset = SCHEDULE_INFO_SIZE + (node_index * ACTIVITY_NODE_SIZE);
        // Move the read/write pointer to the desired data field
        return_value = DeviceIoctl(schedule_deviceno, IOCTL_FS_SEEK, &offset, sizeof(offset));
        if (return_value == RET_SUCCESSFUL)
        {
            // Then write data in table
            return_value = DeviceWrite(schedule_deviceno, (data_t)activity_node, ACTIVITY_NODE_SIZE);
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
