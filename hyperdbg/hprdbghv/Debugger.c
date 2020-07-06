/**
 * @file Debugger.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger functions
 * @details
 * 
 * @version 0.1
 * @date 2020-04-13
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntddk.h>
#include "Common.h"
#include "Debugger.h"
#include "DebuggerEvents.h"
#include "GlobalVariables.h"
#include "Hooks.h"
#include "ExtensionCommands.h"
#include "HypervisorRoutines.h"
#include "DpcRoutines.h"
#include "InlineAsm.h"

BOOLEAN
DebuggerInitialize()
{
    //
    // Allocate buffer for saving events
    //
    if (!g_Events)
    {
        g_Events = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_CORE_EVENTS), POOLTAG);
    }

    if (!g_Events)
    {
        return FALSE;
    }

    //
    // Zero the buffer
    //
    RtlZeroBytes(g_Events, sizeof(DEBUGGER_CORE_EVENTS));

    //
    // Initialize lists relating to the debugger events store
    //

    InitializeListHead(&g_Events->EptHookExecCcEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadAndWriteEventsHead);
    InitializeListHead(&g_Events->HiddenHookReadEventsHead);
    InitializeListHead(&g_Events->HiddenHookWriteEventsHead);
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

    //
    // Initialize the list of hidden hooks headers
    //
    InitializeListHead(&g_EptHook2sDetourListHead);

    //
    // Enabled Debugger Events
    //
    g_EnableDebuggerEvents = TRUE;

    return TRUE;
}

// should not be called in vmx root
PDEBUGGER_EVENT
DebuggerCreateEvent(BOOLEAN                  Enabled,
                    UINT32                   CoreId,
                    UINT32                   ProcessId,
                    DEBUGGER_EVENT_TYPE_ENUM EventType,
                    UINT64                   Tag,
                    UINT64                   OptionalParam1,
                    UINT64                   OptionalParam2,
                    UINT64                   OptionalParam3,
                    UINT64                   OptionalParam4,
                    UINT32                   ConditionsBufferSize,
                    PVOID                    ConditionBuffer)
{
    //
    // As this function uses ExAllocatePoolWithTag,
    // we have to make sure that it will not be called in vmx root
    //
    if (g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return NULL;
    }

    //
    // Initialize the event structure
    //
    PDEBUGGER_EVENT Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_EVENT) + ConditionsBufferSize, POOLTAG);
    if (!Event)
    {
        //
        // There is a problem with allocating event
        //
        return NULL;
    }
    RtlZeroMemory(Event, sizeof(DEBUGGER_EVENT) + ConditionsBufferSize);

    Event->CoreId         = CoreId;
    Event->ProcessId      = ProcessId;
    Event->Enabled        = Enabled;
    Event->EventType      = EventType;
    Event->Tag            = Tag;
    Event->CountOfActions = 0; // currently there is no action
    Event->OptionalParam1 = OptionalParam1;
    Event->OptionalParam2 = OptionalParam2;
    Event->OptionalParam3 = OptionalParam3;
    Event->OptionalParam4 = OptionalParam4;

    //
    // check if this event is conditional or not
    //
    if (ConditionBuffer != 0)
    {
        //
        // It's condtional
        //
        Event->ConditionsBufferSize   = ConditionsBufferSize;
        Event->ConditionBufferAddress = (UINT64)Event + sizeof(DEBUGGER_EVENT);

        //
        // copy the condtion buffer to the end of the buffer of the event
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

// should not be called in vmx root
PDEBUGGER_EVENT_ACTION
DebuggerAddActionToEvent(PDEBUGGER_EVENT Event, DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType, BOOLEAN SendTheResultsImmediately, PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE InTheCaseOfCustomCode, PDEBUGGER_EVENT_ACTION_LOG_CONFIGURATION InTheCaseOfLogTheStates)
{
    PDEBUGGER_EVENT_ACTION Action;

    //
    // As this function uses ExAllocatePoolWithTag,
    // we have to make sure that it will not be called in vmx root
    //
    if (g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return NULL;
    }

    //
    // Allocate action + allocate code for custom code
    //

    if (InTheCaseOfCustomCode == NULL)
    {
        //
        // We shouldn't allocate extra buffer as there is no custom code
        //
        Action = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_EVENT_ACTION), POOLTAG);

        if (!Action)
        {
            //
            // There was an error in allocation
            //
            return NULL;
        }

        RtlZeroMemory(Action, sizeof(DEBUGGER_EVENT_ACTION));
    }
    else
    {
        //
        // We should allocate extra buffer for custom code
        //
        Action = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_EVENT_ACTION) + InTheCaseOfCustomCode->CustomCodeBufferSize, POOLTAG);

        if (!Action)
        {
            //
            // There was an error in allocation
            //
            return NULL;
        }

        RtlZeroMemory(Action, sizeof(DEBUGGER_EVENT_ACTION) + InTheCaseOfCustomCode->CustomCodeBufferSize);
    }

    //
    // If the user needs a buffer to be passed to the debugger then
    // we should allocate it here (Requested buffer is only available for custom code types)
    //
    if (ActionType == RUN_CUSTOM_CODE && InTheCaseOfCustomCode->OptionalRequestedBufferSize != 0)
    {
        //
        // Check if the optional buffer is not more that the size
        // we can send to usermode
        //
        if (InTheCaseOfCustomCode->OptionalRequestedBufferSize >= MaximumPacketsCapacity)
        {
            return NULL;
        }

        //
        // User needs a buffer to play with
        //
        PVOID RequestedBuffer = ExAllocatePoolWithTag(NonPagedPool, InTheCaseOfCustomCode->OptionalRequestedBufferSize, POOLTAG);

        if (!RequestedBuffer)
        {
            //
            // There was an error in allocation
            //
            ExFreePoolWithTag(Action, POOLTAG);
            return NULL;
        }
        RtlZeroMemory(RequestedBuffer, InTheCaseOfCustomCode->OptionalRequestedBufferSize);

        //
        // Add it to the action
        //
        Action->RequestedBuffer.EnabledRequestBuffer = TRUE;
        Action->RequestedBuffer.RequestBufferSize    = InTheCaseOfCustomCode->OptionalRequestedBufferSize;
        Action->RequestedBuffer.RequstBufferAddress  = RequestedBuffer;
    }

    if (ActionType == RUN_CUSTOM_CODE)
    {
        //
        // Check if it's a Custom code without custom code buffer which is invalid
        //
        if (InTheCaseOfCustomCode->CustomCodeBufferSize == 0)
            return NULL;

        //
        // Move the custom code buffer to the end of the action
        //

        Action->CustomCodeBufferSize    = InTheCaseOfCustomCode->CustomCodeBufferSize;
        Action->CustomCodeBufferAddress = (UINT64)Action + sizeof(DEBUGGER_EVENT_ACTION);

        //
        // copy the custom code buffer to the end of the buffer of the action
        //
        memcpy(Action->CustomCodeBufferAddress, InTheCaseOfCustomCode->CustomCodeBufferAddress, InTheCaseOfCustomCode->CustomCodeBufferSize);
    }

    //
    // If it's log the states action type
    //
    if (ActionType == LOG_THE_STATES)
    {
        Action->LogConfiguration.LogLength = InTheCaseOfLogTheStates->LogLength;
        Action->LogConfiguration.LogMask   = InTheCaseOfLogTheStates->LogMask;
        Action->LogConfiguration.LogType   = InTheCaseOfLogTheStates->LogType;
        Action->LogConfiguration.LogValue  = InTheCaseOfLogTheStates->LogValue;
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

    //
    // Now we should add the action to the event's LIST_ENTRY of actions
    //
    InsertHeadList(&Event->ActionsListHead, &(Action->ActionsList));

    return Action;
}

BOOLEAN
DebuggerRegisterEvent(PDEBUGGER_EVENT Event)
{
    UINT32 ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Register the event
    //

    switch (Event->EventType)
    {
    case HIDDEN_HOOK_READ_AND_WRITE:
        InsertHeadList(&g_Events->HiddenHookReadAndWriteEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case HIDDEN_HOOK_READ:
        InsertHeadList(&g_Events->HiddenHookReadEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case HIDDEN_HOOK_WRITE:
        InsertHeadList(&g_Events->HiddenHookWriteEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case HIDDEN_HOOK_EXEC_DETOURS:
        InsertHeadList(&g_Events->EptHook2sExecDetourEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case HIDDEN_HOOK_EXEC_CC:
        InsertHeadList(&g_Events->EptHookExecCcEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case SYSCALL_HOOK_EFER_SYSCALL:
        InsertHeadList(&g_Events->SyscallHooksEferSyscallEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case SYSCALL_HOOK_EFER_SYSRET:
        InsertHeadList(&g_Events->SyscallHooksEferSysretEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case CPUID_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->CpuidInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case RDMSR_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->RdmsrInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case WRMSR_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->WrmsrInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case EXCEPTION_OCCURRED:
        InsertHeadList(&g_Events->ExceptionOccurredEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case TSC_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->TscInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case PMC_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->PmcInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case IN_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->InInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case OUT_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->OutInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case DEBUG_REGISTERS_ACCESSED:
        InsertHeadList(&g_Events->DebugRegistersAccessedEventsHead, &(Event->EventsOfSameTypeList));
        break;
    case EXTERNAL_INTERRUPT_OCCURRED:
        InsertHeadList(&g_Events->ExternalInterruptOccurredEventsHead, &(Event->EventsOfSameTypeList));
        break;  
    case VMCALL_INSTRUCTION_EXECUTION:
        InsertHeadList(&g_Events->VmcallInstructionExecutionEventsHead, &(Event->EventsOfSameTypeList));
        break;
    default:
        //
        // Wrong event type
        //
        return FALSE;
        break;
    }
}

BOOLEAN
DebuggerTriggerEvents(DEBUGGER_EVENT_TYPE_ENUM EventType, PGUEST_REGS Regs, PVOID Context)
{
    ULONG                       CurrentProcessorIndex;
    PLIST_ENTRY                 TempList  = 0;
    PLIST_ENTRY                 TempList2 = 0;
    DebuggerCheckForCondition * ConditionFunc;

    //
    // Check if triggering debugging actions are allowed or not
    //
    if (!g_EnableDebuggerEvents)
    {
        //
        // Debugger is not enabled
        //
        return FALSE;
    }

    //
    // Search for this event in this core (get the core index)
    //
    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // Find the debugger events list base on the type of the event
    //
    switch (EventType)
    {
    case HIDDEN_HOOK_READ_AND_WRITE:
    {
        TempList  = &g_Events->HiddenHookReadAndWriteEventsHead;
        TempList2 = TempList;
        break;
    }
    case HIDDEN_HOOK_READ:
    {
        TempList  = &g_Events->HiddenHookReadEventsHead;
        TempList2 = TempList;
        break;
    }
    case HIDDEN_HOOK_WRITE:
    {
        TempList  = &g_Events->HiddenHookWriteEventsHead;
        TempList2 = TempList;
        break;
    }
    case HIDDEN_HOOK_EXEC_DETOURS:
    {
        TempList  = &g_Events->EptHook2sExecDetourEventsHead;
        TempList2 = TempList;
        break;
    }
    case HIDDEN_HOOK_EXEC_CC:
    {
        TempList  = &g_Events->EptHookExecCcEventsHead;
        TempList2 = TempList;
        break;
    }
    case SYSCALL_HOOK_EFER_SYSCALL:
    {
        TempList  = &g_Events->SyscallHooksEferSyscallEventsHead;
        TempList2 = TempList;
        break;
    }
    case SYSCALL_HOOK_EFER_SYSRET:
    {
        TempList  = &g_Events->SyscallHooksEferSysretEventsHead;
        TempList2 = TempList;
        break;
    }
    case CPUID_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->CpuidInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case RDMSR_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->RdmsrInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case WRMSR_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->WrmsrInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case EXCEPTION_OCCURRED:
    {
        TempList  = &g_Events->ExceptionOccurredEventsHead;
        TempList2 = TempList;
        break;
    }
    case TSC_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->TscInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case PMC_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->PmcInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case IN_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->InInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case OUT_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->OutInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    case DEBUG_REGISTERS_ACCESSED:
    {
        TempList  = &g_Events->DebugRegistersAccessedEventsHead;
        TempList2 = TempList;
        break;
    }
    case EXTERNAL_INTERRUPT_OCCURRED:
    {
        TempList  = &g_Events->ExternalInterruptOccurredEventsHead;
        TempList2 = TempList;
        break;
    }   
    case VMCALL_INSTRUCTION_EXECUTION:
    {
        TempList  = &g_Events->VmcallInstructionExecutionEventsHead;
        TempList2 = TempList;
        break;
    }
    default:
        //
        // Event type is not found
        //
        return FALSE;
        break;
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
        if (CurrentEvent->CoreId != DEBUGGER_EVENT_APPLY_TO_ALL_CORES && CurrentEvent->CoreId != CurrentProcessorIndex)
        {
            //
            // This event is not related to either or core or all cores
            //
            continue;
        }

        //
        // Check if this event is for this process or not
        //
        if (CurrentEvent->ProcessId != DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES && CurrentEvent->ProcessId != PsGetCurrentProcessId())
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
            if (Context != CurrentEvent->OptionalParam1)
            {
                //
                // The interrupt is not for this event
                //
                continue;
            }
            break;
        case HIDDEN_HOOK_READ_AND_WRITE:
        case HIDDEN_HOOK_READ:
        case HIDDEN_HOOK_WRITE:
            //
            // For hidden hook read/writes we check whether the address
            // is in the range of what user specified or not, this is because
            // we get the events for all hidden hooks in a page granularity
            //

            //
            // Context is the physical address
            //
            if (!(Context >= CurrentEvent->OptionalParam1 && Context < CurrentEvent->OptionalParam2))
            {
                //
                // The value is not withing our expected range
                //
                continue;
            }
            break;
        case HIDDEN_HOOK_EXEC_DETOURS:
            //
            // Here we check if it's HIDDEN_HOOK_EXEC_DETOURS then it means
            // that it's detours hidden hook exec so we have to make sure
            // to perform its actions, only if the hook is triggered for
            // the address described in event, note that address in event
            // is a physical address and the address that the function that
            // triggers these events and sent here as the context is also
            // converted to its physical form
            //
            // This way we are sure that no one can bypass our hook by remapping
            // address to another virtual address as everything is physical
            //
            if (Context != CurrentEvent->OptionalParam1)
            {
                //
                // Context is the physical address
                //

                //
                // The hook is not for this (physical) address
                //
                continue;
            }
            break;
        case RDMSR_INSTRUCTION_EXECUTION:
        case WRMSR_INSTRUCTION_EXECUTION:
            //
            // check if MSR exit is what we want or not
            //
            if (CurrentEvent->OptionalParam1 != DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS && CurrentEvent->OptionalParam1 != Context)
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
            if (CurrentEvent->OptionalParam1 != DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES && CurrentEvent->OptionalParam1 != Context)
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
            if (CurrentEvent->OptionalParam1 != DEBUGGER_EVENT_ALL_IO_PORTS && CurrentEvent->OptionalParam1 != Context)
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
            // check syscall number
            //
            if (CurrentEvent->OptionalParam1 != DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS && CurrentEvent->OptionalParam1 != Context)
            {
                //
                // The syscall number is not what we want
                //
                continue;
            }
            break;
        default:
            break;
        }

        //
        // Check if condtion is met or not , if the condition
        // is not met then we have to avoid performing the actions
        //

        if (CurrentEvent->ConditionsBufferSize != 0)
        {
            //
            // Means that there is some conditions
            //
            ConditionFunc = CurrentEvent->ConditionBufferAddress;

            //
            // Run and check for results
            //
            // Because the user might change the nonvolatile registers, we save fastcall nonvolatile registers
            //
            if (AsmDebuggerConditionCodeHandler(Regs, Context, ConditionFunc) == 0)
            {
                //
                // The condition function returns null, mean that the
                // condition didn't met, we can ignore this event
                //
                continue;
            }
        }

        //
        // perform the actions
        //
        DebuggerPerformActions(CurrentEvent, Regs, Context);
    }

    return TRUE;
}

VOID
DebuggerPerformActions(PDEBUGGER_EVENT Event, PGUEST_REGS Regs, PVOID Context)
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
            DebuggerPerformBreakToDebugger(Event->Tag, CurrentAction, Regs, Context);
            break;
        case LOG_THE_STATES:
            DebuggerPerformLogTheStates(Event->Tag, CurrentAction, Regs, Context);
            break;
        case RUN_CUSTOM_CODE:
            DebuggerPerformRunTheCustomCode(Event->Tag, CurrentAction, Regs, Context);
            break;
        default:
            //
            // Invalid action type
            //
            break;
        }
    }
}

VOID
DebuggerPerformBreakToDebugger(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    DbgBreakPoint();
}

VOID
DebuggerPerformLogTheStates(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    //
    // Context point to the registers
    //
    DbgBreakPoint();

    //   Action->LogConfiguration.LogValue
    //   Action->LogConfiguration.LogType
    //   Action->LogConfiguration.LogMask
    //   Action->LogConfiguration.LogLength
    //   Action->ImmediatelySendTheResults
    //   Action->ActionOrderCode
}

VOID
DebuggerPerformRunTheCustomCode(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    PVOID                       ReturnBufferToUsermodeAddress = 0;

    if (Action->CustomCodeBufferSize == 0)
    {
        //
        // Sth went wrong ! the buffer size for custom code shouldn't be zero
        //
        return;
    }

    //
    // -----------------------------------------------------------------------------------------------------
    // Test (Should be removed)
    //
    // LogInfo("%x       Called from : %llx", Tag, Context);
    //

    LogInfo("Process Id : %x , Rax : %llx , R8 : %llx , Context : 0x%llx ", PsGetCurrentProcessId(), Regs->rax, Regs->r8, Context);
    //return;
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
        AsmDebuggerCustomCodeHandler(NULL, Regs, Context, Action->CustomCodeBufferAddress);
    }
    else
    {
        //
        // Because the user might change the nonvolatile registers, we save fastcall nonvolatile registers
        //
        AsmDebuggerCustomCodeHandler(Action->RequestedBuffer.RequstBufferAddress, Regs, Context, Action->CustomCodeBufferAddress);

    }

    //
    // Check if we need to send the buffer to the usermode or not we only send
    // buffer in usermode if the user requested a pre allocated buffer and
    //return its address (in RAX), it's obvious the user might request a buffer
    // and at last return another address (which is not the address of pre]
    // allocated buffer), no matter, we send the user specific buffer with the
    // size of the request for pre allocated buffer
    //
    if (ReturnBufferToUsermodeAddress != 0 && Action->RequestedBuffer.RequestBufferSize != 0)
    {
        //
        // Send the buffer to the usermode
        //

        //
        // check if buffer is valid or not, actually MS recommends not to use
        // MmIsAddressValid, but we're not doing anything fancy here just wanna
        // see if it's a valid address or not, because if this routine return
        // null then it means that we didn't find a valid physical address for
        // it, so if the programmer unintentionally forget to zero RAX, we can
        // avoid doing sth bad here. (This function uses MmGetPhysicalAddress)
        //
        if (VirtualAddressToPhysicalAddress(ReturnBufferToUsermodeAddress) != 0)
        {
            //
            // Address is valid, let send it with specific tag
            //
            LogSendBuffer(Tag, ReturnBufferToUsermodeAddress, Action->RequestedBuffer.RequestBufferSize);
        }
    }
}

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

BOOLEAN
DebuggerRemoveAllActionsFromEvent(PDEBUGGER_EVENT Event)
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
        if (CurrentAction->RequestedBuffer.RequestBufferSize != 0 && CurrentAction->RequestedBuffer.RequstBufferAddress != NULL)
        {
            //
            // There is a buffer
            //
            ExFreePoolWithTag(CurrentAction->RequestedBuffer.RequstBufferAddress, POOLTAG);
        }

        //
        // Remove the action and free the pool,
        // if it's a custom buffer then the buffer
        // is appended to the Action
        //
        ExFreePoolWithTag(CurrentAction, POOLTAG);
    }
    //
    // Remember to free the pool
    //
    return TRUE;
}

BOOLEAN
DebuggerRemoveEvent(UINT64 Tag)
{
    UINT32          ProcessorCount;
    PDEBUGGER_EVENT Event;
    PLIST_ENTRY     TempList  = 0;
    PLIST_ENTRY     TempList2 = 0;

    ProcessorCount = KeQueryActiveProcessorCount(0);

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
    DebuggerRemoveAllActionsFromEvent(Event);

    //
    // Free the pools of Event, when we free the pool,
    // ConditionsBufferAddress is also a part of the
    // event pool (ConditionBufferAddress and event
    // are both allocate in a same pool ) so both of
    // them are freed
    //
    ExFreePoolWithTag(Event, POOLTAG);

    return TRUE;
}

/**
 * @brief routines for parsing events
 * 
 * @return VOID 
 */
BOOLEAN
DebuggerParseEventFromUsermode(PDEBUGGER_GENERAL_EVENT_DETAIL EventDetails, UINT32 BufferLength, PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER ResultsToReturnUsermode)
{
    PDEBUGGER_EVENT Event;
    UINT64          PagesBytes;
    UINT32          TempPid;
    UINT32          ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // ----------------------------------------------------------------------------------
    // Validate the Event's  parameters
    // ----------------------------------------------------------------------------------
    //

    //
    // Check whether the core Id is valid or not, we read cores count
    // here because we use it in later parts
    //
    if (EventDetails->CoreId != DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
    {
        //
        // Check if the core number is not invalid
        //
        if (EventDetails->CoreId >= ProcessorCount)
        {
            //
            // CoreId is invalid (Set the error)
            //

            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INVALID_CORE_ID;
            return FALSE;
        }
    }

    if (EventDetails->EventType == EXCEPTION_OCCURRED)
    {
        //
        // Check if the exception entry doesn't exceed the first 32 entry
        //
        if (EventDetails->OptionalParam1 != DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES && EventDetails->OptionalParam1 >= 33)
        {
            //
            // We don't support entries other than first 32 IDT indexes,
            // it is because we use exception bitmaps and in order to support
            // more than 32 indexes we should use pin-based external interrupt
            // exiting which is completely different
            //
            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES;
            return FALSE;
        }
    }
    else if (EventDetails->EventType == EXTERNAL_INTERRUPT_OCCURRED)
    {
        //
        // Check if the exception entry is between 33 to 255
        //
        if (!(EventDetails->OptionalParam1 >= 33 && EventDetails->OptionalParam1 <= 0xff))
        {
            //
            // The IDT Entry is either invalid or is not in the range
            // of the pin-based external interrupt exiting controls
            //
            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID;
            return FALSE;
        }
    }
    else if (EventDetails->EventType == HIDDEN_HOOK_EXEC_DETOURS)
    {
        //
        // First check if the address are valid
        //
        TempPid = EventDetails->ProcessId;
        if (TempPid == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES)
        {
            TempPid = PsGetCurrentProcessId();
        }

        if (VirtualAddressToPhysicalAddressByProcessId(EventDetails->OptionalParam1, TempPid) == NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }
    else if (EventDetails->EventType == HIDDEN_HOOK_READ_AND_WRITE ||
             EventDetails->EventType == HIDDEN_HOOK_READ ||
             EventDetails->EventType == HIDDEN_HOOK_WRITE)
    {
        //
        // First check if the address are valid
        //
        if (VirtualAddressToPhysicalAddress(EventDetails->OptionalParam1) == NULL || VirtualAddressToPhysicalAddress(EventDetails->OptionalParam2) == NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }

        //
        // Check if the 'to' is greater that 'from'
        //
        if (EventDetails->OptionalParam1 >= EventDetails->OptionalParam2)
        {
            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }

    //
    // ----------------------------------------------------------------------------------
    // Create Event
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
                                    EventDetails->OptionalParam1,
                                    EventDetails->OptionalParam2,
                                    EventDetails->OptionalParam3,
                                    EventDetails->OptionalParam4,
                                    EventDetails->ConditionBufferSize,
                                    (UINT64)EventDetails + sizeof(DEBUGGER_GENERAL_EVENT_DETAIL));
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
                                    EventDetails->OptionalParam1,
                                    EventDetails->OptionalParam2,
                                    EventDetails->OptionalParam3,
                                    EventDetails->OptionalParam4,
                                    0,
                                    NULL);
    }

    if (Event == NULL)
    {
        //
        // Set the error
        //
        ResultsToReturnUsermode->IsSuccessful = FALSE;
        ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_UNABLE_TO_CREATE_EVENT;
        return FALSE;
    }
    //
    // Register the event
    //
    DebuggerRegisterEvent(Event);

    //
    // ----------------------------------------------------------------------------------
    // Enable Event
    // ----------------------------------------------------------------------------------
    //

    //
    // Now we should configure the cpu to generate the events
    //
    if (EventDetails->EventType == HIDDEN_HOOK_READ_AND_WRITE)
    {
        PagesBytes = PAGE_ALIGN(EventDetails->OptionalParam1);
        PagesBytes = EventDetails->OptionalParam2 - PagesBytes;

        for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
        {
            DebuggerEventEnableMonitorReadAndWriteForAddress((UINT64)EventDetails->OptionalParam1 + (i * PAGE_SIZE), EventDetails->ProcessId, TRUE, TRUE);
        }

        //
        // We convert the Event's optional parameters physical address because
        // vm-exit occurs and we have the physical address to compare in the case of
        // hidden hook rw events.
        //
        Event->OptionalParam1 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam1);
        Event->OptionalParam2 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam2);
    }
    else if (EventDetails->EventType == HIDDEN_HOOK_READ)
    {
        PagesBytes = PAGE_ALIGN(EventDetails->OptionalParam1);
        PagesBytes = EventDetails->OptionalParam2 - PagesBytes;

        for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
        {
            DebuggerEventEnableMonitorReadAndWriteForAddress((UINT64)EventDetails->OptionalParam1 + (i * PAGE_SIZE), EventDetails->ProcessId, TRUE, TRUE);
        }

        //
        // We convert the Event's optional parameters physical address because
        // vm-exit occurs and we have the physical address to compare in the case of
        // hidden hook rw events.
        //
        Event->OptionalParam1 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam1);
        Event->OptionalParam2 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam2);
    }
    else if (EventDetails->EventType == HIDDEN_HOOK_WRITE)
    {
        //
        // Read should be enabled by default, we can't ignore it
        //
        PagesBytes = PAGE_ALIGN(EventDetails->OptionalParam1);
        PagesBytes = EventDetails->OptionalParam2 - PagesBytes;

        for (size_t i = 0; i <= PagesBytes / PAGE_SIZE; i++)
        {
            DebuggerEventEnableMonitorReadAndWriteForAddress((UINT64)EventDetails->OptionalParam1 + (i * PAGE_SIZE), EventDetails->ProcessId, TRUE, TRUE);
        }

        //
        // We convert the Event's optional parameters physical address because
        // vm-exit occurs and we have the physical address to compare in the case of
        // hidden hook rw events.
        //
        Event->OptionalParam1 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam1);
        Event->OptionalParam2 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam2);
    }
    else if (EventDetails->EventType == HIDDEN_HOOK_EXEC_DETOURS)
    {
        EptPageHook(EventDetails->OptionalParam1, AsmGeneralDetourHook, EventDetails->ProcessId, FALSE, FALSE, TRUE);

        //
        // We set events OptionalParam1 here to make sure that our event is
        // executed not for all hooks but for this special hook
        //
        Event->OptionalParam1 = VirtualAddressToPhysicalAddress(EventDetails->OptionalParam1);
    }
    else if (EventDetails->EventType == RDMSR_INSTRUCTION_EXECUTION)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // All cores
            //
            ExtensionCommandChangeAllMsrBitmapReadAllCores(EventDetails->OptionalParam1);
        }
        else
        {
            //
            // Just one core
            //
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformChangeMsrBitmapReadOnSingleCore, EventDetails->OptionalParam1);
        }

        //
        // Setting an indicator to MSR
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == WRMSR_INSTRUCTION_EXECUTION)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // All cores
            //
            ExtensionCommandChangeAllMsrBitmapWriteAllCores(EventDetails->OptionalParam1);
        }
        else
        {
            //
            // Just one core
            //
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformChangeMsrBitmapWriteOnSingleCore, EventDetails->OptionalParam1);
        }

        //
        // Setting an indicator to MSR
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == IN_INSTRUCTION_EXECUTION || EventDetails->EventType == OUT_INSTRUCTION_EXECUTION)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // All cores
            //
            ExtensionCommandIoBitmapChangeAllCores(EventDetails->OptionalParam1);
        }
        else
        {
            //
            // Just one core
            //
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformChangeIoBitmapOnSingleCore, EventDetails->OptionalParam1);
        }

        //
        // Setting an indicator to MSR
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == TSC_INSTRUCTION_EXECUTION)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
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
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformEnableRdtscExitingOnSingleCore, NULL);
        }
    }
    else if (EventDetails->EventType == PMC_INSTRUCTION_EXECUTION)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
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
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformEnableRdpmcExitingOnSingleCore, NULL);
        }
    }
    else if (EventDetails->EventType == DEBUG_REGISTERS_ACCESSED)
    {
        //
        // Let's see if it is for all cores or just one core
        //
        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // All cores
            //
            ExtensionCommandEnableMovDebugRegistersExiyingAllCores();
        }
        else
        {
            //
            // Just one core
            //
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformEnableMovToDebugRegistersExiting, NULL);
        }
    }
    else if (EventDetails->EventType == EXCEPTION_OCCURRED)
    {
        //
        // Let's see if it is for all cores or just one core
        //

        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // All cores
            //
            ExtensionCommandSetExceptionBitmapAllCores(EventDetails->OptionalParam1);
        }
        else
        {
            //
            // Just one core
            //
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformSetExceptionBitmapOnSingleCore, EventDetails->OptionalParam1);
        }

        //
        // Set the event's target exception
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == EXTERNAL_INTERRUPT_OCCURRED)
    {
        //
        // Let's see if it is for all cores or just one core
        //

        if (EventDetails->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
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
            DpcRoutineRunTaskOnSingleCore(EventDetails->CoreId, DpcRoutinePerformSetExternalInterruptExitingOnSingleCore, NULL);
        }

        //
        // Set the event's target interrupt
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == SYSCALL_HOOK_EFER_SYSCALL)
    {
        DebuggerEventEnableEferOnAllProcessors();

        //
        // Set the event's target syscall number
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else if (EventDetails->EventType == SYSCALL_HOOK_EFER_SYSRET)
    {
        DebuggerEventEnableEferOnAllProcessors();

        //
        // Set the event's target syscall number
        //
        Event->OptionalParam1 = EventDetails->OptionalParam1;
    }
    else
    {
        //
        // Set the error
        //
        ResultsToReturnUsermode->IsSuccessful = FALSE;
        ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_EVENT_TYPE_IS_INVALID;
        return FALSE;
    }

    //
    // Set the status
    //
    ResultsToReturnUsermode->IsSuccessful = TRUE;
    ResultsToReturnUsermode->Error        = 0;

    return TRUE;
}

/**
 * @brief routines for parsing actions
 * 
 * @return VOID 
 */
BOOLEAN
DebuggerParseActionFromUsermode(PDEBUGGER_GENERAL_ACTION Action, UINT32 BufferLength, PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER ResultsToReturnUsermode)
{
    //
    // Check if Tag is valid or not
    //
    PDEBUGGER_EVENT Event = DebuggerGetEventByTag(Action->EventTag);

    if (Event == NULL)
    {
        //
        // Set the appropriate error
        //
        ResultsToReturnUsermode->IsSuccessful = FALSE;
        ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_TAG_NOT_EXISTS;

        //
        // Show that the
        //
        return FALSE;
    }

    if (Action->ActionType == RUN_CUSTOM_CODE)
    {
        //
        // Check if buffer is not invalid
        //
        if (Action->CustomCodeBufferSize == 0)
        {
            //
            // Set the appropriate error
            //
            ResultsToReturnUsermode->IsSuccessful = FALSE;
            ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO;

            //
            // Show that the
            //
            return FALSE;
        }
        //
        // Add action for RUN_CUSTOM_CODE
        //
        DEBUGGER_EVENT_REQUEST_CUSTOM_CODE CustomCode = {0};

        CustomCode.CustomCodeBufferSize        = Action->CustomCodeBufferSize;
        CustomCode.CustomCodeBufferAddress     = (UINT64)Action + sizeof(DEBUGGER_GENERAL_ACTION);
        CustomCode.OptionalRequestedBufferSize = Action->PreAllocatedBuffer;

        //
        // Add action to event
        //
        DebuggerAddActionToEvent(Event, RUN_CUSTOM_CODE, TRUE, &CustomCode, NULL);

        //
        // Enable the event
        //
        DebuggerEnableEvent(Event->Tag);
    }
    else if (Action->ActionType == LOG_THE_STATES)
    {
        DbgBreakPoint();
    }
    else if (Action->ActionType == BREAK_TO_DEBUGGER)
    {
        DbgBreakPoint();
    }
    else
    {
        //
        // Set the appropriate error
        //
        ResultsToReturnUsermode->IsSuccessful = FALSE;
        ResultsToReturnUsermode->Error        = DEBUGEER_ERROR_INVALID_ACTION_TYPE;

        //
        // Show that the
        //
        return FALSE;
    }

    ResultsToReturnUsermode->IsSuccessful = TRUE;
    ResultsToReturnUsermode->Error        = 0;

    return TRUE;
}

//
//   //
//   //---------------------------------------------------------------------------
//   // Example of using events and actions
//   //
//
//   //
//   // Create condition buffer
//   //
//   char CondtionBuffer[8];
//   CondtionBuffer[0] = 0x90; //nop
//   CondtionBuffer[1] = 0x90; //nop
//   CondtionBuffer[2] = 0xcc; //int 3
//   CondtionBuffer[3] = 0x90; //nop
//   CondtionBuffer[4] = 0xcc; //int 3
//   CondtionBuffer[5] = 0x90; //nop
//   CondtionBuffer[6] = 0x90; //nop
//   CondtionBuffer[7] = 0xc3; // ret
//
//   //
//   // Create event based on condition buffer
//   //
//   PDEBUGGER_EVENT Event1 = DebuggerCreateEvent(TRUE, DEBUGGER_EVENT_APPLY_TO_ALL_CORES, DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES, SYSCALL_HOOK_EFER, 0x85858585, sizeof(CondtionBuffer), CondtionBuffer);
//
//   if (!Event1)
//   {
//       LogError("Error in creating event");
//   }
//
//   //
//   // *** Add Actions example ***
//   //
//
//   //
//   // Add action for LOG_THE_STATES
//   //
//   DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION LogConfiguration = {0};
//   LogConfiguration.LogType                                 = GUEST_LOG_READ_GENERAL_PURPOSE_REGISTERS;
//   LogConfiguration.LogLength                               = 0x10;
//   LogConfiguration.LogMask                                 = 0x1;
//   LogConfiguration.LogValue                                = 0x4;
//
//   DebuggerAddActionToEvent(Event1, LOG_THE_STATES, TRUE, NULL, &LogConfiguration);
//
//   //
//   // Add action for RUN_CUSTOM_CODE
//   //
//   DEBUGGER_EVENT_REQUEST_CUSTOM_CODE CustomCode = {0};
//
//   char CustomCodeBuffer[8];
//   CustomCodeBuffer[0] = 0x90; //nop
//   CustomCodeBuffer[1] = 0x90; //nop
//   CustomCodeBuffer[2] = 0xcc; //int 3
//   CustomCodeBuffer[3] = 0x90; //nop
//   CustomCodeBuffer[4] = 0xcc; //int 3
//   CustomCodeBuffer[5] = 0x90; //nop
//   CustomCodeBuffer[6] = 0x90; //nop
//   CustomCodeBuffer[7] = 0xc3; // ret
//
//   CustomCode.CustomCodeBufferSize        = sizeof(CustomCodeBuffer);
//   CustomCode.CustomCodeBufferAddress     = CustomCodeBuffer;
//   CustomCode.OptionalRequestedBufferSize = 0x100;
//
//   DebuggerAddActionToEvent(Event1, RUN_CUSTOM_CODE, TRUE, &CustomCode, NULL);
//
//   //
//   // Add action for BREAK_TO_DEBUGGER
//   //
//   DebuggerAddActionToEvent(Event1, BREAK_TO_DEBUGGER, FALSE, NULL, NULL);
//
//   //
//   // Call to register
//   //
//   DebuggerRegisterEvent(Event1);
//
//   DbgBreakPoint();
//
//   //
//   //---------------------------------------------------------------------------
//   //
//
