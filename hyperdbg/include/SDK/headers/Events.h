/**
 * @file Events.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK Headers for Events
 * @details This file contains definitions of event datatypes
 * @version 0.2
 * @date 2022-06-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//               System Events                  //
//////////////////////////////////////////////////

/**
 * @brief Exceptions enum
 *
 */
typedef enum _EXCEPTION_VECTORS
{
    EXCEPTION_VECTOR_DIVIDE_ERROR,
    EXCEPTION_VECTOR_DEBUG_BREAKPOINT,
    EXCEPTION_VECTOR_NMI,
    EXCEPTION_VECTOR_BREAKPOINT,
    EXCEPTION_VECTOR_OVERFLOW,
    EXCEPTION_VECTOR_BOUND_RANGE_EXCEEDED,
    EXCEPTION_VECTOR_UNDEFINED_OPCODE,
    EXCEPTION_VECTOR_NO_MATH_COPROCESSOR,
    EXCEPTION_VECTOR_DOUBLE_FAULT,
    EXCEPTION_VECTOR_RESERVED0,
    EXCEPTION_VECTOR_INVALID_TASK_SEGMENT_SELECTOR,
    EXCEPTION_VECTOR_SEGMENT_NOT_PRESENT,
    EXCEPTION_VECTOR_STACK_SEGMENT_FAULT,
    EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
    EXCEPTION_VECTOR_PAGE_FAULT,
    EXCEPTION_VECTOR_RESERVED1,
    EXCEPTION_VECTOR_MATH_FAULT,
    EXCEPTION_VECTOR_ALIGNMENT_CHECK,
    EXCEPTION_VECTOR_MACHINE_CHECK,
    EXCEPTION_VECTOR_SIMD_FLOATING_POINT_NUMERIC_ERROR,
    EXCEPTION_VECTOR_VIRTUAL_EXCEPTION,
    EXCEPTION_VECTOR_RESERVED2,
    EXCEPTION_VECTOR_RESERVED3,
    EXCEPTION_VECTOR_RESERVED4,
    EXCEPTION_VECTOR_RESERVED5,
    EXCEPTION_VECTOR_RESERVED6,
    EXCEPTION_VECTOR_RESERVED7,
    EXCEPTION_VECTOR_RESERVED8,
    EXCEPTION_VECTOR_RESERVED9,
    EXCEPTION_VECTOR_RESERVED10,
    EXCEPTION_VECTOR_RESERVED11,
    EXCEPTION_VECTOR_RESERVED12,

    //
    // NT (Windows) specific exception vectors.
    //
    APC_INTERRUPT   = 31,
    DPC_INTERRUPT   = 47,
    CLOCK_INTERRUPT = 209,
    IPI_INTERRUPT   = 225,
    PMI_INTERRUPT   = 254,

} EXCEPTION_VECTORS;

//////////////////////////////////////////////////
//			     Callback Enums                 //
//////////////////////////////////////////////////

/**
 * @brief The status of triggering events
 *
 */
typedef enum _VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE
{
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL_NO_INITIALIZED = 0,
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL                = 0,
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT   = 1,
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_DEBUGGER_NOT_ENABLED      = 2,
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_INVALID_EVENT_TYPE        = 3,

} VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE;

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

/**
 * @brief enum to show type of all HyperDbg events
 *
 */
typedef enum _VMM_EVENT_TYPE_ENUM
{

    //
    // EPT Memory Monitoring Events
    //
    HIDDEN_HOOK_READ_AND_WRITE_AND_EXECUTE,
    HIDDEN_HOOK_READ_AND_WRITE,
    HIDDEN_HOOK_READ_AND_EXECUTE,
    HIDDEN_HOOK_WRITE_AND_EXECUTE,
    HIDDEN_HOOK_READ,
    HIDDEN_HOOK_WRITE,
    HIDDEN_HOOK_EXECUTE,

    //
    // EPT Hook Events
    //
    HIDDEN_HOOK_EXEC_DETOURS,
    HIDDEN_HOOK_EXEC_CC,

    //
    // System-call Events
    //
    SYSCALL_HOOK_EFER_SYSCALL,
    SYSCALL_HOOK_EFER_SYSRET,

    //
    // CPUID Instruction Execution Events
    //
    CPUID_INSTRUCTION_EXECUTION,

    //
    // Model-Specific Registers (MSRs) Reads/Modifications Events
    //
    RDMSR_INSTRUCTION_EXECUTION,
    WRMSR_INSTRUCTION_EXECUTION,

    //
    // PMIO Events
    //
    IN_INSTRUCTION_EXECUTION,
    OUT_INSTRUCTION_EXECUTION,

    //
    // Interrupts/Exceptions/Faults Events
    //
    EXCEPTION_OCCURRED,
    EXTERNAL_INTERRUPT_OCCURRED,

    //
    // Debug Registers Events
    //
    DEBUG_REGISTERS_ACCESSED,

    //
    // Timing & Performance Events
    //
    TSC_INSTRUCTION_EXECUTION,
    PMC_INSTRUCTION_EXECUTION,

    //
    // VMCALL Instruction Execution Events
    //
    VMCALL_INSTRUCTION_EXECUTION,

    //
    // Control Registers Events
    //
    CONTROL_REGISTER_MODIFIED,
    CONTROL_REGISTER_READ,
    CONTROL_REGISTER_3_MODIFIED,

    //
    // Execution Trap Events
    //
    TRAP_EXECUTION_MODE_CHANGED,
    TRAP_EXECUTION_INSTRUCTION_TRACE,

    //
    // XSETBV Instruction Execution Events
    //
    XSETBV_INSTRUCTION_EXECUTION

} VMM_EVENT_TYPE_ENUM;

/**
 * @brief Type of Actions
 *
 */
typedef enum _DEBUGGER_EVENT_ACTION_TYPE_ENUM
{
    BREAK_TO_DEBUGGER,
    RUN_SCRIPT,
    RUN_CUSTOM_CODE

} DEBUGGER_EVENT_ACTION_TYPE_ENUM;

/**
 * @brief Type of handling !syscall or !sysret
 *
 */
typedef enum _DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE
{
    DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY = 0,
    DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD      = 1,

} DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE;

#define SIZEOF_DEBUGGER_MODIFY_EVENTS sizeof(DEBUGGER_MODIFY_EVENTS)

/**
 * @brief Type of mode change traps
 *
 */
typedef enum _DEBUGGER_EVENT_MODE_TYPE
{
    DEBUGGER_EVENT_MODE_TYPE_USER_MODE_AND_KERNEL_MODE = 1,
    DEBUGGER_EVENT_MODE_TYPE_USER_MODE                 = 3,
    DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE               = 0,
    DEBUGGER_EVENT_MODE_TYPE_INVALID                   = 0xffffffff,

} DEBUGGER_EVENT_MODE_TYPE;

/**
 * @brief Type of tracing events
 *
 */
typedef enum _DEBUGGER_EVENT_TRACE_TYPE
{
    DEBUGGER_EVENT_TRACE_TYPE_INVALID                 = 0,
    DEBUGGER_EVENT_TRACE_TYPE_STEP_IN                 = 1,
    DEBUGGER_EVENT_TRACE_TYPE_STEP_OUT                = 2,
    DEBUGGER_EVENT_TRACE_TYPE_INSTRUMENTATION_STEP_IN = 3,

} DEBUGGER_EVENT_TRACE_TYPE;

/**
 * @brief different types of modifying events request (enable/disable/clear)
 *
 */
typedef enum _DEBUGGER_MODIFY_EVENTS_TYPE
{
    DEBUGGER_MODIFY_EVENTS_QUERY_STATE,
    DEBUGGER_MODIFY_EVENTS_ENABLE,
    DEBUGGER_MODIFY_EVENTS_DISABLE,
    DEBUGGER_MODIFY_EVENTS_CLEAR,
} DEBUGGER_MODIFY_EVENTS_TYPE;

/**
 * @brief request for modifying events (enable/disable/clear)
 *
 */
typedef struct _DEBUGGER_MODIFY_EVENTS
{
    UINT64 Tag;          // Tag of the target event that we want to modify
    UINT64 KernelStatus; // Kernel put the status in this field
    DEBUGGER_MODIFY_EVENTS_TYPE
    TypeOfAction;      // Determines what's the action (enable | disable | clear)
    BOOLEAN IsEnabled; // Determines what's the action (enable | disable | clear)

} DEBUGGER_MODIFY_EVENTS, *PDEBUGGER_MODIFY_EVENTS;

/**
 * @brief request for performing a short-circuiting event
 *
 */
typedef struct _DEBUGGER_SHORT_CIRCUITING_EVENT
{
    UINT64  KernelStatus;      // Kernel put the status in this field
    BOOLEAN IsShortCircuiting; // Determines whether to perform short circuting (on | off)

} DEBUGGER_SHORT_CIRCUITING_EVENT, *PDEBUGGER_SHORT_CIRCUITING_EVENT;

//////////////////////////////////////////////////
//                Event Options                 //
//////////////////////////////////////////////////

/**
 * @brief request for performing a short-circuiting event
 *
 */
typedef struct _DEBUGGER_EVENT_OPTIONS
{
    UINT64 OptionalParam1; // Optional parameter
    UINT64 OptionalParam2; // Optional parameter
    UINT64 OptionalParam3; // Optional parameter
    UINT64 OptionalParam4; // Optional parameter
    UINT64 OptionalParam5; // Optional parameter
    UINT64 OptionalParam6; // Optional parameter

} DEBUGGER_EVENT_OPTIONS, *PDEBUGGER_EVENT_OPTIONS;

//////////////////////////////////////////////////
//    Enums For Event And Debugger Resources    //
//////////////////////////////////////////////////

/**
 * @brief Things to consider when applying resources
 *
 */
typedef enum _PROTECTED_HV_RESOURCES_PASSING_OVERS
{
    //
    // for exception bitmap
    //
    PASSING_OVER_NONE                                  = 0,
    PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK = 1,
    PASSING_OVER_EXCEPTION_EVENTS,

    //
    // for external interupts-exitings
    //
    PASSING_OVER_INTERRUPT_EVENTS,

    //
    // for external rdtsc/p exitings
    //
    PASSING_OVER_TSC_EVENTS,

    //
    // for external mov to hardware debug registers exitings
    //
    PASSING_OVER_MOV_TO_HW_DEBUG_REGS_EVENTS,

    //
    // for external mov to control registers exitings
    //
    PASSING_OVER_MOV_TO_CONTROL_REGS_EVENTS,

} PROTECTED_HV_RESOURCES_PASSING_OVERS;

/**
 * @brief Type of protected (multi-used) resources
 *
 */
typedef enum _PROTECTED_HV_RESOURCES_TYPE
{
    PROTECTED_HV_RESOURCES_EXCEPTION_BITMAP,

    PROTECTED_HV_RESOURCES_EXTERNAL_INTERRUPT_EXITING,

    PROTECTED_HV_RESOURCES_RDTSC_RDTSCP_EXITING,

    PROTECTED_HV_RESOURCES_MOV_TO_DEBUG_REGISTER_EXITING,

    PROTECTED_HV_RESOURCES_MOV_CONTROL_REGISTER_EXITING,

    PROTECTED_HV_RESOURCES_MOV_TO_CR3_EXITING,

} PROTECTED_HV_RESOURCES_TYPE;

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

/**
 * @brief Each command is like the following struct, it also used for
 * tracing works in user mode and sending it to the kernl mode
 * @details THIS IS NOT WHAT HYPERDBG SAVES FOR EVENTS IN KERNEL-MODE
 */
typedef struct _DEBUGGER_GENERAL_EVENT_DETAIL
{
    LIST_ENTRY
    CommandsEventList; // Linked-list of commands list (used for tracing purpose
                       // in user mode)

    time_t CreationTime; // Date of creating this event

    UINT32 CoreId; // determines the core index to apply this event to, if it's
                   // 0xffffffff means that we have to apply it to all cores

    UINT32 ProcessId; // determines the process id to apply this to
                      // only that 0xffffffff means that we have to
                      // apply it to all processes

    BOOLEAN IsEnabled;

    BOOLEAN EnableShortCircuiting; // indicates whether the short-circuiting event
                                   // is enabled or not for this event

    VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE EventStage; // reveals the calling stage of the event
    // (whether it's a all- pre- or post- event)

    BOOLEAN HasCustomOutput; // Shows whether this event has a custom output
                             // source or not

    UINT64
    OutputSourceTags
        [DebuggerOutputSourceMaximumRemoteSourceForSingleEvent]; // tags of
                                                                 // multiple
                                                                 // sources which
                                                                 // can be used to
                                                                 // send the event
                                                                 // results of
                                                                 // scripts to
                                                                 // remote sources

    UINT32 CountOfActions;

    UINT64              Tag; // is same as operation code
    VMM_EVENT_TYPE_ENUM EventType;

    DEBUGGER_EVENT_OPTIONS Options;

    PVOID CommandStringBuffer;

    UINT32 ConditionBufferSize;

} DEBUGGER_GENERAL_EVENT_DETAIL, *PDEBUGGER_GENERAL_EVENT_DETAIL;

/**
 * @brief Each event can have multiple actions
 * @details THIS STRUCTURE IS ONLY USED IN USER MODE
 * WE USE SEPARATE STRUCTURE FOR ACTIONS IN
 * KERNEL MODE
 */
typedef struct _DEBUGGER_GENERAL_ACTION
{
    UINT64                          EventTag;
    DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType;
    BOOLEAN                         ImmediateMessagePassing;
    UINT32                          PreAllocatedBuffer;

    UINT32 CustomCodeBufferSize;
    UINT32 ScriptBufferSize;
    UINT32 ScriptBufferPointer;

} DEBUGGER_GENERAL_ACTION, *PDEBUGGER_GENERAL_ACTION;

/**
 * @brief Status of register buffers
 *
 */
typedef struct _DEBUGGER_EVENT_AND_ACTION_RESULT
{
    BOOLEAN IsSuccessful;
    UINT32  Error; // If IsSuccessful was, FALSE

} DEBUGGER_EVENT_AND_ACTION_RESULT, *PDEBUGGER_EVENT_AND_ACTION_RESULT;

#define SIZEOF_REGISTER_EVENT sizeof(REGISTER_NOTIFY_BUFFER)
