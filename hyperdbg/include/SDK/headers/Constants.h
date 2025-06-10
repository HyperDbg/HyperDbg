/**
 * @file Constants.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK constants
 * @details This file contains definitions of constants
 * used in HyperDbg
 * @version 0.2
 * @date 2022-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			 Version Information                //
//////////////////////////////////////////////////

#define VERSION_MAJOR 0
#define VERSION_MINOR 14
#define VERSION_PATCH 0

//
// Example of __DATE__ string: "Jul 27 2012"
//                              01234567890

#define BUILD_YEAR_CH0 (__DATE__[7])
#define BUILD_YEAR_CH1 (__DATE__[8])
#define BUILD_YEAR_CH2 (__DATE__[9])
#define BUILD_YEAR_CH3 (__DATE__[10])

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1                                         \
    (                                                           \
        (BUILD_MONTH_IS_JAN) ? '1' : (BUILD_MONTH_IS_FEB) ? '2' \
                                 : (BUILD_MONTH_IS_MAR)   ? '3' \
                                 : (BUILD_MONTH_IS_APR)   ? '4' \
                                 : (BUILD_MONTH_IS_MAY)   ? '5' \
                                 : (BUILD_MONTH_IS_JUN)   ? '6' \
                                 : (BUILD_MONTH_IS_JUL)   ? '7' \
                                 : (BUILD_MONTH_IS_AUG)   ? '8' \
                                 : (BUILD_MONTH_IS_SEP)   ? '9' \
                                 : (BUILD_MONTH_IS_OCT)   ? '0' \
                                 : (BUILD_MONTH_IS_NOV)   ? '1' \
                                 : (BUILD_MONTH_IS_DEC)   ? '2' \
                                                          : /* error default */ '?')

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[5])

//
// Example of __TIME__ string: "21:06:19"
//                              01234567

#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])

#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#define BUILD_SEC_CH0 (__TIME__[6])
#define BUILD_SEC_CH1 (__TIME__[7])

#ifdef __cplusplus // becasue it's not valid in C

const unsigned char BuildDateTime[] = {
    BUILD_YEAR_CH0,
    BUILD_YEAR_CH1,
    BUILD_YEAR_CH2,
    BUILD_YEAR_CH3,
    '-',
    BUILD_MONTH_CH0,
    BUILD_MONTH_CH1,
    '-',
    BUILD_DAY_CH0,
    BUILD_DAY_CH1,
    ' ',
    BUILD_HOUR_CH0,
    BUILD_HOUR_CH1,
    ':',
    BUILD_MIN_CH0,
    BUILD_MIN_CH1,
    ':',
    BUILD_SEC_CH0,
    BUILD_SEC_CH1,

    '\0'};

// Macro to convert a number to a string
#    define STRINGIFY(x) #x
#    define TOSTRING(x)  STRINGIFY(x)

// Complete version as a string
#    define HYPERDBG_COMPLETE_VERSION "v" TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH) "\0"

const unsigned char CompleteVersion[] = HYPERDBG_COMPLETE_VERSION;

const unsigned char BuildVersion[] = {
    BUILD_YEAR_CH0,
    BUILD_YEAR_CH1,
    BUILD_YEAR_CH2,
    BUILD_YEAR_CH3,
    BUILD_MONTH_CH0,
    BUILD_MONTH_CH1,
    BUILD_DAY_CH0,
    BUILD_DAY_CH1,
    '.',
    BUILD_HOUR_CH0,
    BUILD_HOUR_CH1,
    BUILD_MIN_CH0,
    BUILD_MIN_CH1,

    '\0'};

const unsigned char BuildSignature[] = {
    TOSTRING(VERSION_MAJOR)[0],
    '.',
    TOSTRING(VERSION_MINOR)[0],
    '.',
    TOSTRING(VERSION_PATCH)[0],
    '-',
    BUILD_YEAR_CH0,
    BUILD_YEAR_CH1,
    BUILD_YEAR_CH2,
    BUILD_YEAR_CH3,
    BUILD_MONTH_CH0,
    BUILD_MONTH_CH1,
    BUILD_DAY_CH0,
    BUILD_DAY_CH1,
    '.',
    BUILD_HOUR_CH0,
    BUILD_HOUR_CH1,
    BUILD_MIN_CH0,
    BUILD_MIN_CH1,

    '\0'};

#endif

//////////////////////////////////////////////////
//				Message Tracing                 //
//////////////////////////////////////////////////

/**
 * @brief Default buffer count of packets for message tracing
 * @details number of packets storage for regular buffers
 */
#define MaximumPacketsCapacity 1000

/**
 * @brief Default buffer count of packets for message tracing
 * @details number of packets storage for priority buffers
 */
#define MaximumPacketsCapacityPriority 50

/**
 * @brief Size of normal OS (processor) pages
 */
#define NORMAL_PAGE_SIZE 4096 // PAGE_SIZE

/**
 * @brief Size of each packet
 */
#define PacketChunkSize NORMAL_PAGE_SIZE

/**
 * @brief size of user-mode buffer
 * @details Because of operation code at the start of the
 * buffer + 1 for null-termminating
 *
 */
#define UsermodeBufferSize sizeof(UINT32) + PacketChunkSize + 1

/**
 * @brief size of buffer for serial
 * @details the maximum packet size for sending over serial
 *
 */
#define MaxSerialPacketSize 20 * NORMAL_PAGE_SIZE

/**
 * @brief Final storage size of message tracing
 *
 */
#define LogBufferSize \
    MaximumPacketsCapacity *(PacketChunkSize + sizeof(BUFFER_HEADER))

/**
 * @brief Final storage size of message tracing
 *
 */
#define LogBufferSizePriority \
    MaximumPacketsCapacityPriority *(PacketChunkSize + sizeof(BUFFER_HEADER))

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
 * @brief The seeds that user-mode thread detail token start with it
 * @details This seed should not start with zero (0), otherwise it's
 * interpreted as error
 */
#define DebuggerThreadDebuggingTagStartSeed 0x1000000

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

/**
 * @brief The size of each chunk of memory used in the 'memcpy' function
 * of the script engine for transferring buffers in the VMX-root mode
 *
 */
#define DebuggerScriptEngineMemcpyMovingBufferSize 64

//////////////////////////////////////////////////
//                   EPT Hook                   //
//////////////////////////////////////////////////

/**
 * @brief Maximum number of initial pre-allocated EPT hooks
 *
 */
#define MAXIMUM_NUMBER_OF_INITIAL_PREALLOCATED_EPT_HOOKS 5

//////////////////////////////////////////////////
//             Instant Event Configs            //
//////////////////////////////////////////////////

/**
 * @brief Maximum number of (regular) instant events that are pre-allocated
 *
 */
#define MAXIMUM_REGULAR_INSTANT_EVENTS 20

/**
 * @brief Maximum number of (big) instant events that are pre-allocated
 *
 */
#define MAXIMUM_BIG_INSTANT_EVENTS 0

/**
 * @brief Pre-allocated size for a regular event + conditions buffer
 *
 */
#define REGULAR_INSTANT_EVENT_CONDITIONAL_BUFFER sizeof(DEBUGGER_EVENT) + 100

/**
 * @brief Pre-allocated size for a big event + conditions buffer
 *
 */
#define BIG_INSTANT_EVENT_CONDITIONAL_BUFFER sizeof(DEBUGGER_EVENT) + PAGE_SIZE

/**
 * @brief Pre-allocated size for a regular action + custom code or script buffer
 *
 */
#define REGULAR_INSTANT_EVENT_ACTION_BUFFER sizeof(DEBUGGER_EVENT_ACTION) + (PAGE_SIZE * 2)

/**
 * @brief Pre-allocated size for a big action + custom code or script buffer
 *
 */
#define BIG_INSTANT_EVENT_ACTION_BUFFER sizeof(DEBUGGER_EVENT_ACTION) + MaxSerialPacketSize

/**
 * @brief Pre-allocated size for a regular requested safe buffer
 *
 */
#define REGULAR_INSTANT_EVENT_REQUESTED_SAFE_BUFFER PAGE_SIZE

/**
 * @brief Pre-allocated size for a big requested safe buffer
 *
 */
#define BIG_INSTANT_EVENT_REQUESTED_SAFE_BUFFER MaxSerialPacketSize

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
//             VMCALL Numbers                  //
//////////////////////////////////////////////////

/**
 * @brief The start number of VMCALL number allowed to be
 * used by top-level drivers
 *
 */
#define TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER 0x00000200

/**
 * @brief The start number of VMCALL number allowed to be
 * used by top-level drivers
 *
 */
#define TOP_LEVEL_DRIVERS_VMCALL_ENDING_NUMBER TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER + 0x100

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
#define OPERATION_LOG_INFO_MESSAGE          1U
#define OPERATION_LOG_WARNING_MESSAGE       2U
#define OPERATION_LOG_ERROR_MESSAGE         3U
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE 4U
#define OPERATION_LOG_WITH_TAG              5U

#define OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM \
    6U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_USER_INPUT     7U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_REGISTER_EVENT 8U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT \
    9 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_CLEAR_EVENTS                            10U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_CLEAR_EVENTS_WITHOUT_NOTIFYING_DEBUGGER 11U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED \
    12U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS \
    13U | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_COMMAND_FROM_DEBUGGER_RELOAD_SYMBOL \
    14U | OPERATION_MANDATORY_DEBUGGEE_BIT

#define OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE \
    15U | OPERATION_MANDATORY_DEBUGGEE_BIT

//////////////////////////////////////////////////
//       Breakpoints & Debug Breakpoints        //
//////////////////////////////////////////////////

/**
 * @brief maximum number of buffers to be allocated for a single
 * breakpoint
 */
#define MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE 100

/**
 * @brief maximum number of thread/process ids to be allocated for a simultaneous
 * debugging
 * @details it shows the maximum number of threads/processes that HyperDbg sets
 * trap flag for them
 *
 */
#define MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRAPS 200

//////////////////////////////////////////////////
//          Pool tags used in HyperDbg          //
//////////////////////////////////////////////////

/**
 * @brief Pool tag
 *
 */
#define POOLTAG 0x48444247 // [H]yper[DBG] (HDBG)

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

/**
 * @brief count of characters for tcp end of buffer
 */
#define TCP_END_OF_BUFFER_CHARS_COUNT 0x4

/**
 * @brief characters of the buffer that we set at the end of
 * buffers for tcp
 */
#define TCP_END_OF_BUFFER_CHAR_1 0x10
#define TCP_END_OF_BUFFER_CHAR_2 0x20
#define TCP_END_OF_BUFFER_CHAR_3 0x33
#define TCP_END_OF_BUFFER_CHAR_4 0x44

//////////////////////////////////////////////////
//                 Name of OS                    //
//////////////////////////////////////////////////

/**
 * @brief maximum name for OS name buffer
 *
 */
#define MAXIMUM_CHARACTER_FOR_OS_NAME 256

//////////////////////////////////////////////////
//              Processor Details               //
//////////////////////////////////////////////////

/**
 * @brief maximum instruction size in Intel
 */
#define MAXIMUM_INSTR_SIZE 16

/**
 * @brief maximum size for call instruction in Intel
 */
#define MAXIMUM_CALL_INSTR_SIZE 7

//////////////////////////////////////////////////
//              Symbols Details                 //
//////////////////////////////////////////////////

/**
 * @brief maximum supported modules to load
 * their symbol information
 */
#define MAXIMUM_SUPPORTED_SYMBOLS 1000

/**
 * @brief maximum size for GUID and Age of PE
 * @detail It seems that 33 bytes is enough but let's
 * have more space because there might be sth that we
 * missed :)
 */
#define MAXIMUM_GUID_AND_AGE_SIZE 60

//////////////////////////////////////////////////
//            Debuggee Communication            //
//////////////////////////////////////////////////

/**
 * @brief constant indicator of a HyperDbg packet
 * @warning used in hwdbg
 *
 */
#define INDICATOR_OF_HYPERDBG_PACKET \
    0x4859504552444247 // HYPERDBG = 0x4859504552444247

//////////////////////////////////////////////////
//               Command Details                //
//////////////////////////////////////////////////

/**
 * @brief maximum results that will be returned by !s* s*
 * command
 *
 */
#define MaximumSearchResults 0x1000

//////////////////////////////////////////////////
//                 Script Engine                //
//////////////////////////////////////////////////

/**
 * @brief EFLAGS/RFLAGS
 *
 */
#define X86_FLAGS_CF                 (1 << 0)
#define X86_FLAGS_PF                 (1 << 2)
#define X86_FLAGS_AF                 (1 << 4)
#define X86_FLAGS_ZF                 (1 << 6)
#define X86_FLAGS_SF                 (1 << 7)
#define X86_FLAGS_TF                 (1 << 8)
#define X86_FLAGS_IF                 (1 << 9)
#define X86_FLAGS_DF                 (1 << 10)
#define X86_FLAGS_OF                 (1 << 11)
#define X86_FLAGS_STATUS_MASK        (0xfff)
#define X86_FLAGS_IOPL_MASK          (3 << 12)
#define X86_FLAGS_IOPL_SHIFT         (12)
#define X86_FLAGS_IOPL_SHIFT_2ND_BIT (13)
#define X86_FLAGS_NT                 (1 << 14)
#define X86_FLAGS_RF                 (1 << 16)
#define X86_FLAGS_VM                 (1 << 17)
#define X86_FLAGS_AC                 (1 << 18)
#define X86_FLAGS_VIF                (1 << 19)
#define X86_FLAGS_VIP                (1 << 20)
#define X86_FLAGS_ID                 (1 << 21)
#define X86_FLAGS_RESERVED_ONES      0x2
#define X86_FLAGS_RESERVED           0xffc0802a

#define X86_FLAGS_RESERVED_BITS 0xffc38028
#define X86_FLAGS_FIXED         0x00000002

#ifndef LOWORD
#    define LOWORD(l) ((WORD)(l))
#endif // !LOWORD

#ifndef HIWORD
#    define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif // !HIWORD

#ifndef LOBYTE
#    define LOBYTE(w) ((BYTE)(w))
#endif // !LOBYTE

#ifndef HIBYTE
#    define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif // !HIBYTE

/**
 * @brief Maximum number of stack buffer count in the script engine
 */
#define MAX_STACK_BUFFER_COUNT 256

/**
 * @brief Maximum number of variables that can be used in the script engine
 */
#define MAX_EXECUTION_COUNT 1000000

// TODO: Extract number of variables from input of ScriptEngine
// and allocate variableList Dynamically.
#define MAX_VAR_COUNT 512

#define MAX_FUNCTION_NAME_LENGTH 32

//////////////////////////////////////////////////
//                  Debugger                    //
//////////////////////////////////////////////////

/**
 * @brief Segment selector registers in x86
 *
 */
typedef enum _SEGMENT_REGISTERS
{
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
} SEGMENT_REGISTERS;

/*
 * @brief Windows IRQ Levels
 */
#define PASSIVE_LEVEL  0  // Passive release level
#define LOW_LEVEL      0  // Lowest interrupt level
#define APC_LEVEL      1  // APC interrupt level
#define DISPATCH_LEVEL 2  // Dispatcher level
#define CMCI_LEVEL     5  // CMCI handler level
#define CLOCK_LEVEL    13 // Interval clock level
#define IPI_LEVEL      14 // Interprocessor interrupt level
#define DRS_LEVEL      14 // Deferred Recovery Service level
#define POWER_LEVEL    14 // Power failure level
#define PROFILE_LEVEL  15 // timer used for profiling.
#define HIGH_LEVEL     15 // Highest interrupt level

/**
 * @brief Intel CPU flags in CR0
 */
#define X86_CR0_PE 0x00000001 /* Enable Protected Mode    (RW) */
#define X86_CR0_MP 0x00000002 /* Monitor Coprocessor      (RW) */
#define X86_CR0_EM 0x00000004 /* Require FPU Emulation    (RO) */
#define X86_CR0_TS 0x00000008 /* Task Switched            (RW) */
#define X86_CR0_ET 0x00000010 /* Extension type           (RO) */
#define X86_CR0_NE 0x00000020 /* Numeric Error Reporting  (RW) */
#define X86_CR0_WP 0x00010000 /* Supervisor Write Protect (RW) */
#define X86_CR0_AM 0x00040000 /* Alignment Checking       (RW) */
#define X86_CR0_NW 0x20000000 /* Not Write-Through        (RW) */
#define X86_CR0_CD 0x40000000 /* Cache Disable            (RW) */
#define X86_CR0_PG 0x80000000 /* Paging                         */

/**
 * @brief Intel CPU features in CR4
 *
 */
#define X86_CR4_VME        0x0001 /* enable vm86 extensions */
#define X86_CR4_PVI        0x0002 /* virtual interrupts flag enable */
#define X86_CR4_TSD        0x0004 /* disable time stamp at ipl 3 */
#define X86_CR4_DE         0x0008 /* enable debugging extensions */
#define X86_CR4_PSE        0x0010 /* enable page size extensions */
#define X86_CR4_PAE        0x0020 /* enable physical address extensions */
#define X86_CR4_MCE        0x0040 /* Machine check enable */
#define X86_CR4_PGE        0x0080 /* enable global pages */
#define X86_CR4_PCE        0x0100 /* enable performance counters at ipl 3 */
#define X86_CR4_OSFXSR     0x0200 /* enable fast FPU save and restore */
#define X86_CR4_OSXMMEXCPT 0x0400 /* enable unmasked SSE exceptions */
#define X86_CR4_VMXE       0x2000 /* enable VMX */

/*
 * @brief Segment register and corresponding GDT meaning in Windows
 */
#define KGDT64_NULL      (0 * 16)     // NULL descriptor
#define KGDT64_R0_CODE   (1 * 16)     // kernel mode 64-bit code
#define KGDT64_R0_DATA   (1 * 16) + 8 // kernel mode 64-bit data (stack)
#define KGDT64_R3_CMCODE (2 * 16)     // user mode 32-bit code
#define KGDT64_R3_DATA   (2 * 16) + 8 // user mode 32-bit data
#define KGDT64_R3_CODE   (3 * 16)     // user mode 64-bit code
#define KGDT64_SYS_TSS   (4 * 16)     // kernel mode system task state
#define KGDT64_R3_CMTEB  (5 * 16)     // user mode 32-bit TEB
#define KGDT64_R0_CMCODE (6 * 16)     // kernel mode 32-bit code
#define KGDT64_LAST      (7 * 16)     // last entry

/**
 * @brief PCID Flags
 *
 */
#define PCID_NONE 0x000
#define PCID_MASK 0x003

/**
 * @brief The Microsoft Hypervisor interface defined constants
 *
 */
#define CPUID_HV_VENDOR_AND_MAX_FUNCTIONS 0x40000000
#define CPUID_HV_INTERFACE                0x40000001

/**
 * @brief Cpuid to get virtual address width
 *
 */
#define CPUID_ADDR_WIDTH 0x80000008

/**
 * @brief CPUID Features
 *
 */
#define CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS 0x00000001

/**
 * @brief Hypervisor reserved range for RDMSR and WRMSR
 *
 */
#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x400000F0

/**
 * @brief Apply event modifications to all tags
 *
 */
#define DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG 0xffffffffffffffff

/**
 * @brief Maximum length for a function (to be used in showing distance
 * from symbol functions in the 'u' command)
 *
 */
#define DISASSEMBLY_MAXIMUM_DISTANCE_FROM_OBJECT_NAME 0xffff

/**
 * @brief Read and write MSRs to all cores
 *
 */
#define DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES 0xffffffff

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
 * @brief for reading all registers in r command.
 *
 */
#define DEBUGGEE_SHOW_ALL_REGISTERS 0xffffffff
