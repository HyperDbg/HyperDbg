/**
 * @file Logging.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Message logging and tracing
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once
#include "Definition.h"

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief The usermode request
 * 
 */
typedef struct _NOTIFY_RECORD
{
    NOTIFY_TYPE Type;
    union
    {
        PKEVENT Event;
        PIRP    PendingIrp;
    } Message;
    KDPC    Dpc;
    BOOLEAN CheckVmxRootMessagePool; // Set so that notify callback can understand where to check (Vmx root or Vmx non-root)
} NOTIFY_RECORD, *PNOTIFY_RECORD;

/**
 * @brief Message buffer structure
 * 
 */
typedef struct _BUFFER_HEADER
{
    UINT32  OpeationNumber; // Operation ID to user-mode
    UINT32  BufferLength;   // The actual length
    BOOLEAN Valid;          // Determine whether the buffer was valid to send or not
} BUFFER_HEADER, *PBUFFER_HEADER;

/**
 * @brief Core-specific buffers
 * 
 */
typedef struct _LOG_BUFFER_INFORMATION
{
    UINT64 BufferStartAddress; // Start address of the buffer
    UINT64 BufferEndAddress;   // End address of the buffer

    UINT64 BufferForMultipleNonImmediateMessage; // Start address of the buffer for accumulating non-immadiate messages
    UINT32 CurrentLengthOfNonImmBuffer;          // the current size of the buffer for accumulating non-immadiate messages

    UINT32 CurrentIndexToSend;  // Current buffer index to send to user-mode
    UINT32 CurrentIndexToWrite; // Current buffer index to write new messages

} LOG_BUFFER_INFORMATION, *PLOG_BUFFER_INFORMATION;

// Each core has one of the structure in g_GuestState
typedef struct _DEBUGGER_CORE_EVENTS
{
    //
    // Warnings : Only list entries should be in this list, nothing else
    //

    //
    // Do not add varialbe to this this list, just LIST_ENTRY is allowed
    //
    LIST_ENTRY HiddenHookReadEventsHead;          // HIDDEN_HOOK_READ  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY HiddenHookWriteEventsHead;         // HIDDEN_HOOK_WRITE  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY HiddenHooksExecDetourEventsHead;   // HIDDEN_HOOK_EXEC_DETOUR [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY HiddenHookExecCcEventsHead;        // HIDDEN_HOOK_EXEC_CC [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY SyscallHooksEferSyscallEventsHead; // SYSCALL_HOOK_EFER_SYSCALL [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY SyscallHooksEferSysretEventsHead;  // SYSCALL_HOOK_EFER_SYSRET [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]

} DEBUGGER_CORE_EVENTS, *PDEBUGGER_CORE_EVENTS;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

/* Global Variable for buffer on all cores */
LOG_BUFFER_INFORMATION * MessageBufferInformation;

/* Vmx-root lock for logging */
volatile LONG VmxRootLoggingLock;

/* Vmx-root lock for logging */
volatile LONG VmxRootLoggingLockForNonImmBuffers;

/* Vmx non-root lock for logging */
volatile LONG VmxNonRootLoggingLock;

/* Vmx non-root lock for logging */
volatile LONG VmxNonRootLoggingLockForNonImmBuffers;


//////////////////////////////////////////////////
//					Illustration				//
//////////////////////////////////////////////////

/*
A core buffer is like this , it's divided into MaximumPacketsCapacity chucks,
each chunk has PacketChunkSize + sizeof(BUFFER_HEADER) size

			 _________________________
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|
			|						  |
			|						  |
			|						  |
			|						  |
			|			.			  |
			|			.			  |
			|			.			  |
			|						  |
			|						  |
			|						  |
			|						  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|						  |
			|           BODY		  |
			|         (Buffer)		  |
			| size = PacketChunkSize  |
			|						  |
			|_________________________|

*/

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
LogInitialize();
VOID
LogUnInitialize();
BOOLEAN
LogSendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength);
BOOLEAN
LogReadBuffer(BOOLEAN IsVmxRoot, PVOID BufferToSaveMessage, UINT32 * ReturnedLength);
BOOLEAN
LogCheckForNewMessage(BOOLEAN IsVmxRoot);
BOOLEAN
LogSendMessageToQueue(UINT32 OperationCode, BOOLEAN IsImmediateMessage, BOOLEAN ShowCurrentSystemTime, const char * Fmt, ...);
VOID
LogNotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
NTSTATUS
LogRegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
