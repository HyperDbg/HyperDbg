#pragma once
#include<ntddk.h>

typedef struct _PROCESSOR_DEBUGGING_STATE
{
	UINT64 UndefinedInstructionAddress;								// #UD Location of instruction (used by EFER Syscall)

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

