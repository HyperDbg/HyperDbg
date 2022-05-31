/**
 * @file DebuggerEvents.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
    KeGenericCallDpc(DpcRoutineEnableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for !syscall command (disable syscall hook)
 * 
 * @return VOID 
 */
VOID
DebuggerEventDisableEferOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineDisableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 * 
 * @return VOID 
 */
VOID
DebuggerEventEnableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 * 
 * @return VOID 
 */
VOID
DebuggerEventDisableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineDisableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines to generally handle breakpoint hit for detour 
 * 
 * @return PVOID 
 */
PVOID
DebuggerEventEptHook2GeneralDetourEventHandler(PGUEST_REGS Regs, PVOID CalledFrom)
{
    PLIST_ENTRY                 TempList    = 0;
    EPT_HOOKS_TEMPORARY_CONTEXT TempContext = {0};

    //
    // The RSP register is the at the RCX and we just added (reverse by stack) to it's
    // values by the size of the GUEST_REGS
    //
    Regs->rsp = (UINT64)Regs - sizeof(GUEST_REGS);

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
    // Create temporary context
    //
    TempContext.VirtualAddress  = CalledFrom;
    TempContext.PhysicalAddress = VirtualAddressToPhysicalAddress(CalledFrom);

    //
    // As the context to event trigger, we send the address of function
    // which is current hidden hook is triggered for it
    //
    DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_DETOURS, Regs, &TempContext);

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
    // that's an error, generally we can't do anything but as the user
    // might already cleaned the hook and the structures are removed
    // so we just return the original caller address and continue the
    // guest normally
    //
    return CalledFrom;
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
    // such a thing, we will enable the Read silently here if, this problem will be
    // solved when the trigger works, the trigger routines won't enable reads
    //
    if (EnableForWrite)
    {
        EnableForRead = TRUE;
    }

    //
    // Perform the EPT Hook
    //
    return EptHook2(Address, NULL, ProcessId, EnableForRead, EnableForWrite, FALSE);
}
