/**
 * @file Datatypes.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK data type definitions
 * @details This file contains definitions of structures, enums, etc.
 * used in HyperDbg
 * @version 0.2
 * @date 2022-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            Callback Definitions              //
//////////////////////////////////////////////////

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

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

    UINT64 rax; // 0x00
    UINT64 rcx; // 0x08
    UINT64 rdx; // 0x10
    UINT64 rbx; // 0x18
    UINT64 rsp; // 0x20
    UINT64 rbp; // 0x28
    UINT64 rsi; // 0x30
    UINT64 rdi; // 0x38
    UINT64 r8;  // 0x40
    UINT64 r9;  // 0x48
    UINT64 r10; // 0x50
    UINT64 r11; // 0x58
    UINT64 r12; // 0x60
    UINT64 r13; // 0x68
    UINT64 r14; // 0x70
    UINT64 r15; // 0x78

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
    UINT16 CS;
    UINT16 DS;
    UINT16 FS;
    UINT16 GS;
    UINT16 ES;
    UINT16 SS;
    UINT64 RFLAGS;
    UINT64 RIP;
} GUEST_EXTRA_REGISTERS, *PGUEST_EXTRA_REGISTERS;

//////////////////////////////////////////////////
//              Symbols Details                 //
//////////////////////////////////////////////////

/**
 * @brief structures for sending and saving details
 * about each module and symbols details
 *
 */
typedef struct _MODULE_SYMBOL_DETAIL
{
    BOOLEAN IsSymbolDetailsFound; // TRUE if the details of symbols found, FALSE if not found
    BOOLEAN IsLocalSymbolPath;    // TRUE if the ModuleSymbolPath is a real path
                                  // and FALSE if ModuleSymbolPath is just a module name
    BOOLEAN IsSymbolPDBAvaliable; // TRUE if the module's pdb is avilable(if exists in the sympath)
    BOOLEAN IsUserMode;           // TRUE if the module is a user-mode module
    UINT64  BaseAddress;
    char    FilePath[MAX_PATH];
    char    ModuleSymbolPath[MAX_PATH];
    char    ModuleSymbolGuidAndAge[MAXIMUM_GUID_AND_AGE_SIZE];

} MODULE_SYMBOL_DETAIL, *PMODULE_SYMBOL_DETAIL;

typedef struct _USERMODE_LOADED_MODULE_SYMBOLS
{
    UINT64  BaseAddress;
    UINT64  Entrypoint;
    wchar_t FilePath[MAX_PATH];

} USERMODE_LOADED_MODULE_SYMBOLS, *PUSERMODE_LOADED_MODULE_SYMBOLS;

typedef struct _USERMODE_LOADED_MODULE_DETAILS
{
    UINT32  ProcessId;
    BOOLEAN OnlyCountModules;
    UINT32  ModulesCount;
    UINT32  Result;

    //
    // Here is a list of USERMODE_LOADED_MODULE_SYMBOLS (appended)
    //

} USERMODE_LOADED_MODULE_DETAILS, *PUSERMODE_LOADED_MODULE_DETAILS;

/**
 * @brief Callback type that should be used to add
 * list of Addresses to ObjectNames
 *
 */
typedef VOID (*SymbolMapCallback)(UINT64 Address, char * ModuleName, char * ObjectName, unsigned int ObjectSize);

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

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
 * @brief Type of handling !syscall or !sysret
 *
 */
typedef enum _DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE
{
    DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY = 0,
    DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD      = 1,

} DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE;

//////////////////////////////////////////////////
//            Debuggee Communication            //
//////////////////////////////////////////////////

/**
 * @brief enum for reasons why debuggee is paused
 *
 */
typedef enum _DEBUGGEE_PAUSING_REASON
{

    //
    // For both kernel & user debugger
    //
    DEBUGGEE_PAUSING_REASON_NOT_PAUSED = 0,
    DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM,
    DEBUGGEE_PAUSING_REASON_REQUEST_FROM_DEBUGGER,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_CORE_SWITCHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_THREAD_SWITCHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_ENTRY_POINT_REACHED,

    //
    // Only for user-debugger
    //
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
    DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_THREAD_INTERCEPTED,

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
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CALLSTACK,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_TEST_QUERY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_PROCESS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_THREAD,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_RUN_SCRIPT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_USER_INPUT_BUFFER,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SEARCH_QUERY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_REGISTER_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_ADD_ACTION_TO_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_AND_MODIFY_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_REGISTERS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_EDIT_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_BP,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_LIST_OR_MODIFY_BREAKPOINTS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_RELOAD,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_PA2VA_AND_VA2PA,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_QUERY_PTE,

    //
    // Debuggee to debugger
    //
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION = 0,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_LOGGING_MECHANISM,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_CORE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_PROCESS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_THREAD,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_RUNNING_SCRIPT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FORMATS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FLUSH,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CALLSTACK,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_TEST_QUERY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_REGISTERS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_EDITING_MEMORY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_BP,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_LIST_OR_MODIFY_BREAKPOINTS,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_UPDATE_SYMBOL_INFO,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SYMBOL_FINISHED,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SEARCH_QUERY,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_PTE,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_VA2PA_AND_PA2VA,

} DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION;

//////////////////////////////////////////////////
//                  Debugger                    //
//////////////////////////////////////////////////

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

/**
 * @brief request to add new symbol detail or update a previous
 * symbol table entry
 *
 */
typedef struct _DEBUGGER_UPDATE_SYMBOL_TABLE
{
    UINT32               TotalSymbols;
    UINT32               CurrentSymbolIndex;
    MODULE_SYMBOL_DETAIL SymbolDetailPacket;

} DEBUGGER_UPDATE_SYMBOL_TABLE, *PDEBUGGER_UPDATE_SYMBOL_TABLE;

/**
 * @brief check so the DEBUGGER_UPDATE_SYMBOL_TABLE should be smaller than packet size
 *
 */
static_assert(sizeof(DEBUGGER_UPDATE_SYMBOL_TABLE) < PacketChunkSize,
              "err (static_assert), size of PacketChunkSize should be bigger than DEBUGGER_UPDATE_SYMBOL_TABLE (MODULE_SYMBOL_DETAIL)");

/*
==============================================================================================
 */

/**
 * @brief request that shows, symbol reload process is finished
 *
 */
typedef struct _DEBUGGEE_SYMBOL_UPDATE_RESULT
{
    UINT64 KernelStatus; // Kerenl put the status in this field

} DEBUGGEE_SYMBOL_UPDATE_RESULT, *PDEBUGGEE_SYMBOL_UPDATE_RESULT;

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
    UINT32 ProcessId;

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
    UINT32  KernelStatus;

} DEBUGGER_VA2PA_AND_PA2VA_COMMANDS, *PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_DT_COMMAND_OPTIONS \
    sizeof(DEBUGGER_DT_COMMAND_OPTIONS)

/**
 * @brief requests options for dt and struct command
 *
 */
typedef struct _DEBUGGER_DT_COMMAND_OPTIONS
{
    const char * TypeName;
    UINT64       SizeOfTypeName;
    UINT64       Address;
    BOOLEAN      IsStruct;
    PVOID        BufferAddress;
    UINT32       TargetPid;
    const char * AdditionalParameters;

} DEBUGGER_DT_COMMAND_OPTIONS, *PDEBUGGER_DT_COMMAND_OPTIONS;

/* ==============================================================================================
 */

/**
 * @brief different types of prealloc requests
 *
 */
typedef enum _DEBUGGER_PREALLOC_COMMAND_TYPE
{
    DEBUGGER_PREALLOC_COMMAND_TYPE_MONITOR,
    DEBUGGER_PREALLOC_COMMAND_TYPE_THREAD_INTERCEPTION,
} DEBUGGER_PREALLOC_COMMAND_TYPE;

#define SIZEOF_DEBUGGER_PREALLOC_COMMAND \
    sizeof(DEBUGGER_PREALLOC_COMMAND)

/**
 * @brief requests for prealloc commands
 *
 */
typedef struct _DEBUGGER_PREALLOC_COMMAND
{
    DEBUGGER_PREALLOC_COMMAND_TYPE Type;
    UINT64                         Count;
    UINT32                         KernelStatus;

} DEBUGGER_PREALLOC_COMMAND, *PDEBUGGER_PREALLOC_COMMAND;

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
    DEBUGGER_SHOW_COMMAND_DT = 1,
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
    UINT32                       Pid; // Read from cr3 of what process
    UINT64                       Address;
    UINT32                       Size;
    DEBUGGER_READ_MEMORY_TYPE    MemoryType;
    DEBUGGER_READ_READING_TYPE   ReadingType;
    PDEBUGGER_DT_COMMAND_OPTIONS DtDetails;
    DEBUGGER_SHOW_MEMORY_STYLE   Style;        // not used in local debugging
    UINT32                       ReturnLength; // not used in local debugging
    UINT32                       KernelStatus; // not used in local debugging

} DEBUGGER_READ_MEMORY, *PDEBUGGER_READ_MEMORY;

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

#define SIZEOF_DEBUGGER_TEST_QUERY_BUFFER \
    sizeof(DEBUGGER_TEST_QUERY_BUFFER)

/**
 * @brief request for test query buffers
 *
 */
typedef struct _DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER
{
    UINT32 RequestIndex;
    UINT32 KernelStatus;

} DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER, *PDEBUGGER_DEBUGGER_TEST_QUERY_BUFFER;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS \
    sizeof(DEBUGGER_PERFORM_KERNEL_TESTS)

/**
 * @brief request performing kernel tests
 *
 */
typedef struct _DEBUGGER_PERFORM_KERNEL_TESTS
{
    UINT32 KernelStatus;

} DEBUGGER_PERFORM_KERNEL_TESTS, *PDEBUGGER_PERFORM_KERNEL_TESTS;

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

#define SIZEOF_DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION \
    sizeof(DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION)

/**
 * @brief request for collecting debuggee's kernel-side test information
 *
 */
typedef struct _DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION
{
    UINT64 Value;
    char   Tag[32];

} DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION,
    *PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION;

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
 * @brief request for send a user-mode message to debugger
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
    SEARCH_VIRTUAL_MEMORY,
    SEARCH_PHYSICAL_FROM_VIRTUAL_MEMORY,

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

    UINT64 KernelStatus; /* DEBUGGER_OPERATION_WAS_SUCCESSFULL ,
                          DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER
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
    UINT64 NtoskrnlBaseAddress;
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
 * @brief different actions of switchings
 *
 */
typedef enum _DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_TYPE
{
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_DETACH,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_REMOVE_HOOKS,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_KILL_PROCESS,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_PAUSE_PROCESS,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_SWITCH_BY_PROCESS_OR_THREAD,
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_QUERY_COUNT_OF_ACTIVE_DEBUGGING_THREADS,

} DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_TYPE;

/**
 * @brief request for attaching user-mode process
 *
 */
typedef struct _DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS
{
    BOOLEAN                                              IsStartingNewProcess;
    UINT32                                               ProcessId;
    UINT32                                               ThreadId;
    BOOLEAN                                              Is32Bit;
    BOOLEAN                                              IsPaused; // used in switching to threads
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_TYPE Action;
    UINT32                                               CountOfActiveDebuggingThreadsAndProcesses; // used in showing the list of active threads/processes
    UINT64                                               Token;
    UINT64                                               Result;

} DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,
    *PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS;

/* ==============================================================================================
 */
#define SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS \
    sizeof(DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS)

/**
 * @brief different type of process or thread queries
 *
 */
typedef enum _DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_TYPES
{
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_COUNT   = 1,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_COUNT    = 2,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_LIST    = 3,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_LIST     = 4,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_CURRENT_PROCESS = 5,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_CURRENT_THREAD  = 6,

} DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_TYPES;

/**
 * @brief different actions on showing or querying list of process or threads
 *
 */
typedef enum _DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTIONS
{
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY     = 1,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT        = 2,
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS = 3,

} DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTIONS;

/**
 * @brief The structure of needed information to get the details
 * of the process from nt!_EPROCESS and location of needed variables
 *
 */
typedef struct _DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS
{
    UINT64 PsActiveProcessHead;      // nt!PsActiveProcessHead
    ULONG  ImageFileNameOffset;      // nt!_EPROCESS.ImageFileName
    ULONG  UniquePidOffset;          // nt!_EPROCESS.UniqueProcessId
    ULONG  ActiveProcessLinksOffset; // nt!_EPROCESS.ActiveProcessLinks

} DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS, *PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS;

/**
 * @brief The structure of needed information to get the details
 * of the thread from nt!_ETHREAD and location of needed variables
 *
 */
typedef struct _DEBUGGEE_THREAD_LIST_NEEDED_DETAILS
{
    UINT32 ThreadListHeadOffset;     // nt!_EPROCESS.ThreadListHead
    UINT32 ThreadListEntryOffset;    // nt!_ETHREAD.ThreadListEntry
    UINT32 CidOffset;                // nt!_ETHREAD.Cid
    UINT64 PsActiveProcessHead;      // nt!PsActiveProcessHead
    ULONG  ActiveProcessLinksOffset; // nt!_EPROCESS.ActiveProcessLinks
    UINT64 Process;

} DEBUGGEE_THREAD_LIST_NEEDED_DETAILS, *PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS;

/**
 * @brief The structure showing list of processes (details of each
 * entry)
 *
 */
typedef struct _DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY
{
    UINT64 Eprocess;
    UINT32 Pid;
    UINT64 Cr3;
    UCHAR  ImageFileName[15 + 1];

} DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY, *PDEBUGGEE_PROCESS_LIST_DETAILS_ENTRY;

/**
 * @brief The structure showing list of threads (details of each
 * entry)
 *
 */
typedef struct _DEBUGGEE_THREAD_LIST_DETAILS_ENTRY
{
    UINT64 Eprocess;
    UINT64 Ethread;
    UINT64 Pid;
    UINT64 Tid;
    UCHAR  ImageFileName[15 + 1];

} DEBUGGEE_THREAD_LIST_DETAILS_ENTRY, *PDEBUGGEE_THREAD_LIST_DETAILS_ENTRY;

/**
 * @brief request for query count of active processes and threads
 *
 */
typedef struct _DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS
{
    DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS               ProcessListNeededDetails;
    DEBUGGEE_THREAD_LIST_NEEDED_DETAILS                ThreadListNeededDetails;
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_TYPES   QueryType;
    DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTIONS QueryAction;
    UINT32                                             Count;
    UINT64                                             Result;

} DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS,
    *PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS;

/* ==============================================================================================
 */

/**
 * @brief The structure for saving the callstack frame of one parameter
 *
 */
typedef struct _DEBUGGER_SINGLE_CALLSTACK_FRAME
{
    BOOLEAN IsStackAddressValid;
    BOOLEAN IsValidAddress;
    BOOLEAN IsExecutable;
    UINT64  Value;
    BYTE    InstructionBytesOnRip[MAXIMUM_CALL_INSTR_SIZE];

} DEBUGGER_SINGLE_CALLSTACK_FRAME, *PDEBUGGER_SINGLE_CALLSTACK_FRAME;

#define SIZEOF_DEBUGGER_CALLSTACK_REQUEST \
    sizeof(DEBUGGER_CALLSTACK_REQUEST)

/**
 * @brief callstack showing method
 *
 */
typedef enum _DEBUGGER_CALLSTACK_DISPLAY_METHOD
{
    DEBUGGER_CALLSTACK_DISPLAY_METHOD_WITHOUT_PARAMS,
    DEBUGGER_CALLSTACK_DISPLAY_METHOD_WITH_PARAMS,

} DEBUGGER_CALLSTACK_DISPLAY_METHOD;

/**
 * @brief request for callstack frames
 *
 */
typedef struct _DEBUGGER_CALLSTACK_REQUEST
{
    BOOLEAN                           Is32Bit;
    UINT32                            KernelStatus;
    DEBUGGER_CALLSTACK_DISPLAY_METHOD DisplayMethod;
    UINT32                            Size;
    UINT32                            FrameCount;
    UINT64                            BaseAddress;
    UINT64                            BufferSize;

    //
    // Here is the size of stack frames
    //

} DEBUGGER_CALLSTACK_REQUEST, *PDEBUGGER_CALLSTACK_REQUEST;

/* ==============================================================================================
 */
#define SIZEOF_USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS \
    sizeof(USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS)

typedef struct _USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS
{
    UINT32  ProcessId;
    UINT32  ThreadId;
    BOOLEAN IsProcess;

} USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS, *PUSERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS;

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
 * @brief The structure of remote packets in HyperDbg
 *
 */
typedef struct _DEBUGGER_REMOTE_PACKET
{
    BYTE                                    Checksum;
    UINT64                                  Indicator; /* Shows the type of the packet */
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
 * @brief User-mode debugging actions
 *
 */
typedef enum _DEBUGGER_UD_COMMAND_ACTION_TYPE
{
    DEBUGGER_UD_COMMAND_ACTION_TYPE_NONE = 0,
    DEBUGGER_UD_COMMAND_ACTION_TYPE_PAUSE,
    DEBUGGER_UD_COMMAND_ACTION_TYPE_CONTINUE,
    DEBUGGER_UD_COMMAND_ACTION_TYPE_REGULAR_STEP,

} DEBUGGER_UD_COMMAND_ACTION_TYPE;

/**
 * @brief Description of user-mode debugging actions
 *
 */
typedef struct _DEBUGGER_UD_COMMAND_ACTION
{
    DEBUGGER_UD_COMMAND_ACTION_TYPE ActionType;
    UINT64                          OptionalParam1;
    UINT64                          OptionalParam2;
    UINT64                          OptionalParam3;
    UINT64                          OptionalParam4;

} DEBUGGER_UD_COMMAND_ACTION, *PDEBUGGER_UD_COMMAND_ACTION;

/**
 * @brief The structure of command packet in uHyperDbg
 *
 */
typedef struct _DEBUGGER_UD_COMMAND_PACKET
{
    DEBUGGER_UD_COMMAND_ACTION UdAction;
    UINT64                     ProcessDebuggingDetailToken;
    UINT32                     TargetThreadId;
    BOOLEAN                    ApplyToAllPausedThreads;
    UINT32                     Result;

} DEBUGGER_UD_COMMAND_PACKET, *PDEBUGGER_UD_COMMAND_PACKET;

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
 * @brief Debugger process switch and process details
 *
 */
typedef enum _DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE
{

    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_DETAILS,
    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_LIST,
    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PERFORM_SWITCH,

} DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE;

/**
 * @brief The structure of changing process and show process
 * packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET
{
    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE ActionType;
    UINT32                                   ProcessId;
    UINT64                                   Process;
    BOOLEAN                                  IsSwitchByClkIntr;
    UCHAR                                    ProcessName[16];
    DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS     ProcessListSymDetails;
    UINT32                                   Result;

} DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET, *PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET;

/**
 * @brief Debugger size of DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET
 *
 */
#define SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET \
    sizeof(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET)

/**
 * @brief Debugger thread switch and thread details
 *
 */
typedef enum _DEBUGGEE_DETAILS_AND_SWITCH_THREAD_TYPE
{

    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PERFORM_SWITCH,
    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_DETAILS,
    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_LIST,

} DEBUGGEE_DETAILS_AND_SWITCH_THREAD_TYPE;

/**
 * @brief The structure of changing thead and show thread
 * packet in HyperDbg
 */
typedef struct _DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET
{
    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_TYPE ActionType;
    UINT32                                  ThreadId;
    UINT32                                  ProcessId;
    UINT64                                  Thread;
    UINT64                                  Process;
    BOOLEAN                                 CheckByClockInterrupt;
    UCHAR                                   ProcessName[16];
    DEBUGGEE_THREAD_LIST_NEEDED_DETAILS     ThreadListSymDetails;
    UINT32                                  Result;

} DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET, *PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET;

/**
 * @brief Debugger size of DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET
 *
 */
#define SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET \
    sizeof(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET)

/**
 * @brief stepping types
 *
 */
typedef enum _DEBUGGER_REMOTE_STEPPING_REQUEST
{

    DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER,
    DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN,
    DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN,

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
 * @brief The structure of .sym reload packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_SYMBOL_REQUEST_PACKET
{
    UINT32 ProcessId;

} DEBUGGEE_SYMBOL_REQUEST_PACKET, *PDEBUGGEE_SYMBOL_REQUEST_PACKET;

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
 * @brief The structure of result of search packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_RESULT_OF_SEARCH_PACKET
{
    UINT32 CountOfResults;
    UINT32 Result;

} DEBUGGEE_RESULT_OF_SEARCH_PACKET, *PDEBUGGEE_RESULT_OF_SEARCH_PACKET;

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
    UINT32  CommandLen;
    BOOLEAN IgnoreFinishedSignal;
    UINT32  Result;

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
