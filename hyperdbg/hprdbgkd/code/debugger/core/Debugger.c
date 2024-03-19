/**
 * @file Debugger.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of Debugger functions
 * @details
 *
 * @version 0.1
 * @date 2020-04-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief A wrapper for GetRegValue() in script-engine
 *
 * @return BOOLEAN Value of register
 */
UINT64
DebuggerGetRegValueWrapper(PGUEST_REGS GuestRegs, UINT32 /* REGS_ENUM */ RegId)
{
    return GetRegValue(GuestRegs, RegId);
}

/**
 * @brief Debugger get the last error
 *
 * @return UINT32 Error value
 */
UINT32
DebuggerGetLastError()
{
    return g_LastError;
}

/**
 * @brief Debugger set the last error
 * @param LastError The value of last error
 *
 * @return VOID
 */
VOID
DebuggerSetLastError(UINT32 LastError)
{
    g_LastError = LastError;
}

/**
 * @brief Initialize Debugger Structures and Routines
 *
 * @return BOOLEAN Shows whether the initialization process was successful
 * or not
 */
BOOLEAN
DebuggerInitialize()
{
    ULONG                       ProcessorsCount      = KeQueryActiveProcessorCount(0);
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState = NULL;

    //
    // Also allocate the debugging state
    //
    if (!GlobalDebuggingStateAllocateZeroedMemory())
    {
        return FALSE;
    }

    //
    // Allocate buffer for saving events
    //
    if (GlobalEventsAllocateZeroedMemory() == FALSE)
    {
        return FALSE;
    }

    //
    // Set the core's IDs
    //
    for (UINT32 i = 0; i < ProcessorsCount; i++)
    {
        g_DbgState[i].CoreId = i;
    }

    //
    // Initialize lists relating to the debugger events store
    //
    InitializeListHead(&g_Events->EptHookExecCcEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadAndWriteAndExecuteEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadAndWriteEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadAndExecuteEventsHead);
    InitializeListHead(&g_Events->HiddenHookWriteAndExecuteEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadEventsHead);
    InitializeListHead(&g_Events->HiddenHookWriteEventsHead);
    InitializeListHead(&g_Events->HiddenHookExecuteEventsHead);
    InitializeListHead(&g_Events->EptHook2sExecDetourEventsHead);
    InitializeListHead(&g_Events->SyscallHooksEferSyscallEventsHead);
    InitializeListHead(&g_Events->SyscallHooksEferSysretEventsHead);
    InitializeListHead(&g_Events->CpuidInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->RdmsrInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->WrmsrInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->ExceptionOccurredEventsHead);
    InitializeListHead(&g_Events->TscInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->PmcInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->InInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->OutInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->DebugRegistersAccessedEventsHead);
    InitializeListHead(&g_Events->ExternalInterruptOccurredEventsHead);
    InitializeListHead(&g_Events->VmcallInstructionExecutionEventsHead);
    InitializeListHead(&g_Events->TrapExecutionModeChangedEventsHead);
    InitializeListHead(&g_Events->TrapExecutionInstructionTraceEventsHead);
    InitializeListHead(&g_Events->ControlRegister3ModifiedEventsHead);
    InitializeListHead(&g_Events->ControlRegisterModifiedEventsHead);

    //
    // Enabled Debugger Events
    //
    g_EnableDebuggerEvents = TRUE;

    //
    // Set initial state of triggering events for VMCALLs
    //
    VmFuncSetTriggerEventForVmcalls(FALSE);

    //
    // Set initial state of triggering events for VMCALLs
    //
    VmFuncSetTriggerEventForCpuids(FALSE);

    //
    // Initialize script engines global variables holder
    //
    if (!g_ScriptGlobalVariables)
    {
        g_ScriptGlobalVariables = CrsAllocateNonPagedPool(MAX_VAR_COUNT * sizeof(UINT64));
    }

    if (!g_ScriptGlobalVariables)
    {
        //
        // Out of resource, initialization of script engine's global variable holders failed
        //
        return FALSE;
    }

    //
    // Zero the global variables memory
    //
    RtlZeroMemory(g_ScriptGlobalVariables, MAX_VAR_COUNT * sizeof(UINT64));

    //
    // Zero the TRAP FLAG state memory
    //
    RtlZeroMemory(&g_TrapFlagState, sizeof(DEBUGGER_TRAP_FLAG_STATE));

    //
    // Initialize the local and temp variables
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        CurrentDebuggerState = &g_DbgState[i];

        if (!CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable)
        {
            CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable = CrsAllocateNonPagedPool(MAX_VAR_COUNT * sizeof(UINT64));
        }

        if (!CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable)
        {
            //
            // Out of resource, initialization of script engine's local variable holders failed
            //
            return FALSE;
        }

        if (!CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable)
        {
            CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable = CrsAllocateNonPagedPool(MAX_TEMP_COUNT * sizeof(UINT64));
        }

        if (!CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable)
        {
            //
            // Out of resource, initialization of script engine's local variable holders failed
            //
            return FALSE;
        }

        //
        // Zero the local and temp variables memory
        //
        RtlZeroMemory(CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable, MAX_VAR_COUNT * sizeof(UINT64));
        RtlZeroMemory(CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable, MAX_TEMP_COUNT * sizeof(UINT64));
    }

    //
    // Initialize NMI broadcasting mechanism
    //
    VmFuncVmxBroadcastInitialize();

    //
    // Initialize attaching mechanism,
    // we'll use the functionalities of the attaching in reading modules
    // of user mode applications (other than attaching mechanism itself)
    //
    if (!AttachingInitialize())
    {
        return FALSE;
    }

    //
    // Pre-allocate pools for possible EPT hooks
    //
    ConfigureEptHookReservePreallocatedPoolsForEptHooks(MAXIMUM_NUMBER_OF_INITIAL_PREALLOCATED_EPT_HOOKS);

    if (!PoolManagerCheckAndPerformAllocationAndDeallocation())
    {
        LogWarning("Warning, cannot allocate the pre-allocated pools for EPT hooks");

        //
        // BTW, won't fail the starting phase because of this
        //
    }

    return TRUE;
}

/**
 * @brief Uninitialize Debugger Structures and Routines
 *
 */
VOID
DebuggerUninitialize()
{
    ULONG                       ProcessorsCount;
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState = NULL;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    //  *** Disable, terminate and clear all the events ***
    //

    //
    // Because we want to delete all the objects and buffers (pools)
    // after we finished termination, the debugger might still use
    // the buffers for events and action, for solving this problem
    // we first disable the tag(s) and this way the debugger no longer
    // use that event and this way we can safely remove and deallocate
    // the buffers later after termination
    //

    //
    // Disable triggering events
    //
    g_EnableDebuggerEvents = FALSE;

    //
    // Clear all events (Check if the kernel debugger is enable
    // and whether the instant event mechanism is working or not)
    //
    if (g_KernelDebuggerState && EnableInstantEventMechanism)
    {
        DebuggerClearAllEvents(FALSE, TRUE);
    }
    else
    {
        DebuggerClearAllEvents(FALSE, FALSE);
    }

    //
    // Uninitialize kernel debugger
    //
    KdUninitializeKernelDebugger();

    //
    // Uninitialize user debugger
    //
    UdUninitializeUserDebugger();

    //
    // Uninitialize NMI broadcasting mechanism
    //
    VmFuncVmxBroadcastUninitialize();

    //
    // Free g_Events
    //
    GlobalEventsFreeMemory();

    //
    // Free g_ScriptGlobalVariables
    //
    if (g_ScriptGlobalVariables != NULL)
    {
        CrsFreePool(g_ScriptGlobalVariables);
        g_ScriptGlobalVariables = NULL;
    }

    //
    // Free core specific local and temp variables
    //
    for (SIZE_T i = 0; i < ProcessorsCount; i++)
    {
        CurrentDebuggerState = &g_DbgState[i];

        if (CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable != NULL)
        {
            CrsFreePool(CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable);
            CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable = NULL;
        }

        if (CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable != NULL)
        {
            CrsFreePool(CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable);
            CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable = NULL;
        }
    }

    //
    // Free g_DbgState
    //
    GlobalDebuggingStateFreeMemory();
}

/**
 * @brief Create an Event Object
 *
 * @details should NOT be called in vmx-root
 *
 * @param Enabled Is the event enabled or disabled
 * @param CoreId The core id that this event is allowed to run
 * @param ProcessId The process id that this event is allowed to run
 * @param EventType The type of event
 * @param Tag User-mode generated unique tag (id) of the event
 * @param Options Optional parameters for the event
 * @param ConditionsBufferSize Size of condition code buffer (if any)
 * @param ConditionBuffer Address of condition code buffer (if any)
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return PDEBUGGER_EVENT Returns null in the case of error and event
 * object address when it's successful
 */
PDEBUGGER_EVENT
DebuggerCreateEvent(BOOLEAN                           Enabled,
                    UINT32                            CoreId,
                    UINT32                            ProcessId,
                    VMM_EVENT_TYPE_ENUM               EventType,
                    UINT64                            Tag,
                    DEBUGGER_EVENT_OPTIONS *          Options,
                    UINT32                            ConditionsBufferSize,
                    PVOID                             ConditionBuffer,
                    PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                    BOOLEAN                           InputFromVmxRoot)
{
    PDEBUGGER_EVENT Event           = NULL;
    UINT32          EventBufferSize = sizeof(DEBUGGER_EVENT) + ConditionsBufferSize;

    //
    // Initialize the event structure
    //
    if (InputFromVmxRoot)
    {
        //
        // *** The buffer is coming from VMX-root mode ***
        //

        //
        // If the buffer is smaller than regular instant events
        //
        if (REGULAR_INSTANT_EVENT_CONDITIONAL_BUFFER >= EventBufferSize)
        {
            //
            // The buffer fits into a regular instant event
            //
            Event = (DEBUGGER_EVENT *)PoolManagerRequestPool(INSTANT_REGULAR_EVENT_BUFFER, TRUE, REGULAR_INSTANT_EVENT_CONDITIONAL_BUFFER);

            if (!Event)
            {
                //
                // Here we try again to see if we could store it into a big instant event instead
                //
                Event = (DEBUGGER_EVENT *)PoolManagerRequestPool(INSTANT_BIG_EVENT_BUFFER, TRUE, BIG_INSTANT_EVENT_CONDITIONAL_BUFFER);

                if (!Event)
                {
                    //
                    // Set the error
                    //
                    ResultsToReturn->IsSuccessful = FALSE;
                    ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_REGULAR_PREALLOCATED_BUFFER_NOT_FOUND;

                    //
                    // There is a problem with allocating event
                    //
                    return NULL;
                }
            }
        }
        else if (BIG_INSTANT_EVENT_CONDITIONAL_BUFFER >= EventBufferSize)
        {
            //
            // The buffer fits into a big instant event
            //
            Event = (DEBUGGER_EVENT *)PoolManagerRequestPool(INSTANT_BIG_EVENT_BUFFER, TRUE, BIG_INSTANT_EVENT_CONDITIONAL_BUFFER);

            if (!Event)
            {
                //
                // Set the error
                //
                ResultsToReturn->IsSuccessful = FALSE;
                ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_BIG_PREALLOCATED_BUFFER_NOT_FOUND;

                //
                // There is a problem with allocating event
                //
                return NULL;
            }
        }
        else
        {
            //
            // The buffer doesn't fit into any of the regular or big event's preallocated buffers
            //

            //
            // Set the error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_PREALLOCATED_BUFFER_IS_NOT_ENOUGH_FOR_EVENT_AND_CONDITIONALS;

            return NULL;
        }
    }
    else
    {
        //
        // If it's not coming from the VMX-root mode then we're allocating it from the OS buffers
        //
        Event = CrsAllocateZeroedNonPagedPool(EventBufferSize);

        if (!Event)
        {
            //
            // Set the error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_UNABLE_TO_CREATE_EVENT;

            //
            // There is a problem with allocating event
            //
            return NULL;
        }
    }

    Event->CoreId         = CoreId;
    Event->ProcessId      = ProcessId;
    Event->Enabled        = Enabled;
    Event->EventType      = EventType;
    Event->Tag            = Tag;
    Event->CountOfActions = 0; // currently there is no action

    //
    // Copy Options
    //
    memcpy(&Event->InitOptions, Options, sizeof(DEBUGGER_EVENT_OPTIONS));

    //
    // check if this event is conditional or not
    //
    if (ConditionBuffer != 0)
    {
        //
        // It's conditional
        //
        Event->ConditionsBufferSize   = ConditionsBufferSize;
        Event->ConditionBufferAddress = (PVOID)((UINT64)Event + sizeof(DEBUGGER_EVENT));

        //
        // copy the condition buffer to the end of the buffer of the event
        //
        memcpy(Event->ConditionBufferAddress, ConditionBuffer, ConditionsBufferSize);
    }
    else
    {
        //
        // It's unconditioanl
        //
        Event->ConditionsBufferSize = 0;
    }

    //
    // Make the action lists ready
    //
    InitializeListHead(&Event->ActionsListHead);

    //
    // Return our event
    //
    return Event;
}

/**
 * @brief Allocates buffer for requested safe buffer
 *
 * @param SizeOfRequestedSafeBuffer The size of the requested safe buffer
 * @param ResultsToReturn The buffer address that should be returned
 * to the user-mode as the result
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return PVOID
 */
PVOID
DebuggerAllocateSafeRequestedBuffer(SIZE_T                            SizeOfRequestedSafeBuffer,
                                    PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                                    BOOLEAN                           InputFromVmxRoot)
{
    PVOID RequestedBuffer = NULL;

    //
    // Check whether the buffer comes from VMX-root mode or non-root mode
    //
    if (InputFromVmxRoot)
    {
        //
        // *** The buffer is coming from VMX-root mode ***
        //

        //
        // If the requested safe buffer is smaller than regular safe buffers
        //
        if (REGULAR_INSTANT_EVENT_REQUESTED_SAFE_BUFFER >= SizeOfRequestedSafeBuffer)
        {
            //
            // The buffer fits into a regular safe requested buffer
            //
            RequestedBuffer = (PVOID)PoolManagerRequestPool(INSTANT_REGULAR_SAFE_BUFFER_FOR_EVENTS, TRUE, REGULAR_INSTANT_EVENT_REQUESTED_SAFE_BUFFER);

            if (!RequestedBuffer)
            {
                //
                // Here we try again to see if we could store it into a big instant event safe requested buffer instead
                //
                RequestedBuffer = (PVOID)PoolManagerRequestPool(INSTANT_BIG_SAFE_BUFFER_FOR_EVENTS, TRUE, BIG_INSTANT_EVENT_REQUESTED_SAFE_BUFFER);

                if (!RequestedBuffer)
                {
                    //
                    // Set the error
                    //
                    ResultsToReturn->IsSuccessful = FALSE;
                    ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_REGULAR_REQUESTED_SAFE_BUFFER_NOT_FOUND;

                    //
                    // There is a problem with allocating requested safe buffer
                    //
                    return NULL;
                }
            }
        }
        else if (BIG_INSTANT_EVENT_REQUESTED_SAFE_BUFFER >= SizeOfRequestedSafeBuffer)
        {
            //
            // The buffer fits into a big instant requested safe buffer
            //
            RequestedBuffer = (PVOID)PoolManagerRequestPool(INSTANT_BIG_SAFE_BUFFER_FOR_EVENTS, TRUE, BIG_INSTANT_EVENT_REQUESTED_SAFE_BUFFER);

            if (!RequestedBuffer)
            {
                //
                // Set the error
                //
                ResultsToReturn->IsSuccessful = FALSE;
                ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_BIG_REQUESTED_SAFE_BUFFER_NOT_FOUND;

                //
                // There is a problem with allocating event
                //
                return NULL;
            }
        }
        else
        {
            //
            // The buffer doesn't fit into any of the regular or big safe requested buffers
            //

            //
            // Set the error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_PREALLOCATED_BUFFER_IS_NOT_ENOUGH_FOR_REQUESTED_SAFE_BUFFER;

            return NULL;
        }
    }
    else
    {
        RequestedBuffer = CrsAllocateZeroedNonPagedPool(SizeOfRequestedSafeBuffer);

        if (!RequestedBuffer)
        {
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_UNABLE_TO_ALLOCATE_REQUESTED_SAFE_BUFFER;

            return NULL;
        }
    }

    return RequestedBuffer;
}

/**
 * @brief Create an action and add the action to an event
 *
 * @param Event Target event object
 * @param ActionType Type of action
 * @param SendTheResultsImmediately whether the results should be received
 * by the user-mode immediately
 * @param InTheCaseOfCustomCode Custom code structure (if any)
 * @param InTheCaseOfRunScript Run script structure (if any)
 * @param ResultsToReturn The buffer address that should be returned
 * to the user-mode as the result
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return PDEBUGGER_EVENT_ACTION
 */
PDEBUGGER_EVENT_ACTION
DebuggerAddActionToEvent(PDEBUGGER_EVENT                                 Event,
                         DEBUGGER_EVENT_ACTION_TYPE_ENUM                 ActionType,
                         BOOLEAN                                         SendTheResultsImmediately,
                         PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE             InTheCaseOfCustomCode,
                         PDEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION InTheCaseOfRunScript,
                         PDEBUGGER_EVENT_AND_ACTION_RESULT               ResultsToReturn,
                         BOOLEAN                                         InputFromVmxRoot)
{
    PDEBUGGER_EVENT_ACTION Action;
    SIZE_T                 ActionBufferSize;
    PVOID                  RequestedBuffer = NULL;

    //
    // Allocate action + allocate code for custom code
    //

    if (InTheCaseOfCustomCode != NULL)
    {
        //
        // We should allocate extra buffer for custom code
        //
        ActionBufferSize = sizeof(DEBUGGER_EVENT_ACTION) + InTheCaseOfCustomCode->CustomCodeBufferSize;
    }
    else if (InTheCaseOfRunScript != NULL)
    {
        //
        // We should allocate extra buffer for script
        //
        ActionBufferSize = sizeof(DEBUGGER_EVENT_ACTION) + InTheCaseOfRunScript->ScriptLength;
    }
    else
    {
        //
        // We shouldn't allocate extra buffer as there is no custom code
        //
        ActionBufferSize = sizeof(DEBUGGER_EVENT_ACTION);
    }

    //
    // Allocate buffer for storing the action
    //

    if (InputFromVmxRoot)
    {
        //
        // *** The buffer is coming from VMX-root mode ***
        //

        //
        // If the buffer is smaller than regular instant events's action
        //
        if (REGULAR_INSTANT_EVENT_ACTION_BUFFER >= ActionBufferSize)
        {
            //
            // The buffer fits into a regular instant event's action
            //
            Action = (DEBUGGER_EVENT_ACTION *)PoolManagerRequestPool(INSTANT_REGULAR_EVENT_ACTION_BUFFER, TRUE, REGULAR_INSTANT_EVENT_ACTION_BUFFER);

            if (!Action)
            {
                //
                // Here we try again to see if we could store it into a big instant event's action buffer instead
                //
                Action = (DEBUGGER_EVENT_ACTION *)PoolManagerRequestPool(INSTANT_BIG_EVENT_ACTION_BUFFER, TRUE, BIG_INSTANT_EVENT_ACTION_BUFFER);

                if (!Action)
                {
                    //
                    // Set the error
                    //
                    ResultsToReturn->IsSuccessful = FALSE;
                    ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_ACTION_REGULAR_PREALLOCATED_BUFFER_NOT_FOUND;

                    //
                    // There is a problem with allocating event's action
                    //
                    return NULL;
                }
            }
        }
        else if (BIG_INSTANT_EVENT_ACTION_BUFFER >= ActionBufferSize)
        {
            //
            // The buffer fits into a big instant event's action buffer
            //
            Action = (DEBUGGER_EVENT_ACTION *)PoolManagerRequestPool(INSTANT_BIG_EVENT_ACTION_BUFFER, TRUE, BIG_INSTANT_EVENT_ACTION_BUFFER);

            if (!Action)
            {
                //
                // Set the error
                //
                ResultsToReturn->IsSuccessful = FALSE;
                ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_ACTION_BIG_PREALLOCATED_BUFFER_NOT_FOUND;

                //
                // There is a problem with allocating event's action buffer
                //
                return NULL;
            }
        }
        else
        {
            //
            // The buffer doesn't fit into any of the regular or big event's action preallocated buffers
            //

            //
            // Set the error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_PREALLOCATED_BUFFER_IS_NOT_ENOUGH_FOR_ACTION_BUFFER;

            return NULL;
        }
    }
    else
    {
        //
        // If it's not coming from the VMX-root mode then we're allocating it from the OS buffers
        //
        Action = CrsAllocateZeroedNonPagedPool(ActionBufferSize);

        if (Action == NULL)
        {
            //
            // Set the appropriate error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_UNABLE_TO_CREATE_ACTION_CANNOT_ALLOCATE_BUFFER;

            //
            // There was an error in allocation
            //
            return NULL;
        }
    }

    //
    // If the user needs a buffer to be passed to the debugger then
    // we should allocate it here (Requested buffer is only available for custom code types)
    //
    if (ActionType == RUN_CUSTOM_CODE &&
        InTheCaseOfCustomCode != NULL &&
        InTheCaseOfCustomCode->OptionalRequestedBufferSize != 0)
    {
        //
        // Check if the optional buffer is not more that the size
        // we can send to usermode
        //
        if (InTheCaseOfCustomCode->OptionalRequestedBufferSize >= MaximumPacketsCapacity)
        {
            //
            // There was an error
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);
            }
            else
            {
                CrsFreePool(Action);
            }

            //
            // Set the appropriate error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_REQUESTED_OPTIONAL_BUFFER_IS_BIGGER_THAN_DEBUGGERS_SEND_RECEIVE_STACK;

            return NULL;
        }

        //
        // User needs a buffer to play with
        //
        RequestedBuffer = DebuggerAllocateSafeRequestedBuffer(InTheCaseOfCustomCode->OptionalRequestedBufferSize, ResultsToReturn, InputFromVmxRoot);

        if (!RequestedBuffer)
        {
            //
            // There was an error in allocation
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);
            }
            else
            {
                CrsFreePool(Action);
            }

            //
            // Not need to set error as the above function already adjust the error
            //
            return NULL;
        }

        //
        // Add it to the action
        //
        Action->RequestedBuffer.EnabledRequestBuffer = TRUE;
        Action->RequestedBuffer.RequestBufferSize    = InTheCaseOfCustomCode->OptionalRequestedBufferSize;
        Action->RequestedBuffer.RequstBufferAddress  = (UINT64)RequestedBuffer;
    }

    //
    // If the user needs a buffer to be passed to the debugger script then
    // we should allocate it here (Requested buffer is only available for custom code types)
    //
    if (ActionType == RUN_SCRIPT &&
        InTheCaseOfRunScript != NULL &&
        InTheCaseOfRunScript->OptionalRequestedBufferSize != 0)
    {
        //
        // Check if the optional buffer is not more that the size
        // we can send to usermode
        //
        if (InTheCaseOfRunScript->OptionalRequestedBufferSize >= MaximumPacketsCapacity)
        {
            //
            // There was an error
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);
            }
            else
            {
                CrsFreePool(Action);
            }

            //
            // Set the appropriate error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INSTANT_EVENT_REQUESTED_OPTIONAL_BUFFER_IS_BIGGER_THAN_DEBUGGERS_SEND_RECEIVE_STACK;

            return NULL;
        }

        //
        // User needs a buffer to play with
        //
        RequestedBuffer = DebuggerAllocateSafeRequestedBuffer(InTheCaseOfRunScript->OptionalRequestedBufferSize, ResultsToReturn, InputFromVmxRoot);

        if (!RequestedBuffer)
        {
            //
            // There was an error in allocation
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);
            }
            else
            {
                CrsFreePool(Action);
            }

            //
            // Not need to set error as the above function already adjust the error
            //
            return NULL;
        }

        //
        // Add it to the action
        //
        Action->RequestedBuffer.EnabledRequestBuffer = TRUE;
        Action->RequestedBuffer.RequestBufferSize    = InTheCaseOfRunScript->OptionalRequestedBufferSize;
        Action->RequestedBuffer.RequstBufferAddress  = (UINT64)RequestedBuffer;
    }

    if (ActionType == RUN_CUSTOM_CODE && InTheCaseOfCustomCode != NULL)
    {
        //
        // Check if it's a Custom code without custom code buffer which is invalid
        //
        if (InTheCaseOfCustomCode != NULL && InTheCaseOfCustomCode->CustomCodeBufferSize == 0)
        {
            //
            // There was an error
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);

                if (RequestedBuffer != NULL)
                {
                    PoolManagerFreePool((UINT64)RequestedBuffer);
                }
            }
            else
            {
                CrsFreePool(Action);

                if (RequestedBuffer != NULL)
                {
                    CrsFreePool(RequestedBuffer);
                }
            }

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO;

            return NULL;
        }

        //
        // Move the custom code buffer to the end of the action
        //
        Action->CustomCodeBufferSize    = InTheCaseOfCustomCode->CustomCodeBufferSize;
        Action->CustomCodeBufferAddress = (PVOID)((UINT64)Action + sizeof(DEBUGGER_EVENT_ACTION));

        //
        // copy the custom code buffer to the end of the buffer of the action
        //
        memcpy(Action->CustomCodeBufferAddress, InTheCaseOfCustomCode->CustomCodeBufferAddress, InTheCaseOfCustomCode->CustomCodeBufferSize);
    }

    //
    // If it's run script action type
    //
    else if (ActionType == RUN_SCRIPT && InTheCaseOfRunScript != NULL)
    {
        //
        // Check the buffers of run script
        //
        if (InTheCaseOfRunScript->ScriptBuffer == NULL64_ZERO || InTheCaseOfRunScript->ScriptLength == NULL_ZERO)
        {
            //
            // There was an error
            //
            if (InputFromVmxRoot)
            {
                PoolManagerFreePool((UINT64)Action);

                if (RequestedBuffer != 0)
                {
                    PoolManagerFreePool((UINT64)RequestedBuffer);
                }
            }
            else
            {
                CrsFreePool(Action);

                if (RequestedBuffer != 0)
                {
                    CrsFreePool(RequestedBuffer);
                }
            }

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO;

            return NULL;
        }

        //
        // Allocate the buffer from a non-page pool on the script
        //
        Action->ScriptConfiguration.ScriptBuffer = (UINT64)((BYTE *)Action + sizeof(DEBUGGER_EVENT_ACTION));

        //
        // Copy the memory of script to our non-paged pool
        //
        RtlCopyMemory((void *)Action->ScriptConfiguration.ScriptBuffer, (const void *)InTheCaseOfRunScript->ScriptBuffer, InTheCaseOfRunScript->ScriptLength);

        //
        // Set other fields
        //
        Action->ScriptConfiguration.ScriptLength                = InTheCaseOfRunScript->ScriptLength;
        Action->ScriptConfiguration.ScriptPointer               = InTheCaseOfRunScript->ScriptPointer;
        Action->ScriptConfiguration.OptionalRequestedBufferSize = InTheCaseOfRunScript->OptionalRequestedBufferSize;
    }

    //
    // Create an order code for the current action
    // and also increase the Count of action in event
    //
    Event->CountOfActions++;
    Action->ActionOrderCode = Event->CountOfActions;

    //
    // Fill other parts of the action
    //
    Action->ImmediatelySendTheResults = SendTheResultsImmediately;
    Action->ActionType                = ActionType;
    Action->Tag                       = Event->Tag;

    //
    // Now we should add the action to the event's LIST_ENTRY of actions
    //
    InsertHeadList(&Event->ActionsListHead, &(Action->ActionsList));

    return Action;
}

/**
 * @brief Register an event to a list of active events
 *
 * @param Event Event structure
 * @return BOOLEAN TRUE if it successfully registered and FALSE if not registered
 */
BOOLEAN
DebuggerRegisterEvent(PDEBUGGER_EVENT Event)
{
    PLIST_ENTRY TargetEventList = NULL;

    //
    // Register the event
    //
    TargetEventList = DebuggerGetEventListByEventType(Event->EventType);

    if (TargetEventList != NULL)
    {
        InsertHeadList(TargetEventList, &(Event->EventsOfSameTypeList));

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Trigger events of a special type to be managed by debugger
 *
 * @param EventType Type of events
 * @param CallingStage Stage of calling (pre-event or post-event)
 * @param Context An optional parameter (different in each event)
 * @param PostEventRequired Whether the caller is requested to
 * trigger a post-event event
 * @param Regs Guest gp-registers
 *
 * @return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE returns the status
 * of handling events
 */
VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE
DebuggerTriggerEvents(VMM_EVENT_TYPE_ENUM                   EventType,
                      VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE CallingStage,
                      PVOID                                 Context,
                      BOOLEAN *                             PostEventRequired,
                      GUEST_REGS *                          Regs)
{
    DebuggerCheckForCondition *      ConditionFunc;
    DEBUGGER_TRIGGERED_EVENT_DETAILS EventTriggerDetail = {0};
    PEPT_HOOKS_CONTEXT               EptContext;
    PLIST_ENTRY                      TempList  = 0;
    PLIST_ENTRY                      TempList2 = 0;
    PROCESSOR_DEBUGGING_STATE *      DbgState  = NULL;

    //
    // Check if triggering debugging actions are allowed or not
    //
    if (!g_EnableDebuggerEvents || g_InterceptBreakpointsAndEventsForCommandsInRemoteComputer)
    {
        //
        // Debugger is not enabled
        //
        return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_DEBUGGER_NOT_ENABLED;
    }

    //
    // Find the debugging state structure
    //
    DbgState = &g_DbgState[KeGetCurrentProcessorNumberEx(NULL)];

    //
    // Set the registers for debug state
    //
    DbgState->Regs = Regs;

    //
    // Find the debugger events list base on the type of the event
    //
    TempList  = DebuggerGetEventListByEventType(EventType);
    TempList2 = TempList;

    if (TempList == NULL)
    {
        return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_INVALID_EVENT_TYPE;
    }

    while (TempList2 != TempList->Flink)
    {
        TempList                     = TempList->Flink;
        PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

        //
        // check if the event is enabled or not
        //
        if (!CurrentEvent->Enabled)
        {
            continue;
        }

        //
        // Check if this event is for this core or not
        //
        if (CurrentEvent->CoreId != DEBUGGER_EVENT_APPLY_TO_ALL_CORES && CurrentEvent->CoreId != DbgState->CoreId)
        {
            //
            // This event is not related to either or core or all cores
            //
            continue;
        }

        //
        // Check if this event is for this process or not
        //
        if (CurrentEvent->ProcessId != DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES && CurrentEvent->ProcessId != HANDLE_TO_UINT32(PsGetCurrentProcessId()))
        {
            //
            // This event is not related to either our process or all processes
            //
            continue;
        }

        //
        // Check event type specific conditions
        //
        switch (CurrentEvent->EventType)
        {
        case EXTERNAL_INTERRUPT_OCCURRED:

            //
            // For external interrupt exiting events we check whether the
            // vector match the event's vector or not
            //
            // Context is the physical address
            //
            if ((UINT64)Context != CurrentEvent->Options.OptionalParam1)
            {
                //
                // The interrupt is not for this event
                //
                continue;
            }

            break;

        case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
        case HIDDEN_HOOK_READ_AND_WRITE:
        case HIDDEN_HOOK_READ_AND_EXECUTE:
        case HIDDEN_HOOK_WRITE_AND_EXECUTE:
        case HIDDEN_HOOK_READ:
        case HIDDEN_HOOK_WRITE:
        case HIDDEN_HOOK_EXECUTE:

            //
            // For hidden hook read/write/execute we check whether the address
            // is in the range of what user specified or not, this is because
            // we get the events for all hidden hooks in a page granularity
            //

            EptContext = (PEPT_HOOKS_CONTEXT)Context;

            //
            // Context should be checked with hooking tag
            // The hooking tag is same as the event tag if both
            // of them match together
            //

            if (EptContext->HookingTag != CurrentEvent->Tag)
            {
                //
                // The value is not within our expected range
                //
                continue;
            }
            else
            {
                //
                // Fix the context to virtual address
                //
                Context = (PVOID)EptContext->VirtualAddress;
            }

            break;

        case HIDDEN_HOOK_EXEC_CC:

            //
            // Here we check if it's HIDDEN_HOOK_EXEC_CC then it means
            // so we have to make sure to perform its actions only if
            // the hook is triggered for the address described in
            // event, note that address in event is a virtual address
            //
            if ((UINT64)Context != CurrentEvent->Options.OptionalParam1)
            {
                //
                // Context is the virtual address
                //

                //
                // The hook is not for this (virtual) address
                //
                continue;
            }

            break;

        case HIDDEN_HOOK_EXEC_DETOURS:

            //
            // Here we check if it's HIDDEN_HOOK_EXEC_DETOURS
            // then it means that it's detours hidden hook exec so we have
            // to make sure to perform its actions, only if the hook is triggered
            // for the address described in event, note that address in event is
            // a physical address and the address that the function that triggers
            // these events and sent here as the context is also converted to its
            // physical form
            // This way we are sure that no one can bypass our hook by remapping
            // address to another virtual address as everything is physical
            //
            if (((PEPT_HOOKS_CONTEXT)Context)->PhysicalAddress != CurrentEvent->Options.OptionalParam1)
            {
                //
                // Context is the physical address
                //

                //
                // The hook is not for this (physical) address
                //
                continue;
            }
            else
            {
                //
                // Convert it to virtual address
                //
                Context = (PVOID)(((PEPT_HOOKS_CONTEXT)Context)->VirtualAddress);
            }

            break;

        case RDMSR_INSTRUCTION_EXECUTION:
        case WRMSR_INSTRUCTION_EXECUTION:

            //
            // check if MSR exit is what we want or not
            //
            if (CurrentEvent->Options.OptionalParam1 != DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS && CurrentEvent->Options.OptionalParam1 != (UINT64)Context)
            {
                //
                // The msr is not what we want
                //
                continue;
            }

            break;

        case EXCEPTION_OCCURRED:

            //
            // check if exception is what we need or not
            //
            if (CurrentEvent->Options.OptionalParam1 != DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES && CurrentEvent->Options.OptionalParam1 != (UINT64)Context)
            {
                //
                // The exception is not what we want
                //
                continue;
            }

            break;

        case IN_INSTRUCTION_EXECUTION:
        case OUT_INSTRUCTION_EXECUTION:

            //
            // check if I/O port is what we want or not
            //
            if (CurrentEvent->Options.OptionalParam1 != DEBUGGER_EVENT_ALL_IO_PORTS && CurrentEvent->Options.OptionalParam1 != (UINT64)Context)
            {
                //
                // The port is not what we want
                //
                continue;
            }

            break;

        case SYSCALL_HOOK_EFER_SYSCALL:

            //
            // case SYSCALL_HOOK_EFER_SYSRET:
            //
            // I don't know how to find syscall number when sysret is executed so
            // that's why we don't support extra argument for sysret
            //

            //
            // check syscall number
            //
            if (CurrentEvent->Options.OptionalParam1 != DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS && CurrentEvent->Options.OptionalParam1 != (UINT64)Context)
            {
                //
                // The syscall number is not what we want
                //
                continue;
            }

            break;

        case CPUID_INSTRUCTION_EXECUTION:

            //
            // check if CPUID is what we want or not
            //
            if (CurrentEvent->Options.OptionalParam1 != (UINT64)NULL /*FALSE*/ && CurrentEvent->Options.OptionalParam2 != (UINT64)Context)
            {
                //
                // The CPUID is not what we want (and the user didn't intend to get all CPUIDs)
                //
                continue;
            }

            break;

        case CONTROL_REGISTER_MODIFIED:

            //
            // check if CR exit is what we want or not
            //
            if (CurrentEvent->Options.OptionalParam1 != (UINT64)Context)
            {
                //
                // The CR is not what we want
                //
                continue;
            }

            break;

        case TRAP_EXECUTION_MODE_CHANGED:

            //
            // check if the debugger needs user-to-kernel or kernel-to-user events
            //
            if (CurrentEvent->Options.OptionalParam1 != DEBUGGER_EVENT_MODE_TYPE_USER_MODE_AND_KERNEL_MODE)
            {
                if ((CurrentEvent->Options.OptionalParam1 == DEBUGGER_EVENT_MODE_TYPE_USER_MODE &&
                     Context == (PVOID)DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE) ||
                    (CurrentEvent->Options.OptionalParam1 == DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE &&
                     Context == (PVOID)DEBUGGER_EVENT_MODE_TYPE_USER_MODE))
                {
                    continue;
                }
            }

            break;

        default:
            break;
        }

        //
        // Check the stage of calling (pre, all, or post event)
        //
        if (CallingStage == VMM_CALLBACK_CALLING_STAGE_PRE_EVENT_EMULATION &&
            (CurrentEvent->EventMode == VMM_CALLBACK_CALLING_STAGE_ALL_EVENT_EMULATION ||
             CurrentEvent->EventMode == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION))
        {
            //
            // Here it means that the current event is a post, or all event event
            // and the current stage of calling is for the pre-event events, thus
            // this event is not supposed to be ran at the current stage.
            // However, we'll set a flag so the caller will know that there is
            // a valid post-event available for the parameters related to this
            // event.
            // This mechanism notifies the caller to trigger the event after
            // emulation, we implement it in a way that the caller knows when
            // to trigger a post-event thus it optimizes the number of times
            // that the caller triggers the events and avoid unnecessary triggering
            // of the event (for post-event) but at the same time we have the
            // flexibility of having both pre-event and post-event concepts
            //
            *PostEventRequired = TRUE;

            if (CurrentEvent->EventMode == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION)
            {
                //
                // If it's not an 'all' event and it is only the 'post' event,
                // then we ignore the trigger stage
                //
                continue;
            }
        }

        //
        // Check if condition is met or not , if the condition
        // is not met then we have to avoid performing the actions
        //
        if (CurrentEvent->ConditionsBufferSize != 0)
        {
            //
            // Means that there is some conditions
            //
            ConditionFunc = (DebuggerCheckForCondition *)CurrentEvent->ConditionBufferAddress;

            //
            // Run and check for results
            //
            // Because the user might change the nonvolatile registers, we save fastcall nonvolatile registers
            //
            if (AsmDebuggerConditionCodeHandler((UINT64)DbgState->Regs, (UINT64)Context, (UINT64)ConditionFunc) == 0)
            {
                //
                // The condition function returns null, mean that the
                // condition didn't met, we can ignore this event
                //
                continue;
            }
        }

        //
        // Reset the event ignorance mechanism (apply 'sc on/off' to the events)
        //
        DbgState->ShortCircuitingEvent = CurrentEvent->EnableShortCircuiting;

        //
        // Setup event trigger detail
        //
        EventTriggerDetail.Context = Context;
        EventTriggerDetail.Tag     = CurrentEvent->Tag;
        EventTriggerDetail.Stage   = CallingStage;

        //
        // perform the actions
        //
        DebuggerPerformActions(DbgState, CurrentEvent, &EventTriggerDetail);
    }

    //
    // Check if the event should be ignored or not
    //
    if (DbgState->ShortCircuitingEvent)
    {
        //
        // Reset the event ignorance (short-circuit) mechanism
        //
        DbgState->ShortCircuitingEvent = FALSE;

        //
        // Event should be ignored
        //
        return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT;
    }
    else
    {
        //
        // Event shouldn't be ignored
        //
        return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL;
    }
}

/**
 * @brief Run a special event's action(s)
 *
 * @param DbgState The state of the debugger on the current core
 * @param Event Event Object
 * @param EventTriggerDetail Event trigger details
 *
 * @return VOID
 */
VOID
DebuggerPerformActions(PROCESSOR_DEBUGGING_STATE *        DbgState,
                       DEBUGGER_EVENT *                   Event,
                       DEBUGGER_TRIGGERED_EVENT_DETAILS * EventTriggerDetail)
{
    PLIST_ENTRY TempList = 0;

    //
    // Find and run all the actions in this Event
    //
    TempList = &Event->ActionsListHead;
    while (&Event->ActionsListHead != TempList->Flink)
    {
        TempList                             = TempList->Flink;
        PDEBUGGER_EVENT_ACTION CurrentAction = CONTAINING_RECORD(TempList, DEBUGGER_EVENT_ACTION, ActionsList);

        //
        // Perform the action
        //
        switch (CurrentAction->ActionType)
        {
        case BREAK_TO_DEBUGGER:

            DebuggerPerformBreakToDebugger(DbgState, CurrentAction, EventTriggerDetail);

            break;

        case RUN_SCRIPT:

            DebuggerPerformRunScript(DbgState, CurrentAction, NULL, EventTriggerDetail);

            break;

        case RUN_CUSTOM_CODE:

            DebuggerPerformRunTheCustomCode(DbgState, CurrentAction, EventTriggerDetail);

            break;

        default:

            //
            // Invalid action type
            //
            break;
        }
    }
}

/**
 * @brief Managing run script action
 *
 * @param DbgState The state of the debugger on the current core
 * @param Action Action object
 * @param ScriptDetails Details of script
 * @param EventTriggerDetail Event trigger detail
 * @return BOOLEAN
 */
BOOLEAN
DebuggerPerformRunScript(PROCESSOR_DEBUGGING_STATE *        DbgState,
                         DEBUGGER_EVENT_ACTION *            Action,
                         DEBUGGEE_SCRIPT_PACKET *           ScriptDetails,
                         DEBUGGER_TRIGGERED_EVENT_DETAILS * EventTriggerDetail)
{
    SYMBOL_BUFFER                CodeBuffer    = {0};
    ACTION_BUFFER                ActionBuffer  = {0};
    SYMBOL                       ErrorSymbol   = {0};
    SCRIPT_ENGINE_VARIABLES_LIST VariablesList = {0};

    if (Action != NULL)
    {
        //
        // Fill the action buffer's calling stage
        //
        if (EventTriggerDetail->Stage == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION)
        {
            ActionBuffer.CallingStage = 1;
        }
        else
        {
            ActionBuffer.CallingStage = 0;
        }

        //
        // Fill the action buffer
        //
        ActionBuffer.Context                   = (UINT64)EventTriggerDetail->Context;
        ActionBuffer.Tag                       = EventTriggerDetail->Tag;
        ActionBuffer.ImmediatelySendTheResults = Action->ImmediatelySendTheResults;
        ActionBuffer.CurrentAction             = (UINT64)Action;

        //
        // Context point to the registers
        //
        CodeBuffer.Head    = (PSYMBOL)Action->ScriptConfiguration.ScriptBuffer;
        CodeBuffer.Size    = Action->ScriptConfiguration.ScriptLength;
        CodeBuffer.Pointer = Action->ScriptConfiguration.ScriptPointer;
    }
    else if (ScriptDetails != NULL)
    {
        //
        // Fill the action buffer's calling stage
        //
        if (EventTriggerDetail->Stage == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION)
        {
            ActionBuffer.CallingStage = 1;
        }
        else
        {
            ActionBuffer.CallingStage = 0;
        }

        //
        // Fill the action buffer
        //
        ActionBuffer.Context                   = (UINT64)EventTriggerDetail->Context;
        ActionBuffer.Tag                       = EventTriggerDetail->Tag;
        ActionBuffer.ImmediatelySendTheResults = TRUE;
        ActionBuffer.CurrentAction             = (UINT64)NULL;

        //
        // Context point to the registers
        //
        CodeBuffer.Head    = (SYMBOL *)((CHAR *)ScriptDetails + sizeof(DEBUGGEE_SCRIPT_PACKET));
        CodeBuffer.Size    = ScriptDetails->ScriptBufferSize;
        CodeBuffer.Pointer = ScriptDetails->ScriptBufferPointer;
    }
    else
    {
        //
        // The parameters are wrong !
        //
        return FALSE;
    }

    //
    // Fill the variables list for this run
    //
    VariablesList.GlobalVariablesList = g_ScriptGlobalVariables;
    VariablesList.LocalVariablesList  = DbgState->ScriptEngineCoreSpecificLocalVariable;
    VariablesList.TempList            = DbgState->ScriptEngineCoreSpecificTempVariable;

    for (UINT64 i = 0; i < CodeBuffer.Pointer;)
    {
        //
        // If has error, show error message and abort.
        //

        if (ScriptEngineExecute(DbgState->Regs,
                                &ActionBuffer,
                                &VariablesList,
                                &CodeBuffer,
                                &i,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &ErrorSymbol) == TRUE)
        {
            CHAR NameOfOperator[MAX_FUNCTION_NAME_LENGTH] = {0};
            ScriptEngineGetOperatorName(&ErrorSymbol, NameOfOperator);
            LogInfo("Invalid returning address for operator: %s", NameOfOperator);
            break;
        }
    }

    return TRUE;
}

/**
 * @brief Manage running the custom code action
 *
 * @param DbgState The state of the debugger on the current core
 * @param Action Action object
 * @param EventTriggerDetail Event trigger detail
 *
 * @return VOID
 */
VOID
DebuggerPerformRunTheCustomCode(PROCESSOR_DEBUGGING_STATE *        DbgState,
                                DEBUGGER_EVENT_ACTION *            Action,
                                DEBUGGER_TRIGGERED_EVENT_DETAILS * EventTriggerDetail)
{
    if (Action->CustomCodeBufferSize == 0)
    {
        //
        // Sth went wrong ! the buffer size for custom code shouldn't be zero
        //
        return;
    }

    //
    // -----------------------------------------------------------------------------------------------------
    // Test
    //
    // LogInfo("%X       Called from : %llx", Tag, Context);
    //
    //
    // LogInfo("Process Id : %x , Rax : %llx , R8 : %llx , Context : 0x%llx ", PsGetCurrentProcessId(), Regs->rax, Regs->r8, Context);
    // return;
    //
    // -----------------------------------------------------------------------------------------------------
    //

    //
    // Run the custom code
    //
    if (Action->RequestedBuffer.RequestBufferSize == 0)
    {
        //
        // Because the user might change the nonvolatile registers, we save fastcall nonvolatile registers
        //
        AsmDebuggerCustomCodeHandler((UINT64)NULL,
                                     (UINT64)DbgState->Regs,
                                     (UINT64)EventTriggerDetail->Context,
                                     (UINT64)Action->CustomCodeBufferAddress);
    }
    else
    {
        //
        // Because the user might change the nonvolatile registers, we save fastcall nonvolatile registers
        //
        AsmDebuggerCustomCodeHandler((UINT64)Action->RequestedBuffer.RequstBufferAddress,
                                     (UINT64)DbgState->Regs,
                                     (UINT64)EventTriggerDetail->Context,
                                     (UINT64)Action->CustomCodeBufferAddress);
    }
}

/**
 * @brief Manage breaking to the debugger action
 *
 * @param DbgState The state of the debugger on the current core
 * @param Tag Tag of event
 * @param Action Action object
 * @param Context Optional parameter
 * @param EventTriggerDetail Event trigger detail
 *
 * @return VOID
 */
VOID
DebuggerPerformBreakToDebugger(PROCESSOR_DEBUGGING_STATE *        DbgState,
                               DEBUGGER_EVENT_ACTION *            Action,
                               DEBUGGER_TRIGGERED_EVENT_DETAILS * EventTriggerDetail)
{
    UNREFERENCED_PARAMETER(Action);

    if (VmFuncVmxGetCurrentExecutionMode() == TRUE)
    {
        //
        // The guest is already in vmx-root mode
        // Halt other cores
        //

        KdHandleBreakpointAndDebugBreakpoints(
            DbgState,
            DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,
            EventTriggerDetail);
    }
    else
    {
        //
        // The guest is on vmx non-root mode and this is an event
        //
        VmFuncVmxVmcall(DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT,
                        (UINT64)EventTriggerDetail,
                        (UINT64)DbgState->Regs,
                        (UINT64)NULL);
    }
}

/**
 * @brief Find event object by tag
 *
 * @param Tag Tag of event
 * @return PDEBUGGER_EVENT Returns null if not found and event object if found
 */
PDEBUGGER_EVENT
DebuggerGetEventByTag(UINT64 Tag)
{
    PLIST_ENTRY TempList  = 0;
    PLIST_ENTRY TempList2 = 0;

    //
    // We have to iterate through all events
    //
    for (size_t i = 0; i < sizeof(DEBUGGER_CORE_EVENTS) / sizeof(LIST_ENTRY); i++)
    {
        TempList  = (PLIST_ENTRY)((UINT64)(g_Events) + (i * sizeof(LIST_ENTRY)));
        TempList2 = TempList;

        while (TempList2 != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // Check if we find the event or not
            //
            if (CurrentEvent->Tag == Tag)
            {
                return CurrentEvent;
            }
        }
    }

    //
    // We didn't find anything, so return null
    //
    return NULL;
}

/**
 * @brief Enable or disable all events from all the types
 *
 * @param IsEnable If you want to enable then true and if
 * you want to disable then false
 * @return BOOLEAN if at least one event enabled/disabled then
 * it returns true, and otherwise false
 */
BOOLEAN
DebuggerEnableOrDisableAllEvents(BOOLEAN IsEnable)
{
    BOOLEAN     FindAtLeastOneEvent = FALSE;
    PLIST_ENTRY TempList            = 0;
    PLIST_ENTRY TempList2           = 0;

    //
    // We have to iterate through all events
    //
    for (size_t i = 0; i < sizeof(DEBUGGER_CORE_EVENTS) / sizeof(LIST_ENTRY); i++)
    {
        TempList  = (PLIST_ENTRY)((UINT64)(g_Events) + (i * sizeof(LIST_ENTRY)));
        TempList2 = TempList;

        while (TempList2 != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // Check if we find at least one event or not
            //
            if (!FindAtLeastOneEvent)
            {
                FindAtLeastOneEvent = TRUE;
            }

            //
            // Enable or disable event
            // (We could directly modify the "enabled" flag here, however
            // in the case of any possible callback for enabling/disabling let's
            // modify the state of being enable all of them in a single place)
            //
            if (IsEnable)
            {
                DebuggerEnableEvent(CurrentEvent->Tag);
            }
            else
            {
                DebuggerDisableEvent(CurrentEvent->Tag);
            }
        }
    }

    return FindAtLeastOneEvent;
}

/**
 * @brief Terminate effect and configuration to vmx-root
 * and non-root for all the events
 *
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN if at least one event terminated then
 * it returns true, and otherwise false
 */
BOOLEAN
DebuggerTerminateAllEvents(BOOLEAN InputFromVmxRoot)
{
    BOOLEAN     FindAtLeastOneEvent = FALSE;
    PLIST_ENTRY TempList            = 0;
    PLIST_ENTRY TempList2           = 0;

    //
    // We have to iterate through all events
    //
    for (size_t i = 0; i < sizeof(DEBUGGER_CORE_EVENTS) / sizeof(LIST_ENTRY); i++)
    {
        TempList  = (PLIST_ENTRY)((UINT64)(g_Events) + (i * sizeof(LIST_ENTRY)));
        TempList2 = TempList;

        while (TempList2 != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // Check if we find at least one event or not
            //
            if (!FindAtLeastOneEvent)
            {
                FindAtLeastOneEvent = TRUE;
            }

            //
            // Terminate the current event
            //
            DebuggerTerminateEvent(CurrentEvent->Tag, InputFromVmxRoot);
        }
    }

    return FindAtLeastOneEvent;
}

/**
 * @brief Remove all the events from all the lists
 * and also de-allocate their structures and actions
 *
 * @details should not be called from vmx-root mode, also
 * it won't terminate their effects, so the events should
 * be terminated first then we can remove them
 *
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return BOOLEAN if at least one event removed then
 * it returns true, and otherwise false
 */
BOOLEAN
DebuggerRemoveAllEvents(BOOLEAN PoolManagerAllocatedMemory)
{
    BOOLEAN     FindAtLeastOneEvent = FALSE;
    PLIST_ENTRY TempList            = 0;
    PLIST_ENTRY TempList2           = 0;

    //
    // We have to iterate through all events
    //
    for (size_t i = 0; i < sizeof(DEBUGGER_CORE_EVENTS) / sizeof(LIST_ENTRY); i++)
    {
        TempList  = (PLIST_ENTRY)((UINT64)(g_Events) + (i * sizeof(LIST_ENTRY)));
        TempList2 = TempList;

        while (TempList2 != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // Check if we find at least one event or not
            //
            if (!FindAtLeastOneEvent)
            {
                FindAtLeastOneEvent = TRUE;
            }

            //
            // Remove the current event
            //
            DebuggerRemoveEvent(CurrentEvent->Tag, PoolManagerAllocatedMemory);
        }
    }

    return FindAtLeastOneEvent;
}

/**
 * @brief Count the list of events in a special list
 *
 * @param TargetEventList target event list
 * @return UINT32 count of events on the list
 */
UINT32
DebuggerEventListCount(PLIST_ENTRY TargetEventList)
{
    PLIST_ENTRY TempList = 0;
    UINT32      Counter  = 0;

    //
    // We have to iterate through all events of this list
    //
    TempList = TargetEventList;

    while (TargetEventList != TempList->Flink)
    {
        TempList = TempList->Flink;
        /* PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList); */

        //
        // Increase the counter
        //
        Counter++;
    }

    return Counter;
}

/**
 * @brief Get List of event based on event type
 *
 * @param EventType type of event
 * @return PLIST_ENTRY
 */
PLIST_ENTRY
DebuggerGetEventListByEventType(VMM_EVENT_TYPE_ENUM EventType)
{
    PLIST_ENTRY ResultList = NULL;
    //
    // Register the event
    //
    switch (EventType)
    {
    case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
        ResultList = &g_Events->HiddenHookReadAndWriteAndExecuteEventsHead;
        break;
    case HIDDEN_HOOK_READ_AND_WRITE:
        ResultList = &g_Events->HiddenHookReadAndWriteEventsHead;
        break;
    case HIDDEN_HOOK_READ_AND_EXECUTE:
        ResultList = &g_Events->HiddenHookReadAndExecuteEventsHead;
        break;
    case HIDDEN_HOOK_WRITE_AND_EXECUTE:
        ResultList = &g_Events->HiddenHookWriteAndExecuteEventsHead;
        break;
    case HIDDEN_HOOK_READ:
        ResultList = &g_Events->HiddenHookReadEventsHead;
        break;
    case HIDDEN_HOOK_WRITE:
        ResultList = &g_Events->HiddenHookWriteEventsHead;
        break;
    case HIDDEN_HOOK_EXECUTE:
        ResultList = &g_Events->HiddenHookExecuteEventsHead;
        break;
    case HIDDEN_HOOK_EXEC_DETOURS:
        ResultList = &g_Events->EptHook2sExecDetourEventsHead;
        break;
    case HIDDEN_HOOK_EXEC_CC:
        ResultList = &g_Events->EptHookExecCcEventsHead;
        break;
    case SYSCALL_HOOK_EFER_SYSCALL:
        ResultList = &g_Events->SyscallHooksEferSyscallEventsHead;
        break;
    case SYSCALL_HOOK_EFER_SYSRET:
        ResultList = &g_Events->SyscallHooksEferSysretEventsHead;
        break;
    case CPUID_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->CpuidInstructionExecutionEventsHead;
        break;
    case RDMSR_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->RdmsrInstructionExecutionEventsHead;
        break;
    case WRMSR_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->WrmsrInstructionExecutionEventsHead;
        break;
    case EXCEPTION_OCCURRED:
        ResultList = &g_Events->ExceptionOccurredEventsHead;
        break;
    case TSC_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->TscInstructionExecutionEventsHead;
        break;
    case PMC_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->PmcInstructionExecutionEventsHead;
        break;
    case IN_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->InInstructionExecutionEventsHead;
        break;
    case OUT_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->OutInstructionExecutionEventsHead;
        break;
    case DEBUG_REGISTERS_ACCESSED:
        ResultList = &g_Events->DebugRegistersAccessedEventsHead;
        break;
    case EXTERNAL_INTERRUPT_OCCURRED:
        ResultList = &g_Events->ExternalInterruptOccurredEventsHead;
        break;
    case VMCALL_INSTRUCTION_EXECUTION:
        ResultList = &g_Events->VmcallInstructionExecutionEventsHead;
        break;
    case TRAP_EXECUTION_MODE_CHANGED:
        ResultList = &g_Events->TrapExecutionModeChangedEventsHead;
        break;
    case TRAP_EXECUTION_INSTRUCTION_TRACE:
        ResultList = &g_Events->TrapExecutionInstructionTraceEventsHead;
        break;
    case CONTROL_REGISTER_3_MODIFIED:
        ResultList = &g_Events->ControlRegister3ModifiedEventsHead;
        break;
    case CONTROL_REGISTER_MODIFIED:
        ResultList = &g_Events->ControlRegisterModifiedEventsHead;
        break;
    default:

        //
        // Wrong event type
        //
        LogError("Err, wrong event type is specified");
        ResultList = NULL;
        break;
    }

    return ResultList;
}

/**
 * @brief Count the list of events in a special list that
 * are activate on a target core
 *
 * @param TargetEventList target event list
 * @param TargetCore target core
 * @return UINT32 count of events on the list which is activated
 * on the target core
 */
UINT32
DebuggerEventListCountByCore(PLIST_ENTRY TargetEventList, UINT32 TargetCore)
{
    PLIST_ENTRY TempList = 0;
    UINT32      Counter  = 0;

    //
    // We have to iterate through all events of this list
    //
    TempList = TargetEventList;

    while (TargetEventList != TempList->Flink)
    {
        TempList                     = TempList->Flink;
        PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

        if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES || CurrentEvent->CoreId == TargetCore)
        {
            //
            // Increase the counter
            //
            Counter++;
        }
    }

    return Counter;
}

/**
 * @brief Count the list of events by a special event type that
 * are activate on a target core
 *
 * @param EventType target event type
 * @param TargetCore target core
 *
 * @return UINT32 count of events on the list which is activated
 * on the target core
 */
UINT32
DebuggerEventListCountByEventType(VMM_EVENT_TYPE_ENUM EventType, UINT32 TargetCore)
{
    PLIST_ENTRY TempList = 0;
    UINT32      Counter  = 0;

    PLIST_ENTRY TargetEventList = DebuggerGetEventListByEventType(EventType);

    //
    // We have to iterate through all events of this list
    //
    TempList = TargetEventList;

    while (TargetEventList != TempList->Flink)
    {
        TempList                     = TempList->Flink;
        PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

        if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES || CurrentEvent->CoreId == TargetCore)
        {
            //
            // Increase the counter
            //
            Counter++;
        }
    }

    return Counter;
}

/**
 * @brief Get the mask related to the !exception command for the
 * target core
 *
 * @param CoreIndex The index of core
 *
 * @return UINT32 Returns the current mask for the core
 */
UINT32
DebuggerExceptionEventBitmapMask(UINT32 CoreIndex)
{
    PLIST_ENTRY TempList      = 0;
    UINT32      ExceptionMask = 0;

    //
    // We have to iterate through all events of this list
    //
    TempList = &g_Events->ExceptionOccurredEventsHead;

    while (&g_Events->ExceptionOccurredEventsHead != TempList->Flink)
    {
        TempList                     = TempList->Flink;
        PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

        if (CurrentEvent->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES || CurrentEvent->CoreId == CoreIndex)
        {
            ExceptionMask |= CurrentEvent->Options.OptionalParam1;
        }
    }

    return ExceptionMask;
}

/**
 * @brief Enable an event by tag
 *
 * @param Tag Tag of target event
 * @return BOOLEAN TRUE if event enabled and FALSE if event not
 * found
 */
BOOLEAN
DebuggerEnableEvent(UINT64 Tag)
{
    PDEBUGGER_EVENT Event;
    //
    // Search all the cores for enable this event
    //
    Event = DebuggerGetEventByTag(Tag);

    //
    // Check if tag is valid or not
    //
    if (Event == NULL)
    {
        return FALSE;
    }

    //
    // Enable the event
    //
    Event->Enabled = TRUE;

    return TRUE;
}

/**
 * @brief returns whether an event is enabled/disabled by tag
 * @details this function won't check for Tag validity and if
 * not found then returns false
 *
 * @param Tag Tag of target event
 * @return BOOLEAN TRUE if event enabled and FALSE if event not
 * found
 */
BOOLEAN
DebuggerQueryStateEvent(UINT64 Tag)
{
    PDEBUGGER_EVENT Event;
    //
    // Search all the cores for enable this event
    //
    Event = DebuggerGetEventByTag(Tag);

    //
    // Check if tag is valid or not
    //
    if (Event == NULL)
    {
        return FALSE;
    }

    return Event->Enabled;
}

/**
 * @brief Disable an event by tag
 *
 * @param Tag Tag of target event
 * @return BOOLEAN TRUE if event enabled and FALSE if event not
 * found
 */
BOOLEAN
DebuggerDisableEvent(UINT64 Tag)
{
    PDEBUGGER_EVENT Event;

    //
    // Search all the cores for enable this event
    //
    Event = DebuggerGetEventByTag(Tag);

    //
    // Check if tag is valid or not
    //
    if (Event == NULL)
    {
        return FALSE;
    }

    //
    // Disable the event
    //
    Event->Enabled = FALSE;

    return TRUE;
}

/**
 * @brief Clear an event by tag
 *
 * @param Tag Tag of target event
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return BOOLEAN
 *
 */
BOOLEAN
DebuggerClearEvent(UINT64 Tag, BOOLEAN InputFromVmxRoot, BOOLEAN PoolManagerAllocatedMemory)
{
    //
    // Because we want to delete all the objects and buffers (pools)
    // after we finished termination, the debugger might still use
    // the buffers for events and action, for solving this problem
    // we first disable the tag(s) and this way the debugger no longer
    // use that event and this way we can safely remove and deallocate
    // the buffers later after termination
    //

    //
    // First, disable just one event
    //
    DebuggerDisableEvent(Tag);

    //
    // Second, terminate it
    //
    DebuggerTerminateEvent(Tag, InputFromVmxRoot);

    //
    // Third, remove it from the list
    //
    return DebuggerRemoveEvent(Tag, PoolManagerAllocatedMemory);
}

/**
 * @brief Clear all events
 *
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return VOID
 */
VOID
DebuggerClearAllEvents(BOOLEAN InputFromVmxRoot, BOOLEAN PoolManagerAllocatedMemory)
{
    //
    // Because we want to delete all the objects and buffers (pools)
    // after we finished termination, the debugger might still use
    // the buffers for events and action, for solving this problem
    // we first disable the tag(s) and this way the debugger no longer
    // use that event and this way we can safely remove and deallocate
    // the buffers later after termination
    //

    //
    // First, disable all events
    //
    DebuggerEnableOrDisableAllEvents(FALSE);

    //
    // Second, terminate all events
    //
    DebuggerTerminateAllEvents(InputFromVmxRoot);

    //
    // Third, remove all events
    //
    DebuggerRemoveAllEvents(PoolManagerAllocatedMemory);
}

/**
 * @brief Detect whether the tag exists or not
 *
 * @param Tag Tag of target event
 * @return BOOLEAN TRUE if event found and FALSE if event not found
 */
BOOLEAN
DebuggerIsTagValid(UINT64 Tag)
{
    PDEBUGGER_EVENT Event;

    //
    // Search this event
    //
    Event = DebuggerGetEventByTag(Tag);

    //
    // Check if tag is valid or not
    //
    if (Event == NULL)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Detect whether the user or kernel debugger
 * is active or not
 *
 * @return BOOLEAN TRUE if any of the are activated and FALSE if not
 */
BOOLEAN
DebuggerQueryDebuggerStatus()
{
    if (g_KernelDebuggerState || g_UserDebuggerState)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Remove the event from event list by its tag
 *
 * @details should not be called from vmx-root mode, also
 * it won't terminate their effects, so the events should
 * be terminated first then we can remove them
 *
 * @param Tag Target events tag
 * @return BOOLEAN If the event was removed then TRUE and FALSE
 * if not found
 */
BOOLEAN
DebuggerRemoveEventFromEventList(UINT64 Tag)
{
    PLIST_ENTRY TempList  = 0;
    PLIST_ENTRY TempList2 = 0;

    //
    // We have to iterate through all events
    //
    for (size_t i = 0; i < sizeof(DEBUGGER_CORE_EVENTS) / sizeof(LIST_ENTRY); i++)
    {
        TempList  = (PLIST_ENTRY)((UINT64)(g_Events) + (i * sizeof(LIST_ENTRY)));
        TempList2 = TempList;

        while (TempList2 != TempList->Flink)
        {
            TempList                     = TempList->Flink;
            PDEBUGGER_EVENT CurrentEvent = CONTAINING_RECORD(TempList, DEBUGGER_EVENT, EventsOfSameTypeList);

            //
            // Check if we find the event or not
            //
            if (CurrentEvent->Tag == Tag)
            {
                //
                // We have to remove the event from the list
                //
                RemoveEntryList(&CurrentEvent->EventsOfSameTypeList);
                return TRUE;
            }
        }
    }

    //
    // We didn't find anything, so return null
    //
    return FALSE;
}

/**
 * @brief Remove the actions and de-allocate its buffer
 *
 * @details should not be called from vmx-root mode, also
 * it won't terminate their effects, so the events should
 * be terminated first then we can remove them *
 *
 * @param Event Event Object
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return BOOLEAN TRUE if it was successful and FALSE if not successful
 */
BOOLEAN
DebuggerRemoveAllActionsFromEvent(PDEBUGGER_EVENT Event, BOOLEAN PoolManagerAllocatedMemory)
{
    PLIST_ENTRY TempList  = 0;
    PLIST_ENTRY TempList2 = 0;

    //
    // Remove all actions
    //
    TempList  = &Event->ActionsListHead;
    TempList2 = TempList;

    while (TempList2 != TempList->Flink)
    {
        TempList                             = TempList->Flink;
        PDEBUGGER_EVENT_ACTION CurrentAction = CONTAINING_RECORD(TempList, DEBUGGER_EVENT_ACTION, ActionsList);

        //
        // Check if it has a OptionalRequestedBuffer probably for
        // CustomCode
        //
        if (CurrentAction->RequestedBuffer.RequestBufferSize != 0 && CurrentAction->RequestedBuffer.RequstBufferAddress != (UINT64)NULL)
        {
            //
            // There is a buffer
            //
            if (PoolManagerAllocatedMemory)
            {
                PoolManagerFreePool(CurrentAction->RequestedBuffer.RequstBufferAddress);
            }
            else
            {
                CrsFreePool((PVOID)CurrentAction->RequestedBuffer.RequstBufferAddress);
            }
        }

        //
        // Remove the action and free the pool,
        // if it's a custom buffer then the buffer
        // is appended to the Action
        //
        if (PoolManagerAllocatedMemory)
        {
            PoolManagerFreePool((UINT64)CurrentAction);
        }
        else
        {
            CrsFreePool(CurrentAction);
        }
    }
    //
    // Remember to free the pool
    //
    return TRUE;
}

/**
 * @brief Remove the event by its tags and also remove its actions
 * and de-allocate their buffers
 *
 * @details it won't terminate their effects, so the events should
 * be terminated first then we can remove them
 *
 * @param Tag Target event tag
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return BOOLEAN TRUE if it was successful and FALSE if not successful
 */
BOOLEAN
DebuggerRemoveEvent(UINT64 Tag, BOOLEAN PoolManagerAllocatedMemory)
{
    PDEBUGGER_EVENT Event;

    //
    // First of all, we disable event
    //
    if (!DebuggerDisableEvent(Tag))
    {
        //
        // Not found, tag is wrong !
        //
        return FALSE;
    }

    //
    // When we're here, we are sure that the tag is valid
    // because if it was not valid, then we have to return
    // for the above function (DebuggerDisableEvent)
    //
    Event = DebuggerGetEventByTag(Tag);

    //
    // Now we get the PDEBUGGER_EVENT so we have to remove
    // it from the event list
    //
    if (!DebuggerRemoveEventFromEventList(Tag))
    {
        return FALSE;
    }

    //
    // Remove all of the actions and free its pools
    //
    DebuggerRemoveAllActionsFromEvent(Event, PoolManagerAllocatedMemory);

    //
    // Free the pools of Event, when we free the pool,
    // ConditionsBufferAddress is also a part of the
    // event pool (ConditionBufferAddress and event
    // are both allocate in a same pool ) so both of
    // them are freed
    //
    if (PoolManagerAllocatedMemory)
    {
        PoolManagerFreePool((UINT64)Event);
    }
    else
    {
        CrsFreePool(Event);
    }

    return TRUE;
}

/**
 * @brief validating events
 *
 * @param EventDetails The structure that describes event that came
 * from the user-mode or VMX-root mode
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN TRUE if the event was valid otherwise returns FALSE
 */
BOOLEAN
DebuggerValidateEvent(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                      PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                      BOOLEAN                           InputFromVmxRoot)
{
    //
    // Check whether the event mode (calling stage)  to see whether
    // short-cicuiting event is used along with the post-event,
    // it is because using the short-circuiting mechanism with
    // post-events doesn't make sense; it's not supported!
    //
    if ((EventDetails->EventStage == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION ||
         EventDetails->EventStage == VMM_CALLBACK_CALLING_STAGE_ALL_EVENT_EMULATION) &&
        EventDetails->EnableShortCircuiting == TRUE)
    {
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_USING_SHORT_CIRCUITING_EVENT_WITH_POST_EVENT_MODE_IS_FORBIDDEDN;
        return FALSE;
    }

    //
    // Check whether the core Id is valid or not, we read cores count
    // here because we use it in later parts
    //
    if (EventDetails->CoreId != DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // Check if the core number is not invalid
        //
        if (!CommonValidateCoreNumber(EventDetails->CoreId))
        {
            //
            // CoreId is invalid (Set the error)
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_CORE_ID;
            return FALSE;
        }
    }

    //
    // Check if process id is valid or not, we won't touch process id here
    // because some of the events use the exact value of DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    //
    if (EventDetails->ProcessId != DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES && EventDetails->ProcessId != 0)
    {
        //
        // Here we prefer not to validate the process id, if it's applied from VMX-root mode
        //
        if (!InputFromVmxRoot)
        {
            //
            // The used specified a special pid, let's check if it's valid or not
            //
            if (!CommonIsProcessExist(EventDetails->ProcessId))
            {
                ResultsToReturn->IsSuccessful = FALSE;
                ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_PROCESS_ID;
                return FALSE;
            }
        }
    }

    //
    // *** Event specific validations ***
    //
    switch (EventDetails->EventType)
    {
    case EXCEPTION_OCCURRED:
    {
        //
        // Check if exception parameters are valid
        //
        if (!ValidateEventException(EventDetails, ResultsToReturn, InputFromVmxRoot))
        {
            //
            // Event parameters are not valid, let break the further execution at this stage
            //
            return FALSE;
        }

        break;
    }
    case EXTERNAL_INTERRUPT_OCCURRED:
    {
        //
        // Check if interrupt parameters are valid
        //
        if (!ValidateEventInterrupt(EventDetails, ResultsToReturn, InputFromVmxRoot))
        {
            //
            // Event parameters are not valid, let break the further execution at this stage
            //
            return FALSE;
        }

        break;
    }
    case TRAP_EXECUTION_MODE_CHANGED:
    {
        //
        // Check if trap exec mode parameters are valid
        //
        if (!ValidateEventTrapExec(EventDetails, ResultsToReturn, InputFromVmxRoot))
        {
            //
            // Event parameters are not valid, let break the further execution at this stage
            //
            return FALSE;
        }

        break;
    }
    case HIDDEN_HOOK_EXEC_DETOURS:
    case HIDDEN_HOOK_EXEC_CC:
    {
        //
        // Check if EPT hook exec (hidden breakpoint and inline hook) parameters are valid
        //
        if (!ValidateEventEptHookHiddenBreakpointAndInlineHooks(EventDetails, ResultsToReturn, InputFromVmxRoot))
        {
            //
            // Event parameters are not valid, let break the further execution at this stage
            //
            return FALSE;
        }

        break;
    }
    case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ_AND_WRITE:
    case HIDDEN_HOOK_READ_AND_EXECUTE:
    case HIDDEN_HOOK_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ:
    case HIDDEN_HOOK_WRITE:
    case HIDDEN_HOOK_EXECUTE:
    {
        //
        // Check if EPT memory monitor hook parameters are valid
        //
        if (!ValidateEventMonitor(EventDetails, ResultsToReturn, InputFromVmxRoot))
        {
            //
            // Event parameters are not valid, let break the further execution at this stage
            //
            return FALSE;
        }

        break;
    }
    default:

        //
        // Other not specified events doesn't have any special validation
        //
        break;
    }

    //
    // As we reached, all the checks are passed and it means the event is valid
    //
    return TRUE;
}

/**
 * @brief Applying events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN TRUE if the event was applied otherwise returns FALSE
 */
BOOLEAN
DebuggerApplyEvent(PDEBUGGER_EVENT                   Event,
                   PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                   BOOLEAN                           InputFromVmxRoot)
{
    //
    // Now we should configure the cpu to generate the events
    //
    switch (Event->EventType)
    {
    case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ_AND_WRITE:
    case HIDDEN_HOOK_READ_AND_EXECUTE:
    case HIDDEN_HOOK_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ:
    case HIDDEN_HOOK_WRITE:
    case HIDDEN_HOOK_EXECUTE:
    {
        //
        // Apply the monitor memory hook events
        //
        if (!ApplyEventMonitorEvent(Event, ResultsToReturn, InputFromVmxRoot))
        {
            goto ClearTheEventAfterCreatingEvent;
        }

        break;
    }
    case HIDDEN_HOOK_EXEC_CC:
    {
        //
        // Apply the EPT hidden hook (hidden breakpoint) events
        //
        if (!ApplyEventEptHookExecCcEvent(Event, ResultsToReturn, InputFromVmxRoot))
        {
            goto ClearTheEventAfterCreatingEvent;
        }

        break;
    }
    case HIDDEN_HOOK_EXEC_DETOURS:
    {
        //
        // Apply the EPT hook trampoline (inline hook) events
        //
        if (!ApplyEventEpthookInlineEvent(Event, ResultsToReturn, InputFromVmxRoot))
        {
            goto ClearTheEventAfterCreatingEvent;
        }

        break;
    }
    case RDMSR_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the RDMSR execution exiting events
        //
        ApplyEventRdmsrExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case WRMSR_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the WRMSR execution exiting events
        //
        ApplyEventWrmsrExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case IN_INSTRUCTION_EXECUTION:
    case OUT_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the IN/OUT instructions execution exiting events
        //
        ApplyEventInOutExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case TSC_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the RDTSC/RDTSCP instructions execution exiting events
        //
        ApplyEventTscExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case PMC_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the RDPMC instruction execution exiting events
        //
        ApplyEventRdpmcExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case DEBUG_REGISTERS_ACCESSED:
    {
        //
        // Apply the mov 2 debug register exiting events
        //
        ApplyEventMov2DebugRegExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case CONTROL_REGISTER_MODIFIED:
    {
        //
        // Apply the control register access exiting events
        //
        ApplyEventControlRegisterAccessedEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case EXCEPTION_OCCURRED:
    {
        //
        // Apply the exception events
        //
        ApplyEventExceptionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case EXTERNAL_INTERRUPT_OCCURRED:
    {
        //
        // Apply the interrupt events
        //
        ApplyEventInterruptEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case SYSCALL_HOOK_EFER_SYSCALL:
    {
        //
        // Apply the EFER SYSCALL hook events
        //
        ApplyEventEferSyscallHookEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case SYSCALL_HOOK_EFER_SYSRET:
    {
        //
        // Apply the EFER SYSRET hook events
        //
        ApplyEventEferSysretHookEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case VMCALL_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the VMCALL instruction interception events
        //
        ApplyEventVmcallExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case TRAP_EXECUTION_MODE_CHANGED:
    {
        //
        // Apply the trap mode change and single instruction trace events
        //
        if (!ApplyEventTrapModeChangeEvent(Event, ResultsToReturn, InputFromVmxRoot))
        {
            goto ClearTheEventAfterCreatingEvent;
        }

        break;
    }
    case CPUID_INSTRUCTION_EXECUTION:
    {
        //
        // Apply the CPUID instruction execution events
        //
        ApplyEventCpuidExecutionEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    case TRAP_EXECUTION_INSTRUCTION_TRACE:
    {
        //
        // Apply the tracing events
        //
        ApplyEventTracingEvent(Event, ResultsToReturn, InputFromVmxRoot);

        break;
    }
    default:
    {
        //
        // Set the error
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_EVENT_TYPE_IS_INVALID;
        goto ClearTheEventAfterCreatingEvent;

        break;
    }
    }

    //
    // Set the status
    //
    ResultsToReturn->IsSuccessful = TRUE;
    ResultsToReturn->Error        = 0;

    //
    // Event was applied successfully
    //
    return TRUE;

ClearTheEventAfterCreatingEvent:

    return FALSE;
}

/**
 * @brief Routine for parsing events
 *
 * @param EventDetails The structure that describes event that came
 * from the user-mode
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN TRUE if the event was valid and registered without error,
 * otherwise returns FALSE
 */
BOOLEAN
DebuggerParseEvent(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                   PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                   BOOLEAN                           InputFromVmxRoot)
{
    PDEBUGGER_EVENT Event;

    //
    // ----------------------------------------------------------------------------------
    // ***                     Validating the Event's parameters                      ***
    // ----------------------------------------------------------------------------------
    //

    //
    // Validate the event parameters
    //
    if (!DebuggerValidateEvent(EventDetails, ResultsToReturn, InputFromVmxRoot))
    {
        //
        // Input event is not valid
        //
        return FALSE;
    }

    //
    // ----------------------------------------------------------------------------------
    // ***                                Create Event                                ***
    // ----------------------------------------------------------------------------------
    //

    //
    // We initialize event with disabled mode as it doesn't have action yet
    //
    if (EventDetails->ConditionBufferSize != 0)
    {
        //
        // Conditional Event
        //
        Event = DebuggerCreateEvent(FALSE,
                                    EventDetails->CoreId,
                                    EventDetails->ProcessId,
                                    EventDetails->EventType,
                                    EventDetails->Tag,
                                    &EventDetails->Options,
                                    EventDetails->ConditionBufferSize,
                                    (PVOID)((UINT64)EventDetails + sizeof(DEBUGGER_GENERAL_EVENT_DETAIL)),
                                    ResultsToReturn,
                                    InputFromVmxRoot);
    }
    else
    {
        //
        // Unconditional Event
        //
        Event = DebuggerCreateEvent(FALSE,
                                    EventDetails->CoreId,
                                    EventDetails->ProcessId,
                                    EventDetails->EventType,
                                    EventDetails->Tag,
                                    &EventDetails->Options,
                                    0,
                                    NULL,
                                    ResultsToReturn,
                                    InputFromVmxRoot);
    }

    if (Event == NULL)
    {
        //
        // Error is already set in the creation function
        //
        return FALSE;
    }

    //
    // Register the event
    //
    DebuggerRegisterEvent(Event);

    //
    // ----------------------------------------------------------------------------------
    // ***                            Apply & Enable Event                            ***
    // ----------------------------------------------------------------------------------
    //
    if (DebuggerApplyEvent(Event, ResultsToReturn, InputFromVmxRoot))
    {
        //
        // *** Set the short-circuiting state ***
        //
        Event->EnableShortCircuiting = EventDetails->EnableShortCircuiting;

        //
        // Set the event stage (pre- post- event)
        //
        if (EventDetails->EventStage == VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION)
        {
            Event->EventMode = VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION;
        }
        else if (EventDetails->EventStage == VMM_CALLBACK_CALLING_STAGE_ALL_EVENT_EMULATION)
        {
            Event->EventMode = VMM_CALLBACK_CALLING_STAGE_ALL_EVENT_EMULATION;
        }
        else
        {
            //
            // Any other value results to be pre-event
            //
            Event->EventMode = VMM_CALLBACK_CALLING_STAGE_PRE_EVENT_EMULATION;
        }

        return TRUE;
    }
    else
    {
        //
        // Remove the event as it was not successful
        // The same input as of input from VMX-root is
        // selected here because we apply it directly in the
        // above function and based on this input we can
        // conclude whether the pool is allocated from the
        // pool manager or not
        //
        if (Event != NULL)
        {
            DebuggerRemoveEvent(Event->Tag, InputFromVmxRoot);
        }

        return FALSE;
    }
}

/**
 * @brief Routine for validating and parsing actions that are coming from
 * the user-mode
 *
 * @param ActionDetails Structure that describes the action that comes from the
 * user-mode
 * @param ResultsToReturn The buffer address that should be returned
 * to the user-mode as the result
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN if action was parsed and added successfully, return TRUE
 * otherwise, returns FALSE
 */
BOOLEAN
DebuggerParseAction(PDEBUGGER_GENERAL_ACTION          ActionDetails,
                    PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                    BOOLEAN                           InputFromVmxRoot)
{
    DEBUGGER_EVENT_ACTION * Action = NULL;

    //
    // Check if Tag is valid or not
    //
    PDEBUGGER_EVENT Event = DebuggerGetEventByTag(ActionDetails->EventTag);

    if (Event == NULL)
    {
        //
        // Set the appropriate error
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_TAG_NOT_EXISTS;

        //
        // Show that the
        //
        return FALSE;
    }

    if (ActionDetails->ActionType == RUN_CUSTOM_CODE)
    {
        //
        // Check if buffer is not invalid
        //
        if (ActionDetails->CustomCodeBufferSize == 0)
        {
            //
            // Set the appropriate error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO;

            //
            // Show that the
            //
            return FALSE;
        }

        //
        // Add action for RUN_CUSTOM_CODE
        //
        DEBUGGER_EVENT_REQUEST_CUSTOM_CODE CustomCode = {0};

        CustomCode.CustomCodeBufferSize        = ActionDetails->CustomCodeBufferSize;
        CustomCode.CustomCodeBufferAddress     = (PVOID)((UINT64)ActionDetails + sizeof(DEBUGGER_GENERAL_ACTION));
        CustomCode.OptionalRequestedBufferSize = ActionDetails->PreAllocatedBuffer;

        //
        // Add action to event
        //
        Action = DebuggerAddActionToEvent(Event,
                                          RUN_CUSTOM_CODE,
                                          ActionDetails->ImmediateMessagePassing,
                                          &CustomCode,
                                          NULL,
                                          ResultsToReturn,
                                          InputFromVmxRoot);

        if (!Action)
        {
            //
            // Show that there was an error (error is set by the above function)
            //
            return FALSE;
        }
    }
    else if (ActionDetails->ActionType == RUN_SCRIPT)
    {
        //
        // Check if buffer is not invalid
        //
        if (ActionDetails->ScriptBufferSize == 0)
        {
            //
            // Set the appropriate error
            //
            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO;

            //
            // Show that the
            //
            return FALSE;
        }

        //
        // Add action for RUN_SCRIPT
        //
        DEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION UserScriptConfig = {0};
        UserScriptConfig.ScriptBuffer                                   = (UINT64)ActionDetails + sizeof(DEBUGGER_GENERAL_ACTION);
        UserScriptConfig.ScriptLength                                   = ActionDetails->ScriptBufferSize;
        UserScriptConfig.ScriptPointer                                  = ActionDetails->ScriptBufferPointer;
        UserScriptConfig.OptionalRequestedBufferSize                    = ActionDetails->PreAllocatedBuffer;

        Action = DebuggerAddActionToEvent(Event,
                                          RUN_SCRIPT,
                                          ActionDetails->ImmediateMessagePassing,
                                          NULL,
                                          &UserScriptConfig,
                                          ResultsToReturn,
                                          InputFromVmxRoot);

        if (!Action)
        {
            //
            // Show that there was an error (error is set by the above function)
            //
            return FALSE;
        }
    }
    else if (ActionDetails->ActionType == BREAK_TO_DEBUGGER)
    {
        //
        // Add action BREAK_TO_DEBUGGER to event
        //
        Action = DebuggerAddActionToEvent(Event,
                                          BREAK_TO_DEBUGGER,
                                          ActionDetails->ImmediateMessagePassing,
                                          NULL,
                                          NULL,
                                          ResultsToReturn,
                                          InputFromVmxRoot);

        if (!Action)
        {
            //
            // Show that there was an error (error is set by the above function)
            //
            return FALSE;
        }
    }
    else
    {
        //
        // Set the appropriate error
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ACTION_TYPE;

        //
        // Show that there was an error
        //
        return FALSE;
    }

    //
    // Enable the event
    //
    DebuggerEnableEvent(Event->Tag);

    ResultsToReturn->IsSuccessful = TRUE;
    ResultsToReturn->Error        = 0;

    return TRUE;
}

/**
 * @brief Terminate one event's effect by its tag
 *
 * @details This function won't remove the event from
 * the lists of event or de-allocated them, this should
 * be called BEFORE the removing function
 *
 * @param Tag Target event's tag
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN if it was found and terminated without error
 * then it returns TRUE, otherwise FALSE
 */
BOOLEAN
DebuggerTerminateEvent(UINT64 Tag, BOOLEAN InputFromVmxRoot)
{
    PDEBUGGER_EVENT Event;
    BOOLEAN         Result = FALSE;

    //
    // Find the event by its tag
    //
    Event = DebuggerGetEventByTag(Tag);

    if (Event == NULL)
    {
        //
        // event, not found
        //
        return FALSE;
    }

    //
    // Check the event type of our specific tag
    //
    switch (Event->EventType)
    {
    case EXTERNAL_INTERRUPT_OCCURRED:
    {
        //
        // Call external interrupt terminator
        //
        TerminateExternalInterruptEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ_AND_WRITE:
    case HIDDEN_HOOK_READ_AND_EXECUTE:
    case HIDDEN_HOOK_WRITE_AND_EXECUTE:
    case HIDDEN_HOOK_READ:
    case HIDDEN_HOOK_WRITE:
    case HIDDEN_HOOK_EXECUTE:
    {
        //
        // Call read and write and execute ept hook terminator
        //
        TerminateHiddenHookReadAndWriteAndExecuteEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case HIDDEN_HOOK_EXEC_CC:
    {
        //
        // Call ept hook (hidden breakpoint) terminator
        //
        TerminateHiddenHookExecCcEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case HIDDEN_HOOK_EXEC_DETOURS:
    {
        //
        // Call ept hook (hidden inline hook) terminator
        //
        TerminateHiddenHookExecDetoursEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case RDMSR_INSTRUCTION_EXECUTION:
    {
        //
        // Call rdmsr execution event terminator
        //
        TerminateRdmsrExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case WRMSR_INSTRUCTION_EXECUTION:
    {
        //
        // Call wrmsr execution event terminator
        //
        TerminateWrmsrExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case EXCEPTION_OCCURRED:
    {
        //
        // Call exception events terminator
        //
        TerminateExceptionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case IN_INSTRUCTION_EXECUTION:
    {
        //
        // Call IN instruction execution event terminator
        //
        TerminateInInstructionExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case OUT_INSTRUCTION_EXECUTION:
    {
        //
        // Call OUT instruction execution event terminator
        //
        TerminateOutInstructionExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case SYSCALL_HOOK_EFER_SYSCALL:
    {
        //
        // Call syscall hook event terminator
        //
        TerminateSyscallHookEferEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case SYSCALL_HOOK_EFER_SYSRET:
    {
        //
        // Call sysret hook event terminator
        //
        TerminateSysretHookEferEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case VMCALL_INSTRUCTION_EXECUTION:
    {
        //
        // Call vmcall instruction execution event terminator
        //
        TerminateVmcallExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case TRAP_EXECUTION_MODE_CHANGED:
    {
        //
        // Call mode execution trap event terminator
        //
        TerminateExecTrapModeChangedEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case TSC_INSTRUCTION_EXECUTION:
    {
        //
        // Call rdtsc/rdtscp instruction execution event terminator
        //
        TerminateTscEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case PMC_INSTRUCTION_EXECUTION:
    {
        //
        // Call rdtsc/rdtscp instructions execution event terminator
        //
        TerminatePmcEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case DEBUG_REGISTERS_ACCESSED:
    {
        //
        // Call mov to debugger register event terminator
        //
        TerminateDebugRegistersEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case CPUID_INSTRUCTION_EXECUTION:
    {
        //
        // Call cpuid instruction execution event terminator
        //
        TerminateCpuidExecutionEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    case CONTROL_REGISTER_MODIFIED:
    {
        //
        // Call mov to control register event terminator
        //
        TerminateControlRegistersEvent(Event, InputFromVmxRoot);
        Result = TRUE;

        break;
    }
    default:
        LogError("Err, unknown event for termination");
        Result = FALSE;

        break;
    }

    //
    // Return status
    //
    return Result;
}

/**
 * @brief Parse and validate requests to enable/disable/clear
 * from the user-mode
 *
 * @param DebuggerEventModificationRequest event modification request details
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 * @param PoolManagerAllocatedMemory Whether the pools are allocated from the
 * pool manager or original OS pools
 *
 * @return BOOLEAN returns TRUE if there was no error, and FALSE if there was
 * an error
 */
BOOLEAN
DebuggerParseEventsModification(PDEBUGGER_MODIFY_EVENTS DebuggerEventModificationRequest,
                                BOOLEAN                 InputFromVmxRoot,
                                BOOLEAN                 PoolManagerAllocatedMemory)
{
    BOOLEAN IsForAllEvents = FALSE;

    //
    // Check if the tag is valid or not
    //
    if (DebuggerEventModificationRequest->Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
    {
        IsForAllEvents = TRUE;
    }
    else if (!DebuggerIsTagValid(DebuggerEventModificationRequest->Tag))
    {
        //
        // Tag is invalid
        //
        DebuggerEventModificationRequest->KernelStatus = DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TAG;

        return FALSE;
    }

    //
    // ***************************************************************************
    //

    //
    // Check if it's a ENABLE, DISABLE or CLEAR
    //
    if (DebuggerEventModificationRequest->TypeOfAction == DEBUGGER_MODIFY_EVENTS_ENABLE)
    {
        if (IsForAllEvents)
        {
            //
            // Enable all events
            //
            DebuggerEnableOrDisableAllEvents(TRUE);
        }
        else
        {
            //
            // Enable just one event
            //
            DebuggerEnableEvent(DebuggerEventModificationRequest->Tag);
        }
    }
    else if (DebuggerEventModificationRequest->TypeOfAction == DEBUGGER_MODIFY_EVENTS_DISABLE)
    {
        if (IsForAllEvents)
        {
            //
            // Disable all events
            //
            DebuggerEnableOrDisableAllEvents(FALSE);
        }
        else
        {
            //
            // Disable just one event
            //
            DebuggerDisableEvent(DebuggerEventModificationRequest->Tag);
        }
    }
    else if (DebuggerEventModificationRequest->TypeOfAction == DEBUGGER_MODIFY_EVENTS_CLEAR)
    {
        if (IsForAllEvents)
        {
            //
            // Clear all events
            //
            DebuggerClearAllEvents(InputFromVmxRoot, PoolManagerAllocatedMemory);
        }
        else
        {
            //
            // Clear just one event
            //
            DebuggerClearEvent(DebuggerEventModificationRequest->Tag, InputFromVmxRoot, PoolManagerAllocatedMemory);
        }
    }
    else if (DebuggerEventModificationRequest->TypeOfAction == DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
    {
        //
        // check if tag is valid or not
        //
        if (!DebuggerIsTagValid(DebuggerEventModificationRequest->Tag))
        {
            DebuggerEventModificationRequest->KernelStatus = DEBUGGER_ERROR_TAG_NOT_EXISTS;
            return FALSE;
        }

        //
        // Set event state
        //
        if (DebuggerQueryStateEvent(DebuggerEventModificationRequest->Tag))
        {
            DebuggerEventModificationRequest->IsEnabled = TRUE;
        }
        else
        {
            DebuggerEventModificationRequest->IsEnabled = FALSE;
        }
    }
    else
    {
        //
        // Invalid parameter specified in TypeOfAction
        //
        DebuggerEventModificationRequest->KernelStatus = DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TYPE_OF_ACTION;

        return FALSE;
    }

    //
    // The function was successful
    //
    DebuggerEventModificationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    return TRUE;
}
