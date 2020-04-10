/**
 * @file DebuggerCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Commands for debugger
 * @details This file contains general commands that are implemented for debugger 
 * These command mostly start without "!" or "."
 * 
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once
#include <ntddk.h>
#include "Logging.h"

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Saves the debugger state
 * Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 * 
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    UINT64 UndefinedInstructionAddress; // #UD Location of instruction (used by EFER Syscall)

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

//////////////////////////////////////////////////
//					Log wit Tag					//
//////////////////////////////////////////////////

/* Send buffer to the usermode with a tag that shows what was the action */
#define LogWithTag(tag, IsImmediate, format, ...) \
    LogSendMessageToQueue(OPERATION_LOG_WITH_TAG, IsImmediate, FALSE, "%016x" format, tag, __VA_ARGS__);
