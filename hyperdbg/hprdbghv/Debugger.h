/**
 * @file Debugger.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
//				Memory Manager		    		//
//////////////////////////////////////////////////

NTSTATUS
MemoryManagerReadProcessMemoryNormal(HANDLE PID, PVOID Address, DEBUGGER_READ_MEMORY_TYPE MemType, PVOID UserBuffer, SIZE_T Size, PSIZE_T ReturnSize);

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
 * @brief Use to trace the execution in the case of instrument in
 * command (i command)
 * 
 */
typedef struct _DEBUGGEE_INSTRUMENT_IN_TRACE
{
    BOOLEAN WaitForStepOnMtf;
    UINT16  CsSel; // the cs value to trace execution modes

} DEBUGGEE_INSTRUMENT_IN_TRACE, *PDEBUGGEE_INSTRUMENT_IN_TRACE;

/**
 * @brief Saves the debugger state
 * @details Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 * 
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    volatile LONG                         Lock;
    volatile BOOLEAN                      CurrentOperatingCore;
    PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE MsrState;
    PDEBUGGEE_BP_DESCRIPTOR               SoftwareBreakpointState;
    DEBUGGEE_INSTRUMENT_IN_TRACE          InstrumentInTrace;
    BOOLEAN                               EnableExternalInterruptsOnContinue;
    BOOLEAN                               PassErrorsToWindbg;
    BOOLEAN                               DisableTrapFlagOnContinue;
    BOOLEAN                               WaitingForNmi;
    BOOLEAN                               DoNotNmiNotifyOtherCoresByThisCore;
    UINT16                                InstructionLengthHint;
    PGUEST_REGS                           GuestRegs;
    UINT64                                HardwareDebugRegisterForStepping;

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

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
//					Functions					//
//////////////////////////////////////////////////

UINT64
DebuggerGetRegValueWrapper(PGUEST_REGS GuestRegs, UINT32 /* REGS_ENUM */ RegId);

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

BOOLEAN
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
