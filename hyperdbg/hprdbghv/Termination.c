/**
 * @file Termination.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger functions for terminating events
 * @details
 * 
 * @version 0.1
 * @date 2020-08-16
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Termination function for external-interrupts
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateExternalInterruptEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->ExternalInterruptOccurredEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandUnsetExternalInterruptExitingAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->ExternalInterruptOccurredEventsHead;

        while (&g_Events->ExternalInterruptOccurredEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandSetExternalInterruptExitingAllCores();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformSetExternalInterruptExitingOnSingleCore, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to disable on all cores
        //
        ExtensionCommandUnsetExternalInterruptExitingAllCores();
    }
}

/**
 * @brief Termination function for hidden hook read/write
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateHiddenHookReadAndWriteEvent(PDEBUGGER_EVENT Event)
{
    UINT64 PagesBytes;
    UINT64 TempOptionalParam1;

    //
    // Because there are different EPT hooks, like READ, WRITE, READ WRITE,
    // DETOURS INLINE HOOK, HIDDEN BREAKPOINT HOOK and all of them are
    // unhooked with a same routine, we will not check whther the list of
    // all of them is empty or not and instead, we remove just a single
    // hook, this way is better as hidden hooks and ept modifications are
    // not dependant to a single bit and if we remove or add any other hook
    // then it won't cause any problem for other hooks
    //

    TempOptionalParam1 = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam1, Event->ProcessId);

    PagesBytes = PAGE_ALIGN(TempOptionalParam1);
    PagesBytes = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam2, Event->ProcessId) - PagesBytes;

    for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
    {
        EptHookUnHookSingleAddress((UINT64)TempOptionalParam1 + (i * PAGE_SIZE), Event->ProcessId);
    }
}

/**
 * @brief Termination function for hidden hook read
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateHiddenHookReadEvent(PDEBUGGER_EVENT Event)
{
    UINT64 PagesBytes;
    UINT64 TempOptionalParam1;

    //
    // Because there are different EPT hooks, like READ, WRITE, READ WRITE,
    // DETOURS INLINE HOOK, HIDDEN BREAKPOINT HOOK and all of them are
    // unhooked with a same routine, we will not check whther the list of
    // all of them is empty or not and instead, we remove just a single
    // hook, this way is better as hidden hooks and ept modifications are
    // not dependant to a single bit and if we remove or add any other hook
    // then it won't cause any problem for other hooks
    //

    TempOptionalParam1 = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam1, Event->ProcessId);

    PagesBytes = PAGE_ALIGN(TempOptionalParam1);
    PagesBytes = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam2, Event->ProcessId) - PagesBytes;

    for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
    {
        EptHookUnHookSingleAddress((UINT64)TempOptionalParam1 + (i * PAGE_SIZE), Event->ProcessId);
    }
}

/**
 * @brief Termination function for hidden hook write
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateHiddenHookWriteEvent(PDEBUGGER_EVENT Event)
{
    UINT64 PagesBytes;
    UINT64 TempOptionalParam1;

    //
    // Because there are different EPT hooks, like READ, WRITE, READ WRITE,
    // DETOURS INLINE HOOK, HIDDEN BREAKPOINT HOOK and all of them are
    // unhooked with a same routine, we will not check whther the list of
    // all of them is empty or not and instead, we remove just a single
    // hook, this way is better as hidden hooks and ept modifications are
    // not dependant to a single bit and if we remove or add any other hook
    // then it won't cause any problem for other hooks
    //

    TempOptionalParam1 = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam1, Event->ProcessId);

    PagesBytes = PAGE_ALIGN(TempOptionalParam1);
    PagesBytes = PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam2, Event->ProcessId) - PagesBytes;

    for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
    {
        EptHookUnHookSingleAddress((UINT64)TempOptionalParam1 + (i * PAGE_SIZE), Event->ProcessId);
    }
}

/**
 * @brief Termination function for hidden hook (hidden breakpoints)
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateHiddenHookExecCcEvent(PDEBUGGER_EVENT Event)
{
    //
    // Because there are different EPT hooks, like READ, WRITE, READ WRITE,
    // DETOURS INLINE HOOK, HIDDEN BREAKPOINT HOOK and all of them are
    // unhooked with a same routine, we will not check whther the list of
    // all of them is empty or not and instead, we remove just a single
    // hook, this way is better as hidden hooks and ept modifications are
    // not dependant to a single bit and if we remove or add any other hook
    // then it won't cause any problem for other hooks
    //

    //
    // In this hook Event->OptionalParam1 is the virtual address of the
    // target address that we put hook on it
    //
    EptHookUnHookSingleAddress(Event->OptionalParam1, Event->ProcessId);
}

/**
 * @brief Termination function for hidden hook (detours)
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateHiddenHookExecDetoursEvent(PDEBUGGER_EVENT Event)
{
    //
    // Because there are different EPT hooks, like READ, WRITE, READ WRITE,
    // DETOURS INLINE HOOK, HIDDEN BREAKPOINT HOOK and all of them are
    // unhooked with a same routine, we will not check whther the list of
    // all of them is empty or not and instead, we remove just a single
    // hook, this way is better as hidden hooks and ept modifications are
    // not dependant to a single bit and if we remove or add any other hook
    // then it won't cause any problem for other hooks
    //

    //
    // In HIDDEN_HOOK_EXEC_DETOURS, OptionalParam1 points to a physical
    // address that represent the virtual address, we have to convert
    // this address to virtual address as the unhook routine works on
    // virtual addresses
    //
    EptHookUnHookSingleAddress(
        PhysicalAddressToVirtualAddressByProcessId(Event->OptionalParam1, Event->ProcessId),
        Event->ProcessId);
}

/**
 * @brief Termination function for msr read events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateRdmsrExecutionEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->RdmsrInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandResetChangeAllMsrBitmapReadAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->RdmsrInstructionExecutionEventsHead;

        while (&g_Events->RdmsrInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandChangeAllMsrBitmapReadAllCores(CurrentEvent->OptionalParam1);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformChangeMsrBitmapReadOnSingleCore, CurrentEvent->OptionalParam1);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to reset msr bitmap on all cores
        //
        ExtensionCommandResetChangeAllMsrBitmapReadAllCores();
    }
}

/**
 * @brief Termination function for msr write events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateWrmsrExecutionEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->WrmsrInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandResetAllMsrBitmapWriteAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->WrmsrInstructionExecutionEventsHead;

        while (&g_Events->WrmsrInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandChangeAllMsrBitmapWriteAllCores(CurrentEvent->OptionalParam1);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformChangeMsrBitmapWriteOnSingleCore, CurrentEvent->OptionalParam1);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to reset msr bitmap on all cores
        //
        ExtensionCommandResetAllMsrBitmapWriteAllCores();
    }
}

/**
 * @brief Termination function for exception events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateExceptionEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->ExceptionOccurredEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandResetExceptionBitmapAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->ExceptionOccurredEventsHead;

        while (&g_Events->ExceptionOccurredEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //

                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandSetExceptionBitmapAllCores(CurrentEvent->OptionalParam1);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformSetExceptionBitmapOnSingleCore, CurrentEvent->OptionalParam1);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to reset exception bitmap on all cores
        //
        ExtensionCommandResetExceptionBitmapAllCores();
    }
}

/**
 * @brief Termination function for IN instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateInInstructionExecutionEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    //
    // For this event we should also check for out instructions events too
    // because both of them are emulated by a single bit in vmx controls
    // so if there is anything in out events list then we can remove all
    // the events
    //
    if (DebuggerEventListCount(&g_Events->InInstructionExecutionEventsHead) > 1 ||
        DebuggerEventListCount(&g_Events->OutInstructionExecutionEventsHead) >= 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandIoBitmapResetAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->InInstructionExecutionEventsHead;

        while (&g_Events->InInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandIoBitmapChangeAllCores(CurrentEvent->OptionalParam1);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformChangeIoBitmapOnSingleCore, CurrentEvent->OptionalParam1);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to reset i/o bitmap on all cores
        //
        ExtensionCommandIoBitmapResetAllCores();
    }
}

/**
 * @brief Termination function for OUT Instructions events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateOutInstructionExecutionEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    //
    // For this event we should also check for out instructions events too
    // because both of them are emulated by a single bit in vmx controls
    // so if there is anything in out events list then we can remove all
    // the events
    //
    if (DebuggerEventListCount(&g_Events->OutInstructionExecutionEventsHead) > 1 ||
        DebuggerEventListCount(&g_Events->InInstructionExecutionEventsHead) >= 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandIoBitmapResetAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->OutInstructionExecutionEventsHead;

        while (&g_Events->OutInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandIoBitmapChangeAllCores(CurrentEvent->OptionalParam1);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformChangeIoBitmapOnSingleCore, CurrentEvent->OptionalParam1);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Broadcast to reset i/o bitmap on all cores
        //
        ExtensionCommandIoBitmapResetAllCores();
    }
}

/**
 * @brief Termination function for VMCALL Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateVmcallExecutionEvent(PDEBUGGER_EVENT Event)
{
    if (DebuggerEventListCount(&g_Events->VmcallInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // Nothing we can do for this event type, let it work because of other events
        //
        return;
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // We set the global variable related to the vmcall to FALSE
        // so the vm-exit handler, no longer triggers events related
        // to the vmcalls (still they cause vm-exits as vmcall is an
        // unconditional instruction for vm-exit)
        //
        g_TriggerEventForVmcalls = FALSE;
    }
}

/**
 * @brief Termination function for CPUID Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateCpuidExecutionEvent(PDEBUGGER_EVENT Event)
{
    if (DebuggerEventListCount(&g_Events->CpuidInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // Nothing we can do for this event type, let it work because of other events
        //
        return;
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // We set the global variable related to the cpuid to FALSE
        // so the vm-exit handler, no longer triggers events related
        // to the cpuids (still they cause vm-exits as cpuid is an
        // unconditional instruction for vm-exit)
        //
        g_TriggerEventForCpuids = FALSE;
    }
}

/**
 * @brief Termination function for RDTSC/RDTSCP Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateTscEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->TscInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandDisableRdtscExitingAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->TscInstructionExecutionEventsHead;

        while (&g_Events->TscInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandEnableRdtscExitingAllCores();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformEnableRdtscExitingOnSingleCore, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Disable it on all cores
        //
        ExtensionCommandDisableRdtscExitingAllCores();
    }
}

/**
 * @brief Termination function for RDPMC Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminatePmcEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->PmcInstructionExecutionEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandDisableRdpmcExitingAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->PmcInstructionExecutionEventsHead;

        while (&g_Events->PmcInstructionExecutionEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandEnableRdpmcExitingAllCores();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformEnableRdpmcExitingOnSingleCore, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Disable it on all the cores
        //
        ExtensionCommandDisableRdpmcExitingAllCores();
    }
}

/**
 * @brief Termination function for MOV to debug registers events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateDebugRegistersEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->DebugRegistersAccessedEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandDisableMovDebugRegistersExitingAllCores();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->DebugRegistersAccessedEventsHead;

        while (&g_Events->DebugRegistersAccessedEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    ExtensionCommandEnableMovDebugRegistersExitingAllCores();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformEnableMovToDebugRegistersExiting, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Disable it on all cores
        //
        ExtensionCommandDisableMovDebugRegistersExitingAllCores();
    }
}

/**
 * @brief Termination function for SYSCALL Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateSyscallHookEferEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    //
    // For this event we should also check for sysret instructions events too
    // because both of them are emulated by a single bit in vmx controls
    // and a MSR so if there is anything in out events list then we can
    // remove all the events
    //
    if (DebuggerEventListCount(&g_Events->SyscallHooksEferSyscallEventsHead) > 1 ||
        DebuggerEventListCount(&g_Events->SyscallHooksEferSysretEventsHead) >= 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        DebuggerEventDisableEferOnAllProcessors();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->SyscallHooksEferSyscallEventsHead;

        while (&g_Events->SyscallHooksEferSyscallEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    DebuggerEventEnableEferOnAllProcessors();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformEnableEferSyscallHookOnSingleCore, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Disable it on all cores
        //
        DebuggerEventDisableEferOnAllProcessors();
    }
}

/**
 * @brief Termination function for SYSRET Instruction events
 * 
 * @param Event Target Event Object
 * @return VOID 
 */
VOID
TerminateSysretHookEferEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    //
    // For this event we should also check for syscall instructions events too
    // because both of them are emulated by a single bit in vmx controls
    // and a MSR so if there is anything in out events list then we can
    // remove all the events
    //
    if (DebuggerEventListCount(&g_Events->SyscallHooksEferSysretEventsHead) > 1 ||
        DebuggerEventListCount(&g_Events->SyscallHooksEferSyscallEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        DebuggerEventDisableEferOnAllProcessors();

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->SyscallHooksEferSysretEventsHead;

        while (&g_Events->SyscallHooksEferSysretEventsHead != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // We have to check because we don't want to re-apply
            // the terminated event
            //
            if (CurrentEvent->Tag != Event->Tag)
            {
                //
                // re-apply the event
                //

                //
                // Let's see if it is for all cores or just one core
                //
                if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
                {
                    //
                    // All cores
                    //
                    DebuggerEventEnableEferOnAllProcessors();
                }
                else
                {
                    //
                    // Just one core
                    //
                    DpcRoutineRunTaskOnSingleCore(CurrentEvent->CoreId, DpcRoutinePerformEnableEferSyscallHookOnSingleCore, NULL);
                }
            }
        }
    }
    else
    {
        //
        // Nothing else is in the list, we have to restore everything to default
        // as the current event is the only event in the list
        //

        //
        // Disable it on all cores
        //
        DebuggerEventDisableEferOnAllProcessors();
    }
}
