/**
 * @file Termination.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger functions for terminating events
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
        ExtensionCommandUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();

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
                    ConfigureSetExternalInterruptExitingOnSingleCore(CurrentEvent->CoreId);
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
        ExtensionCommandUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();
    }
}

/**
 * @brief Termination function for hidden hook read/write/execute
 *
 * @param Event Target Event Object
 * @return VOID
 */
VOID
TerminateHiddenHookReadAndWriteAndExecuteEvent(PDEBUGGER_EVENT Event)
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

    TempOptionalParam1 = Event->OptionalParam3;

    PagesBytes = PAGE_ALIGN(TempOptionalParam1);
    PagesBytes = Event->OptionalParam4 - PagesBytes;

    for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
    {
        ConfigureEptHookUnHookSingleAddress((UINT64)TempOptionalParam1 + (i * PAGE_SIZE), NULL, Event->ProcessId);
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
    ConfigureEptHookUnHookSingleAddress(Event->OptionalParam1, NULL, Event->ProcessId);
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
    ConfigureEptHookUnHookSingleAddress(NULL, Event->OptionalParam1, Event->ProcessId);
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
                    ConfigureChangeMsrBitmapReadOnSingleCore(CurrentEvent->CoreId, CurrentEvent->OptionalParam1);
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
                    ConfigureChangeMsrBitmapWriteOnSingleCore(CurrentEvent->CoreId, CurrentEvent->OptionalParam1);
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
                    ConfigureSetExceptionBitmapOnSingleCore(CurrentEvent->CoreId, CurrentEvent->OptionalParam1);
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
                    ConfigureChangeIoBitmapOnSingleCore(CurrentEvent->CoreId, CurrentEvent->OptionalParam1);
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
                    ConfigureChangeIoBitmapOnSingleCore(CurrentEvent->CoreId, CurrentEvent->OptionalParam1);
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
        VmFuncSetTriggerEventForVmcalls(FALSE);
    }
}

/**
 * @brief Termination function for user-mode exec trap events
 *
 * @param Event Target Event Object
 * @return VOID
 */
VOID
TerminateUserModeExecTrapEvent(PDEBUGGER_EVENT Event)
{
    if (DebuggerEventListCount(&g_Events->UserModeExecutionTrapEventsHead) > 1)
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
        // We have to uninitialize the event
        //
        ConfigureUninitializeUserExecTrapOnAllProcessors();
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
        VmFuncSetTriggerEventForCpuids(FALSE);
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
        ExtensionCommandDisableRdtscExitingForClearingEventsAllCores();

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
                    ConfigureEnableRdtscExitingOnSingleCore(CurrentEvent->CoreId);
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
        ExtensionCommandDisableRdtscExitingForClearingEventsAllCores();
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
                    ConfigureEnableRdpmcExitingOnSingleCore(CurrentEvent->CoreId);
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
 * @brief Termination function for MOV to control registers events
 *
 * @param Event Target Event Object
 * @return VOID
 */
VOID
TerminateControlRegistersEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TempList = 0;

    if (DebuggerEventListCount(&g_Events->ControlRegisterModifiedEventsHead) > 1)
    {
        //
        // There are still other events in the queue (list), we should only remove
        // this special event (not all events)
        //

        //
        // For this purpose, first we disable all the events by
        // disabling all of them
        //
        ExtensionCommandDisableMov2ControlRegsExitingForClearingEventsAllCores(Event);

        //
        // Then we iterate through the list of this event to re-apply
        // the previous events
        //
        TempList = &g_Events->ControlRegisterModifiedEventsHead;

        while (&g_Events->ControlRegisterModifiedEventsHead != TempList->Flink)
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
                    ExtensionCommandEnableMovControlRegisterExitingAllCores(CurrentEvent);
                }
                else
                {
                    //
                    // Just one core
                    //
                    DEBUGGER_BROADCASTING_OPTIONS BroadcastingOption = {0};

                    BroadcastingOption.OptionalParam1 = CurrentEvent->OptionalParam1;
                    BroadcastingOption.OptionalParam2 = CurrentEvent->OptionalParam2;

                    ConfigureEnableMovToControlRegisterExitingOnSingleCore(CurrentEvent->CoreId, &BroadcastingOption);
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
        ExtensionCommandDisableMov2ControlRegsExitingForClearingEventsAllCores(Event);
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
        ExtensionCommandDisableMov2DebugRegsExitingForClearingEventsAllCores();

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
                    ConfigureEnableMovToDebugRegistersExitingOnSingleCore(CurrentEvent->CoreId);
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
        ExtensionCommandDisableMov2DebugRegsExitingForClearingEventsAllCores();
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
                    DebuggerEventEnableEferOnAllProcessors((DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE)CurrentEvent->OptionalParam2);
                }
                else
                {
                    //
                    // Just one core
                    //
                    ConfigureEnableEferSyscallHookOnSingleCore(CurrentEvent->CoreId, (DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE)CurrentEvent->OptionalParam2);
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
                    DebuggerEventEnableEferOnAllProcessors((DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE)CurrentEvent->OptionalParam2);
                }
                else
                {
                    //
                    // Just one core
                    //
                    ConfigureEnableEferSyscallHookOnSingleCore(CurrentEvent->CoreId, (DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE)CurrentEvent->OptionalParam2);
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
 * @brief Check and modify state of exception bitmap
 *
 * @param CoreId Core specific resource
 * @param BitmapMask The current bitmask of the resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceExceptionBitmap(UINT32                               CoreId,
                                              UINT32 *                             BitmapMask,
                                              PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_EXCEPTION_EVENTS))
    {
        //
        // we have to check for !exception events and apply the mask
        //
        *BitmapMask = *BitmapMask | DebuggerExceptionEventBitmapMask(CoreId);
    }

    //
    // Check if it's because of disabling !syscall or !sysret commands
    // or not, if it's because of clearing #UD in these events then we
    // can ignore the checking for this command, otherwise, we have to
    // check it
    //
    if (!(PassOver & PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK))
    {
        //
        // Check if the debugger has events relating to syscall or sysret,
        // if no, we can safely ignore #UDs, otherwise, #UDs should be
        // activated
        //
        if (DebuggerEventListCountByEventType(SYSCALL_HOOK_EFER_SYSCALL, CoreId) != 0 ||
            DebuggerEventListCountByEventType(SYSCALL_HOOK_EFER_SYSRET, CoreId) != 0)
        {
            //
            // #UDs should be activated
            //
            *BitmapMask = *BitmapMask | (1 << EXCEPTION_VECTOR_UNDEFINED_OPCODE);
        }
    }

    //
    // Check for kernel or user debugger's presence
    //
    if (DebuggerQueryDebuggerStatus())
    {
        *BitmapMask = *BitmapMask | (1 << EXCEPTION_VECTOR_BREAKPOINT);
        *BitmapMask = *BitmapMask | (1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT);
    }

    //
    // Check for intercepting #DB by threads tracer
    //
    if (KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(CoreId,
                                                                  DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_DEBUG_REGISTER_INTERCEPTION))
    {
        *BitmapMask = *BitmapMask | (1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT);
    }

    //
    // Do not terminate the operation
    //
    return FALSE;
}

/**
 * @brief Check and modify state of external interrupt exiting
 *
 * @param CoreId Core specific resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceExternalInterruptExiting(UINT32                               CoreId,
                                                       PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_INTERRUPT_EVENTS))
    {
        //
        // we have to check for !interrupt events and decide whether to
        // ignore this event or not
        //
        if (DebuggerEventListCountByEventType(EXTERNAL_INTERRUPT_OCCURRED, CoreId) != 0)
        {
            //
            // We should ignore this unset, because !interrupt is enabled for this core
            //
            return TRUE;
        }
    }

    //
    // Check if it should remain active for thread or process changing or not
    //
    if (KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(CoreId,
                                                                  DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_THREAD_CHANGE) ||
        KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(CoreId,
                                                                  DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_PROCESS_CHANGE))
    {
        //
        // Terminate the operation
        //
        return TRUE;
    }

    //
    // Not terminate it
    //
    return FALSE;
}

/**
 * @brief Check and modify state of TSC exiting
 *
 * @param CoreId Core specific resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceTscExiting(UINT32                               CoreId,
                                         PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_TSC_EVENTS))
    {
        //
        // we have to check for !tsc events and decide whether to
        // ignore this event or not
        //
        if (DebuggerEventListCountByEventType(TSC_INSTRUCTION_EXECUTION, CoreId) != 0)
        {
            //
            // We should ignore this unset, because !tsc is enabled for this core
            //
            return TRUE;
        }
    }

    //
    // Not terminate it
    //
    return FALSE;
}

/**
 * @brief Check and modify state of mov 2 debug regs exiting
 *
 * @param CoreId Core specific resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceMov2DebugRegExiting(UINT32                               CoreId,
                                                  PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_MOV_TO_HW_DEBUG_REGS_EVENTS))
    {
        //
        // we have to check for !dr events and decide whether to
        // ignore this event or not
        //
        if (DebuggerEventListCountByEventType(DEBUG_REGISTERS_ACCESSED, CoreId) != 0)
        {
            //
            // We should ignore this unset, because !dr is enabled for this core
            //

            return TRUE;
        }
    }

    //
    // Check if thread switching is enabled or not
    //
    if (KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(CoreId,
                                                                  DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_DEBUG_REGISTER_INTERCEPTION))
    {
        //
        // We should ignore it as we want this to switch to new thread
        //
        return TRUE;
    }

    //
    // Not terminate
    //
    return FALSE;
}

/**
 * @brief Check and modify state of move to control register exiting
 *
 * @param CoreId Core specific resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceMovControlRegsExiting(UINT32                               CoreId,
                                                    PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_MOV_TO_CONTROL_REGS_EVENTS))
    {
        //
        // we have to check for !dr events and decide whether to
        // ignore this event or not
        //
        if (DebuggerEventListCountByEventType(CONTROL_REGISTER_MODIFIED, CoreId) != 0)
        {
            //
            // We should ignore this unset, because !crwrite is enabled for this core
            //
            return TRUE;
        }
    }

    //
    // Not terminate
    //
    return FALSE;
}

/**
 * @brief Check and modify state of move to cr3 control register exiting
 *
 * @param CoreId Core specific resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN
 */
BOOLEAN
TerminateQueryDebuggerResourceMovToCr3Exiting(UINT32                               CoreId,
                                              PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Check if process switching is enabled or not
    //
    if (KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(CoreId,
                                                                  DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_WAITING_FOR_MOV_CR3_VM_EXITS))
    {
        //
        // We should ignore it as we want this to switch to new process
        //
        return TRUE;
    }

    //
    // Do not terminate
    //
    return FALSE;
}

/**
 * @brief Termination query state of debugger
 *
 * @param CoreId Core specific resource
 * @param ResourceType Type of resource
 * @param Context The context specified to the resource
 * @param PassOver The pass over option
 *
 * @return BOOLEAN If TRUE the caller might terminate the operation
 */
BOOLEAN
TerminateQueryDebuggerResource(UINT32                               CoreId,
                               PROTECTED_HV_RESOURCES_TYPE          ResourceType,
                               PVOID                                Context,
                               PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    BOOLEAN Result = FALSE;

    switch (ResourceType)
    {
    case PROTECTED_HV_RESOURCES_EXCEPTION_BITMAP:

        Result = TerminateQueryDebuggerResourceExceptionBitmap(CoreId, (UINT32 *)Context, PassOver);

        break;

    case PROTECTED_HV_RESOURCES_EXTERNAL_INTERRUPT_EXITING:

        Result = TerminateQueryDebuggerResourceExternalInterruptExiting(CoreId, PassOver);

        break;

    case PROTECTED_HV_RESOURCES_RDTSC_RDTSCP_EXITING:

        Result = TerminateQueryDebuggerResourceTscExiting(CoreId, PassOver);

        break;

    case PROTECTED_HV_RESOURCES_MOV_TO_DEBUG_REGISTER_EXITING:

        Result = TerminateQueryDebuggerResourceMov2DebugRegExiting(CoreId, PassOver);

        break;

    case PROTECTED_HV_RESOURCES_MOV_CONTROL_REGISTER_EXITING:

        Result = TerminateQueryDebuggerResourceMovControlRegsExiting(CoreId, PassOver);

        break;

    case PROTECTED_HV_RESOURCES_MOV_TO_CR3_EXITING:

        Result = TerminateQueryDebuggerResourceMovToCr3Exiting(CoreId, PassOver);

        break;

    default:

        Result = FALSE;

        LogError("Err, invalid protected type");

        break;
    }

    //
    // Check termination result
    //
    return Result;
}
