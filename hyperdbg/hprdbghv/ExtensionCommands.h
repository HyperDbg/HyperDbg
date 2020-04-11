/**
 * @file ExtensionCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Debugger Commands (Extensions)
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntddk.h>
#include "DebuggerCommands.h"

//////////////////////////////////////////////////
//				Exported Functions				//
//////////////////////////////////////////////////

/* Provide All Process Syscall Hook EFER for Debugger */
VOID
DebuggerEnableSyscallHookEfer(PDEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT UsermodeRequestBuffer);
