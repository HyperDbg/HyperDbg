#pragma once
#include<ntddk.h>
#include "Logging.h"

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

typedef struct _PROCESSOR_DEBUGGING_STATE
{
	UINT64 UndefinedInstructionAddress;								// #UD Location of instruction (used by EFER Syscall)

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

//////////////////////////////////////////////////
//					Log wit Tag					//
//////////////////////////////////////////////////

// Send buffer to the usermode with a tag that shows what was the action
#define LogWithTag(tag, IsImmediate, format, ...)  \
    LogSendMessageToQueue(OPERATION_LOG_WITH_TAG, IsImmediate, FALSE, "%016x" format, tag, __VA_ARGS__);

