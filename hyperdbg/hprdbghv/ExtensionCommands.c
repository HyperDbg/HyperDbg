//#include "Definitions.h"
#include "DebuggerCommands.h"
#include "Common.h"
#include "Process.h"

/* Provide All Process Syscall Hook EFER for Debugger */
VOID DebuggerEnableSyscallHookEforAllProcess(PDEBUGGER_ACTION DebuggerAction) {

	switch (DebuggerAction->Action)
	{
	case BREAK_TO_DEBUGGER:

		break;

	case LOG_TO_DEBUGGER:

		break;

	default:
		// Not valid Action
		LogWithTag(DebuggerAction->Tag, TRUE, "invalid parameter specified in \"Action\" ");
		break;
	}
}

/* Provide  Syscall Hook EFER for Debugger */
VOID DebuggerEnableSyscallHookEfer(PDEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT UsermodeRequestBuffer)
{
	if (UsermodeRequestBuffer->Type == ALL_SYSCALLS)
	{
		DbgBreakPoint();
		DebuggerEnableSyscallHookEforAllProcess(&UsermodeRequestBuffer->Action);
	}
	else if (UsermodeRequestBuffer->Type == SPECIFIC_PROCESS_SYSCALLS)
	{
		DbgBreakPoint();
	}
	else
	{
		DbgBreakPoint();
		LogWithTag(UsermodeRequestBuffer->Action.Tag, TRUE, "invalid parameter specified in \"Type\" ");
	}

}