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

    KSPIN_LOCK BufferLock;                 // SpinLock to protect access to the queue
    KSPIN_LOCK BufferLockForNonImmMessage; // SpinLock to protect access to the queue of non-imm messages

    UINT32 CurrentIndexToSend;  // Current buffer index to send to user-mode
    UINT32 CurrentIndexToWrite; // Current buffer index to write new messages

} LOG_BUFFER_INFORMATION, *PLOG_BUFFER_INFORMATION;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

/**
 * @brief Global Variable for buffer on all cores
 * 
 */
LOG_BUFFER_INFORMATION * MessageBufferInformation;

/**
 * @brief Vmx-root lock for logging
 * 
 */
volatile LONG VmxRootLoggingLock;

/**
 * @brief Vmx-root lock for logging
 * 
 */
volatile LONG VmxRootLoggingLockForNonImmBuffers;

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

UINT32
LogMarkAllAsRead(BOOLEAN IsVmxRoot);

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
