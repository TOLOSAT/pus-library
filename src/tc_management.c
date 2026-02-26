/**
 * @file    tc_management.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for TC management
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/******************************* Include Files *******************************/

#include <string.h>

#include "kernel.h"
#include "tc_management.h"
#include "tools/tables_management.h"
#include "tools/schedule_management.h"
#include "tools/crc_computation.h"
#include "services/pus1.h"
#include "tools/tc_parser.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

static returnCode_t ProcessValidTC(pusReceiveContext_t *receive_context, pusTC_t *tc, pusTM_t *acceptance_tm);
static returnCode_t FormatTC(pusTC_t *tc);
static returnCode_t SendAcptAckTM(const pusTC_t *tc, pusTM_t *acceptance_tm, deviceNo_t dev_ack);
static returnCode_t SendAcptNackTM(const pusTC_t *tc, pusTM_t *acceptance_tm, deviceNo_t dev_ack, pusAcceptanceError_t acceptance_error);
static returnCode_t SendExecAckTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack);
static returnCode_t SendExecNackTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack, pusExecutionError_t execution_error);

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @fn          InitTCReceiveContext(pusReceiveContext_t *receive_context)
 * @brief       Function that initialise the receive context for TC handling
 * @param[in]   receive_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if a pointer is a null pointer
 * @retval      #RET_INVALID_PARAM if routing table size is zero
 * @retval      #RET_INVALID_PARAM if PUS receive buffer size is smaller than TC max size
 * @retval      #RET_INVALID_PARAM if buffer size is zero when rx_type is DEVICE_TYPE_PERIPHERAL
 * @retval      #RET_ERROR if initialisation failed because of device binding or execution table initialisation
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitTCReceiveContext(pusReceiveContext_t *receive_context)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    returnCode_t device_status;

    // Check parameter(s)
    if ((receive_context != NULL) && (receive_context->routing_table.entries != NULL) && (receive_context->routing_table.size != 0u)
        && ((receive_context->rx_type == DEVICE_TYPE_BUFFER)
            || ((receive_context->rx_buffer != NULL) && (receive_context->rx_buffer_size > TC_MAX_SIZE))))
    {
        // First initialise the routing table
        return_value = InitRoutingTable(&receive_context->routing_table);
        if (return_value == RET_SUCCESSFUL)
        {
            // If nothing wrong happened, initialise the RX resource (buffer or peripheral)
            device_status = DeviceOpen(&receive_context->dev_rx, receive_context->rx_type, receive_context->ref_rx);

            // If nothing wrong happened and a ACK TM buffer is required, initialise device for ACK TM buffer
            if ((device_status == RET_SUCCESSFUL) && (receive_context->buffer_ack != NO_BUFFER))
            {
                device_status = DeviceOpen(&receive_context->dev_ack, DEVICE_TYPE_BUFFER, receive_context->buffer_ack);
            }

            // Start reception for the RX device if is a peripheral
            if ((device_status == RET_SUCCESSFUL) && (receive_context->rx_type == DEVICE_TYPE_PERIPHERAL))
            {
                device_status =
                    DeviceIoctl(receive_context->dev_rx, IOCTL_PERIPHERAL_START_RX, receive_context->rx_buffer, receive_context->rx_buffer_size);
            }

            // Finally check everything went right
            if (device_status == RET_SUCCESSFUL)
            {
                receive_context->status = PUS_CONTEXT_INITIALIZED;
            }
            else
            {
                return_value            = RET_ERROR;
                receive_context->status = PUS_CONTEXT_ERROR;
            }
        }
        else
        {
            receive_context->status = PUS_CONTEXT_ERROR;
        }
    }
    else
    {
        return_value = RET_ERROR;
    }

    return return_value;
}

/**
 * @fn          ReceiveTC(pusReceiveContext_t *receive_context)
 * @brief       Function that get a TC and and routes it toward it's corresponding task
 * @param[in]   receive_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if receive_context is not initialised
 * @retval      #RET_NOT_AVAILABLE if there is no TC available
 * @retval      #RET_ERROR if receiving the TC is not working
 * @retval      #RET_ERROR if cannot format TC
 * @retval      #RET_ERROR if cannot write TC into it's device
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ReceiveTC(pusReceiveContext_t *receive_context)
{
    returnCode_t return_value             = RET_SUCCESSFUL;
    pusTM_t acceptance_tm                 = { 0 };
    pusAcceptanceError_t acceptance_error = PUS_ACCEPTANCE_NO_ERROR;
    pusTC_t tc                            = { 0 };

    // Check parameter(s)
    if (receive_context->status == PUS_CONTEXT_INITIALIZED)
    {
        length_t write_index = 0u;

        // First, check if a new TC has been received
        if (receive_context->rx_type == DEVICE_TYPE_PERIPHERAL)
        {
            length_t counter = 0u;
            // Check write index
            return_value = DeviceIoctl(receive_context->dev_rx, IOCTL_PERIPHERAL_GET_RX_COUNT, &counter, sizeof(length_t));
            // Flip counter to get write index
            write_index = receive_context->rx_buffer_size - counter;

            // While there is new data in the RX buffer, try to parse TCs
            while ((return_value == RET_SUCCESSFUL) && (write_index != receive_context->read_index))
            {
                pusParsingContext_t tc_parsing_context = {
                    .p_buffer    = receive_context->rx_buffer,
                    .buffer_size = receive_context->rx_buffer_size,
                    .read_index  = receive_context->read_index,
                    .write_index = write_index,
                };
                // First, check the validity of the TC.
                return_value = ParseBuffer(&tc_parsing_context, &tc, &acceptance_error);
                if (return_value == RET_SUCCESSFUL)
                {
                    // Update read index in the receive context
                    receive_context->read_index = tc_parsing_context.read_index;

                    // TC is valid, process it.
                    return_value = ProcessValidTC(receive_context, &tc, &acceptance_tm);
                }
                else if (return_value == RET_NOT_AVAILABLE)
                {
                    // Do nothing, wait for more data
                }
                else
                {
                    return_value = RET_SUCCESSFUL;
                    // Update read index to skip bad TC
                    receive_context->read_index = tc_parsing_context.read_index;
                    // Invalid TC, TC will be non-acknowledged.
                    (void)SendAcptNackTM(&tc, &acceptance_tm, receive_context->dev_ack, acceptance_error);
                }
            }
        }
        else
        {
            // TC is coming from a queue not from an hardware peripheral
            // Read the TC directly from the queue.
            return_value = DeviceRead(receive_context->dev_rx, (data_t)&tc, TC_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                // First, check the validity of the TC.
                return_value = CheckTCValidity(&tc, &acceptance_error);
                if (return_value == RET_SUCCESSFUL)
                {
                    // TC is valid, process it.
                    return_value = ProcessValidTC(receive_context, &tc, &acceptance_tm);
                }
                else
                {
                    // Invalid TC, TC will be non-acknowledged.
                    (void)SendAcptNackTM(&tc, &acceptance_tm, receive_context->dev_ack, acceptance_error);
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
 * @fn          InitTCExecutionContext(pusExecutionContext_t *execution_context)
 * @brief       Function that initialise the execution context for TC handling
 * @param[in]   execution_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if a pointer is a null pointer or routing table size is null
 * @retval      #RET_ERROR if initialisation failed because of device binding or execution table initialisation
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t InitTCExecutionContext(pusExecutionContext_t *execution_context)
{
    returnCode_t return_value  = RET_SUCCESSFUL;
    returnCode_t device_status = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((execution_context != NULL) && (execution_context->execution_table.entries != NULL) && (execution_context->execution_table.size != 0u))
    {
        // First initialise the execution table
        return_value = InitExecutionTable(&execution_context->execution_table);
        if (return_value == RET_SUCCESSFUL)
        {
            // If nothing wrong happened and a TC buffer is requested, initialise device for TM buffer
            if (execution_context->buffer_tc != NO_BUFFER)
            {
                device_status = DeviceOpen(&execution_context->dev_tc, DEVICE_TYPE_BUFFER, execution_context->buffer_tc);
            }

            // If nothing wrong happened and a TM buffer is requested, initialise device for TM buffer
            if ((device_status == RET_SUCCESSFUL) && (execution_context->buffer_tm != NO_BUFFER))
            {
                device_status = DeviceOpen(&execution_context->dev_tm, DEVICE_TYPE_BUFFER, execution_context->buffer_tm);
            }

            // If nothing wrong happened and a ACK TM buffer is required, initialise device for ACK TM buffer
            if ((device_status == RET_SUCCESSFUL) && (execution_context->buffer_ack != NO_BUFFER))
            {
                device_status = DeviceOpen(&execution_context->dev_ack, DEVICE_TYPE_BUFFER, execution_context->buffer_ack);
            }

            // Finally check everything went right
            if (device_status == RET_SUCCESSFUL)
            {
                execution_context->status = PUS_CONTEXT_INITIALIZED;
            }
            else
            {
                return_value              = RET_ERROR;
                execution_context->status = PUS_CONTEXT_ERROR;
            }
        }
        else
        {
            execution_context->status = PUS_CONTEXT_ERROR;
        }
    }
    else
    {
        return_value = RET_ERROR;
    }

    return return_value;
}

/**
 * @fn          ExecuteTC(pusExecutionContext_t *execution_context)
 * @brief       This function executes incoming TC.
 * @param[in]   execution_context Execution context for the task dealing with TC execution
 * @retval      #RET_INVALID_PARAM if execution_context is empty or contains an empty field
 * @retval      #RET_ERROR if cannot recognize TC or has an error with device management
 * @retval      #RET_SUCCESSFUL else
 */
returnCode_t ExecuteTC(pusExecutionContext_t *execution_context)
{
    returnCode_t return_value = RET_SUCCESSFUL;
    pusTC_t tc                = { 0 };
    pusTM_t tm                = { 0 };
    pusTM_t execution_tm      = { 0 };
    length_t nb_message       = 0u;

    // Check parameter(s)
    if (execution_context->status == PUS_CONTEXT_INITIALIZED)
    {
        do
        {
            // First, check if there is a TC.
            return_value = DeviceRead(execution_context->dev_tc, (data_t)&tc, TC_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                // Read the number of message in the buffer
                return_value = DeviceIoctl(execution_context->dev_tc, IOCTL_BUFFER_GET_COUNT, &nb_message, sizeof(length_t));
                if (return_value == RET_SUCCESSFUL)
                {
                    pusExecutionTableEntry_t *p_entry = NULL;

                    // Compute the routing key
                    uint32_t key = BUILD_ROUTING_KEY((APID_MASK & tc.spp_header.packet_id), tc.tc_header.service, tc.tc_header.subservice);

                    // Then, find which TC have to be executed
                    return_value = ExecutionSearch(key, &execution_context->execution_table, &p_entry);
                    if (return_value == RET_SUCCESSFUL)
                    {
                        pusExecutionError_t error_code = PUS_EXECUTION_FAILED;
                        // Now execute the TC
                        return_value = p_entry->execution_function(p_entry->env, &tc, &tm, &error_code);
                        if (return_value == RET_SUCCESSFUL)
                        {
                            // Acknowledge TC execution
                            (void)SendExecAckTM(&tc, &execution_tm, execution_context->dev_ack);

                            // Check if a specific TM has to be send
                            if (p_entry->tm_requested == TM_REQUESTED)
                            {
                                // Send specific TM
                                return_value = DeviceWrite(execution_context->dev_tm, (data_t)&tm, TM_MAX_SIZE);
                                if (return_value == RET_SUCCESSFUL)
                                {
                                    taskNo_t tm_sender = NO_TASK;
                                    return_value = DeviceIoctl(execution_context->dev_tm, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                                    if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                                    {
                                        return_value = SendSignal(tm_sender, SIGNAL_TC);
                                    }
                                }
                            }
                        }
                        else
                        {
                            // TC Failed to be executed
                            (void)SendExecNackTM(&tc, &execution_tm, execution_context->dev_ack, error_code);
                        }
                    }
                    else
                    {
                        // TC does not have execution procedure
                        (void)SendExecNackTM(&tc, &execution_tm, execution_context->dev_ack, PUS_EXECUTION_UNAVAILABLE);
                    }
                }
            }
            // Check if there are still messages in the buffer
        } while ((return_value == RET_SUCCESSFUL) && (nb_message > 0u));
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          ProcessValidTC(pusReceiveContext_t *receive_context, pusTC_t *tc, pusTM_t *acceptance_tm)
 * @brief       Function that format, route, acknowledge and forward a valid TC to its execution task
 * @param[in]   receive_context Execution context for the task dealing with TC reception
 * @param[in]   tc TC to process
 * @param[out]  acceptance_tm Pointer to the acceptance TM
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if cannot format TC or cannot route TC or cannot write TC into its device
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t ProcessValidTC(pusReceiveContext_t *receive_context, pusTC_t *tc, pusTM_t *acceptance_tm)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((receive_context != NULL) && (tc != NULL) && (acceptance_tm != NULL))
    {
        // If TC is valid, format the TC because of endianness.
        return_value = FormatTC(tc);
        if (return_value == RET_SUCCESSFUL)
        {
            pusRoutingTableEntry_t *p_entry = NULL;

            // Compute the routing key
            uint32_t key = BUILD_ROUTING_KEY((APID_MASK & tc->spp_header.packet_id), tc->tc_header.service, tc->tc_header.subservice);

            // Then, route the TC toward the task that will execute it.
            return_value = RouteSearch(key, &receive_context->routing_table, &p_entry);
            if (return_value == RET_SUCCESSFUL)
            {
                // Acknowledge TC Acceptation
                if ((tc->tc_header.version_flags & PUS_FLAG_ACK_ACC) == PUS_FLAG_ACK_ACC)
                {
                    (void)SendAcptAckTM(tc, acceptance_tm, receive_context->dev_ack);
                }

                // Send TC to the task that will execute it
                return_value = DeviceWrite(p_entry->dev_route, (data_t)tc, TC_MAX_SIZE);
                if (return_value == RET_SUCCESSFUL)
                {
                    taskNo_t tc_processor = NO_TASK;
                    return_value          = DeviceIoctl(p_entry->dev_route, IOCTL_BUFFER_GET_RECEIVER, &tc_processor, sizeof(taskNo_t));
                    if ((return_value == RET_SUCCESSFUL) && (tc_processor != NO_TASK))
                    {
                        return_value = SendSignal(tc_processor, SIGNAL_TC);
                    }
                }
            }
            else
            {
                // Bad routing so TC non acknowleded
                (void)SendAcptNackTM(tc, acceptance_tm, receive_context->dev_ack, PUS_ACCEPTANCE_INVALID_ROUTE);
                return_value = RET_ERROR;
            }
        }
        else
        {
            // Can't format so TC non acknowleded
            (void)SendAcptNackTM(tc, acceptance_tm, receive_context->dev_ack, PUS_ACCEPTANCE_CANT_FORMAT);
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
 * @fn              FormatTC(pusTC_t *tc)
 * @brief           Function that format TC the right way
 * @param[in,out]   tc TC to format
 * @retval          #RET_INVALID_PARAM if tc is null pointer
 * @retval          #RET_SUCCESSFUL else
 *
 * Because the TC endianess is not the same than the processor
 * TC fields don't have the right endianness, or aren't in
 * the right place.
 */
static returnCode_t FormatTC(pusTC_t *tc)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if (tc != NULL)
    {
        // Endianness Correction
        tc->spp_header.packet_id               = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_id);
        tc->spp_header.packet_sequence_control = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_sequence_control);
        tc->spp_header.packet_data_length      = HALF_WORD_BYTE_SWAP(tc->spp_header.packet_data_length);
        tc->tc_header.source_id                = HALF_WORD_BYTE_SWAP(tc->tc_header.source_id);

        // Put CRC at the right place
        tc->crc = (pusCRC_t)(tc->data[tc->spp_header.packet_data_length - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 1u] << 8u)
                  + (pusCRC_t)(tc->data[tc->spp_header.packet_data_length - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 2u]);
        tc->data[tc->spp_header.packet_data_length - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 1u] = 0u;
        tc->data[tc->spp_header.packet_data_length - TC_HEADER_SIZE - CRC_TRAILER_SIZE + 2u] = 0u;
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}

/**
 * @fn          SendAcptAckTM(const pusTC_t *tc, pusTM_t *acceptance_tm, deviceNo_t dev_ack)
 * @brief       This function send acceptance acknowledgment TM.
 * @param[in]   tc TC that will be acknowledged
 * @param[out]  acceptance_tm Pointer to the acceptance TM
 * @param[in]   dev_ack Device where the ACK TM will be sent
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if cannot write into device
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t SendAcptAckTM(const pusTC_t *tc, pusTM_t *acceptance_tm, deviceNo_t dev_ack)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tc != NULL) && (acceptance_tm != NULL))
    {
        return_value = BuildS1SS1(tc, acceptance_tm);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceWrite(dev_ack, (data_t)acceptance_tm, TM_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                taskNo_t tm_sender = NO_TASK;
                return_value       = DeviceIoctl(dev_ack, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                {
                    return_value = SendSignal(tm_sender, SIGNAL_TC);
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
 * @fn          SendAcptNackTM(const pusTC_t *tc, pusTM_t *acceptance_tm, pusAcceptanceError_t acceptance_error)
 * @brief       This function send acceptance non acknowledgment TM.
 * @param[in]   tc TC that will be non-acknowledged
 * @param[out]  acceptance_tm Pointer to the acceptance TM
 * @param[in]   dev_ack Device where the ACK TM will be sent
 * @param[in]   acceptance_error Code explaining why nack the TC
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if cannot write into device
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t SendAcptNackTM(const pusTC_t *tc, pusTM_t *acceptance_tm, deviceNo_t dev_ack, pusAcceptanceError_t acceptance_error)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tc != NULL) && (acceptance_tm != NULL))
    {
        return_value = BuildS1SS2(tc, acceptance_tm, acceptance_error);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceWrite(dev_ack, (data_t)acceptance_tm, TM_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                taskNo_t tm_sender = NO_TASK;
                return_value       = DeviceIoctl(dev_ack, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                {
                    return_value = SendSignal(tm_sender, SIGNAL_TC);
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
 * @fn          SendExecAckTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack)
 * @brief       This function send execution acknowledgment TM.
 * @param[in]   tc TC that will be acknowledged
 * @param[out]  execution_tm Pointer to the execution TM
 * @param[in]   dev_ack Device where the ACK TM will be sent
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if cannot write into device
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t SendExecAckTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tc != NULL) && (execution_tm != NULL))
    {
        return_value = BuildS1SS7(tc, execution_tm);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceWrite(dev_ack, (data_t)execution_tm, TM_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                taskNo_t tm_sender = NO_TASK;
                return_value       = DeviceIoctl(dev_ack, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                {
                    return_value = SendSignal(tm_sender, SIGNAL_TC);
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
 * @fn          SendExecNackTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack, pusExecutionError_t execution_error)
 * @brief       This function send execution non acknowledgment TM.
 * @param[in]   tc TC that will be non-acknowledged
 * @param[out]  execution_tm Pointer to the execution TM
 * @param[in]   dev_ack Device where the ACK TM will be sent
 * @param[in]   execution_error Code explaining why nack the TC
 * @retval      #RET_INVALID_PARAM if a pointer is null
 * @retval      #RET_ERROR if cannot write into device
 * @retval      #RET_SUCCESSFUL else
 */
static returnCode_t SendExecNackTM(const pusTC_t *tc, pusTM_t *execution_tm, deviceNo_t dev_ack, pusExecutionError_t execution_error)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tc != NULL) && (execution_tm != NULL))
    {
        return_value = BuildS1SS8(tc, execution_tm, execution_error);
        if (return_value == RET_SUCCESSFUL)
        {
            return_value = DeviceWrite(dev_ack, (data_t)execution_tm, TM_MAX_SIZE);
            if (return_value == RET_SUCCESSFUL)
            {
                taskNo_t tm_sender = NO_TASK;
                return_value       = DeviceIoctl(dev_ack, IOCTL_BUFFER_GET_RECEIVER, &tm_sender, sizeof(taskNo_t));
                if ((return_value == RET_SUCCESSFUL) && (tm_sender != NO_TASK))
                {
                    return_value = SendSignal(tm_sender, SIGNAL_TC);
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
