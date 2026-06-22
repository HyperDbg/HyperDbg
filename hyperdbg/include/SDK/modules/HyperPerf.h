/**
 * @file HyperPerf.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for hyperperf project
 * @details This file contains definitions of HyperPerf routines
 * @version 0.21
 * @date 2026-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			     Callback Types                 //
//////////////////////////////////////////////////

/**
 * @brief A function from the message tracer that send the inputs to the
 * queue of the messages
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE)(UINT32       OperationCode,
                                                                  BOOLEAN      IsImmediateMessage,
                                                                  BOOLEAN      ShowCurrentSystemTime,
                                                                  BOOLEAN      Priority,
                                                                  const CHAR * Fmt,
                                                                  va_list      ArgList);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE)(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_BUFFER)(_In_ UINT32                          OperationCode,
                                            _In_reads_bytes_(BufferLength) PVOID Buffer,
                                            _In_ UINT32                          BufferLength,
                                            _In_ BOOLEAN                         Priority);

/**
 * @brief A function that checks whether the priority or regular buffer is full or not
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL)(BOOLEAN Priority);

/**
 * @brief A function that checks whether the current execution mode is VMX-root mode or not
 *
 */
typedef BOOLEAN (*VM_FUNC_VMX_GET_CURRENT_EXECUTION_MODE)();

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by hyperperf module
 *
 */
typedef struct _HYPERPERF_CALLBACKS
{
    //
    // *** Log (Hyperlog) callbacks ***
    //
    LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogCallbackPrepareAndSendMessageToQueueWrapper;
    LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE             LogCallbackSendMessageToQueue;
    LOG_CALLBACK_SEND_BUFFER                       LogCallbackSendBuffer;
    LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL           LogCallbackCheckIfBufferIsFull;

    //
    // *** Hypervisor (Hyperhv) callbacks ***
    //
    VM_FUNC_VMX_GET_CURRENT_EXECUTION_MODE VmFuncVmxGetCurrentExecutionMode;

} HYPERPERF_CALLBACKS, *PHYPERPERF_CALLBACKS;
