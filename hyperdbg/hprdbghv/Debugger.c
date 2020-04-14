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
    char CondtionBuffer[8];
    CondtionBuffer[0] = 0x90; //nop
    CondtionBuffer[1] = 0x90; //nop
    CondtionBuffer[2] = 0xcc; //int 3
    CondtionBuffer[3] = 0x90; //nop
    CondtionBuffer[4] = 0xcc; //int 3
    CondtionBuffer[5] = 0x90; //nop
    CondtionBuffer[6] = 0x90; //nop
    CondtionBuffer[7] = 0xc3; // ret

    DebuggerCreateEvent(TRUE, DEBUGGER_EVENT_APPLY_TO_ALL_CORES, SYSCALL_HOOK_EFER, 0x85858585, sizeof(CondtionBuffer), CondtionBuffer);

    //
    // Add Action to the event
    //
    // DebuggerAddActionToEvent

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
    RtlZeroMemory(Event, sizeof(DEBUGGER_EVENT) + ConditionsBufferSize);
    Event->CoreId                 = CoreId;
    Event->Enabled                = Enabled;
    Event->EventType              = EventType;
    Event->Tag                    = Tag;
    Event->ConditionsBufferSize   = ConditionsBufferSize;
    Event->ConditionBufferAddress = (UINT64)Event + sizeof(DEBUGGER_EVENT);

    //
    // copy the condtion buffer to the end of the buffer of event
    //
    memcpy(Event->ConditionBufferAddress, ConditionBuffer, ConditionsBufferSize);

    //
    // Return our event
    //
    return Event;
}

BOOLEAN
DebuggerAddActionToEvent(PDEBUGGER_EVENT Event)
{
    //
    // Make the condtions lists ready
    //
    // InitializeListHead(&Event1->Actions);

    //////////////// remembr to zero the buffer
    // RtlZeroMemory(Event1, sizeof(DEBUGGER_EVENT) + ConditionBufferSize);
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
    // Todo : Iterate through the Action
    //
    PDEBUGGER_EVENT_ACTION Action = NULL;

    //
    // Allocate buffer if it's a run custom code and needs buffer
    //
    if (Action->ActionType == RUN_CUSTOM_CODE && Action->Buffer.EnabledRequestBuffer)
    {
        //
        // ******* This application needs to pass a buffer to the custom code *******
        //

        //
        // Let's allocate the buffer
        //
        Action->Buffer.RequstBufferAddress = ExAllocatePoolWithTag(NonPagedPool, Action->Buffer.RequestBufferSize, POOLTAG);

        if (!Action->Buffer.RequstBufferAddress)
        {
            LogError("Unable to allocate buffer");
            return FALSE;
        }

        RtlZeroMemory(Action->Buffer.RequstBufferAddress, Action->Buffer.RequestBufferSize);
    }

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
