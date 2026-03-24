/**
 * @file    pus160.c
 * @author  Théo Bessel
 * @brief   Source file for PUS 160 functions (System management)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tm_management.h"
#include "services/pus160.h"

/***************************** Macros Definitions ****************************/

#define PUS_S160SS34_DATA_SIZE 1u  /**< TM(160,34) data size */
#define PUS_S160SS36_DATA_SIZE 2u  /**< TM(160,36) data size */
#define PUS160_MAX_NB_TASK     32u /**< Max number of task supported by this API */

/*************************** Functions Declarations **************************/

static returnCode_t BuildS160SS18(pusTM_t *tm, context_t *context);
static returnCode_t BuildS160SS20(pusTM_t *tm, context_t *context);
static returnCode_t BuildS160SS22(pusTM_t *tm, context_t *context);
static returnCode_t BuildS160SS34(pusTM_t *tm, uint8_t idle_time);
static returnCode_t BuildS160SS36(pusTM_t *tm, uint8_t highest_stack_consumer, uint8_t max_stack_usage);
static returnCode_t BuildS160SS38(pusTM_t *tm, taskUsage_t *tasks_info, uint32_t nb_tasks);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          InitS160(pus160Env_t *pus160_env)
 * @brief       Function that initialises PUS 160
 * @param[in]   pus160_env PUS160 environment used for configuration
 * @retval      #RET_INVALID_PARAM if nb_task is not correct
 * @retval      #RET_ERROR if cannot bind the pus160_dev_reboot to the reboot
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitS160(pus160Env_t *pus160_env)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameters
    if ((pus160_env != NULL) && (pus160_env->nb_tasks <= PUS160_MAX_NB_TASK) && (pus160_env->nb_tasks != 0u))
    {
        // Start S160 by opening a device for rebooting the system
        return_value = DeviceOpen(&pus160_env->dev_reboot, DEVICE_TYPE_SYSTEM, SYSDEV_SYSTEM_REBOOT);

        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceOpen(&pus160_env->dev_context, DEVICE_TYPE_SYSTEM, SYSDEV_SYSTEM_CONTEXT);
        }

        if (return_value == RET_SUCCESSFUL)
        {
            // Start S160 by opening a device for system usage virtual device
            return_value = DeviceOpen(&pus160_env->dev_system_usage, DEVICE_TYPE_SYSTEM, SYSDEV_SYSTEM_USAGE);
        }

        if (return_value == RET_SUCCESSFUL)
        {
            // Start S160 by opening a device for task usages virtual device
            return_value = DeviceOpen(&pus160_env->dev_task_usages, DEVICE_TYPE_SYSTEM, SYSDEV_TASK_USAGES);
        }

        // Setup S160 environment status
        if (return_value == RET_SUCCESSFUL)
        {
            pus160_env->status = PUS_INITIALIZED;
        }
        else
        {
            pus160_env->status = PUS_ERROR;
        }
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn              ExecuteS160SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that achieve a reboot to a chosen software
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
returnCode_t ExecuteS160SS1(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    context_t context         = { 0 };

    // Unused
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Set error
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Read the current context
            return_value = DeviceRead(pus160_env->dev_context, (data_t)&context, sizeof(context_t));

            if (return_value == RET_SUCCESSFUL)
            {
                // Get the current software state
                if (tc->spp_header.packet_data_length == (sizeof(softwareState_t) + CRC_TRAILER_SIZE + TC_HEADER_SIZE - 1u))
                {
                    // Read software state from TC
                    (void)memcpy((uint8_t *)&context.state, tc->data, sizeof(softwareState_t));
                }
                else
                {
                    // If the TC does not have the right size, we reboot to the safe software by default
                    context.state = SOFTWARE_STATE_SAFE;
                }

                // Write the updated context
                (void)DeviceWrite(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
            }

            // Reboot the system (this call is outside the if to ensure that even if there is a context error, we still try to reboot). Here we don't
            // want any error to happen. The context read/write is tested in the bootloader side.
            return_value = DeviceIoctl(pus160_env->dev_reboot, 0u, NULL, 0u);
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that selects the default rebooting software (soft_id, safe/nominal)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
returnCode_t ExecuteS160SS2(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    context_t context         = { 0 };

    // Unused
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (tc != NULL) && (error_code != NULL))
    {
        // Set error
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Get context
            return_value = DeviceRead(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Get the current software state and software ID
                if (tc->spp_header.packet_data_length == (sizeof(softwareSelection_t) + CRC_TRAILER_SIZE + TC_HEADER_SIZE - 1u))
                {
                    softwareSelection_t software_selection = { 0 };

                    // Read software selection from TC
                    (void)memcpy((uint8_t *)&software_selection, tc->data, sizeof(softwareSelection_t));

                    // Check the software state and update the context accordingly
                    if (software_selection.software_state == SOFTWARE_STATE_NOMINAL)
                    {
                        context.nominal_software_id = software_selection.software_id;
                    }
                    else if (software_selection.software_state == SOFTWARE_STATE_SAFE)
                    {
                        context.safe_software_id = software_selection.software_id;
                    }
                    else
                    {
                        *error_code  = PUS_EXECUTION_FAILED;
                        return_value = RET_INVALID_PARAM;
                    }

                    if (return_value == RET_SUCCESSFUL)
                    {
                        // Write the updated context
                        return_value = DeviceWrite(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
                    }
                }
                else
                {
                    *error_code  = PUS_EXECUTION_FAILED;
                    return_value = RET_INVALID_PARAM;
                }
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the system context
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS18 TM
 */
returnCode_t ExecuteS160SS17(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        context_t context = { 0 };

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Get context
            return_value = DeviceRead(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S160S18 : full context
                return_value = BuildS160SS18(tm, &context);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS19(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the reduced system context (without debug info)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS20 TM
 */
returnCode_t ExecuteS160SS19(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        context_t context = { 0 };

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Get context
            return_value = DeviceRead(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S160S20 : context without debug info
                return_value = BuildS160SS20(tm, &context);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS21(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that requests the error context (only debug info)
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS22 TM
 */
returnCode_t ExecuteS160SS21(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        context_t context = { 0 };
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Get context
            return_value = DeviceRead(pus160_env->dev_context, (data_t)&context, sizeof(context_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S160S22 : error context only
                return_value = BuildS160SS22(tm, &context);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS23(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that resets the error context
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for the sent TM
 */
returnCode_t ExecuteS160SS23(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);
    (void)(tm);

    // Check parameter(s)
    if ((env != NULL) && (error_code != NULL))
    {
        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Reset the error context
            return_value = DeviceIoctl(pus160_env->dev_context, 0u, NULL, 0u);
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS33(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS34 TM (idle time report) when requested by a S160SS33
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS34 TM
 */
returnCode_t ExecuteS160SS33(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        systemUsage_t temp_system_usage = { 0 };

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Read system usage
            return_value = DeviceRead(pus160_env->dev_system_usage, (data_t)&temp_system_usage, sizeof(systemUsage_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S161SS2 TM
                return_value = BuildS160SS34(tm, temp_system_usage.idle_time);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
            else
            {
                *error_code = PUS_EXECUTION_FAILED;
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS35(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS36 TM (stack usage report) when requested by a S160SS35
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS36 TM
 */
returnCode_t ExecuteS160SS35(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        systemUsage_t temp_system_usage = { 0 };

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Read system usage
            return_value = DeviceRead(pus160_env->dev_system_usage, (data_t)&temp_system_usage, sizeof(systemUsage_t));
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S161SS4 TM
                return_value = BuildS160SS36(tm, temp_system_usage.highest_stack_consumer, temp_system_usage.max_stack_usage);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
            else
            {
                *error_code = PUS_EXECUTION_FAILED;
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn              ExecuteS160SS37(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
 * @brief           Function that send S160SS38 TM (system usage report) when requested by a S160SS37
 * @param[in,out]   env PUS160 environment
 * @param[in]       tc TC that has been received
 * @param[out]      tm TM that will be sent
 * @param[out]      error_code Indicates which error has been encountered for S160SS38 TM
 */
returnCode_t ExecuteS160SS37(void *env, pusTC_t *tc, pusTM_t *tm, pusExecutionError_t *error_code)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Unused
    (void)(tc);

    // Check parameter(s)
    if ((env != NULL) && (tm != NULL) && (error_code != NULL))
    {
        static taskUsage_t temp_task_usages[PUS160_MAX_NB_TASK] = { 0 };

        // Error code Initialization
        *error_code = PUS_EXECUTION_NO_ERROR;

        // Get environment
        pus160Env_t *pus160_env = (pus160Env_t *)env;

        // Check if pus160 is initialized
        if (pus160_env->status == PUS_INITIALIZED)
        {
            // Read task usages
            return_value = DeviceRead(pus160_env->dev_task_usages, (data_t)&temp_task_usages, sizeof(taskUsage_t) * pus160_env->nb_tasks);
            if (return_value == RET_SUCCESSFUL)
            {
                // Build S161SS4 TM
                return_value = BuildS160SS38(tm, temp_task_usages, pus160_env->nb_tasks);
                if (return_value != RET_SUCCESSFUL)
                {
                    *error_code = PUS_EXECUTION_TM_BUILDING_FAILED;
                }
            }
            else
            {
                *error_code = PUS_EXECUTION_FAILED;
            }
        }
        else
        {
            return_value = RET_INVALID_PARAM;
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
 * @fn          BuildS160SS18(pusTM_t *tm, context_t *context)
 * @brief       Function that sends the full context of the system
 * @param[out]  tm TM that will be sent
 */
static returnCode_t BuildS160SS18(pusTM_t *tm, context_t *context)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tm != NULL) && (context != NULL))
    {
        return_value = BuildTM(tm, 160u, 18u, (pusData_t *)&context, sizeof(context_t));
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          BuildS160SS20(pusTM_t *tm, context_t *context)
 * @brief       Function that sends the reduced memory context of the system
 * @param[out]  tm TM that will be sent
 */
static returnCode_t BuildS160SS20(pusTM_t *tm, context_t *context)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tm != NULL) && (context != NULL))
    {
        length_t reduced_context_length = sizeof(context->version) + sizeof(context->state) + sizeof(context->safe_software_id)
                                          + sizeof(context->nominal_software_id) + sizeof(context->boot) + sizeof(context->critical_error);

        return_value = BuildTM(tm, 160u, 18u, (pusData_t *)&context, reduced_context_length);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          BuildS160SS22(pusTM_t *tm)
 * @brief       Function that sends the error context of the system
 * @param[out]  tm TM that will be sent
 */
static returnCode_t BuildS160SS22(pusTM_t *tm, context_t *context)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tm != NULL) && (context != NULL))
    {
        length_t error_context_length = sizeof(context->cfsr) + sizeof(context->hfsr) + sizeof(context->registers) + sizeof(context->call_stack);
        length_t error_context_offset = offsetof(context_t, cfsr);
        pusData_t *context_bytes      = (pusData_t *)context;

        return_value = BuildTM(tm, 160u, 18u, &context_bytes[error_context_offset / sizeof(pusData_t)], error_context_length);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          BuildS160SS34(pusTM_t *tm, uint8_t idle_time)
 * @brief       Function that send S161SS2 TM (idle time report)
 * @param[out]  tm TM that will be sent
 * @param[in]   idle_time   Idle time
 */
static returnCode_t BuildS160SS34(pusTM_t *tm, uint8_t idle_time)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (tm != NULL)
    {
        // Build TM
        return_value = BuildTM(tm, 160u, 34u, (pusData_t *)&idle_time, PUS_S160SS34_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          BuildS160SS36(pusTM_t *tm, uint8_t highest_stack_consumer, uint8_t max_stack_usage)
 * @brief       Function that send S161SS4 TM (stack usage report)
 * @param[out]  tm TM that will be sent
 * @param[in]   highest_stack_consumer Task that is the highest stack consummer (in percent of its own stack)
 * @param[in]   max_stack_usage Stack usage for that stack
 */
static returnCode_t BuildS160SS36(pusTM_t *tm, uint8_t highest_stack_consumer, uint8_t max_stack_usage)
{
    returnCode_t return_value              = RET_SUCCESSFUL;
    pusData_t data[PUS_S160SS36_DATA_SIZE] = { 0 };

    // Check parameter(s)
    if (tm != NULL)
    {
        // Get highest stack consummer
        data[0] = highest_stack_consumer;

        // Get stack usage
        data[1] = max_stack_usage;

        // Build TM
        return_value = BuildTM(tm, 160u, 36u, (pusData_t *)&data, PUS_S160SS36_DATA_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          BuildS160SS38(pusTM_t *tm, taskUsage_t *tasks_info)
 * @brief       Function that sends S160SS38 TM (system usage report)
 * @param[out]  tm TM that will be sent
 * @param[in]   tasks_info Pointer to task usage informations
 * @param[in]   nb_tasks Number of task concerned
 * @param[in]   tasks_info Pointer towards tasks monitoring information
 */
static returnCode_t BuildS160SS38(pusTM_t *tm, taskUsage_t *tasks_info, uint32_t nb_tasks)
{
    returnCode_t return_value        = RET_SUCCESSFUL;
    pusData_t data[TM_MAX_DATA_SIZE] = { 0 };

    // Check parameter(s)
    if ((tm != NULL) && (tasks_info != NULL))
    {
        // Check if the size of the report can be contained in TM data
        uint32_t report_size = nb_tasks * sizeof(taskUsage_t);
        if (report_size <= TM_MAX_DATA_SIZE)
        {
            // Copy report in data
            for (uint32_t i = 0u; i < report_size; i++)
            {
                (void)memcpy((void *)&data[i * sizeof(taskUsage_t)], (void *)&tasks_info[i], sizeof(taskUsage_t));
            }

            // Build TM
            return_value = BuildTM(tm, 160u, 38u, (pusData_t *)&data, report_size);
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
