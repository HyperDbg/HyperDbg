/**
 * @file DataTypes.h
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
//               Memory Stages                  //
//////////////////////////////////////////////////

/**
 * @brief Different levels of paging
 *
 */
typedef enum _PAGING_LEVEL
{
    PagingLevelPageTable = 0,
    PagingLevelPageDirectory,
    PagingLevelPageDirectoryPointerTable,
    PagingLevelPageMapLevel4
} PAGING_LEVEL;

//////////////////////////////////////////////////
//               CPU Information                //
//////////////////////////////////////////////////

/**
 * @brief Different processor vendors
 *
 */
typedef enum _GENERIC_PROCESSOR_VENDOR
{
    GENERIC_PROCESSOR_VENDOR_INTEL,
    GENERIC_PROCESSOR_VENDOR_AMD,
    GENERIC_PROCESSOR_VENDOR_OTHERS,
} GENERIC_PROCESSOR_VENDOR;

//////////////////////////////////////////////////
//                 List Entries      			//
//////////////////////////////////////////////////

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//
#if defined(__linux__)

typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY * Flink;
    struct _LIST_ENTRY * Blink;
} LIST_ENTRY, *PLIST_ENTRY;

//
// The NT list API (InitializeListHead, InsertHeadList, RemoveEntryList, ...)
// that the shared code calls on this exact layout. Included here, right after
// the layout it operates on, the way BasicTypes.h includes WdkTypes.h.
//
#    include "../../platform/general/header/nt-list.h"

//
// Broken-down calendar time — mirrors the Windows TIME_FIELDS that
// RtlTimeToTimeFields produces (CSHORT == SHORT). See PlatformTime.
//
typedef struct _TIME_FIELDS
{
    SHORT Year;
    SHORT Month;        // 1..12
    SHORT Day;          // 1..31
    SHORT Hour;         // 0..23
    SHORT Minute;       // 0..59
    SHORT Second;       // 0..59
    SHORT Milliseconds; // 0..999
    SHORT Weekday;      // 0..6 (Sunday = 0)
} TIME_FIELDS, *PTIME_FIELDS;

#endif // defined(__linux__)

//////////////////////////////////////////////////
//         Deferred Procedure Call (DPC)         //
//////////////////////////////////////////////////

#if defined(__linux__) && defined(HYPERDBG_KERNEL_MODE)

//
// Windows' KDPC is a "run this small routine soon, from a safe (DISPATCH_LEVEL)
// context" object. The closest Linux analog is a bottom-half (BH) workqueue: it
// runs the work item in softirq context like a DPC, but via the modern,
// non-deprecated workqueue API (tasklets are deprecated). The struct is stored
// BY VALUE inside NOTIFY_RECORD (hyperlog), so it must be a real, fully-sized type.
//
// A Linux work callback receives only the work_struct pointer, whereas Windows
// hands the deferred routine four arguments (Dpc, Context, Arg1, Arg2). We stash
// the routine + context + system arguments here and reconstruct the 4-arg call
// in a trampoline (see PlatformDpc.c).
//
// SKELETON (2026-08-11): compile-clean wiring only. There is no teardown/cancel
// wrapper yet — a queued work item embedded in freed memory is a use-after-free,
// so real callers will need cancel_work_sync() before the owning NOTIFY_RECORD
// is freed. See the TODO(Linux) notes in PlatformDpc.c.
//
struct _KDPC;
typedef VOID (*PKDEFERRED_ROUTINE)(struct _KDPC * Dpc,
                                   PVOID          DeferredContext,
                                   PVOID          SystemArgument1,
                                   PVOID          SystemArgument2);

typedef struct _KDPC
{
    struct work_struct Work;            // Linux deferred-work primitive (BH workqueue = softirq)
    PKDEFERRED_ROUTINE DeferredRoutine; // Windows-style routine, replayed by the trampoline
    PVOID              DeferredContext;
    PVOID              SystemArgument1;
    PVOID              SystemArgument2;
    INT32              TargetCore; // KeSetTargetProcessorDpc's core, or -1 for "any" (see PlatformDpc.c)
} KDPC, *PKDPC, *PRKDPC;

//
// "No target core selected" — the state a KDPC is in until
// PlatformDpcSetTargetProcessor pins it, mirroring an unpinned Windows DPC.
//
#    define KDPC_NO_TARGET_CORE (-1)

//
// Windows KSPIN_LOCK is an integer token; Linux's spinlock_t is a real struct.
// It is embedded BY VALUE in LOG_BUFFER_INFORMATION (Logging.h), so it must be a
// full-sized type. spin_lock()/spin_unlock() = preempt-off (DISPATCH_LEVEL).
//
typedef spinlock_t KSPIN_LOCK, *PKSPIN_LOCK;

#endif // defined(__linux__) && defined(HYPERDBG_KERNEL_MODE)

//////////////////////////////////////////////////
//                 Pool Manager      			//
//////////////////////////////////////////////////

/**
 * @brief Inum of intentions for buffers (buffer tag)
 *
 */
typedef enum _POOL_ALLOCATION_INTENTION
{
    TRACKING_HOOKED_PAGES,
    EXEC_TRAMPOLINE,
    SPLIT_2MB_PAGING_TO_4KB_PAGE,
    DETOUR_HOOK_DETAILS,
    BREAKPOINT_DEFINITION_STRUCTURE,
    PROCESS_THREAD_HOLDER,

    //
    // Instant event buffers
    //
    INSTANT_REGULAR_EVENT_BUFFER,
    INSTANT_BIG_EVENT_BUFFER,
    INSTANT_REGULAR_EVENT_ACTION_BUFFER,
    INSTANT_BIG_EVENT_ACTION_BUFFER,

    //
    // Use for request safe buffers of the event
    //
    INSTANT_REGULAR_SAFE_BUFFER_FOR_EVENTS,
    INSTANT_BIG_SAFE_BUFFER_FOR_EVENTS,

} POOL_ALLOCATION_INTENTION;

//////////////////////////////////////////////////
//	   	Debug Registers Modifications 	    	//
//////////////////////////////////////////////////

typedef enum _DEBUG_REGISTER_TYPE
{
    BREAK_ON_INSTRUCTION_FETCH,
    BREAK_ON_WRITE_ONLY,
    BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED,
    BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH
} DEBUG_REGISTER_TYPE;

//////////////////////////////////////////////////
//              Execution Stages                //
//////////////////////////////////////////////////

typedef enum _VMX_EXECUTION_MODE
{
    VmxExecutionModeNonRoot = FALSE,
    VmxExecutionModeRoot    = TRUE
} VMX_EXECUTION_MODE;

/**
 * @brief Type of calling the event
 *
 */
typedef enum _VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE
{
    VMM_CALLBACK_CALLING_STAGE_INVALID_EVENT_EMULATION = 0,
    VMM_CALLBACK_CALLING_STAGE_PRE_EVENT_EMULATION     = 1,
    VMM_CALLBACK_CALLING_STAGE_POST_EVENT_EMULATION    = 2,
    VMM_CALLBACK_CALLING_STAGE_ALL_EVENT_EMULATION     = 3

} VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE;

/**
 * @brief enum to query different process and thread interception mechanisms
 *
 */
typedef enum _DEBUGGER_THREAD_PROCESS_TRACING
{

    DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_THREAD_CHANGE,
    DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_PROCESS_CHANGE,
    DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_DEBUG_REGISTER_INTERCEPTION,
    DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_WAITING_FOR_MOV_CR3_VM_EXITS,

} DEBUGGER_THREAD_PROCESS_TRACING;

//////////////////////////////////////////////////
//            Callback Definitions              //
//////////////////////////////////////////////////

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function (by passing message as a parameter)
 *
 */
typedef int (*SendMessageWithParamCallback)(const char * Text);

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function (using shared buffer)
 *
 */
typedef int (*SendMessageWithSharedBufferCallback)(VOID);

//////////////////////////////////////////////////
//                Communications                //
//////////////////////////////////////////////////

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

//////////////////////////////////////////////////
//                  Pausing                    //
//////////////////////////////////////////////////

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

/**
 * @brief The structure of detail of a triggered event in HyperDbg
 * @details This structure is also used for transferring breakpoint ids, RIP as the context, etc.
 *
 */
typedef struct _DEBUGGER_TRIGGERED_EVENT_DETAILS
{
    UINT64                                Tag; /* in breakpoints Tag is breakpoint id, not event tag */
    PVOID                                 Context;
    VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE Stage;

} DEBUGGER_TRIGGERED_EVENT_DETAILS, *PDEBUGGER_TRIGGERED_EVENT_DETAILS;

/* ==============================================================================================
 */

/**
 * @brief The structure of pausing packet in kHyperDbg
 *
 */
typedef struct _DEBUGGEE_KD_PAUSED_PACKET
{
    UINT64                                Rip;
    BOOLEAN                               IsProcessorOn32BitMode; // if true shows that the address should be interpreted in 32-bit mode
    BOOLEAN                               IgnoreDisassembling;    // if check if diassembling should be ignored or not
    DEBUGGEE_PAUSING_REASON               PausingReason;
    ULONG                                 CurrentCore;
    UINT64                                EventTag;
    VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE EventCallingStage;
    UINT64                                Rflags;
    BYTE                                  InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];
    UINT16                                ReadInstructionLen;

} DEBUGGEE_KD_PAUSED_PACKET, *PDEBUGGEE_KD_PAUSED_PACKET;

/* ==============================================================================================
 */

/**
 * @brief The structure of pausing packet in uHyperDbg
 *
 */
typedef struct _DEBUGGEE_UD_PAUSED_PACKET
{
    UINT64                                Rip;
    UINT64                                ProcessDebuggingToken;
    BOOLEAN                               Is32Bit; // if true shows that the address should be interpreted in 32-bit mode
    DEBUGGEE_PAUSING_REASON               PausingReason;
    UINT32                                ProcessId;
    UINT32                                ThreadId;
    UINT64                                Rflags;
    UINT64                                EventTag;
    VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE EventCallingStage;
    BYTE                                  InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];
    UINT16                                ReadInstructionLen;

} DEBUGGEE_UD_PAUSED_PACKET, *PDEBUGGEE_UD_PAUSED_PACKET;

//////////////////////////////////////////////////
//            Message Tracing Enums             //
//////////////////////////////////////////////////

/**
 * @brief Type of transferring buffer between user-to-kernel
 *
 */
typedef enum _NOTIFY_TYPE
{
    IRP_BASED,
    EVENT_BASED
} NOTIFY_TYPE;

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

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
 * @brief Used to register event for transferring buffer between user-to-kernel
 *
 */
typedef struct _REGISTER_NOTIFY_BUFFER
{
    NOTIFY_TYPE Type;
    HANDLE      hEvent;

} REGISTER_NOTIFY_BUFFER, *PREGISTER_NOTIFY_BUFFER;

//////////////////////////////////////////////////
//                 Direct VMCALL                //
//////////////////////////////////////////////////

/**
 * @brief Used for sending direct VMCALLs on the VMX root-mode
 *
 */
typedef struct _DIRECT_VMCALL_PARAMETERS
{
    UINT64 OptionalParam1;
    UINT64 OptionalParam2;
    UINT64 OptionalParam3;

} DIRECT_VMCALL_PARAMETERS, *PDIRECT_VMCALL_PARAMETERS;

//////////////////////////////////////////////////
//             Syscall Callbacks                //
//////////////////////////////////////////////////

/**
 * @brief The (optional) context parameters for the transparent-mode
 *
 */
typedef struct _SYSCALL_CALLBACK_CONTEXT_PARAMS
{
    UINT64 OptionalParam1; // Optional parameter
    UINT64 OptionalParam2; // Optional parameter
    UINT64 OptionalParam3; // Optional parameter
    UINT64 OptionalParam4; // Optional parameter

} SYSCALL_CALLBACK_CONTEXT_PARAMS, *PSYSCALL_CALLBACK_CONTEXT_PARAMS;

//////////////////////////////////////////////////
//                  EPT Hook                    //
//////////////////////////////////////////////////

/**
 * @brief different type of memory addresses
 *
 */
typedef enum _DEBUGGER_HOOK_MEMORY_TYPE
{
    DEBUGGER_MEMORY_HOOK_VIRTUAL_ADDRESS,
    DEBUGGER_MEMORY_HOOK_PHYSICAL_ADDRESS
} DEBUGGER_HOOK_MEMORY_TYPE;

/**
 * @brief Temporary $context used in some EPT hook commands
 *
 */
typedef struct _EPT_HOOKS_CONTEXT
{
    UINT64 HookingTag; // This is same as the event tag
    UINT64 PhysicalAddress;
    UINT64 VirtualAddress;
} EPT_HOOKS_CONTEXT, *PEPT_HOOKS_CONTEXT;

/**
 * @brief Setting details for EPT Hooks (!monitor)
 *
 */
typedef struct _EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR
{
    UINT64                    StartAddress;
    UINT64                    EndAddress;
    BOOLEAN                   SetHookForRead;
    BOOLEAN                   SetHookForWrite;
    BOOLEAN                   SetHookForExec;
    DEBUGGER_HOOK_MEMORY_TYPE MemoryType;
    UINT64                    Tag;

} EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR, *PEPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR;

/**
 * @brief Setting details for EPT Hooks (!epthook2)
 *
 */
typedef struct _EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2
{
    PVOID TargetAddress;
    PVOID HookFunction;

} EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2, *PEPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2;

/**
 * @brief Details of unhooking single EPT hooks
 *
 */
typedef struct _EPT_SINGLE_HOOK_UNHOOKING_DETAILS
{
    BOOLEAN                     CallerNeedsToRestoreEntryAndInvalidateEpt;
    BOOLEAN                     RemoveBreakpointInterception;
    SIZE_T                      PhysicalAddress;
    UINT64 /* EPT_PML1_ENTRY */ OriginalEntry;

} EPT_SINGLE_HOOK_UNHOOKING_DETAILS, *PEPT_SINGLE_HOOK_UNHOOKING_DETAILS;

//////////////////////////////////////////////////
//                 Segment Types                //
//////////////////////////////////////////////////

/**
 * @brief Describe segment selector in VMX
 * @details This structure is copied from ia32.h to the SDK to
 * be used as a data type for functions
 *
 */
typedef union
{
    struct
    {
        /**
         * [Bits 3:0] Segment type.
         */
        UINT32 Type : 4;

        /**
         * [Bit 4] S - Descriptor type (0 = system; 1 = code or data).
         */
        UINT32 DescriptorType : 1;

        /**
         * [Bits 6:5] DPL - Descriptor privilege level.
         */
        UINT32 DescriptorPrivilegeLevel : 2;

        /**
         * [Bit 7] P - Segment present.
         */
        UINT32 Present : 1;

        UINT32 Reserved1 : 4;

        /**
         * [Bit 12] AVL - Available for use by system software.
         */
        UINT32 AvailableBit : 1;

        /**
         * [Bit 13] Reserved (except for CS). L - 64-bit mode active (for CS only).
         */
        UINT32 LongMode : 1;

        /**
         * [Bit 14] D/B - Default operation size (0 = 16-bit segment; 1 = 32-bit segment).
         */
        UINT32 DefaultBig : 1;

        /**
         * [Bit 15] G - Granularity.
         */
        UINT32 Granularity : 1;
        /**
         * [Bit 16] Segment unusable (0 = usable; 1 = unusable).
         */
        UINT32 Unusable : 1;
        UINT32 Reserved2 : 15;
    };

    UINT32 AsUInt;
} VMX_SEGMENT_ACCESS_RIGHTS_TYPE;

/**
 * @brief Segment selector
 *
 */
typedef struct _VMX_SEGMENT_SELECTOR
{
    UINT16                         Selector;
    VMX_SEGMENT_ACCESS_RIGHTS_TYPE Attributes;
    UINT32                         Limit;
    UINT64                         Base;
} VMX_SEGMENT_SELECTOR, *PVMX_SEGMENT_SELECTOR;
