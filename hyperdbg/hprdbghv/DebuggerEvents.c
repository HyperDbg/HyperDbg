/**
 * @file DebuggerEvents.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger events (triggers and enable events)
 * 
 * @version 0.1
 * @date 2020-05-12
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief routines for !syscall command (enable syscall hook)
 * 
 * @return VOID 
 */
VOID
DebuggerEventEnableEferOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcEnableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for !syscall command (disable syscall hook)
 * 
 * @return VOID 
 */
VOID
DebuggerEventDisableEferOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcDisableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 * 
 * @return VOID 
 */
VOID
DebuggerEventEnableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcEnableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 * 
 * @return VOID 
 */
VOID
DebuggerEventDisableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(BroadcastDpcDisableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines to generally handle breakpoint hit for detour 
 * 
 * @return VOID 
 */
VOID
DebuggerEventEptHook2GeneralDetourEventHandler(PGUEST_REGS Regs, PVOID CalledFrom)
{
    PLIST_ENTRY TempList = 0;

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
    // As the context to event trigger, we send the address of function
    // which is current hidden hook is triggered for it
    //
    DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_DETOURS, Regs, VirtualAddressToPhysicalAddress(CalledFrom));

    //
    // Iterate through the list of hooked pages details to find
    // and return where want to jump after this functions
    //
    TempList = &g_EptHook2sDetourListHead;

    while (&g_EptHook2sDetourListHead != TempList->Flink)
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

/**
 * @brief Event for address, we don't use address range here, 
 * address ranges should be check in event section
 * 
 * @return VOID 
 */
BOOLEAN
DebuggerEventEnableMonitorReadAndWriteForAddress(UINT64 Address, UINT32 ProcessId, BOOLEAN EnableForRead, BOOLEAN EnableForWrite)
{
    //
    // Check if the detail is ok for either read or write or both
    //
    if (!EnableForRead && !EnableForWrite)
    {
        return FALSE;
    }

    //
    // If the read is FALSE and WRITE is TRUE, then the processor doesn't support
    // such thing, we will enable the Read silently here if, this problem will be
    // solved when the trigger works, the trigger routines won't enable reads
    //
    if (EnableForWrite)
    {
        EnableForRead = TRUE;
    }

    //
    // Check if its DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES then
    // we have to convert it to current process id
    //
    if (ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES)
    {
        ProcessId = PsGetCurrentProcessId();
    }

    //
    // Perform the EPT Hook
    //
    EptHook2(Address, NULL, ProcessId, EnableForRead, EnableForWrite, FALSE);
}
