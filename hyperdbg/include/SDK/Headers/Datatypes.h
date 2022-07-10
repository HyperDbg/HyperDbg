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

/**
 * @brief The structure of pausing packet in kHyperDbg
 *
 */
typedef struct _DEBUGGEE_KD_PAUSED_PACKET
{
    UINT64                  Rip;
    BOOLEAN                 Is32BitAddress; // if true shows that the address should be interpreted in 32-bit mode
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
//                  Debugger                    //
//////////////////////////////////////////////////

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
 * @brief The structure of message packet in HyperDbg
 *
 */
typedef struct _DEBUGGEE_MESSAGE_PACKET
{
    UINT32 OperationCode;
    CHAR   Message[PacketChunkSize];

} DEBUGGEE_MESSAGE_PACKET, *PDEBUGGEE_MESSAGE_PACKET;