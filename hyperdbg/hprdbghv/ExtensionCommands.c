/**
 * @file ExtensionCommands.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger Commands (Extensions)
 * @details Debugger Commands that start with "!"
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "DebuggerCommands.h"
#include "Common.h"
#include "Process.h"

/**
 * @brief Provide All Process Syscall Hook EFER for Debugger
 * 
 * @param DebuggerAction What action should be performed when syscall hook triggered
 * @return VOID 
 */
VOID
DebuggerEnableSyscallHookEforAllProcess(PDEBUGGER_ACTION DebuggerAction)
{
    switch (DebuggerAction->Action)
    {
    case BREAK_TO_DEBUGGER:

        break;

    case LOG_TO_DEBUGGER:

        break;

    default:
        //
        // Not valid Action
        //
        LogWithTag(DebuggerAction->Tag, TRUE, "invalid parameter specified in \"Action\" ");
        break;
    }
}

/**
 * @brief Provide Syscall Hook EFER for Debugger
 * 
 * @param UsermodeRequestBuffer The usermode request buffer
 * @return VOID 
 */
VOID
DebuggerEnableSyscallHookEfer(PDEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT UsermodeRequestBuffer)
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
