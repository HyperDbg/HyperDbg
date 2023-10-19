/**
 * @file ApplyEvents.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger functions for applying events
 * @details
 *
 * @version 0.7
 * @date 2023-10-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Applying monitor memory hook events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ApplyEventMonitorEvent(PDEBUGGER_EVENT                   Event,
                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                       BOOLEAN                           InputFromVmxRoot)
{
    UINT32  TempProcessId;
    UINT64  PagesBytes;
    BOOLEAN ResultOfApplyingEvent = FALSE;

    //
    // Check if process id is equal to DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    // or if process id is 0 then we use the cr3 of current process
    //
    if (Event->ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || Event->ProcessId == 0)
    {
        TempProcessId = PsGetCurrentProcessId();
    }
    else
    {
        TempProcessId = Event->ProcessId;
    }

    PagesBytes = PAGE_ALIGN(Event->InitOptions.OptionalParam1);
    PagesBytes = Event->InitOptions.OptionalParam2 - PagesBytes;

    for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
    {
        //
        // In all the cases we should set both read/write, even if it's only
        // read we should set the write too!
        // Also execute bit has the same conditions here, because if write is set
        // read should be also set
        //
        switch (Event->EventType)
        {
        case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
        case HIDDEN_HOOK_READ_AND_EXECUTE:

            ResultOfApplyingEvent = DebuggerEventEnableMonitorReadWriteExec((UINT64)Event->InitOptions.OptionalParam1 + (i * PAGE_SIZE),
                                                                            TempProcessId,
                                                                            TRUE,
                                                                            TRUE,
                                                                            TRUE);
            break;

        case HIDDEN_HOOK_WRITE_AND_EXECUTE:

            ResultOfApplyingEvent = DebuggerEventEnableMonitorReadWriteExec((UINT64)Event->InitOptions.OptionalParam1 + (i * PAGE_SIZE),
                                                                            TempProcessId,
                                                                            FALSE,
                                                                            TRUE,
                                                                            FALSE);
            break;

        case HIDDEN_HOOK_READ_AND_WRITE:
        case HIDDEN_HOOK_READ:
            ResultOfApplyingEvent = DebuggerEventEnableMonitorReadWriteExec((UINT64)Event->InitOptions.OptionalParam1 + (i * PAGE_SIZE),
                                                                            TempProcessId,
                                                                            TRUE,
                                                                            TRUE,
                                                                            FALSE);

            break;

        case HIDDEN_HOOK_WRITE:
            ResultOfApplyingEvent = DebuggerEventEnableMonitorReadWriteExec((UINT64)Event->InitOptions.OptionalParam1 + (i * PAGE_SIZE),
                                                                            TempProcessId,
                                                                            FALSE,
                                                                            TRUE,
                                                                            FALSE);

            break;

        case HIDDEN_HOOK_EXECUTE:
            ResultOfApplyingEvent = DebuggerEventEnableMonitorReadWriteExec((UINT64)Event->InitOptions.OptionalParam1 + (i * PAGE_SIZE),
                                                                            TempProcessId,
                                                                            FALSE,
                                                                            FALSE,
                                                                            TRUE);
            break;

        default:
            LogError("Err, Invalid monitor hook type");

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_EVENT_TYPE_IS_INVALID;

            goto EventNotApplied;

            break;
        }

        if (!ResultOfApplyingEvent)
        {
            //
            // The event is not applied, won't apply other EPT modifications
            // as we want to remove this event
            //

            //
            // Now we should restore the previously applied events (if any)
            //
            for (size_t j = 0; j < i; j++)
            {
                ConfigureEptHookUnHookSingleAddress((UINT64)Event->InitOptions.OptionalParam1 + (j * PAGE_SIZE), NULL, Event->ProcessId);
            }

            break;
        }
        else
        {
            //
            // We applied the hook and the pre-allocated buffers are used
            // for this hook, as here is a safe PASSIVE_LEVEL we can force
            // the Windows to reallocate some pools for us, thus, if this
            // hook is continued to other pages, we still have pre-alloated
            // buffers ready for our future hooks
            //
            PoolManagerCheckAndPerformAllocationAndDeallocation();
        }
    }

    //
    // We convert the Event's optional parameters physical address because
    // vm-exit occurs and we have the physical address to compare in the case of
    // hidden hook rw events.
    //
    Event->Options.OptionalParam1 = VirtualAddressToPhysicalAddressByProcessId(Event->InitOptions.OptionalParam1, TempProcessId);
    Event->Options.OptionalParam2 = VirtualAddressToPhysicalAddressByProcessId(Event->InitOptions.OptionalParam2, TempProcessId);
    Event->Options.OptionalParam3 = Event->InitOptions.OptionalParam1;
    Event->Options.OptionalParam4 = Event->InitOptions.OptionalParam2;

    //
    // Check if we should restore the event if it was not successful
    //
    if (!ResultOfApplyingEvent)
    {
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DebuggerGetLastError();

        goto EventNotApplied;
    }

    //
    // Applying event was successful
    //
    return TRUE;

EventNotApplied:

    return FALSE;
}

/**
 * @brief Applying EPT hook execution (hidden breakpoints) events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ApplyEventEptHookExecCcEvent(PDEBUGGER_EVENT                   Event,
                             PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                             BOOLEAN                           InputFromVmxRoot)
{
    UINT32 TempProcessId;

    //
    // Check if process id is equal to DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    // or if process id is 0 then we use the cr3 of current process
    //
    if (Event->ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || Event->ProcessId == 0)
    {
        TempProcessId = PsGetCurrentProcessId();
    }
    else
    {
        TempProcessId = Event->ProcessId;
    }

    //
    // Invoke the hooker
    //
    if (!ConfigureEptHook(Event->InitOptions.OptionalParam1, TempProcessId))
    {
        //
        // There was an error applying this event, so we're setting
        // the event
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DebuggerGetLastError();
        goto EventNotApplied;
    }

    //
    // We set events OptionalParam1 here to make sure that our event is
    // executed not for all hooks but for this special hook
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;

    //
    // Applying event was successful
    //
    return TRUE;

EventNotApplied:

    return FALSE;
}

/**
 * @brief Applying EPT hook trampoline (inline hook) events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ApplyEventEpthookInlineEvent(PDEBUGGER_EVENT                   Event,
                             PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                             BOOLEAN                           InputFromVmxRoot)
{
    UINT32 TempProcessId;

    //
    // Check if process id is equal to DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    // or if process id is 0 then we use the cr3 of current process
    //
    if (Event->ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || Event->ProcessId == 0)
    {
        TempProcessId = PsGetCurrentProcessId();
    }
    else
    {
        TempProcessId = Event->ProcessId;
    }

    //
    // Invoke the hooker
    //
    if (!ConfigureEptHook2(PsGetCurrentProcessId(), Event->InitOptions.OptionalParam1, NULL, TempProcessId, FALSE, FALSE, FALSE, TRUE))
    {
        //
        // There was an error applying this event, so we're setting
        // the event
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DebuggerGetLastError();
        goto EventNotApplied;
    }

    //
    // We set events OptionalParam1 here to make sure that our event is
    // executed not for all hooks but for this special hook
    // Also, we are sure that this is not null because we checked it before
    //
    Event->Options.OptionalParam1 = VirtualAddressToPhysicalAddressByProcessId(Event->Options.OptionalParam1, TempProcessId);

    //
    // Applying event was successful
    //
    return TRUE;

EventNotApplied:

    return FALSE;
}

/**
 * @brief Applying RDMSR execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventRdmsrExecutionEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        if (InputFromVmxRoot)
        {
            HaltedBroadcastChangeAllMsrBitmapReadAllCores(Event->InitOptions.OptionalParam1);
        }
        else
        {
            ExtensionCommandChangeAllMsrBitmapReadAllCores(Event->InitOptions.OptionalParam1);
        }
    }
    else
    {
        //
        // Just one core
        //
        if (InputFromVmxRoot)
        {
            HaltedRoutineChangeAllMsrBitmapReadOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
        else
        {
            ConfigureChangeMsrBitmapReadOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
    }

    //
    // Setting an indicator to MSR
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
}

/**
 * @brief Applying WRMSR execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventWrmsrExecutionEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        if (InputFromVmxRoot)
        {
            HaltedBroadcastChangeAllMsrBitmapWriteAllCores(Event->InitOptions.OptionalParam1);
        }
        else
        {
            ExtensionCommandChangeAllMsrBitmapWriteAllCores(Event->InitOptions.OptionalParam1);
        }
    }
    else
    {
        //
        // Just one core
        //
        if (InputFromVmxRoot)
        {
            HaltedRoutineChangeAllMsrBitmapWriteOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
        else
        {
            ConfigureChangeMsrBitmapWriteOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
    }

    //
    // Setting an indicator to MSR
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
}

/**
 * @brief Applying IN/OUT instructions execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventInOutExecutionEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        if (InputFromVmxRoot)
        {
            HaltedBroadcastChangeAllIoBitmapAllCores(Event->InitOptions.OptionalParam1);
        }
        else
        {
            ExtensionCommandIoBitmapChangeAllCores(Event->InitOptions.OptionalParam1);
        }
    }
    else
    {
        //
        // Just one core
        //
        if (InputFromVmxRoot)
        {
            HaltedRoutineChangeIoBitmapOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
        else
        {
            ConfigureChangeIoBitmapOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
        }
    }

    //
    // Setting an indicator to MSR
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
}

/**
 * @brief Applying RDTSC/RDTSCP instructions execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventTscExecutionEvent(PDEBUGGER_EVENT                   Event,
                            PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                            BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        if (InputFromVmxRoot)
        {
            HaltedBroadcastEnableRdtscExitingAllCores();
        }
        else
        {
            ExtensionCommandEnableRdtscExitingAllCores();
        }
    }
    else
    {
        //
        // Just one core
        //
        if (InputFromVmxRoot)
        {
            HaltedRoutineEnableRdtscExitingOnSingleCore(Event->CoreId);
        }
        else
        {
            ConfigureEnableRdtscExitingOnSingleCore(Event->CoreId);
        }
    }
}

/**
 * @brief Applying RDPMC instruction execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventRdpmcExecutionEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        if (InputFromVmxRoot)
        {
            HaltedBroadcastEnableRdpmcExitingAllCores();
        }
        else
        {
            ExtensionCommandEnableRdpmcExitingAllCores();
        }
    }
    else
    {
        //
        // Just one core
        //
        if (InputFromVmxRoot)
        {
            HaltedRoutineEnableRdpmcExitingOnSingleCore(Event->CoreId);
        }
        else
        {
            ConfigureEnableRdpmcExitingOnSingleCore(Event->CoreId);
        }
    }
}

/**
 * @brief Applying mov 2 debug registers events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventMov2DebugRegExecutionEvent(PDEBUGGER_EVENT                   Event,
                                     PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                                     BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
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
        ConfigureEnableMovToDebugRegistersExitingOnSingleCore(Event->CoreId);
    }
}

/**
 * @brief Applying control registers accessed events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventControlRegisterAccessedEvent(PDEBUGGER_EVENT                   Event,
                                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                                       BOOLEAN                           InputFromVmxRoot)
{
    //
    // Setting an indicator to CR
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
    Event->Options.OptionalParam2 = Event->InitOptions.OptionalParam2;

    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        ExtensionCommandEnableMovControlRegisterExitingAllCores(Event);
    }
    else
    {
        //
        // Just one core
        //
        ConfigureEnableMovToControlRegisterExitingOnSingleCore(Event->CoreId, &Event->Options);
    }
}

/**
 * @brief Applying exception events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventExceptionEvent(PDEBUGGER_EVENT                   Event,
                         PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                         BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        ExtensionCommandSetExceptionBitmapAllCores(Event->InitOptions.OptionalParam1);
    }
    else
    {
        //
        // Just one core
        //
        ConfigureSetExceptionBitmapOnSingleCore(Event->CoreId, Event->InitOptions.OptionalParam1);
    }

    //
    // Set the event's target exception
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
}

/**
 * @brief Applying interrupt interception events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventInterruptEvent(PDEBUGGER_EVENT                   Event,
                         PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                         BOOLEAN                           InputFromVmxRoot)
{
    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
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
        ConfigureSetExternalInterruptExitingOnSingleCore(Event->CoreId);
    }

    //
    // Set the event's target interrupt
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
}

/**
 * @brief Applying EFER SYSCALL hook events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventEferSyscallHookEvent(PDEBUGGER_EVENT                   Event,
                               PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                               BOOLEAN                           InputFromVmxRoot)
{
    DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY;

    //
    // whether it's a !syscall2 or !sysret2
    //
    if (Event->InitOptions.OptionalParam2 == DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD)
    {
        SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD;
    }
    else if (Event->InitOptions.OptionalParam2 == DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY)
    {
        SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY;
    }

    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        DebuggerEventEnableEferOnAllProcessors(SyscallHookType);
    }
    else
    {
        //
        // Just one core
        //
        ConfigureEnableEferSyscallHookOnSingleCore(Event->CoreId, SyscallHookType);
    }

    //
    // Set the event's target syscall number and save the approach
    // of handling event details
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
    Event->Options.OptionalParam2 = SyscallHookType;
}

/**
 * @brief Applying EFER SYSRET hook events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventEferSysretHookEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY;

    //
    // whether it's a !syscall2 or !sysret2
    //
    if (Event->InitOptions.OptionalParam2 == DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD)
    {
        SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD;
    }
    else if (Event->InitOptions.OptionalParam2 == DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY)
    {
        SyscallHookType = DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY;
    }

    //
    // Let's see if it is for all cores or just one core
    //
    if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // All cores
        //
        DebuggerEventEnableEferOnAllProcessors(SyscallHookType);
    }
    else
    {
        //
        // Just one core
        //
        ConfigureEnableEferSyscallHookOnSingleCore(Event->CoreId, SyscallHookType);
    }

    //
    // Set the event's target syscall number and save the approach
    // of handling event details
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;
    Event->Options.OptionalParam2 = SyscallHookType;
}

/**
 * @brief Applying VMCALL instruction execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventVmcallExecutionEvent(PDEBUGGER_EVENT                   Event,
                               PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                               BOOLEAN                           InputFromVmxRoot)
{
    //
    // Enable triggering events for VMCALLs
    // This event doesn't support custom optional
    // parameter(s) because it's unconditional
    // users can use condition(s) to check for
    // their custom optional parameters
    //
    VmFuncSetTriggerEventForVmcalls(TRUE);
}

/**
 * @brief Applying trap mode change events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventTrapModeChangeEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Add the process to the watching list
    //
    ConfigureExecTrapAddProcessToWatchingList(Event->ProcessId);

    //
    // Set the event's mode of execution
    //
    Event->Options.OptionalParam1 = Event->InitOptions.OptionalParam1;

    //
    // Enable triggering events for user-mode execution
    // traps. This event doesn't support custom optional
    // parameter(s) because it's unconditional users can
    // use condition(s) to check for their custom optional
    // parameters
    //
    ConfigureInitializeExecTrapOnAllProcessors();
}

/**
 * @brief Applying CPUID instruction execution events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return VOID
 */
VOID
ApplyEventCpuidExecutionEvent(PDEBUGGER_EVENT                   Event,
                              PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                              BOOLEAN                           InputFromVmxRoot)
{
    //
    // Enable triggering events for CPUIDs
    // This event doesn't support custom optional
    // parameter(s) because it's unconditional
    // users can use condition(s) to check for
    // their custom optional parameters
    //
    VmFuncSetTriggerEventForCpuids(TRUE);
}
