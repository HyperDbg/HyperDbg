/**
 * @file Debugger.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger functions (Extensions)
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
#include "ExtensionCommands.h"
#include "GlobalVariables.h"
#include "Hooks.h"

VOID
TestMe()
{
    //
    //---------------------------------------------------------------------------
    // Example of using events and actions
    //

    //
    // Create condition buffer
    //
    char CondtionBuffer[8];
    CondtionBuffer[0] = 0x90; //nop
    CondtionBuffer[1] = 0x90; //nop
    CondtionBuffer[2] = 0xcc; //int 3
    CondtionBuffer[3] = 0x90; //nop
    CondtionBuffer[4] = 0xcc; //int 3
    CondtionBuffer[5] = 0x90; //nop
    CondtionBuffer[6] = 0x90; //nop
    CondtionBuffer[7] = 0xc3; // ret

    //
    // Create event based on condition buffer
    //
    PDEBUGGER_EVENT Event1 = DebuggerCreateEvent(TRUE, DEBUGGER_EVENT_APPLY_TO_ALL_CORES, HIDDEN_HOOK_EXEC_DETOUR, 0x85858585, sizeof(CondtionBuffer), CondtionBuffer);

    if (!Event1)
    {
        LogError("Error in creating event");
    }

    //
    // *** Add Actions example ***
    //

    //
    // Add action for RUN_CUSTOM_CODE
    //
    DEBUGGER_EVENT_REQUEST_CUSTOM_CODE CustomCode = {0};

    char CustomCodeBuffer[8];
    CustomCodeBuffer[0] = 0xcc; //int 3
    CustomCodeBuffer[1] = 0x90; //nop
    CustomCodeBuffer[2] = 0x90; //nop
    CustomCodeBuffer[3] = 0x90; //nop
    CustomCodeBuffer[4] = 0x90; //nop
    CustomCodeBuffer[5] = 0x90; //nop
    CustomCodeBuffer[6] = 0xcc; //int 3
    CustomCodeBuffer[7] = 0xc3; //ret

    CustomCode.CustomCodeBufferSize        = sizeof(CustomCodeBuffer);
    CustomCode.CustomCodeBufferAddress     = CustomCodeBuffer;
    CustomCode.OptionalRequestedBufferSize = 0x100;

    DebuggerAddActionToEvent(Event1, RUN_CUSTOM_CODE, TRUE, &CustomCode, NULL);

    /*
    //
    // Add action for BREAK_TO_DEBUGGER
    //
    DebuggerAddActionToEvent(Event1, BREAK_TO_DEBUGGER, FALSE, NULL, NULL);

    //
    // Add action for LOG_THE_STATES
    //

    DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION LogConfiguration = {0};
    LogConfiguration.LogType                                 = GUEST_LOG_READ_GENERAL_PURPOSE_REGISTERS;
    LogConfiguration.LogLength                               = 0x10;
    LogConfiguration.LogMask                                 = 0x1;
    LogConfiguration.LogValue                                = 0x4;

    DebuggerAddActionToEvent(Event1, LOG_THE_STATES, TRUE, NULL, &LogConfiguration);
    */

    //
    // Call to register
    //
    DebuggerRegisterEvent(Event1);

    //
    // Enable one event to test it
    //
    //ExtensionCommandEnableEferOnAllProcessors();
    HiddenHooksTest();
}

BOOLEAN
DebuggerInitialize()
{
    UINT32 ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Initialize lists relating to the debugger events store
    //
    for (size_t i = 0; i < ProcessorCount; i++)
    {
        //
        // Initialize list of each event
        //

        InitializeListHead(&g_GuestState[i].Events.HiddenHookExecCcEventsHead);
        InitializeListHead(&g_GuestState[i].Events.HiddenHookRwEventsHead);
        InitializeListHead(&g_GuestState[i].Events.HiddenHooksExecDetourEventsHead);
        InitializeListHead(&g_GuestState[i].Events.SyscallHooksEferEventsHead);
    }

    //
    // Initialize the list of hidden hooks headers
    //
    InitializeListHead(&g_HiddenHooksDetourListHead);

    //
    // Enabled Debugger Events
    //
    g_EnableDebuggerEvents = TRUE;

    TestMe();

    return TRUE;
}

// should not be called in vmx root
PDEBUGGER_EVENT
DebuggerCreateEvent(BOOLEAN Enabled, UINT32 CoreId, DEBUGGER_EVENT_TYPE_ENUM EventType, UINT64 Tag, UINT32 ConditionsBufferSize, PVOID ConditionBuffer)
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
    Event->Enabled        = Enabled;
    Event->EventType      = EventType;
    Event->Tag            = Tag;
    Event->CountOfActions = 0; // currently there is no action

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
BOOLEAN
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
            return FALSE;
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
            return FALSE;
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
            return FALSE;
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
            return FALSE;
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
            return FALSE;

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

    return TRUE;
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
    case HIDDEN_HOOK_RW:
        if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // We have to apply this Event to all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                //
                // Add it to the list of the events with same type
                //
                InsertHeadList(&g_GuestState[i].Events.HiddenHookRwEventsHead, &(Event->EventsOfSameTypeList));
            }
        }
        else if (Event->CoreId > ProcessorCount) // Check if the core Id is not invalid
        {
            //
            // Add it to the list of the events with same type
            //
            InsertHeadList(&g_GuestState[Event->CoreId].Events.HiddenHookRwEventsHead, &(Event->EventsOfSameTypeList));
        }
        else
        {
            //
            // Invalid core id
            //
            return FALSE;
        }
        break;
    case HIDDEN_HOOK_EXEC_DETOUR:
        if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // We have to apply this Event to all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                //
                // Add it to the list of the events with same type
                //
                InsertHeadList(&g_GuestState[i].Events.HiddenHooksExecDetourEventsHead, &(Event->EventsOfSameTypeList));
            }
        }
        else if (Event->CoreId > ProcessorCount) // Check if the core Id is not invalid
        {
            //
            // Add it to the list of the events with same type
            //
            InsertHeadList(&g_GuestState[Event->CoreId].Events.HiddenHooksExecDetourEventsHead, &(Event->EventsOfSameTypeList));
        }
        else
        {
            //
            // Invalid core id
            //
            return FALSE;
        }
        break;
    case HIDDEN_HOOK_EXEC_CC:
        if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // We have to apply this Event to all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                //
                // Add it to the list of the events with same type
                //
                InsertHeadList(&g_GuestState[i].Events.HiddenHookExecCcEventsHead, &(Event->EventsOfSameTypeList));
            }
        }
        else if (Event->CoreId > ProcessorCount) // Check if the core Id is not invalid
        {
            //
            // Add it to the list of the events with same type
            //
            InsertHeadList(&g_GuestState[Event->CoreId].Events.HiddenHookExecCcEventsHead, &(Event->EventsOfSameTypeList));
        }
        else
        {
            //
            // Invalid core id
            //
            return FALSE;
        }
        break;
    case SYSCALL_HOOK_EFER:
        if (Event->CoreId == DEBUGGER_EVENT_APPLY_TO_ALL_CORES)
        {
            //
            // We have to apply this Event to all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                //
                // Add it to the list of the events with same type
                //
                InsertHeadList(&g_GuestState[i].Events.SyscallHooksEferEventsHead, &(Event->EventsOfSameTypeList));
            }
        }
        else if (Event->CoreId > ProcessorCount) // Check if the core Id is not invalid
        {
            //
            // Add it to the list of the events with same type
            //
            InsertHeadList(&g_GuestState[Event->CoreId].Events.SyscallHooksEferEventsHead, &(Event->EventsOfSameTypeList));
        }
        else
        {
            //
            // Invalid core id
            //
            return FALSE;
        }
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
    ULONG       CurrentProcessorIndex;
    PLIST_ENTRY TempList  = 0;
    PLIST_ENTRY TempList2 = 0;

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

    if (EventType == HIDDEN_HOOK_RW)
    {
        TempList2 = &g_GuestState[CurrentProcessorIndex].Events.HiddenHookRwEventsHead;
        TempList  = &g_GuestState[CurrentProcessorIndex].Events.HiddenHookRwEventsHead;
    }
    else if (EventType == HIDDEN_HOOK_EXEC_DETOUR)
    {
        TempList2 = &g_GuestState[CurrentProcessorIndex].Events.HiddenHooksExecDetourEventsHead;
        TempList  = &g_GuestState[CurrentProcessorIndex].Events.HiddenHooksExecDetourEventsHead;
    }
    else if (EventType == HIDDEN_HOOK_EXEC_CC)
    {
        TempList2 = &g_GuestState[CurrentProcessorIndex].Events.HiddenHookExecCcEventsHead;
        TempList  = &g_GuestState[CurrentProcessorIndex].Events.HiddenHookExecCcEventsHead;
    }
    else if (EventType == SYSCALL_HOOK_EFER)
    {
        TempList2 = &g_GuestState[CurrentProcessorIndex].Events.SyscallHooksEferEventsHead;
        TempList  = &g_GuestState[CurrentProcessorIndex].Events.SyscallHooksEferEventsHead;
    }
    else
    {
        //
        // Event type is not found
        //
        return FALSE;
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
DebuggerPerformLogTheStates(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    //
    // Context point to the registers
    //
    DbgBreakPoint();

    //   Action->LogConfiguration.LogValue
    //       Action->LogConfiguration.LogType
    //          Action->LogConfiguration.LogMask
    //              Action->LogConfiguration.LogLength
    //                 Action->ImmediatelySendTheResults
    //                    Action->ActionOrderCode
}

VOID
DebuggerPerformBreakToDebugger(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    DbgBreakPoint();
}
VOID
DebuggerPerformRunTheCustomCode(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context)
{
    PVOID                                             ReturnBufferToUsermodeAddress = 0;
    DebuggerRunCustomCodeFunc *                       Func;
    DebuggerRunCustomCodeWithPreAllocatedBufferFunc * FuncWithPreAllocBuffer;

    if (Action->CustomCodeBufferSize == 0)
    {
        //
        // Sth went wrong ! the buffer size for custom code shouldn't be zero
        //
        return;
    }

    //
    // Run the custom code
    //
    if (Action->RequestedBuffer.RequestBufferSize == 0)
    {
        //
        // Means that this custom code doesn't requested a pre-allocated buffer
        //
        Func = (DebuggerRunCustomCodeFunc *)Action->CustomCodeBufferAddress;

        //
        // Execute the code
        //
        ReturnBufferToUsermodeAddress = Func();
    }
    else
    {
        //
        // Means that this custom code doesn't requested a pre-allocated buffer
        //
        FuncWithPreAllocBuffer = (DebuggerRunCustomCodeWithPreAllocatedBufferFunc *)Action->CustomCodeBufferAddress;

        //
        // Execute the code (with requested buffer parameter)
        //
        ReturnBufferToUsermodeAddress = FuncWithPreAllocBuffer(Action->RequestedBuffer.RequstBufferAddress);
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

BOOLEAN
DebuggerRemoveActionFromEvent(PDEBUGGER_EVENT Event)
{
    //
    // Remember to free the pool
    //
}

BOOLEAN
DebuggerUnregisterEvent(UINT64 Tag)
{
    //
    // Seach all the cores for remove this event
    //
}
BOOLEAN
DebuggerDisableEvent(UINT64 Tag)
{
    //
    // Seach all the cores for disable this event
    //
}

BOOLEAN
DebuggerRemoveEvent(PDEBUGGER_EVENT Event)
{
    //
    // Remember to free the pool
    //
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
//   PDEBUGGER_EVENT Event1 = DebuggerCreateEvent(TRUE, DEBUGGER_EVENT_APPLY_TO_ALL_CORES, SYSCALL_HOOK_EFER, 0x85858585, sizeof(CondtionBuffer), CondtionBuffer);
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
