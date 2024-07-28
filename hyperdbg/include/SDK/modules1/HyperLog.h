/**
 * @file HyperLog.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for HyperLog project
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
 * @brief A function that checks whether the current operation
 * is on vmx-root mode or not
 *
 */
typedef BOOLEAN (*CHECK_VMX_OPERATION)();

/**
 * @brief A function that checks whether the immediate message
 * sending is needed or not
 *
 */
typedef BOOLEAN (*CHECK_IMMEDIATE_MESSAGE_SENDING)(UINT32 OperationCode);

/**
 * @brief A function that sends immediate messages
 *
 */
typedef BOOLEAN (*SEND_IMMEDIATE_MESSAGE)(CHAR * OptionalBuffer,
                                          UINT32 OptionalBufferLength,
                                          UINT32 OperationCode);

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by message tracer
 *
 */
typedef struct _MESSAGE_TRACING_CALLBACKS
{
    CHECK_VMX_OPERATION             VmxOperationCheck;
    CHECK_IMMEDIATE_MESSAGE_SENDING CheckImmediateMessageSending;
    SEND_IMMEDIATE_MESSAGE          SendImmediateMessage;

} MESSAGE_TRACING_CALLBACKS, *PMESSAGE_TRACING_CALLBACKS;
