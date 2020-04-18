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
#include "Broadcast.h"
#include "Dpc.h"
#include "Debugger.h"
#include "Logging.h"
#include "Common.h"

/**
 * @brief routines for !syscallhook command (enable syscall hook)
 * 
 * @return VOID 
 */
VOID
ExtensionCommandEnableEferOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcEnableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for !syscallhook command (disable syscall hook)
 * 
 * @return VOID 
 */
VOID
ExtensionCommandDisableEferOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcDisableEferSyscallEvents, 0x0);
}

/**
 * @brief routines to generally handle breakpoint hit for detour 
 * 
 * @return VOID 
 */
VOID
ExtensionCommandHiddenHookGeneralDetourEventHandler(PGUEST_REGS Regs, PVOID CalledFrom)
{
    DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_DETOUR, Regs, 0x0);
    LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x , Number Of Bytes = %d , Pool Type = %d ",
            Regs->rcx,
            Regs->rdx,
            Regs->r8);
}
