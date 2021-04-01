/**
 * @file Definition.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header files for global definitions
 * @details This file contains definitions that are use in both user mode and
 * kernel mode Means that if you change the following files, structures or
 * enums, then these settings apply to both usermode and kernel mode
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				Message Tracing                 //
//////////////////////////////////////////////////

/**
 * @brief Default buffer count of packets for message tracing
 * @details number of packets storage
 */
#define MaximumPacketsCapacity 1000

/**
 * @brief Size of each packet
 * @details NOTE : REMEMBER TO CHANGE IT IN USER-MODE APP TOO
 * @warning we redefine it on ScriptEngineCommon.h change it on
 * that file too
 */
#define PacketChunkSize 3000

/**
 * @brief size of user-mode buffer
 * @details Because of Opeation code at the start of the
 * buffer + 1 for null-termminating
 *
 */
#define UsermodeBufferSize sizeof(UINT32) + PacketChunkSize + 1

/**
 * @brief size of buffer for serial
 * @details the maximum packet size for sending over serial
 * User-mode buffer size + Header Structure Size + Count Of End Buffer Bytes
 *
 */
#define MaxSerialPacketSize                               \
    UsermodeBufferSize + sizeof(DEBUGGER_REMOTE_PACKET) + \
        SERIAL_END_OF_BUFFER_CHARS_COUNT

/**
 * @brief Final storage size of message tracing
 *
 */
#define LogBufferSize \
    MaximumPacketsCapacity *(PacketChunkSize + sizeof(BUFFER_HEADER))

/**
 * @brief limitation of Windows DbgPrint message size
 * @details currently is not functional
 *
 */
#define DbgPrintLimitation 512

/**
 * @brief The seeds that user-mode codes use as the starter
 * of their events' tag
 *
 */
#define DebuggerEventTagStartSeed 0x1000000

/**
 * @brief The seeds that user-mode codes use as the starter
 * of their output source tag
 *
 */
#define DebuggerOutputSourceTagStartSeed 0x1

/**
 * @brief Determines how many sources a debugger can have for
 * a single event
 *
 */
#define DebuggerOutputSourceMaximumRemoteSourceForSingleEvent 0x5

//////////////////////////////////////////////////
//               Remote Connection              //
//////////////////////////////////////////////////

/**
 * @brief default port of HyperDbg for listening by
 * debuggee (server, guest)
 *
 */
#define DEFAULT_PORT "50000"

/**
 * @brief Packet size for TCP connections
 * @details Note that we might add something to the kernel buffers
 * that's why we add 0x100 to it
 */
#define COMMUNICATION_BUFFER_SIZE PacketChunkSize + 0x100

//////////////////////////////////////////////////
//            Breakpoint Backup                 //
//////////////////////////////////////////////////

/**
 * @brief maximum number of buffers to be allocated for a single
 * breakpoint
 */
#define MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE 50

//////////////////////////////////////////////////
//                   Installer                  //
//////////////////////////////////////////////////

/**
 * @brief maximum results that will be returned by !s* s*
 * command
 *
 */
#define MaximumSearchResults 0x1000

/**
 * @brief name of HyperDbg driver
 *
 */
#define DRIVER_NAME "hprdbghv"

//////////////////////////////////////////////////
//                 Name of OS                    //
//////////////////////////////////////////////////

/**
 * @brief maximum name for OS name buffer
 *
 */
#define MAXIMUM_CHARACTER_FOR_OS_NAME 256

//////////////////////////////////////////////////
//             Operation Codes                  //
//////////////////////////////////////////////////

/**
 * @brief If a operation use this bit in its Operation code,
 * then it means that the operation should be performed
 * mandatorily in debuggee and should not be sent to the debugger
 */
#define OPERATION_MANDATORY_DEBUGGEE_BIT (1 << 31)

/**
 * @brief Message logs id that comes from kernel-mode to
 * user-mode
 * @details Message area >= 0x5
 */
#define OPERATION_LOG_INFO_MESSAGE          0x1
#define OPERATION_LOG_WARNING_MESSAGE       0x2
#define OPERATION_LOG_ERROR_MESSAGE         0x3
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE 0x4
#define OPERATION_LOG_WITH_TAG              0x5

#define OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM \
    0x6 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_USER_INPUT     0x7 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_REGISTER_EVENT 0x8 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT \
    0x9 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_CLEAR_EVENTS 0xA | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED \
    0xB | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS \
    0xC | OPERATION_MANDATORY_DEBUGGEE_BIT

//////////////////////////////////////////////////
//				   Test Cases                   //
//////////////////////////////////////////////////

/**
 * @brief Test case number, perform all the tests
 */
#define DEBUGGER_TEST_ALL_COMMANDS 0x0

/**
 * @brief Test case number, test attaching and detaching to processes
 */
#define DEBUGGER_TEST_USER_MODE_INFINITE_LOOP_THREAD 0x1

//////////////////////////////////////////////////
//		Debugger Synchronization Objects        //
//////////////////////////////////////////////////

/**
 * @brief The processor is not important
 */
#define DEBUGGER_PROCESSOR_CORE_NOT_IMPORTANT 0xffffffff

/**
 * @brief Maximum Number of Event Handles
 */
#define DEBUGGER_MAXIMUM_SYNCRONIZATION_OBJECTS 0x40

/**
 * @brief An event to show whether the debugger is running
 * or not
 *
 */
#define DEBUGGER_SYNCRONIZATION_OBJECT_IS_DEBUGGER_RUNNING                 0x0
#define DEBUGGER_SYNCRONIZATION_OBJECT_STARTED_PACKET_RECEIVED             0x1
#define DEBUGGER_SYNCRONIZATION_OBJECT_PAUSED_DEBUGGEE_DETAILS             0x2
#define DEBUGGER_SYNCRONIZATION_OBJECT_CORE_SWITCHING_RESULT               0x3
#define DEBUGGER_SYNCRONIZATION_OBJECT_PROCESS_SWITCHING_RESULT            0x4
#define DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_RUNNING_RESULT               0x5
#define DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_FORMATS_RESULT               0x6
#define DEBUGGER_SYNCRONIZATION_OBJECT_DEBUGGEE_FINISHED_COMMAND_EXECUTION 0x7
#define DEBUGGER_SYNCRONIZATION_OBJECT_FLUSH_RESULT                        0x8
#define DEBUGGER_SYNCRONIZATION_OBJECT_REGISTER_EVENT                      0x9
#define DEBUGGER_SYNCRONIZATION_OBJECT_ADD_ACTION_TO_EVENT                 0xa
#define DEBUGGER_SYNCRONIZATION_OBJECT_MODIFY_AND_QUERY_EVENT              0xb
#define DEBUGGER_SYNCRONIZATION_OBJECT_READ_REGISTERS                      0xc
#define DEBUGGER_SYNCRONIZATION_OBJECT_BP                                  0xd
#define DEBUGGER_SYNCRONIZATION_OBJECT_LIST_OR_MODIFY_BREAKPOINTS          0xe
#define DEBUGGER_SYNCRONIZATION_OBJECT_READ_MEMORY                         0xf
#define DEBUGGER_SYNCRONIZATION_OBJECT_EDIT_MEMORY                         0x10

//////////////////////////////////////////////////
//            End of Buffer Detection           //
//////////////////////////////////////////////////
/**
 * @brief count of characters for serial end of buffer
 */
#define SERIAL_END_OF_BUFFER_CHARS_COUNT 0x4

/**
 * @brief characters of the buffer that we set at the end of
 * buffers for serial
 */
#define SERIAL_END_OF_BUFFER_CHAR_1 0x00
#define SERIAL_END_OF_BUFFER_CHAR_2 0x80
#define SERIAL_END_OF_BUFFER_CHAR_3 0xEE
#define SERIAL_END_OF_BUFFER_CHAR_4 0xFF

//////////////////////////////////////////////////
//              Processor Details               //
//////////////////////////////////////////////////

/**
 * @brief maximum instruction size in intel
 */
#define MAXIMUM_INSTR_SIZE 16

//////////////////////////////////////////////////
//            Callback Definitions              //
//////////////////////////////////////////////////

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

/**
 * @brief Integer gp registers (this structure is defined in
 * two places, make sure to change it in two places)
 *
 */
#ifndef GUEST_REGS_DEFINED
#    define GUEST_REGS_DEFINED

//
// DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
//
typedef struct GUEST_REGS
{
    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

    ULONG64 rax; // 0x00
    ULONG64 rcx; // 0x08
    ULONG64 rdx; // 0x10
    ULONG64 rbx; // 0x18
    ULONG64 rsp; // 0x20
    ULONG64 rbp; // 0x28
    ULONG64 rsi; // 0x30
    ULONG64 rdi; // 0x38
    ULONG64 r8;  // 0x40
    ULONG64 r9;  // 0x48
    ULONG64 r10; // 0x50
    ULONG64 r11; // 0x58
    ULONG64 r12; // 0x60
    ULONG64 r13; // 0x68
    ULONG64 r14; // 0x70
    ULONG64 r15; // 0x78

    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

} GUEST_REGS, *PGUEST_REGS;
#endif

/**
 * @brief struct for extra registers
 *
 */
typedef struct GUEST_EXTRA_REGISTERS
{
    USHORT CS;
    USHORT DS;
    USHORT FS;
    USHORT GS;
    USHORT ES;
    USHORT SS;
    UINT64 RFLAGS;
    UINT64 RIP;
} GUEST_EXTRA_REGISTERS, *PGUEST_EXTRA_REGISTERS;

/**
 * @brief RFLAGS in structure format
 *
 */
typedef union _RFLAGS
{
    struct
    {
        UINT64 CarryFlag : 1;
        UINT64 ReadAs1 : 1;
        UINT64 ParityFlag : 1;
        UINT64 Reserved1 : 1;
        UINT64 AuxiliaryCarryFlag : 1;
        UINT64 Reserved2 : 1;
        UINT64 ZeroFlag : 1;
        UINT64 SignFlag : 1;
        UINT64 TrapFlag : 1;
        UINT64 InterruptEnableFlag : 1;
        UINT64 DirectionFlag : 1;
        UINT64 OverflowFlag : 1;
        UINT64 IoPrivilegeLevel : 2;
        UINT64 NestedTaskFlag : 1;
        UINT64 Reserved3 : 1;
        UINT64 ResumeFlag : 1;
        UINT64 Virtual8086ModeFlag : 1;
        UINT64 AlignmentCheckFlag : 1;
        UINT64 VirtualInterruptFlag : 1;
        UINT64 VirtualInterruptPendingFlag : 1;
        UINT64 IdentificationFlag : 1;
    };

    UINT64 Value;
} RFLAGS, *PRFLAGS;

/**
 * @brief enum to show type of all HyperDbg events
 *
 */
typedef enum _DEBUGGER_EVENT_TYPE_ENUM
{

    HIDDEN_HOOK_READ_AND_WRITE,
    HIDDEN_HOOK_READ,
    HIDDEN_HOOK_WRITE,

    HIDDEN_HOOK_EXEC_DETOURS,
    HIDDEN_HOOK_EXEC_CC,

    SYSCALL_HOOK_EFER_SYSCALL,
    SYSCALL_HOOK_EFER_SYSRET,

    CPUID_INSTRUCTION_EXECUTION,

    RDMSR_INSTRUCTION_EXECUTION,
    WRMSR_INSTRUCTION_EXECUTION,

    IN_INSTRUCTION_EXECUTION,
    OUT_INSTRUCTION_EXECUTION,

    EXCEPTION_OCCURRED,
    EXTERNAL_INTERRUPT_OCCURRED,

    DEBUG_REGISTERS_ACCESSED,

    TSC_INSTRUCTION_EXECUTION,
    PMC_INSTRUCTION_EXECUTION,

    VMCALL_INSTRUCTION_EXECUTION,

} DEBUGGER_EVENT_TYPE_ENUM;

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
 * @brief Each command is like the following struct, it also used for
 * tracing works in user mode and sending it to the kernl mode
 * @details THIS IS NOT WHAT HYPERDBG SAVES FOR EVENTS IN KERNEL MODE
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

    UINT64                   Tag; // is same as operation code
    DEBUGGER_EVENT_TYPE_ENUM EventType;

    UINT64 OptionalParam1;
    UINT64 OptionalParam2;
    UINT64 OptionalParam3;
    UINT64 OptionalParam4;

    PVOID CommandStringBuffer;

    UINT32 ConditionBufferSize;

} DEBUGGER_GENERAL_EVENT_DETAIL, *PDEBUGGER_GENERAL_EVENT_DETAIL;

/**
 * @brief Each event can have mulitple actions
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
typedef struct _DEBUGGER_EVENT_AND_ACTION_REG_BUFFER
{
    BOOLEAN IsSuccessful;
    UINT32  Error; // If IsSuccessful was, FALSE

} DEBUGGER_EVENT_AND_ACTION_REG_BUFFER, *PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER;

//////////////////////////////////////////////////
//            Debuggee Communication            //
//////////////////////////////////////////////////

#define INDICATOR_OF_HYPERDBG_PACKER \
    0x4859504552444247 // HYPERDBG = 0x4859504552444247

/**
 * @brief enum for reasons why debuggee is paused
 *
 */
typedef enum _DEBUGGEE_PAUSING_REASON
{

    DEBUGGEE_PAUSING_REASON_NOT_PAUSED = 0,
    DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM,
    DEBUGGEE_PAUSING_REASON_REQUEST_FROM_DEBUGGER,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_CORE_SWITCHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,

} DEBUGGEE_PAUSING_REASON;

/**
 * @brief enum for diffrent packet types in HyperDbg packets
 *
 */
typedef enum _DEBUGGER_REMOTE_PACKET_TYPE
{

    //
    // Debugger to debuggee (vmx-root)
    //
    DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT = 1,

    //
    // Debugger to debuggee (user-mode)
    //
    DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE,

    //
    // Debuggee to debugger
    //
    DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER

} DEBUGGER_REMOTE_PACKET_TYPE;

/**
 * @brief enum for requested action for HyperDbg packet
 *
 */
typedef enum _DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION
{

    //
    // Debugger to debuggee (user-mode execution)
    //
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_PAUSE = 1,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_DO_NOT_READ_ANY_PACKET,

    //
    // Debugger to debuggee (vmx-root mode execution)
    //
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_STEP,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CONTINUE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CLOSE_AND_UNLOAD_DEBUGGEE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_CORE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_FLUSH_BUFFERS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_PROCESS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_RUN_SCRIPT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_USER_INPUT_BUFFER,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_REGISTER_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_ADD_ACTION_TO_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_AND_MODIFY_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_REGISTERS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_EDIT_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_BP,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_LIST_OR_MODIFY_BREAKPOINTS,

    //
    // Debuggee to debugger
    //
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION = 0,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_LOGGING_MECHANISM,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_CORE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_PROCESS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_RUNNING_SCRIPT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FORMATS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FLUSH,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_REGISTERS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_EDITING_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_BP,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_LIST_OR_MODIFY_BREAKPOINTS,

} DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION;

//////////////////////////////////////////////////
//                  Debugger                    //
//////////////////////////////////////////////////

/* ==============================================================================================
 */

#define SIZEOF_REGISTER_EVENT sizeof(REGISTER_NOTIFY_BUFFER)

typedef enum _NOTIFY_TYPE
{
    IRP_BASED,
    EVENT_BASED
} NOTIFY_TYPE;

typedef struct _REGISTER_NOTIFY_BUFFER
{
    NOTIFY_TYPE Type;
    HANDLE      hEvent;

} REGISTER_NOTIFY_BUFFER, *PREGISTER_NOTIFY_BUFFER;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_MODIFY_EVENTS sizeof(DEBUGGER_MODIFY_EVENTS)

/* Constants */
#define DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG 0xffffffffffffffff

/**
 * @brief different types of modifing events request (enable/disable/clear)
 *
 */
typedef enum _DEBUGGER_MODIFY_EVENTS_TYPE
{
    DEBUGGER_MODIFY_EVENTS_QUERY_STATE,
    DEBUGGER_MODIFY_EVENTS_ENABLE,
    DEBUGGER_MODIFY_EVENTS_DISABLE,
    DEBUGGER_MODIFY_EVENTS_CLEAR
} DEBUGGER_MODIFY_EVENTS_TYPE;

/**
 * @brief request for modifying events (enable/disable/clear)
 *
 */
typedef struct _DEBUGGER_MODIFY_EVENTS
{
    UINT64 Tag;          // Tag of the target event that we want to modify
    UINT64 KernelStatus; // Kerenl put the status in this field
    DEBUGGER_MODIFY_EVENTS_TYPE
    TypeOfAction;      // Determines what's the action (enable | disable | clear)
    BOOLEAN IsEnabled; // Determines what's the action (enable | disable | clear)

} DEBUGGER_MODIFY_EVENTS, *PDEBUGGER_MODIFY_EVENTS;

/*
==============================================================================================
 */

#define SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS \
    sizeof(DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS)

/**
 * @brief request for !pte command
 *
 */
typedef struct _DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS
{
    UINT64 VirtualAddress;

    UINT64 Pml4eVirtualAddress;
    UINT64 Pml4eValue;

    UINT64 PdpteVirtualAddress;
    UINT64 PdpteValue;

    UINT64 PdeVirtualAddress;
    UINT64 PdeValue;

    UINT64 PteVirtualAddress;
    UINT64 PteValue;

    UINT32 KernelStatus;

} DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS,
    *PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS \
    sizeof(DEBUGGER_VA2PA_AND_PA2VA_COMMANDS)

/**
 * @brief requests for !va2pa and !pa2va commands
 *
 */
typedef struct _DEBUGGER_VA2PA_AND_PA2VA_COMMANDS
{
    UINT64  VirtualAddress;
    UINT64  PhysicalAddress;
    UINT32  ProcessId;
    BOOLEAN IsVirtual2Physical;

} DEBUGGER_VA2PA_AND_PA2VA_COMMANDS, *PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_READ_MEMORY sizeof(DEBUGGER_READ_MEMORY)

/**
 * @brief different types of reading memory
 *
 */
typedef enum _DEBUGGER_READ_READING_TYPE
{
    READ_FROM_KERNEL,
    READ_FROM_VMX_ROOT
} DEBUGGER_READ_READING_TYPE;

/**
 * @brief different type of addresses
 *
 */
typedef enum _DEBUGGER_READ_MEMORY_TYPE
{
    DEBUGGER_READ_PHYSICAL_ADDRESS,
    DEBUGGER_READ_VIRTUAL_ADDRESS
} DEBUGGER_READ_MEMORY_TYPE;

/**
 * @brief the way that debugger should show
 * the details of memory or disassemble them
 *
 */
typedef enum _DEBUGGER_SHOW_MEMORY_STYLE
{
    DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
    DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
    DEBUGGER_SHOW_COMMAND_DB,
    DEBUGGER_SHOW_COMMAND_DC,
    DEBUGGER_SHOW_COMMAND_DQ,
    DEBUGGER_SHOW_COMMAND_DD
} DEBUGGER_SHOW_MEMORY_STYLE;

/**
 * @brief request for reading virtual and physical memory
 *
 */
typedef struct _DEBUGGER_READ_MEMORY
{
    UINT32                     Pid; // Read from cr3 of what process
    UINT64                     Address;
    UINT32                     Size;
    DEBUGGER_READ_MEMORY_TYPE  MemoryType;
    DEBUGGER_READ_READING_TYPE ReadingType;
    DEBUGGER_SHOW_MEMORY_STYLE Style;        // not used in local debugging
    UINT32                     ReturnLength; // not used in local debugging
    UINT32                     KernelStatus; // not used in local debugging

} DEBUGGER_READ_MEMORY, *PDEBUGGER_READ_MEMORY;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_STEPPINGS sizeof(DEBUGGER_STEPPINGS)

/**
 * @brief Actions to debugging thread's
 *
 */
typedef enum _DEBUGGER_STEPPINGS_ACTIONS_ENUM
{
    STEPPINGS_ACTION_STEP_INTO,
    STEPPINGS_ACTION_STEP_OUT,
    STEPPINGS_ACTION_CONTINUE

} DEBUGGER_STEPPINGS_ACTIONS_ENUM;

/**
 * @brief request for step-in and step-out
 *
 */
typedef struct _DEBUGGER_STEPPINGS
{
    UINT32                          KernelStatus;
    UINT32                          ProcessId;
    UINT32                          ThreadId;
    DEBUGGER_STEPPINGS_ACTIONS_ENUM SteppingAction;

} DEBUGGER_STEPPINGS, *PDEBUGGER_STEPPINGS;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS \
    sizeof(DEBUGGER_FLUSH_LOGGING_BUFFERS)

/**
 * @brief request for flushing buffers
 *
 */
typedef struct _DEBUGGER_FLUSH_LOGGING_BUFFERS
{
    UINT32 KernelStatus;
    UINT32 CountOfMessagesThatSetAsReadFromVmxRoot;
    UINT32 CountOfMessagesThatSetAsReadFromVmxNonRoot;

} DEBUGGER_FLUSH_LOGGING_BUFFERS, *PDEBUGGER_FLUSH_LOGGING_BUFFERS;
/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL \
    sizeof(DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL)

/**
 * @brief request for send a signal that command execution finished
 *
 */
typedef struct _DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL
{
    UINT32 KernelStatus;

} DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL,
    *PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER \
    sizeof(DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER)

/**
 * @brief request for send general packets from debuggee to debugger
 *
 */
typedef struct _DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER
{
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction;
    UINT32                                  LengthOfBuffer;
    BOOLEAN                                 PauseDebuggeeWhenSent;
    UINT32                                  KernelResult;

    //
    // The buffer for the general packet is here
    //

} DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER,
    *PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER \
    sizeof(DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER)

/**
 * @brief request for send a send user-mode message to debugger
 *
 */
typedef struct _DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER
{
    UINT32 KernelStatus;
    UINT32 Length;

    //
    // Here is the messages
    //

} DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER,
    *PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR \
    sizeof(DEBUGGER_READ_AND_WRITE_ON_MSR)
#define DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES 0xffffffff

/**
 * @brief different types of actions on MSRs
 *
 */
typedef enum _DEBUGGER_MSR_ACTION_TYPE
{
    DEBUGGER_MSR_READ,
    DEBUGGER_MSR_WRITE
} DEBUGGER_MSR_ACTION_TYPE;

/**
 * @brief request to read or write on MSRs
 *
 */
typedef struct _DEBUGGER_READ_AND_WRITE_ON_MSR
{
    UINT64 Msr;        // It's actually a 32-Bit value but let's not mess with a register
    UINT32 CoreNumber; // specifies the core to execute wrmsr or read the msr
                       // (DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES mean all
                       // the cores)
    DEBUGGER_MSR_ACTION_TYPE
    ActionType; // Detects whether user needs wrmsr or rdmsr
    UINT64 Value;

} DEBUGGER_READ_AND_WRITE_ON_MSR, *PDEBUGGER_READ_AND_WRITE_ON_MSR;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_EDIT_MEMORY sizeof(DEBUGGER_EDIT_MEMORY)

/**
 * @brief different type of addresses for editing memory
 *
 */
typedef enum _DEBUGGER_EDIT_MEMORY_TYPE
{
    EDIT_PHYSICAL_MEMORY,
    EDIT_VIRTUAL_MEMORY
} DEBUGGER_EDIT_MEMORY_TYPE;

/**
 * @brief size of editing memory
 *
 */
typedef enum _DEBUGGER_EDIT_MEMORY_BYTE_SIZE
{
    EDIT_BYTE,
    EDIT_DWORD,
    EDIT_QWORD
} DEBUGGER_EDIT_MEMORY_BYTE_SIZE;

/**
 * @brief request for edit virtual and physical memory
 *
 */
typedef struct _DEBUGGER_EDIT_MEMORY
{
    UINT32                         Result;     // Result from kernel
    UINT64                         Address;    // Target adddress to modify
    UINT32                         ProcessId;  // specifies the process id
    DEBUGGER_EDIT_MEMORY_TYPE      MemoryType; // Type of memory
    DEBUGGER_EDIT_MEMORY_BYTE_SIZE ByteSize;   // Modification size
    UINT32                         CountOf64Chunks;
    UINT32                         FinalStructureSize;
    UINT32                         KernelStatus; // not used in local debugging

} DEBUGGER_EDIT_MEMORY, *PDEBUGGER_EDIT_MEMORY;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_SEARCH_MEMORY sizeof(DEBUGGER_SEARCH_MEMORY)

/**
 * @brief different types of address for searching on memory
 *
 */
typedef enum _DEBUGGER_SEARCH_MEMORY_TYPE
{
    SEARCH_PHYSICAL_MEMORY,
    SEARCH_VIRTUAL_MEMORY
} DEBUGGER_SEARCH_MEMORY_TYPE;

/**
 * @brief different sizes on searching memory
 *
 */
typedef enum _DEBUGGER_SEARCH_MEMORY_BYTE_SIZE
{
    SEARCH_BYTE,
    SEARCH_DWORD,
    SEARCH_QWORD
} DEBUGGER_SEARCH_MEMORY_BYTE_SIZE;

/**
 * @brief request for searching memory
 *
 */
typedef struct _DEBUGGER_SEARCH_MEMORY
{
    UINT64                           Address;    // Target adddress to start searching
    UINT64                           Length;     // Length of bytes to search
    UINT32                           ProcessId;  // specifies the process id
    DEBUGGER_SEARCH_MEMORY_TYPE      MemoryType; // Type of memory
    DEBUGGER_SEARCH_MEMORY_BYTE_SIZE ByteSize;   // Modification size
    UINT32                           CountOf64Chunks;
    UINT32                           FinalStructureSize;

} DEBUGGER_SEARCH_MEMORY, *PDEBUGGER_SEARCH_MEMORY;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE \
    sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE)

/**
 * @brief request for enable or disable transparent-mode
 *
 */
typedef struct _DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE
{
    BOOLEAN IsHide;

    UINT64 CpuidAverage;
    UINT64 CpuidStandardDeviation;
    UINT64 CpuidMedian;

    UINT64 RdtscAverage;
    UINT64 RdtscStandardDeviation;
    UINT64 RdtscMedian;

    BOOLEAN TrueIfProcessIdAndFalseIfProcessName;
    UINT32  ProcId;
    UINT32  LengthOfProcessName; // in the case of !hide name xxx, this parameter
                                 // shows the length of xxx

    UINT64 KernelStatus; /* DEBUGEER_OPERATION_WAS_SUCCESSFULL ,
                          DEBUGEER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER
                          */

} DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE,
    *PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_PREPARE_DEBUGGEE sizeof(DEBUGGER_PREPARE_DEBUGGEE)

/**
 * @brief request to make this computer to a debuggee
 *
 */
typedef struct _DEBUGGER_PREPARE_DEBUGGEE
{
    UINT32 PortAddress;
    UINT32 Baudrate;
    UINT32 Result; // Result from the kernel
    CHAR   OsName[MAXIMUM_CHARACTER_FOR_OS_NAME];

} DEBUGGER_PREPARE_DEBUGGEE, *PDEBUGGER_PREPARE_DEBUGGEE;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED \
    sizeof(DEBUGGER_PAUSE_PACKET_RECEIVED)

/**
 * @brief request to pause and halt the system
 *
 */
typedef struct _DEBUGGER_PAUSE_PACKET_RECEIVED
{
    UINT32 Result; // Result from kernel

} DEBUGGER_PAUSE_PACKET_RECEIVED, *PDEBUGGER_PAUSE_PACKET_RECEIVED;

/* ==============================================================================================
 */
#define SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS \
    sizeof(DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS)

/**
 * @brief request for attaching user-mode process
 *
 */
typedef struct _DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS
{
    BOOLEAN IsAttach;
    UINT64  Result;
    UINT32  ProcessId;
    UINT64  ThreadId;

} DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,
    *PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS;

/* ==============================================================================================
 */

/**
 * @brief Apply the event to all the cores
 *
 */
#define DEBUGGER_DEBUGGEE_IS_RUNNING_NO_CORE 0xffffffff

/**
 * @brief Apply the event to all the cores
 *
 */
#define DEBUGGER_EVENT_APPLY_TO_ALL_CORES 0xffffffff

/**
 * @brief Apply the event to all the processes
 *
 */
#define DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES 0xffffffff

/**
 * @brief Apply to all Model Specific Registers
 *
 */
#define DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS 0xffffffff

/**
 * @brief Apply to all first 32 exceptions
 *
 */
#define DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES 0xffffffff

/**
 * @brief Apply to all syscalls and sysrets
 *
 */
#define DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS 0xffffffff

/**
 * @brief Apply to all I/O ports
 *
 */
#define DEBUGGER_EVENT_ALL_IO_PORTS 0xffffffff

/* ==============================================================================================
 */

/**
 * @brief Used for run the script
 *
 */
typedef struct _DEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION
{
    UINT64 ScriptBuffer;
    UINT32 ScriptLength;
    UINT32 ScriptPointer;
    UINT32 OptionalRequestedBufferSize;

} DEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION,
    *PDEBUGGER_EVENT_ACTION_RUN_SCRIPT_CONFIGURATION;

/**
 * @brief used in the case of requesting a "request buffer"
 *
 */
typedef struct _DEBUGGER_EVENT_REQUEST_BUFFER
{
    BOOLEAN EnabledRequestBuffer;
    UINT32  RequestBufferSize;
    UINT64  RequstBufferAddress;

} DEBUGGER_EVENT_REQUEST_BUFFER, *PDEBUGGER_EVENT_REQUEST_BUFFER;

/**
 * @brief used in the case of custom code requests to the debugger
 *
 */
typedef struct _DEBUGGER_EVENT_REQUEST_CUSTOM_CODE
{
    UINT32 CustomCodeBufferSize;
    PVOID  CustomCodeBufferAddress;
    UINT32 OptionalRequestedBufferSize;

} DEBUGGER_EVENT_REQUEST_CUSTOM_CODE, *PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE;

/* ==============================================================================================
 */

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

/**
 * @brief The structure of remote packets in HyperDbg
 *
 */
typedef struct _DEBUGGER_REMOTE_PACKET
{
    BYTE                                    Checksum;
    UINT64                                  Indicator; /* Shows the type of the packet, whether it's a GDB packet
                       or a HyperDbg packet */
    DEBUGGER_REMOTE_PACKET_TYPE             TypeOfThePacket;
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedActionOfThePacket;

} DEBUGGER_REMOTE_PACKET, *PDEBUGGER_REMOTE_PACKET;

/**
 * @brief The structure of detail of a triggered event in HyperDbg
 *
 */
typedef struct _DEBUGGER_TRIGGERED_EVENT_DETAILS
{
    UINT64 Tag; /* in breakpoints Tag is breakpoint id, not event tag */
    PVOID  Context;

} DEBUGGER_TRIGGERED_EVENT_DETAILS, *PDEBUGGER_TRIGGERED_EVENT_DETAILS;

/**
 * @brief The structure of pausing packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_PAUSED_PACKET
{
    UINT64 Rip;
    //
    // if true shows that the address should be interpreted in 32-bit mode
    //
    BOOLEAN                 Is32BitAddress;
    DEBUGGEE_PAUSING_REASON PausingReason;
    ULONG                   CurrentCore;
    UINT64                  EventTag;
    RFLAGS                  Rflags;
    BYTE                    InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];

} DEBUGGEE_PAUSED_PACKET, *PDEBUGGEE_PAUSED_PACKET;

/**
 * @brief The structure of message packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_MESSAGE_PACKET
{
    UINT32 OperationCode;
    CHAR   Message[PacketChunkSize];

} DEBUGGEE_MESSAGE_PACKET, *PDEBUGGEE_MESSAGE_PACKET;

/**
 * @brief The structure of changing core packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_CHANGE_CORE_PACKET
{
    UINT32 NewCore;
    UINT32 Result;

} DEBUGGEE_CHANGE_CORE_PACKET, *PDEBUGGEE_CHANGE_CORE_PACKET;

/**
 * @brief The structure of changing process packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_CHANGE_PROCESS_PACKET
{
    BOOLEAN GetRemotePid;
    UINT32  ProcessId;
    UINT32  Result;

} DEBUGGEE_CHANGE_PROCESS_PACKET, *PDEBUGGEE_CHANGE_PROCESS_PACKET;

/**
 * @brief stepping types
 *
 */
typedef enum _DEBUGGER_REMOTE_STEPPING_REQUEST
{

    DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER,
    DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN,
    DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN_INSTRUMENT,

} DEBUGGER_REMOTE_STEPPING_REQUEST;

/**
 * @brief The structure of stepping packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_STEP_PACKET
{
    DEBUGGER_REMOTE_STEPPING_REQUEST StepType;

    //
    // Only in the case of call instructions
    // the 'p' command
    //
    BOOLEAN IsCurrentInstructionACall;
    UINT32  CallLength;

} DEBUGGEE_STEP_PACKET, *PDEBUGGEE_STEP_PACKET;

/**
 * @brief The structure of .formats result packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_FORMATS_PACKET
{
    UINT64 Value;
    UINT32 Result;

} DEBUGGEE_FORMATS_PACKET, *PDEBUGGEE_FORMATS_PACKET;

/**
 * @brief The constant to apply to all cores for bp command
 *
 */
#define DEBUGGEE_BP_APPLY_TO_ALL_CORES 0xffffffff

/**
 * @brief The constant to apply to all processes for bp command
 *
 */
#define DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES 0xffffffff

/**
 * @brief The constant to apply to all threads for bp command
 *
 */
#define DEBUGGEE_BP_APPLY_TO_ALL_THREADS 0xffffffff

/**
 * @brief The structure of bp command packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_BP_PACKET
{
    UINT64 Address;
    UINT32 Pid;
    UINT32 Tid;
    UINT32 Core;
    UINT32 Result;

} DEBUGGEE_BP_PACKET, *PDEBUGGEE_BP_PACKET;

/**
 * @brief The structure of storing breakpoints
 *
 */
typedef struct _DEBUGGEE_BP_DESCRIPTOR
{
    UINT64     BreakpointId;
    LIST_ENTRY BreakpointsList;
    BOOLEAN    Enabled;
    UINT64     Address;
    UINT64     PhysAddress;
    UINT32     Pid;
    UINT32     Tid;
    UINT32     Core;
    UINT16     InstructionLength;
    BYTE       PreviousByte;
    BOOLEAN    SetRflagsIFBitOnMtf;
    BOOLEAN    AvoidReApplyBreakpoint;

} DEBUGGEE_BP_DESCRIPTOR, *PDEBUGGEE_BP_DESCRIPTOR;

/**
 * @brief breakpoint modification types
 *
 */
typedef enum _DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST
{

    DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_LIST_BREAKPOINTS,
    DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_ENABLE,
    DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_DISABLE,
    DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_CLEAR,

} DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST;

/**
 * @brief The structure of breakpoint modification requests packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_BP_LIST_OR_MODIFY_PACKET
{
    UINT64                                   BreakpointId;
    DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST Request;
    UINT32                                   Result;

} DEBUGGEE_BP_LIST_OR_MODIFY_PACKET, *PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET;

/**
 * @brief Whether a jump is taken or not tak
 *
 */
typedef enum _DEBUGGER_CONDITIONAL_JUMP_STATUS
{

    DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR = 0,
    DEBUGGER_CONDITIONAL_JUMP_STATUS_NOT_CONDITIONAL_JUMP,
    DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN,
    DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN,

} DEBUGGER_CONDITIONAL_JUMP_STATUS;

/**
 * @brief The structure of script packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_SCRIPT_PACKET
{
    UINT32  ScriptBufferSize;
    UINT32  ScriptBufferPointer;
    BOOLEAN IsFormat;
    UINT32  Result;

    //
    // The script buffer is here
    //

} DEBUGGEE_SCRIPT_PACKET, *PDEBUGGEE_SCRIPT_PACKET;

/**
 * @brief for reading all regisers in r command.
 *
 */
#define DEBUGGEE_SHOW_ALL_REGISTERS 0xffffffff

/**
 * @brief Register Descriptor Structure to use in r command.
 *
 */
typedef struct _DEBUGGEE_REGISTER_READ_DESCRIPTION
{
    UINT32 RegisterID; // the number is from REGS_ENUM
    UINT64 Value;
    UINT32 KernelStatus;

} DEBUGGEE_REGISTER_READ_DESCRIPTION, *PDEBUGGEE_REGISTER_READ_DESCRIPTION;

/**
 * @brief The structure of user-input packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_USER_INPUT_PACKET
{
    UINT32 CommandLen;
    UINT32 Result;

    //
    // The user's input is here
    //

} DEBUGGEE_USER_INPUT_PACKET, *PDEBUGGEE_USER_INPUT_PACKET;

/**
 * @brief The structure of user-input packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET
{
    UINT32 Length;

    //
    // The buffer for event and action is here
    //

} DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET,
    *PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET;

//////////////////////////////////////////////////
//		    	Debugger Success Codes          //
//////////////////////////////////////////////////

/**
 * @brief General value to indicate that the operation or
 * request was successful
 *
 */
#define DEBUGEER_OPERATION_WAS_SUCCESSFULL 0xFFFFFFFF

//////////////////////////////////////////////////
//		    	Debugger Error Codes            //
//////////////////////////////////////////////////

/**
 * @brief error, the tag not exist
 *
 */
#define DEBUGEER_ERROR_TAG_NOT_EXISTS 0xc0000000

/**
 * @brief error, invalid type of action
 *
 */
#define DEBUGEER_ERROR_INVALID_ACTION_TYPE 0xc0000001

/**
 * @brief error, the action buffer size is invalid
 *
 */
#define DEBUGEER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO 0xc0000002

/**
 * @brief error, the event type is unknown
 *
 */
#define DEBUGEER_ERROR_EVENT_TYPE_IS_INVALID 0xc0000003

/**
 * @brief error, enable to create event
 *
 */
#define DEBUGEER_ERROR_UNABLE_TO_CREATE_EVENT 0xc0000004

/**
 * @brief error, invalid address specified for debugger
 *
 */
#define DEBUGEER_ERROR_INVALID_ADDRESS 0xc0000005

/**
 * @brief error, the core id is invalid
 *
 */
#define DEBUGEER_ERROR_INVALID_CORE_ID 0xc0000006

/**
 * @brief error, the index is greater than 32 in !exception command
 *
 */
#define DEBUGEER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES 0xc0000007

/**
 * @brief error, the index for !interrupt command is not between 32 to 256
 *
 */
#define DEBUGEER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID 0xc0000008

/**
 * @brief error, unable to hide the debugger and enter to transparent-mode
 *
 */
#define DEBUGEER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER 0xc0000009

/**
 * @brief error, the debugger is already in transparent-mode
 *
 */
#define DEBUGEER_ERROR_DEBUGGER_ALREADY_UHIDE 0xc000000a

/**
 * @brief error, invalid parameters in !e* e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER 0xc000000b

/**
 * @brief error, an invalid address is specified based on current cr3
 * in !e* or e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS \
    0xc000000c

/**
 * @brief error, an invalid address is specified based on anotehr process's cr3
 * in !e* or e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS \
    0xc000000d

/**
 * @brief error, invalid tag for 'events' command (tag id is unknown for kernel)
 *
 */
#define DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TAG 0xc000000e

/**
 * @brief error, type of action (enable/disable/clear) is wrong
 *
 */
#define DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TYPE_OF_ACTION 0xc000000f

/**
 * @brief error, invalid parameters steppings actions
 *
 */
#define DEBUGGER_ERROR_STEPPING_INVALID_PARAMETER 0xc0000010

/**
 * @brief error, thread is invalid (not found) or disabled in
 * stepping (step-in & step-out) requests
 *
 */
#define DEBUGGER_ERROR_STEPPINGS_EITHER_THREAD_NOT_FOUND_OR_DISABLED 0xc0000011

/**
 * @brief error, baud rate is invalid
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_BAUDRATE 0xc0000012

/**
 * @brief error, serial port address is invalid
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT 0xc0000013

/**
 * @brief error, invalid core selected in changing core in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_CORE_IN_REMOTE_DEBUGGE \
    0xc0000014

/**
 * @brief error, invalid process selected in changing process in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_UNABLE_TO_SWITCH_TO_NEW_PROCESS \
    0xc0000015

/**
 * @brief error, unable to run script in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_TO_RUN_SCRIPT 0xc0000016

/**
 * @brief error, invalid register number
 *
 */
#define DEBUGGER_ERROR_INVALID_REGISTER_NUMBER 0xc0000017

/**
 * @brief error, maximum pools were used without continueing debuggee
 *
 */
#define DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_WITHOUT_CONTINUE 0xc0000018

/**
 * @brief error, breakpoint already exists on the target address
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_EXISTS_ON_THE_ADDRESS 0xc0000019

/**
 * @brief error, breakpoint id not found
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND 0xc000001a

/**
 * @brief error, breakpoint already disabled
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_DISABLED 0xc000001b

/**
 * @brief error, breakpoint already enabled
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_ENABLED 0xc000001c

/**
 * @brief error, memory type is invalid
 *
 */
#define DEBUGGER_ERROR_MEMORY_TYPE_INVALID 0xc000001d

//
// WHEN YOU ADD ANYTHING TO THIS LIST OF ERRORS, THEN
// MAKE SURE TO ADD AN ERROR MESSAGE TO ShowErrorMessage(UINT32 Error)
// FUNCTION
//

//////////////////////////////////////////////////
//                   IOCTLs                     //
//////////////////////////////////////////////////

/**
 * @brief ioctl, register a new event
 *
 */
#define IOCTL_REGISTER_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, irp pending mechanism for reading from message tracing buffers
 *
 */
#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to terminate vmx and exit form debugger
 *
 */
#define IOCTL_TERMINATE_VMX \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read memory
 *
 */
#define IOCTL_DEBUGGER_READ_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read or write on a speical MSR
 *
 */
#define IOCTL_DEBUGGER_READ_OR_WRITE_MSR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read page table entries
 *
 */
#define IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, register an event
 *
 */
#define IOCTL_DEBUGGER_REGISTER_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, add action to event
 *
 */
#define IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to enable or disable transparent-mode
 *
 */
#define IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, for !va2pa and !pa2va commands
 *
 */
#define IOCTL_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to edit virtual and physical memory
 *
 */
#define IOCTL_DEBUGGER_EDIT_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80a, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to search virtual and physical memory
 *
 */
#define IOCTL_DEBUGGER_SEARCH_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80b, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to modify an event (enable/disable/clear)
 *
 */
#define IOCTL_DEBUGGER_MODIFY_EVENTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80c, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, flush the kernel buffers
 *
 */
#define IOCTL_DEBUGGER_FLUSH_LOGGING_BUFFERS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80d, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, flush the kernel buffers
 *
 */
#define IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80e, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, steppings (step-in & step-out)
 *
 */
#define IOCTL_DEBUGGER_STEPPINGS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80f, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, print states (Deprecated)
 *
 *
 */
#define IOCTL_DEBUGGER_PRINT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, prepare debuggee
 *
 */
#define IOCTL_PREPARE_DEBUGGEE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, pause and halt the system
 *
 */
#define IOCTL_PAUSE_PACKET_RECEIVED \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x812, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send a signal that execution of command finished
 *
 */
#define IOCTL_SEND_SIGNAL_EXECUTION_IN_DEBUGGEE_FINISHED \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x813, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send user-mode messages to the debugger
 *
 */
#define IOCTL_SEND_USERMODE_MESSAGES_TO_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x814, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send general buffer from debuggee to debugger
 *
 */
#define IOCTL_SEND_GENERAL_BUFFER_FROM_DEBUGGEE_TO_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x815, METHOD_BUFFERED, FILE_ANY_ACCESS)
