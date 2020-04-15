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
#include "GlobalVariables.h"

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

        InitializeListHead(&g_GuestState[i].Events.HiddenHookExecCcEvents);
        InitializeListHead(&g_GuestState[i].Events.HiddenHookRwEvents);
        InitializeListHead(&g_GuestState[i].Events.HiddenHooksExecDetourEvents);
        InitializeListHead(&g_GuestState[i].Events.SyscallHooksEferEvents);
    }
    //
    //---------------------------------------------------------------------------
    // Temporary test everything here
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
    PDEBUGGER_EVENT Event1 = DebuggerCreateEvent(TRUE, DEBUGGER_EVENT_APPLY_TO_ALL_CORES, SYSCALL_HOOK_EFER, 0x85858585, sizeof(CondtionBuffer), CondtionBuffer);

    if (!Event1)
    {
        LogError("Error in creating event");
    }

    //
    // Add Action to the event
    //

    //DebuggerAddActionToEvent

    //
    // Call to register
    //
    // DebuggerRegisterEvent(Event1);

    //
    //---------------------------------------------------------------------------
    //

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
    // Return our event
    //
    return Event;
}

// should not be called in vmx root
BOOLEAN
DebuggerAddActionToEvent(PDEBUGGER_EVENT Event, UINT32 OptionalRequestedBufferSize, DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType, BOOLEAN SendTheResultsImmediately, UINT32 CustomCodeBufferSize, PVOID CustomCodeBuffer)
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
    // Make the condtions lists ready
    //
    InitializeListHead(&Event->Actions);

    //
    // Allocate action + allocate code for custom code
    //
    PDEBUGGER_EVENT_ACTION Action = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_EVENT_ACTION) + CustomCodeBufferSize, POOLTAG);

    if (!Action)
    {
        //
        // There was an error in allocation
        //
        return FALSE;
    }

    RtlZeroMemory(Action, sizeof(DEBUGGER_EVENT_ACTION) + CustomCodeBufferSize);

    //
    // If the user needs a buffer to be passed to the debugger then
    // we should allocate it here (Requested buffer is only available for custom code types)
    //
    if (ActionType == RUN_CUSTOM_CODE && OptionalRequestedBufferSize != 0)
    {
        //
        // Check if the optional buffer is not more that the size
        // we can send to usermode
        //
        if (OptionalRequestedBufferSize >= MaximumPacketsCapacity)
        {
            return FALSE;
        }

        //
        // User needs a buffer to play with
        //
        PVOID RequestedBuffer = ExAllocatePoolWithTag(NonPagedPool, OptionalRequestedBufferSize, POOLTAG);

        if (!RequestedBuffer)
        {
            //
            // There was an error in allocation
            //
            ExFreePoolWithTag(Action, POOLTAG);
            return FALSE;
        }
        RtlZeroMemory(RequestedBuffer, OptionalRequestedBufferSize);

        //
        // Add it to the action
        //
        Action->RequestedBuffer.EnabledRequestBuffer = TRUE;
        Action->RequestedBuffer.RequestBufferSize    = OptionalRequestedBufferSize;
        Action->RequestedBuffer.RequstBufferAddress  = RequestedBuffer;
    }

    if (ActionType == RUN_CUSTOM_CODE)
    {
        //
        // Check if it's a Custom code without custom code buffer which is invalid
        //
        if (CustomCodeBufferSize == 0)
            return FALSE;

        //
        // Move the custom code buffer to the end of the action
        //

        Action->CustomCodeBufferSize    = CustomCodeBufferSize;
        Action->CustomCodeBufferAddress = (UINT64)Action + sizeof(DEBUGGER_EVENT_ACTION);

        //
        // copy the custom code buffer to the end of the buffer of the action
        //
        memcpy(Action->CustomCodeBufferAddress, CustomCodeBuffer, CustomCodeBufferSize);
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

    return TRUE;
}

BOOLEAN
DebuggerRemoveActionFromEvent(PDEBUGGER_EVENT Event)
{
    //
    // Remember to free the pool
    //
}

BOOLEAN
DebuggerRegisterEvent(PDEBUGGER_EVENT Event)
{
    //
    // Register the event
    //
}

BOOLEAN
DebuggerTriggerEvents(DEBUGGER_EVENT_TYPE_ENUM EventType, PVOID Context)
{
    //
    // Search for this event in this core
    //

    //
    // check if event is enabled or not
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
