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

//////////////////////////////////////////////////
//				Message Tracing                 //
//////////////////////////////////////////////////

/* Default buffer size */
#define MaximumPacketsCapacity 1000 // number of packets
#define PacketChunkSize                                                        \
  1000 // NOTE : REMEMBER TO CHANGE IT IN USER-MODE APP TOO
#define UsermodeBufferSize                                                     \
  sizeof(UINT32) + PacketChunkSize +                                           \
      1 /* Becausee of Opeation code at the start of the buffer + 1 for        \
           null-termminating */
#define LogBufferSize                                                          \
  MaximumPacketsCapacity *(PacketChunkSize + sizeof(BUFFER_HEADER))
#define DbgPrintLimitation 512
#define DebuggerEventTagStartSeed 0x1000000

//////////////////////////////////////////////////
//					Installer
////
//////////////////////////////////////////////////

#define DRIVER_NAME "hprdbghv"

//////////////////////////////////////////////////
//				Operation Codes                  //
//////////////////////////////////////////////////

/* Message area >= 0x4 */
#define OPERATION_LOG_INFO_MESSAGE 0x1
#define OPERATION_LOG_WARNING_MESSAGE 0x2
#define OPERATION_LOG_ERROR_MESSAGE 0x3
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE 0x4
#define OPERATION_LOG_WITH_TAG 0x5

//////////////////////////////////////////////////
//		    	Callback Definitions			//
//////////////////////////////////////////////////

typedef int(__stdcall *Callback)(const char *Text);

//////////////////////////////////////////////////
//					Event Details               //
//////////////////////////////////////////////////

typedef enum _DEBUGGER_EVENT_TYPE_ENUM {

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
  PMC_INSTRUCTION_EXECUTION

} DEBUGGER_EVENT_TYPE_ENUM;

typedef enum _DEBUGGER_EVENT_ACTION_TYPE_ENUM {
  BREAK_TO_DEBUGGER,
  LOG_THE_STATES,
  RUN_CUSTOM_CODE

} DEBUGGER_EVENT_ACTION_TYPE_ENUM;

//
// Each command is like the following struct, it also used for tracing works in
// user mode and sending it to the kernl mode,
// THIS IS NOT WHAT WE SAVE FOR EVENTS IN KERNEL MODE
//
typedef struct _DEBUGGER_GENERAL_EVENT_DETAIL {

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

  UINT32 CountOfActions;

  UINT64 Tag; // is same as operation code
  DEBUGGER_EVENT_TYPE_ENUM EventType;

  UINT64 OptionalParam1;
  UINT64 OptionalParam2;
  UINT64 OptionalParam3;
  UINT64 OptionalParam4;

  PVOID CommandStringBuffer;

  UINT32 ConditionBufferSize;

} DEBUGGER_GENERAL_EVENT_DETAIL, *PDEBUGGER_GENERAL_EVENT_DETAIL;

//
// Each event can have mulitple actions
// THIS STRUCTURE IS ONLY USED IN USER MODE
// WE USE SEPARATE STRUCTURE FOR ACTIONS IN
// KERNEL MODE
//
typedef struct _DEBUGGER_GENERAL_ACTION {
  UINT64 EventTag;
  DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType;
  UINT32 PreAllocatedBuffer;

  UINT32 CustomCodeBufferSize;

} DEBUGGER_GENERAL_ACTION, *PDEBUGGER_GENERAL_ACTION;

//
//
//

typedef struct _DEBUGGER_EVENT_AND_ACTION_REG_BUFFER {

  BOOLEAN IsSuccessful;
  UINT32 Error; // If IsSuccessful was, FALSE

} DEBUGGER_EVENT_AND_ACTION_REG_BUFFER, *PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER;

//////////////////////////////////////////////////
//					Debugger                    //
//////////////////////////////////////////////////

#define SIZEOF_REGISTER_EVENT sizeof(REGISTER_NOTIFY_BUFFER)

typedef enum _NOTIFY_TYPE { IRP_BASED, EVENT_BASED } NOTIFY_TYPE;

typedef struct _REGISTER_NOTIFY_BUFFER {
  NOTIFY_TYPE Type;
  HANDLE hEvent;

} REGISTER_NOTIFY_BUFFER, *PREGISTER_NOTIFY_BUFFER;

/* ==============================================================================================
 */

typedef struct _DEBUGGER_MONITOR_COMMAND {
  PVOID StartAddress;
  PVOID EndAddress;
  BOOLEAN MonitorRead;
  BOOLEAN MonitorWrite;

  //
  // The last field should be events
  //
  DEBUGGER_GENERAL_EVENT_DETAIL EventDetail;

} DEBUGGER_MONITOR_COMMAND, *PDEBUGGER_MONITOR_COMMAND;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS                        \
  sizeof(DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS)

typedef struct _DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS {

  UINT64 VirtualAddress;

  UINT64 Pml4eVirtualAddress;
  UINT64 Pml4eValue;

  UINT64 PdpteVirtualAddress;
  UINT64 PdpteValue;

  UINT64 PdeVirtualAddress;
  UINT64 PdeValue;

  UINT64 PteVirtualAddress;
  UINT64 PteValue;

} DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS,
    *PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS;

/* ==============================================================================================
 */

#define SIZEOF_DEBUGGER_READ_MEMORY sizeof(DEBUGGER_READ_MEMORY)

typedef enum _DEBUGGER_READ_READING_TYPE {
  READ_FROM_KERNEL,
  READ_FROM_VMX_ROOT
} DEBUGGER_READ_READING_TYPE;

typedef enum _DEBUGGER_READ_MEMORY_TYPE {
  DEBUGGER_READ_PHYSICAL_ADDRESS,
  DEBUGGER_READ_VIRTUAL_ADDRESS
} DEBUGGER_READ_MEMORY_TYPE;

typedef enum _DEBUGGER_SHOW_MEMORY_STYLE {
  DEBUGGER_SHOW_COMMAND_DISASSEMBLE,
  DEBUGGER_SHOW_COMMAND_DB,
  DEBUGGER_SHOW_COMMAND_DC,
  DEBUGGER_SHOW_COMMAND_DQ,
  DEBUGGER_SHOW_COMMAND_DD
} DEBUGGER_SHOW_MEMORY_STYLE;

typedef struct _DEBUGGER_READ_MEMORY {

  UINT32 Pid; // Read from cr3 of what process
  UINT64 Address;
  UINT32 Size;
  DEBUGGER_READ_MEMORY_TYPE MemoryType;
  DEBUGGER_READ_READING_TYPE ReadingType;

} DEBUGGER_READ_MEMORY, *PDEBUGGER_READ_MEMORY;

/* ==============================================================================================
 */

#define SIZEOF_READ_AND_WRITE_ON_MSR sizeof(DEBUGGER_READ_AND_WRITE_ON_MSR)
#define DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES 0xffffffff

typedef enum _DEBUGGER_MSR_ACTION_TYPE {
  DEBUGGER_MSR_READ,
  DEBUGGER_MSR_WRITE
} DEBUGGER_MSR_ACTION_TYPE;

typedef struct _DEBUGGER_READ_AND_WRITE_ON_MSR {

  UINT64 Msr; // It's actually a 32-Bit value but let's not mess with a register
  UINT32 CoreNumber; // specifies the core to execute wrmsr or read the msr
                     // (DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES mean all
                     // the cores)
  DEBUGGER_MSR_ACTION_TYPE
  ActionType; // Detects whether user needs wrmsr or rdmsr
  UINT64 Value;

} DEBUGGER_READ_AND_WRITE_ON_MSR, *PDEBUGGER_READ_AND_WRITE_ON_MSR;

/* ==============================================================================================
 */

#define DEBUGGER_EVENT_APPLY_TO_ALL_CORES 0xffffffff
#define DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES 0xffffffff
#define DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS 0xffffffff
#define DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES 0xffffffff
#define DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS 0xffffffff
#define DEBUGGER_EVENT_ALL_IO_PORTS 0xffffffff

//
// Pseudo Regs Mask (It's a mask not a value)
//

/** equals to @$proc in windbg that shows the current eprocess */
#define GUEST_PSEUDO_REG_PROC 0x1

/** equals to @$ra in windbg that shows the return address that is currently on
 * the stack */
#define GUEST_PSEUDO_REG_PROC 0x2

/** equals to @$ip in windbg that shows the instruction pointer register */
#define GUEST_PSEUDO_REG_PROC 0x4

/** equals to @$thread in windbg that shows the address of the current thread's
 * ethread */
#define GUEST_PSEUDO_REG_PROC 0x8

/** equals to @$thread in windbg that shows the address of the current thread's
 * ethread */
#define GUEST_PSEUDO_REG_PROC 0x10

/** equals to @$peb in windbg that shows the address of the process environment
 * block(PEB) of the current process */
#define GUEST_PSEUDO_REG_PROC 0x20

/** equals to @$teb in windbg that shows the address of the thread environment
 * block(TEB) of the current thread */
#define GUEST_PSEUDO_REG_PROC 0x40

/** equals to @$tpid in windbg that shows the process ID(PID) for the process
 * that owns the current thread */
#define GUEST_PSEUDO_REG_PROC 0x80

/** equals to @$tid in windbg that shows the thread ID for the current thread */
#define GUEST_PSEUDO_REG_PROC 0x100

//
// GP Regs Mask (It's a mask not a value)
//
#define GUEST_GP_REG_RAX 0x1
#define GUEST_GP_REG_RCX 0x2
#define GUEST_GP_REG_RDX 0x4
#define GUEST_GP_REG_RBX 0x8
#define GUEST_GP_REG_RSP 0x10
#define GUEST_GP_REG_RBP 0x20
#define GUEST_GP_REG_RSI 0x40
#define GUEST_GP_REG_RDI 0x80
#define GUEST_GP_REG_R8 0x100
#define GUEST_GP_REG_R9 0x200
#define GUEST_GP_REG_R10 0x400
#define GUEST_GP_REG_R11 0x800
#define GUEST_GP_REG_R12 0x1000
#define GUEST_GP_REG_R13 0x2000
#define GUEST_GP_REG_R14 0x4000
#define GUEST_GP_REG_R15 0x8000
#define GUEST_GP_REG_RFLAGS 0x10000

typedef enum _DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION_TYPE {

  //
  // Read the results
  //
  GUEST_LOG_READ_GENERAL_PURPOSE_REGISTERS, // r rax
  GUEST_LOG_READ_STATIC_MEMORY_ADDRESS,     // dc fffff80126551180
  GUEST_LOG_READ_REGISTER_MEMORY_ADDRESS,   // dc poi(rax)

  GUEST_LOG_READ_POI_REGISTER_ADD_VALUE,      // dc poi(rax) + xx
  GUEST_LOG_READ_POI_REGISTER_SUBTRACT_VALUE, // dc poi(rax) - xx

  GUEST_LOG_READ_POI_REGISTER_PLUS_VALUE,  // dc poi(rax + xx)
  GUEST_LOG_READ_POI_REGISTER_MINUS_VALUE, // dc poi(rax- xx)

  GUEST_LOG_READ_PSEUDO_REGISTER, // r @$proc

  GUEST_LOG_READ_MEMORY_PSEUDO_REGISTER_ADD_VALUE,      // dc @$proc + xx
  GUEST_LOG_READ_MEMORY_PSEUDO_REGISTER_SUBTRACT_VALUE, // dc @$proc - xx

  GUEST_LOG_READ_MEMORY_PSEUDO_REGISTER_PLUS_VALUE,  // dc poi(@$proc - xx)
  GUEST_LOG_READ_MEMORY_PSEUDO_REGISTER_MINUS_VALUE, // dc poi(@$proc - xx)

} DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION_TYPE;

typedef struct _DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION {
  DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION_TYPE
  LogType;          // Type of log (how to log)
  UINT64 LogMask;   // Mask (e.g register)
  UINT64 LogValue;  // additions or subtraction value
  UINT32 LogLength; // Length of Bytes

} DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION,
    *PDEBUGGER_EVENT_ACTION_LOG_CONFIGURATION;

typedef struct _DEBUGGER_EVENT_REQUEST_BUFFER {
  BOOLEAN EnabledRequestBuffer;
  UINT32 RequestBufferSize;
  UINT64 RequstBufferAddress;

} DEBUGGER_EVENT_REQUEST_BUFFER, *PDEBUGGER_EVENT_REQUEST_BUFFER;

typedef struct _DEBUGGER_EVENT_REQUEST_CUSTOM_CODE {
  UINT32 CustomCodeBufferSize;
  PVOID CustomCodeBufferAddress;
  UINT32 OptionalRequestedBufferSize;

} DEBUGGER_EVENT_REQUEST_CUSTOM_CODE, *PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE;

/* ==============================================================================================
 */

typedef struct _DEBUGGER_EVENT_ACTION {
  UINT32 ActionOrderCode; // The code for this action (it also shows the order)
  LIST_ENTRY ActionsList; // Holds the link list of next actions
  DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType; // What action we wanna perform
  BOOLEAN ImmediatelySendTheResults; // should we send the results immediately
                                     // or store them in another structure and
                                     // send multiple of them each time

  DEBUGGER_EVENT_ACTION_LOG_CONFIGURATION
  LogConfiguration; // If it's Log the Statess

  DEBUGGER_EVENT_REQUEST_BUFFER
  RequestedBuffer; // if it's a custom code and needs a buffer then we use
                   // this structs

  UINT32 CustomCodeBufferSize;   // if null, means it's not custom code type
  PVOID CustomCodeBufferAddress; // address of custom code if any

} DEBUGGER_EVENT_ACTION, *PDEBUGGER_EVENT_ACTION;

/* ==============================================================================================
 */

typedef struct _DEBUGGER_EVENT {
  UINT64 Tag;
  LIST_ENTRY EventsOfSameTypeList; // Linked-list of events of a same type
  DEBUGGER_EVENT_TYPE_ENUM EventType;
  BOOLEAN Enabled;
  UINT32 CoreId; // determines the core index to apply this event to, if it's
                 // 0xffffffff means that we have to apply it to all cores

  UINT32
  ProcessId; // determines the pid to apply this event to, if it's
             // 0xffffffff means that we have to apply it to all processes

  LIST_ENTRY ActionsListHead; // Each entry is in DEBUGGER_EVENT_ACTION struct
  UINT32 CountOfActions;      // The total count of actions

  UINT64 OptionalParam1; // Optional parameter to be used differently by events
  UINT64 OptionalParam2; // Optional parameter to be used differently by events
  UINT64 OptionalParam3; // Optional parameter to be used differently by events
  UINT64 OptionalParam4; // Optional parameter to be used differently by events

  UINT32 ConditionsBufferSize;  // if null, means uncoditional
  PVOID ConditionBufferAddress; // Address of the condition buffer (most of the
                                // time at the end of this buffer)

} DEBUGGER_EVENT, *PDEBUGGER_EVENT;

//////////////////////////////////////////////////
//		    	Debugger Error Codes            //
//////////////////////////////////////////////////

#define DEBUGEER_ERROR_TAG_NOT_EXISTS 0xc0000000
#define DEBUGEER_ERROR_INVALID_ACTION_TYPE 0xc0000001
#define DEBUGEER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO 0xc0000002
#define DEBUGEER_ERROR_EVENT_TYPE_IS_INVALID 0xc0000003
#define DEBUGEER_ERROR_UNABLE_TO_CREATE_EVENT 0xc0000004
#define DEBUGEER_ERROR_INVALID_ADDRESS 0xc0000005
#define DEBUGEER_ERROR_INVALID_CORE_ID 0xc0000006
#define DEBUGEER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES 0xc0000007
#define DEBUGEER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID 0xc0000008

//////////////////////////////////////////////////
//					IOCTLs                      //
//////////////////////////////////////////////////

#define IOCTL_REGISTER_EVENT                                                   \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TERMINATE_VMX                                                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_READ_MEMORY                                             \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_READ_OR_WRITE_MSR                                       \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS                         \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_REGISTER_EVENT                                          \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT                                     \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
