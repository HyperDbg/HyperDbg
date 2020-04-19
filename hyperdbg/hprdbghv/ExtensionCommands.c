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
#include "Hooks.h"
#include "GlobalVariables.h"

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
    PLIST_ENTRY TempList = 0;

    //
    // As the context to event trigger, we send the address of function
    // which is current hidden hook is triggered for it
    //
    DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_DETOUR, Regs, CalledFrom);

    //
    // test
    //

    //
    //LogInfo("Hidden Hooked function Called with : rcx = 0x%llx , rdx = 0x%llx , r8 = 0x%llx ,  r9 = 0x%llx",
    //        Regs->rcx,
    //        Regs->rdx,
    //        Regs->r8,
    //        Regs->r9);
    //

    //
    // Iterate through the list of hooked pages details to find
    // and return where want to jump after this functions
    //
    TempList = &g_HiddenHooksDetourListHead;

    while (&g_HiddenHooksDetourListHead != TempList->Flink)
    {
        TempList                                          = TempList->Flink;
        PHIDDEN_HOOKS_DETOUR_DETAILS CurrentHookedDetails = CONTAINING_RECORD(TempList, HIDDEN_HOOKS_DETOUR_DETAILS, OtherHooksList);

        if (CurrentHookedDetails->HookedFunctionAddress == CalledFrom)
        {
            return CurrentHookedDetails->ReturnAddress;
        }
    }

    //
    // If we reach here, means that we didn't find the return address
    // that's an error, we can't do anything else now :(
    //

    LogError("Couldn't find anything to return");

    return 0;
}
