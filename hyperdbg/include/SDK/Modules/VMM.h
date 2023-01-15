/**
 * @file VMM.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for VMM project
 * @details This file contains definitions of HyperLog routines
 * @version 0.2
 * @date 2023-01-15
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
typedef BOOLEAN (*LOG_PREPARE_AND_SEND_MESSAGE_TO_QUEUE)(UINT32       OperationCode,
                                                         BOOLEAN      IsImmediateMessage,
                                                         BOOLEAN      ShowCurrentSystemTime,
                                                         BOOLEAN      Priority,
                                                         const char * Fmt,
                                                         ...);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_SEND_MESSAGE_TO_QUEUE)(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority);

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by VMM module
 *
 */
typedef struct _VMM_CALLBACKS
{
    LOG_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogPrepareAndSendMessageToQueue;
    LOG_SEND_MESSAGE_TO_QUEUE             LogSendMessageToQueue;

} VMM_CALLBACKS, *PVMM_CALLBACKS;
