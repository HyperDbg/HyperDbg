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
#define SIZEOF_REGISTER_EVENT sizeof(REGISTER_NOTIFY_BUFFER)
#define DbgPrintLimitation 512

//////////////////////////////////////////////////
//					Events                      //
//////////////////////////////////////////////////

typedef enum _NOTIFY_TYPE { IRP_BASED, EVENT_BASED } NOTIFY_TYPE;

typedef struct _REGISTER_NOTIFY_BUFFER {
  NOTIFY_TYPE Type;
  HANDLE hEvent;

} REGISTER_NOTIFY_BUFFER, *PREGISTER_NOTIFY_BUFFER;

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
//				Debugger Structs                //
//////////////////////////////////////////////////

#define DEBUGGER_EVENT_APPLY_TO_ALL_CORES 0xffffffff

typedef struct _DEBUGGER_EVENT_REQUEST_BUFFER {
  BOOLEAN EnabledRequestBuffer;
  UINT32 RequestBufferSize;
  UINT64 RequstBufferAddress;

} DEBUGGER_EVENT_REQUEST_BUFFER, *PDEBUGGER_EVENT_REQUEST_BUFFER;

typedef enum _DEBUGGER_EVENT_ACTION_TYPE_ENUM {
  BREAK_TO_DEBUGGER,
  LOG_THE_STATES,
  RUN_CUSTOM_CODE

} DEBUGGER_EVENT_ACTION_TYPE_ENUM;

typedef struct _DEBUGGER_EVENT_ACTION {
  DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType;
  BOOLEAN ImmediatelySendTheResults;
  DEBUGGER_EVENT_REQUEST_BUFFER Buffer; // If enabled

} DEBUGGER_EVENT_ACTION, *PDEBUGGER_EVENT_ACTION;

typedef enum _DEBUGGER_EVENT_TYPE_ENUM {
  HIDDEN_HOOK_RW,
  HIDDEN_HOOK_EXEC_DETOUR,
  HIDDEN_HOOK_EXEC_CC,
  SYSCALL_HOOK_EFER,

} DEBUGGER_EVENT_TYPE_ENUM;

typedef struct _DEBUGGER_EVENT {
  UINT64 Tag;
  DEBUGGER_EVENT_TYPE_ENUM EventType;
  BOOLEAN Enabled;
  UINT32 CoreId; // determines the core index to apply this event to, if it's
                 // 0xffffffff means that we have to apply it to all cores
  LIST_ENTRY Actions;           // Each entry is in DEBUGGER_EVENT_ACTION struct
  UINT32 ConditionsBufferSize;  // if null, means uncoditional
  PVOID ConditionBufferAddress; // Address of the condition buffer (most of the
                                // time at the end of this buffer)

} DEBUGGER_EVENT, *PDEBUGGER_EVENT;

// Each core has one of the structure in g_GuestState
typedef struct _DEBUGGER_CORE_EVENTS {

  LIST_ENTRY HiddenHookRwEvents;          // HIDDEN_HOOK_RW
  LIST_ENTRY HiddenHooksExecDetourEvents; // HIDDEN_HOOK_EXEC_DETOUR
  LIST_ENTRY HiddenHookExecCcEvents;      // HIDDEN_HOOK_EXEC_CC
  LIST_ENTRY SyscallHooksEferEvents;      // SYSCALL_HOOK_EFER

} DEBUGGER_CORE_EVENTS, *PDEBUGGER_CORE_EVENTS;

//////////////////////////////////////////////////
//					IOCTLs                      //
//////////////////////////////////////////////////

#define IOCTL_REGISTER_EVENT                                                   \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TERMINATE_VMX                                                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEBUGGER_EPT_SYSCALL_HOOK_EFER                                   \
  CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUGGER_EPT_SYSCALL_HOOK_EFER,                \
           METHOD_BUFFERED, FILE_ANY_ACCESS)
