/**
 * @file Debugger.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief General debugger headers
 * @details
 * @version 0.1
 * @date 2020-04-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//  	Global Variable (debugger-related)	    //
//////////////////////////////////////////////////

/**
 * @brief Showes whether the vmcall handler is
 * allowed to trigger an event or not
 *
 */
BOOLEAN g_TriggerEventForVmcalls;

/**
 * @brief Showes whether the cpuid handler is
 * allowed to trigger an event or not
 *
 */
BOOLEAN g_TriggerEventForCpuids;

//////////////////////////////////////////////////
//              Debugger Internals              //
//////////////////////////////////////////////////

/**
 * @brief debug register for step-over
 */
#define DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER 0

/**
 * @brief debug register to monitor thread changes
 */
#define DEBUGGER_DEBUG_REGISTER_FOR_THREAD_MANAGEMENT 1

/**
 * @brief debug register get entrypoint of user-mode process
 */
#define DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT 1

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief List of all the different events
 *
 */
typedef struct _DEBUGGER_CORE_EVENTS
{
    //
    // Warnings : Only list entries should be in this list, nothing else
    //

    //
    // Do not add varialbe to this this list, just LIST_ENTRY is allowed
    //
    LIST_ENTRY HiddenHookReadAndWriteEventsHead;     // HIDDEN_HOOK_READ_AND_WRITE  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY HiddenHookReadEventsHead;             // HIDDEN_HOOK_READ  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY HiddenHookWriteEventsHead;            // HIDDEN_HOOK_WRITE  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY EptHook2sExecDetourEventsHead;        // HIDDEN_HOOK_EXEC_DETOURS [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY EptHookExecCcEventsHead;              // HIDDEN_HOOK_EXEC_CC [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY SyscallHooksEferSyscallEventsHead;    // SYSCALL_HOOK_EFER_SYSCALL [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY SyscallHooksEferSysretEventsHead;     // SYSCALL_HOOK_EFER_SYSRET [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY CpuidInstructionExecutionEventsHead;  // CPUID_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY RdmsrInstructionExecutionEventsHead;  // RDMSR_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY WrmsrInstructionExecutionEventsHead;  // WRMSR_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY ExceptionOccurredEventsHead;          // EXCEPTION_OCCURRED [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY TscInstructionExecutionEventsHead;    // TSC_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY PmcInstructionExecutionEventsHead;    // PMC_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY InInstructionExecutionEventsHead;     // IN_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY OutInstructionExecutionEventsHead;    // OUT_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY DebugRegistersAccessedEventsHead;     // DEBUG_REGISTERS_ACCESSED [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY ExternalInterruptOccurredEventsHead;  // EXTERNAL_INTERRUPT_OCCURRED [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY VmcallInstructionExecutionEventsHead; // VMCALL_INSTRUCTION_EXECUTION [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]
    LIST_ENTRY ControlRegisterModifiedEventsHead;     // CONTROL_REGISTER_MODIFIED [WARNING : MAKE SURE TO INITIALIZE LIST HEAD , Add it to DebuggerRegisterEvent, Add it to DebuggerTriggerEvents, Add termination to DebuggerTerminateEvent ]

} DEBUGGER_CORE_EVENTS, *PDEBUGGER_CORE_EVENTS;

/**
 * @brief Use to modify Msrs or read MSR values
 *
 */
typedef struct _PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE
{
    UINT64 Msr;   // Msr (ecx)
    UINT64 Value; // the value to write on msr

} PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE, *PPROCESSOR_DEBUGGING_MSR_READ_OR_WRITE;

/**
 * @brief Use to trace the execution in the case of instrumentation step-in
 * command (i command)
 *
 */
typedef struct _DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE
{
    BOOLEAN WaitForInstrumentationStepInMtf;
    UINT16  CsSel; // the cs value to trace the execution modes

} DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE, *PDEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE;

/**
 * @brief Structure to save the state of adding trace for threads
 * and processes
 *
 */
typedef struct _DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS
{
    BOOLEAN InitialSetProcessChangeEvent;
    BOOLEAN InitialSetThreadChangeEvent;

    BOOLEAN InitialSetByClockInterrupt;

    //
    // For threads
    //
    UINT64  CurrentThreadLocationOnGs;
    BOOLEAN DebugRegisterInterceptionState;
    BOOLEAN InterceptClockInterruptsForThreadChange;

    //
    // For processes
    //
    BOOLEAN IsWatingForMovCr3VmExits;
    BOOLEAN InterceptClockInterruptsForProcessChange;

} DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS, *PDEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS;

/**
 * @brief Saves the debugger state
 * @details Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 *
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    volatile LONG                              Lock;
    volatile BOOLEAN                           WaitingToBeLocked;
    volatile BOOLEAN                           MainDebuggingCore;
    volatile BOOLEAN                           NmiCalledInVmxRootRelatedToHaltDebuggee;
    volatile NMI_BROADCAST_ACTION_TYPE         NmiBroadcastAction;
    BOOLEAN                                    IgnoreEvent;
    BOOLEAN                                    IgnoreOneMtf;
    BOOLEAN                                    WaitForStepTrap;
    PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE      MsrState;
    PDEBUGGEE_BP_DESCRIPTOR                    SoftwareBreakpointState;
    DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE     InstrumentationStepInTrace;
    BOOLEAN                                    EnableExternalInterruptsOnContinue;
    BOOLEAN                                    EnableExternalInterruptsOnContinueMtf;
    BOOLEAN                                    DisableTrapFlagOnContinue;
    BOOLEAN                                    DoNotNmiNotifyOtherCoresByThisCore;
    DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS ThreadOrProcessTracingDetails;
    BOOLEAN                                    BreakStarterCore;
    UINT16                                     InstructionLengthHint;
    UINT64                                     HardwareDebugRegisterForStepping;
    UINT64 *                                   ScriptEngineCoreSpecificLocalVariable;
    UINT64 *                                   ScriptEngineCoreSpecificTempVariable;

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

/**
 * @brief The structure of actions in HyperDbg
 *
 */
typedef struct _DEBUGGER_EVENT_ACTION
{
    UINT64                          Tag;                       // Action tag is same as Event's tag
    UINT32                          ActionOrderCode;           // The code for this action (it also shows the order)
    LIST_ENTRY                      ActionsList;               // Holds the link list of next actions
    DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType;                // What action we wanna perform
    BOOLEAN                         ImmediatelySendTheResults; // should we send the results immediately
                                                               // or store them in another structure and
                                                               // send multiple of them each time

    DEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION
    ScriptConfiguration; // If it's run script

    DEBUGGER_EVENT_REQUEST_BUFFER
    RequestedBuffer; // if it's a custom code and needs a buffer then we use
                     // this structs

    UINT32 CustomCodeBufferSize;    // if null, means it's not custom code type
    PVOID  CustomCodeBufferAddress; // address of custom code if any

} DEBUGGER_EVENT_ACTION, *PDEBUGGER_EVENT_ACTION;

/* ==============================================================================================
 */

/**
 * @brief The structure of events in HyperDbg
 *
 */
typedef struct _DEBUGGER_EVENT
{
    UINT64                   Tag;
    LIST_ENTRY               EventsOfSameTypeList; // Linked-list of events of a same type
    DEBUGGER_EVENT_TYPE_ENUM EventType;
    BOOLEAN                  Enabled;
    UINT32                   CoreId; // determines the core index to apply this event to, if it's
                                     // 0xffffffff means that we have to apply it to all cores

    UINT32
    ProcessId; // determines the pid to apply this event to, if it's
               // 0xffffffff means that we have to apply it to all processes

    LIST_ENTRY ActionsListHead; // Each entry is in DEBUGGER_EVENT_ACTION struct
    UINT32     CountOfActions;  // The total count of actions

    UINT64 OptionalParam1; // Optional parameter to be used differently by events
    UINT64 OptionalParam2; // Optional parameter to be used differently by events
    UINT64 OptionalParam3; // Optional parameter to be used differently by events
    UINT64 OptionalParam4; // Optional parameter to be used differently by events

    UINT32 ConditionsBufferSize;   // if null, means uncoditional
    PVOID  ConditionBufferAddress; // Address of the condition buffer (most of the
                                   // time at the end of this buffer)

} DEBUGGER_EVENT, *PDEBUGGER_EVENT;

/* ==============================================================================================
 */

//////////////////////////////////////////////////
//	    				Enums					//
//////////////////////////////////////////////////

/**
 * @brief The status of triggering events
 *
 */
typedef enum _DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL              = 0,
    DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT = 1,
    DEBUGGER_TRIGGERING_EVENT_STATUS_DEBUGGER_NOT_ENABLED    = 2,
    DEBUGGER_TRIGGERING_EVENT_STATUS_INVALID_EVENT_TYPE      = 3,

} DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE;

//////////////////////////////////////////////////
//					Data Type					//
//////////////////////////////////////////////////

/**
 * @brief The prototype that Condition codes are called
 *
 * @param Regs Guest registers
 * @param Context Optional parameter which is different
 * for each event and shows a unique description about
 * the event
 * @return UINT64 if return 0 then the event is not allowed
 * to trigger and if any other value then the event is allowed
 * to be triggered
 * return value should be on RAX
 */
typedef UINT64
DebuggerCheckForCondition(PGUEST_REGS Regs, PVOID Context);

/**
 * @brief The prototype that Custom code buffers are called
 *
 * @param PreAllocatedBufferAddress The address of a pre-allocated non-paged pool
 * if the user-requested for it
 * @param Regs Guest registers
 * @param Context Optional parameter which is different
 * for each event and shows a unique description about
 * the event
 * @return PVOID A pointer to a buffer that should be delivered to the user-mode
 * if returns null or an invalid address then nothing will be delivered
 * return address should be on RAX
 */
typedef PVOID
DebuggerRunCustomCodeFunc(PVOID PreAllocatedBufferAddress, PGUEST_REGS Regs, PVOID Context);

//////////////////////////////////////////////////
//				Memory Manager		    		//
//////////////////////////////////////////////////

NTSTATUS
MemoryManagerReadProcessMemoryNormal(HANDLE PID, PVOID Address, DEBUGGER_READ_MEMORY_TYPE MemType, PVOID UserBuffer, SIZE_T Size, PSIZE_T ReturnSize);

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

UINT64
DebuggerGetRegValueWrapper(PGUEST_REGS GuestRegs, UINT32 /* REGS_ENUM */ RegId);

UINT32
DebuggerGetLastError();

VOID
DebuggerSetLastError(UINT32 LastError);

BOOLEAN
DebuggerInitialize();

VOID
DebuggerUninitialize();

PDEBUGGER_EVENT
DebuggerCreateEvent(BOOLEAN Enabled, UINT32 CoreId, UINT32 ProcessId, DEBUGGER_EVENT_TYPE_ENUM EventType, UINT64 Tag, UINT64 OptionalParam1, UINT64 OptionalParam2, UINT64 OptionalParam3, UINT64 OptionalParam4, UINT32 ConditionsBufferSize, PVOID ConditionBuffer);

PDEBUGGER_EVENT_ACTION
DebuggerAddActionToEvent(PDEBUGGER_EVENT Event, DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType, BOOLEAN SendTheResultsImmediately, PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE InTheCaseOfCustomCode, PDEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION InTheCaseOfRunScript);

BOOLEAN
DebuggerRegisterEvent(PDEBUGGER_EVENT Event);

DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE
DebuggerTriggerEvents(DEBUGGER_EVENT_TYPE_ENUM EventType, PGUEST_REGS Regs, PVOID Context);

PDEBUGGER_EVENT
DebuggerGetEventByTag(UINT64 Tag);

BOOLEAN
DebuggerRemoveEvent(UINT64 Tag);

BOOLEAN
DebuggerParseEventFromUsermode(PDEBUGGER_GENERAL_EVENT_DETAIL EventDetails, UINT32 BufferLength, PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER ResultsToReturnUsermode);

BOOLEAN
DebuggerParseActionFromUsermode(PDEBUGGER_GENERAL_ACTION Action, UINT32 BufferLength, PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER ResultsToReturnUsermode);

BOOLEAN
DebuggerParseEventsModificationFromUsermode(PDEBUGGER_MODIFY_EVENTS DebuggerEventModificationRequest);

BOOLEAN
DebuggerTerminateEvent(UINT64 Tag);

BOOLEAN
DebuggerEnableOrDisableAllEvents(BOOLEAN IsEnable);

BOOLEAN
DebuggerRemoveAllEvents();

BOOLEAN
DebuggerTerminateAllEvents();

UINT32
DebuggerEventListCount(PLIST_ENTRY TargetEventList);

UINT32
DebuggerEventListCountByCore(PLIST_ENTRY TargetEventList, UINT32 TargetCore);

UINT32
DebuggerExceptionEventBitmapMask(UINT32 CoreIndex);

BOOLEAN
DebuggerIsTagValid(UINT64 Tag);

BOOLEAN
DebuggerEnableEvent(UINT64 Tag);

BOOLEAN
DebuggerQueryStateEvent(UINT64 Tag);

BOOLEAN
DebuggerDisableEvent(UINT64 Tag);

VOID
DebuggerPerformActions(PDEBUGGER_EVENT Event, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformBreakToDebugger(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);

BOOLEAN
DebuggerPerformRunScript(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PDEBUGGEE_SCRIPT_PACKET ScriptDetails, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformRunTheCustomCode(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);
