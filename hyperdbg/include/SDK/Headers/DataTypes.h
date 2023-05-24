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
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

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
 * @brief The structure of pausing packet in kHyperDbg
 *
 */
typedef struct _DEBUGGEE_KD_PAUSED_PACKET
{
    UINT64                  Rip;
    BOOLEAN                 Is32BitAddress;      // if true shows that the address should be interpreted in 32-bit mode
    BOOLEAN                 IgnoreDisassembling; // if check if diassembling should be ignored or not
    DEBUGGEE_PAUSING_REASON PausingReason;
    ULONG                   CurrentCore;
    UINT64                  EventTag;
    UINT64                  Rflags;
    BYTE                    InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];
    UINT16                  ReadInstructionLen;

} DEBUGGEE_KD_PAUSED_PACKET, *PDEBUGGEE_KD_PAUSED_PACKET;

/* ==============================================================================================
 */

/**
 * @brief The structure of pausing packet in uHyperDbg
 *
 */
typedef struct _DEBUGGEE_UD_PAUSED_PACKET
{
    UINT64                  Rip;
    UINT64                  ProcessDebuggingToken;
    BOOLEAN                 Is32Bit; // if true shows that the address should be interpreted in 32-bit mode
    DEBUGGEE_PAUSING_REASON PausingReason;
    UINT32                  ProcessId;
    UINT32                  ThreadId;
    UINT64                  EventTag;
    UINT64                  Rflags;
    BYTE                    InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];
    UINT16                  ReadInstructionLen;
    GUEST_REGS              GuestRegs;

} DEBUGGEE_UD_PAUSED_PACKET, *PDEBUGGEE_UD_PAUSED_PACKET;

/**
 * @brief check so the DEBUGGEE_UD_PAUSED_PACKET should be smaller than packet size
 *
 */
static_assert(sizeof(DEBUGGEE_UD_PAUSED_PACKET) < PacketChunkSize,
              "err (static_assert), size of PacketChunkSize should be bigger than DEBUGGEE_UD_PAUSED_PACKET");

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
//                  EPT Hook                    //
//////////////////////////////////////////////////

/**
 * @brief Temporary $context used in some EPT hook commands
 *
 */
typedef struct _EPT_HOOKS_CONTEXT
{
    UINT64 PhysicalAddress;
    UINT64 VirtualAddress;
} EPT_HOOKS_CONTEXT, *PEPT_HOOKS_CONTEXT;

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
