#include<ntddk.h>
#include "DebuggerCommands.h"


//////////////////////////////////////////////////
//				Exported Functions				//
//////////////////////////////////////////////////

// Provide All Process Syscall Hook EFER for Debugger
VOID DebuggerEnableSyscallHookEfer(PDEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT UsermodeRequestBuffer);
