/**
 * @file Logging.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Message logging and tracing implementation
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <ntddk.h>
#include <ntstrsafe.h>
#include <Windef.h>

#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_HYPER_LOG

#include "SDK/HyperDbgSdk.h"
#include "SDK/Modules/HyperLog.h"
#include "SDK/Imports/HyperDbgHyperLogImports.h"
#include "components/spinlock/header/Spinlock.h"
#include "Logging.h"

/**
 * @brief Checks whether the message tracing operates on vmx-root mode or not
 *
 * @return BOOLEAN
 */
BOOLEAN inline LogCheckVmxOperation()
{
    CHECK_VMX_OPERATION VmxOpeationCheck = g_MsgTracingCallbacks.VmxOpeationCheck;

    if (VmxOpeationCheck == NULL)
    {
        //
        // As the caller didn't defined a checker for vmx operation, we assume
        // that it's not operating on vmx-root
        //
        return FALSE;
    }

    //
    // The user specified a vmx checker
    //
    return VmxOpeationCheck();
}

/**
 * @brief Checks whether the immediate sending is needed or not
 *
 * @return BOOLEAN
 */
BOOLEAN inline LogCheckImmediateSend(UINT32 OperationCode)
{
    CHECK_IMMEDIATE_MESSAGE_SENDING ImmediateMessageCheck = g_MsgTracingCallbacks.CheckImmediateMessageSending;

    if (ImmediateMessageCheck == NULL)
    {
        //
        // As the caller didn't defined a checker we assume there is no
        // need to send messages immediately
        //
        return FALSE;
    }

    //
    // The user specified a vmx checker
    //
    return ImmediateMessageCheck(OperationCode);
}

/**
 * @brief Checks whether the immediate sending is needed or not
 * @param OptionalBuffer
 * @param OptionalBufferLength
 * @param OperationCode
 *
 * @return BOOLEAN
 */
BOOLEAN inline LogSendImmediateMessage(CHAR * OptionalBuffer,
                                       UINT32 OptionalBufferLength,
                                       UINT32 OperationCode)
{
    SEND_IMMEDIATE_MESSAGE SendImmediateMessage = g_MsgTracingCallbacks.SendImmediateMessage;

    if (SendImmediateMessage == NULL)
    {
        //
        // As the caller didn't defined a checker we assume there is no
        // need to send messages immediately
        //
        return FALSE;
    }

    //
    // The user specified a vmx checker
    //
    return SendImmediateMessage(OptionalBuffer, OptionalBufferLength, OperationCode);
}

/**
 * @brief Initialize the buffer relating to log message tracing
 * @param MsgTracingCallbacks specify the callbacks
 *
 * @return BOOLEAN
 */
BOOLEAN
LogInitialize(MESSAGE_TRACING_CALLBACKS * MsgTracingCallbacks)
{
    ULONG CoreCount = 0;

    CoreCount = KeQueryActiveProcessorCount(0);

    //
    // Initialize buffers for trace message and data messages
    //(we have two buffers one for vmx root and one for vmx non-root)
    //
    MessageBufferInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(LOG_BUFFER_INFORMATION) * 2, POOLTAG);

    if (!MessageBufferInformation)
    {
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES
    }

    //
    // Zeroing the memory
    //
    RtlZeroMemory(MessageBufferInformation, sizeof(LOG_BUFFER_INFORMATION) * 2);

    //
    // Allocate VmxTempMessage and VmxLogMessage
    //
    VmxTempMessage = NULL;
    VmxTempMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize * CoreCount, POOLTAG);

    if (!VmxTempMessage)
    {
        ExFreePoolWithTag(MessageBufferInformation, POOLTAG);
        MessageBufferInformation = NULL;
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES
    }

    VmxLogMessage = NULL;
    VmxLogMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize * CoreCount, POOLTAG);

    if (!VmxLogMessage)
    {
        ExFreePoolWithTag(MessageBufferInformation, POOLTAG);
        MessageBufferInformation = NULL;

        ExFreePoolWithTag(VmxTempMessage, POOLTAG);
        VmxTempMessage = NULL;

        return FALSE; // STATUS_INSUFFICIENT_RESOURCES
    }

    //
    // Initialize the lock for Vmx-root mode (HIGH_IRQL Spinlock)
    //
    VmxRootLoggingLock = 0;

    //
    // Allocate buffer for messages and initialize the core buffer information
    //
    for (int i = 0; i < 2; i++)
    {
        //
        // initialize the lock
        // Actually, only the 0th buffer use this spinlock but let initialize it
        // for both but the second buffer spinlock is useless
        // as we use our custom spinlock
        //
        KeInitializeSpinLock(&MessageBufferInformation[i].BufferLock);
        KeInitializeSpinLock(&MessageBufferInformation[i].BufferLockForNonImmMessage);

        //
        // allocate the buffer for regular buffers
        //
        MessageBufferInformation[i].BufferStartAddress                   = ExAllocatePoolWithTag(NonPagedPool, LogBufferSize, POOLTAG);
        MessageBufferInformation[i].BufferForMultipleNonImmediateMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize, POOLTAG);

        if (!MessageBufferInformation[i].BufferStartAddress ||
            !MessageBufferInformation[i].BufferForMultipleNonImmediateMessage)
        {
            return FALSE; // STATUS_INSUFFICIENT_RESOURCES
        }

        //
        // allocate the buffer for priority buffers
        //
        MessageBufferInformation[i].BufferStartAddressPriority = ExAllocatePoolWithTag(NonPagedPool, LogBufferSizePriority, POOLTAG);

        if (!MessageBufferInformation[i].BufferStartAddressPriority)
        {
            return FALSE; // STATUS_INSUFFICIENT_RESOURCES
        }

        //
        // Zeroing the buffer
        //
        RtlZeroMemory(MessageBufferInformation[i].BufferStartAddress, LogBufferSize);
        RtlZeroMemory(MessageBufferInformation[i].BufferForMultipleNonImmediateMessage, PacketChunkSize);
        RtlZeroMemory(MessageBufferInformation[i].BufferStartAddressPriority, LogBufferSizePriority);

        //
        // Set the end address
        //
        MessageBufferInformation[i].BufferEndAddress         = (UINT64)MessageBufferInformation[i].BufferStartAddress + LogBufferSize;
        MessageBufferInformation[i].BufferEndAddressPriority = (UINT64)MessageBufferInformation[i].BufferStartAddressPriority + LogBufferSizePriority;
    }

    //
    // Copy the callbacks into the global callback holder
    //
    RtlCopyBytes(&g_MsgTracingCallbacks, MsgTracingCallbacks, sizeof(MESSAGE_TRACING_CALLBACKS));

    return TRUE;
}

/**
 * @brief Uninitialize the buffer relating to log message tracing
 *
 * @return VOID
 */
VOID
LogUnInitialize()
{
    //
    // de-allocate buffer for messages and initialize the core buffer information (for vmx-root core)
    //
    for (int i = 0; i < 2; i++)
    {
        //
        // Free each buffers
        //
        ExFreePoolWithTag(MessageBufferInformation[i].BufferStartAddress, POOLTAG);
        ExFreePoolWithTag(MessageBufferInformation[i].BufferStartAddressPriority, POOLTAG);
        ExFreePoolWithTag(MessageBufferInformation[i].BufferForMultipleNonImmediateMessage, POOLTAG);
    }

    //
    // de-allocate buffers for trace message and data messages
    //
    ExFreePoolWithTag(MessageBufferInformation, POOLTAG);
    MessageBufferInformation = NULL;
}

/**
 * @brief Checks whether the priority or regular buffer is full or not
 *
 * @param Priority Whether the buffer has priority
 * @return BOOLEAN Returns true if the buffer is full, otherwise, return false
 */
BOOLEAN
LogCallbackCheckIfBufferIsFull(BOOLEAN Priority)
{
    UINT32  Index;
    BOOLEAN IsVmxRoot;
    UINT32  CurrentIndexToWrite;
    UINT32  CurrentIndexToWritePriority;

    //
    // Check that if we're in vmx root-mode
    //
    IsVmxRoot = LogCheckVmxOperation();

    if (IsVmxRoot)
    {
        //
        // Set the index
        //
        Index = 1;
    }
    else
    {
        //
        // Set the index
        //
        Index = 0;
    }

    //
    // check if the buffer is filled to it's maximum index or not
    //
    if (Priority)
    {
        CurrentIndexToWritePriority = MessageBufferInformation[Index].CurrentIndexToWritePriority;

        if (MessageBufferInformation[Index].CurrentIndexToWritePriority > MaximumPacketsCapacityPriority - 1)
        {
            //
            // start from the beginning
            //
            CurrentIndexToWritePriority = 0;
        }
    }
    else
    {
        CurrentIndexToWrite = MessageBufferInformation[Index].CurrentIndexToWrite;

        if (MessageBufferInformation[Index].CurrentIndexToWrite > MaximumPacketsCapacity - 1)
        {
            //
            // start from the beginning
            //
            CurrentIndexToWrite = 0;
        }
    }

    //
    // Compute the start of the buffer header
    //
    BUFFER_HEADER * Header;

    if (Priority)
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (CurrentIndexToWritePriority * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }
    else
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }

    //
    // If the next item is valid, then it means the buffer is full and the next
    // item will replace the previous (not served items)
    //
    return Header->Valid;
}

/**
 * @brief Save buffer to the pool
 *
 * @param OperationCode The operation code that will be send to user mode
 * @param Buffer Buffer to be send to user mode
 * @param BufferLength Length of the buffer
 * @param Priority Whether the buffer has priority
 * @return BOOLEAN Returns true if the buffer succssfully set to be
 * send to user mode and false if there was an error
 */
_Use_decl_annotations_
BOOLEAN
LogCallbackSendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength, BOOLEAN Priority)
{
    KIRQL   OldIRQL;
    UINT32  Index;
    BOOLEAN IsVmxRoot;

    if (BufferLength > PacketChunkSize - 1 || BufferLength == 0)
    {
        //
        // We can't save this huge buffer
        //
        return FALSE;
    }

    //
    // Check that if we're in vmx root-mode
    //
    IsVmxRoot = LogCheckVmxOperation();

    //
    // Check if we're connected to remote debugger, send it directly to the debugger
    // and the OPERATION_MANDATORY_DEBUGGEE_BIT should not be set because those operation
    // codes that their MSB are set should be handled locally
    //
    if (LogCheckImmediateSend(OperationCode))
    {
        //
        // if we're in vmx non-root then in order to avoid scheduling we raise the IRQL
        // to DISPATCH_LEVEL because we will get the lock of sending over serial in the
        // next function. In vmx-root RFLAGS.IF is cleared so no interrupt happens and
        // we're safe to get the lock, the same approach is for KeAcquireSpinLock
        //
        if (!IsVmxRoot)
        {
            //
            // vmx non-root
            //
            OldIRQL = KeRaiseIrqlToDpcLevel();
        }

        //
        // Kernel debugger is active, we should send the bytes over serial
        //
        LogSendImmediateMessage(
            Buffer,
            BufferLength,
            OperationCode);

        //
        // Release the vmx non-root lock
        //
        if (!IsVmxRoot)
        {
            //
            // vmx non-root
            //
            KeLowerIrql(OldIRQL);
        }

        return TRUE;
    }

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        //
        // Set the index
        //
        Index = 1;
        SpinlockLock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Set the index
        //
        Index = 0;

        //
        // Acquire the lock
        //
        KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
    }

    //
    // check if the buffer is filled to it's maximum index or not
    //
    if (Priority)
    {
        if (MessageBufferInformation[Index].CurrentIndexToWritePriority > MaximumPacketsCapacityPriority - 1)
        {
            //
            // start from the beginning
            //
            MessageBufferInformation[Index].CurrentIndexToWritePriority = 0;
        }
    }
    else
    {
        if (MessageBufferInformation[Index].CurrentIndexToWrite > MaximumPacketsCapacity - 1)
        {
            //
            // start from the beginning
            //
            MessageBufferInformation[Index].CurrentIndexToWrite = 0;
        }
    }

    //
    // Compute the start of the buffer header
    //
    BUFFER_HEADER * Header;

    if (Priority)
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (MessageBufferInformation[Index].CurrentIndexToWritePriority * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }
    else
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }

    //
    // Set the header
    //
    Header->OpeationNumber = OperationCode;
    Header->BufferLength   = BufferLength;
    Header->Valid          = TRUE;

    //
    // ******** Now it's time to fill the buffer ********
    //

    //
    // compute the saving index
    //
    PVOID SavingBuffer;

    if (Priority)
    {
        SavingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (MessageBufferInformation[Index].CurrentIndexToWritePriority * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
    }
    else
    {
        SavingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToWrite * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
    }

    //
    // Copy the buffer
    //
    RtlCopyBytes(SavingBuffer, Buffer, BufferLength);

    //
    // Increment the next index to write
    //
    if (Priority)
    {
        MessageBufferInformation[Index].CurrentIndexToWritePriority = MessageBufferInformation[Index].CurrentIndexToWritePriority + 1;
    }
    else
    {
        MessageBufferInformation[Index].CurrentIndexToWrite = MessageBufferInformation[Index].CurrentIndexToWrite + 1;
    }

    //
    // check if there is any thread in IRP Pending state, so we can complete their request
    //
    if (g_GlobalNotifyRecord != NULL)
    {
        //
        // there is some threads that needs to be completed
        //

        //
        // set the target pool
        //
        g_GlobalNotifyRecord->CheckVmxRootMessagePool = IsVmxRoot;

        //
        // Insert dpc to queue
        //
        KeInsertQueueDpc(&g_GlobalNotifyRecord->Dpc, g_GlobalNotifyRecord, NULL);

        //
        // set notify routine to null
        //
        g_GlobalNotifyRecord = NULL;
    }

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        SpinlockUnlock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Release the lock
        //
        KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
    }

    return TRUE;
}

/**
 * @brief Mark all buffers as read
 * @details Priority buffers won't be set as read
 *
 * @param IsVmxRoot Determine whether you want to read vmx root buffer or vmx non root buffer
 * @return UINT32 return count of messages that set to invalid
 */
UINT32
LogMarkAllAsRead(BOOLEAN IsVmxRoot)
{
    KIRQL  OldIRQL;
    UINT32 Index;
    UINT32 ResultsOfBuffersSetToRead = 0;

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        //
        // Set the index
        //
        Index = 1;

        //
        // Acquire the lock
        //
        SpinlockLock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Set the index
        //
        Index = 0;

        //
        // Acquire the lock
        //
        KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
    }

    //
    // We have iterate through the all indexes
    //
    for (size_t i = 0; i < MaximumPacketsCapacity; i++)
    {
        //
        // Compute the current buffer to read
        //
        BUFFER_HEADER * Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddress +
                                                   (MessageBufferInformation[Index].CurrentIndexToSend *
                                                    (PacketChunkSize + sizeof(BUFFER_HEADER))));

        if (!Header->Valid)
        {
            //
            // there is nothing to send
            //

            //
            // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
            // if not we use the windows spinlock
            //
            if (IsVmxRoot)
            {
                SpinlockUnlock(&VmxRootLoggingLock);
            }
            else
            {
                //
                // Release the lock
                //
                KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
            }

            return ResultsOfBuffersSetToRead;
        }

        //
        // If we reached here, means that there is sth to send
        //
        ResultsOfBuffersSetToRead++;

        //
        // Second, save the buffer contents
        //
        PVOID SendingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));

        //
        // Finally, set the current index to invalid as we sent it
        //
        Header->Valid = FALSE;

        //
        // Last step is to clear the current buffer (we can't do it once when CurrentIndexToSend is zero because
        // there might be multiple messages on the start of the queue that didn't read yet)
        // we don't free the header
        //
        RtlZeroMemory(SendingBuffer, Header->BufferLength);

        //
        // Check to see whether we passed the index or not
        //
        if (MessageBufferInformation[Index].CurrentIndexToSend > MaximumPacketsCapacity - 2)
        {
            MessageBufferInformation[Index].CurrentIndexToSend = 0;
        }
        else
        {
            //
            // Increment the next index to read
            //
            MessageBufferInformation[Index].CurrentIndexToSend = MessageBufferInformation[Index].CurrentIndexToSend + 1;
        }
    }

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        SpinlockUnlock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Release the lock
        //
        KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
    }

    return ResultsOfBuffersSetToRead;
}

/**
 * @brief Attempt to read the buffer
 *
 * @param IsVmxRoot Determine whether you want to read vmx root buffer or vmx non root buffer
 * @param BufferToSaveMessage Target buffer to save the message
 * @param ReturnedLength The actual length of the buffer that this function used it
 * @return BOOLEAN return of this function shows whether the read was successfull
 * or not (e.g FALSE shows there's no new buffer available.)
 */
BOOLEAN
LogReadBuffer(BOOLEAN IsVmxRoot, PVOID BufferToSaveMessage, UINT32 * ReturnedLength)
{
    KIRQL   OldIRQL;
    UINT32  Index;
    BOOLEAN PriorityMessageIsAvailable = FALSE;

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        //
        // Set the index
        //
        Index = 1;

        //
        // Acquire the lock
        //
        SpinlockLock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Set the index
        //
        Index = 0;

        //
        // Acquire the lock
        //
        KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLock, &OldIRQL);
    }

    //
    // Compute the current buffer to read
    //
    BUFFER_HEADER * Header;

    //
    // Check for priority message
    //
    Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (MessageBufferInformation[Index].CurrentIndexToSendPriority * (PacketChunkSize + sizeof(BUFFER_HEADER))));

    if (!Header->Valid)
    {
        //
        // Check for regular message
        //
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));

        if (!Header->Valid)
        {
            //
            // there is nothing to send
            //

            //
            // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
            // if not we use the windows spinlock
            //
            if (IsVmxRoot)
            {
                SpinlockUnlock(&VmxRootLoggingLock);
            }
            else
            {
                //
                // Release the lock
                //
                KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
            }

            return FALSE;
        }
    }
    else
    {
        PriorityMessageIsAvailable = TRUE;
    }

    //
    // If we reached here, means that there is sth to send
    //

    //
    // First copy the header
    //
    RtlCopyBytes(BufferToSaveMessage, &Header->OpeationNumber, sizeof(UINT32));

    //
    // Second, save the buffer contents
    //
    PVOID SendingBuffer;

    if (PriorityMessageIsAvailable)
    {
        SendingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (MessageBufferInformation[Index].CurrentIndexToSendPriority * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
    }
    else
    {
        SendingBuffer = ((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))) + sizeof(BUFFER_HEADER));
    }

    PVOID SavingAddress = ((UINT64)BufferToSaveMessage + sizeof(UINT32)); /* Because we want to pass the header of usermode header */
    RtlCopyBytes(SavingAddress, SendingBuffer, Header->BufferLength);

#if ShowMessagesOnDebugger

    //
    // Means that show just messages
    //
    if (Header->OpeationNumber <= OPERATION_LOG_NON_IMMEDIATE_MESSAGE)
    {
        //
        // We're in Dpc level here so it's safe to use DbgPrint
        // DbgPrint limitation is 512 Byte
        //
        if (Header->BufferLength > DbgPrintLimitation)
        {
            for (size_t i = 0; i <= Header->BufferLength / DbgPrintLimitation; i++)
            {
                if (i != 0)
                {
                    DbgPrint("%s", (char *)((UINT64)SendingBuffer + (DbgPrintLimitation * i) - 2));
                }
                else
                {
                    DbgPrint("%s", (char *)((UINT64)SendingBuffer + (DbgPrintLimitation * i)));
                }
            }
        }
        else
        {
            DbgPrint("%s", (char *)SendingBuffer);
        }
    }
#endif

    //
    // Finally, set the current index to invalid as we sent it
    //
    Header->Valid = FALSE;

    //
    // Set the length to show as the ReturnedByted in usermode ioctl funtion + size of header
    //
    *ReturnedLength = Header->BufferLength + sizeof(UINT32);

    //
    // Last step is to clear the current buffer (we can't do it once when CurrentIndexToSend is zero because
    // there might be multiple messages on the start of the queue that didn't read yet)
    // we don't free the header
    //
    RtlZeroMemory(SendingBuffer, Header->BufferLength);

    if (PriorityMessageIsAvailable)
    {
        //
        // Check to see whether we passed the index or not
        //
        if (MessageBufferInformation[Index].CurrentIndexToSendPriority > MaximumPacketsCapacityPriority - 2)
        {
            MessageBufferInformation[Index].CurrentIndexToSendPriority = 0;
        }
        else
        {
            //
            // Increment the next index to read
            //
            MessageBufferInformation[Index].CurrentIndexToSendPriority = MessageBufferInformation[Index].CurrentIndexToSendPriority + 1;
        }
    }
    else
    {
        //
        // Check to see whether we passed the index or not
        //
        if (MessageBufferInformation[Index].CurrentIndexToSend > MaximumPacketsCapacity - 2)
        {
            MessageBufferInformation[Index].CurrentIndexToSend = 0;
        }
        else
        {
            //
            // Increment the next index to read
            //
            MessageBufferInformation[Index].CurrentIndexToSend = MessageBufferInformation[Index].CurrentIndexToSend + 1;
        }
    }

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    if (IsVmxRoot)
    {
        SpinlockUnlock(&VmxRootLoggingLock);
    }
    else
    {
        //
        // Release the lock
        //
        KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLock, OldIRQL);
    }

    return TRUE;
}

/**
 * @brief Check if new message is available or not
 *
 * @param IsVmxRoot Check vmx root pool for message or check vmx non root pool
 * @param Priority Whether the buffer has priority
 *
 * @return BOOLEAN return of this function shows whether the read was successful or not
 * (e.g FALSE shows there's no new buffer available.)
 */
BOOLEAN
LogCheckForNewMessage(BOOLEAN IsVmxRoot, BOOLEAN Priority)
{
    KIRQL  OldIRQL;
    UINT32 Index;

    if (IsVmxRoot)
    {
        Index = 1;
    }
    else
    {
        Index = 0;
    }

    //
    // Compute the current buffer to read
    //
    BUFFER_HEADER * Header;

    if (Priority)
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddressPriority + (MessageBufferInformation[Index].CurrentIndexToSendPriority * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }
    else
    {
        Header = (BUFFER_HEADER *)((UINT64)MessageBufferInformation[Index].BufferStartAddress + (MessageBufferInformation[Index].CurrentIndexToSend * (PacketChunkSize + sizeof(BUFFER_HEADER))));
    }

    if (!Header->Valid)
    {
        //
        // there is nothing to send
        //
        return FALSE;
    }

    //
    // If we reached here, means that there is sth to send
    //
    return TRUE;
}

/**
 * @brief Prepare a printf-style message mapping and send string messages
 * and tracing for logging and monitoring
 *
 * @param OperationCode Optional operation code
 * @param IsImmediateMessage Should be sent immediately
 * @param ShowCurrentSystemTime Show system-time
 * @param Priority Whether the message has priority
 * @param Fmt Message format-string
 * @param ...
 * @return BOOLEAN if it was successful then return TRUE, otherwise returns FALSE
 */
BOOLEAN
LogCallbackPrepareAndSendMessageToQueueWrapper(UINT32       OperationCode,
                                               BOOLEAN      IsImmediateMessage,
                                               BOOLEAN      ShowCurrentSystemTime,
                                               BOOLEAN      Priority,
                                               const char * Fmt,
                                               va_list      ArgList)
{
    int     SprintfResult;
    size_t  WrittenSize;
    BOOLEAN IsVmxRootMode;
    BOOLEAN Result         = FALSE; // by default, we assume error happens
    char *  LogMessage     = NULL;
    char *  TempMessage    = NULL;
    char    TimeBuffer[20] = {0};
    ULONG   CoreId         = KeGetCurrentProcessorNumberEx(NULL);

    //
    // Set Vmx State
    //
    IsVmxRootMode = LogCheckVmxOperation();

    //
    // Set the buffer here, we avoid use stack (local variables) because stack might growth
    // and be problematic
    //
    if (IsVmxRootMode)
    {
        LogMessage  = &VmxLogMessage[CoreId * PacketChunkSize];
        TempMessage = &VmxTempMessage[CoreId * PacketChunkSize];
    }
    else
    {
        //
        // To avoid buffer collision and buffer re-writing in VMX non-root, allocate pool
        //
        LogMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize, POOLTAG);

        if (LogMessage == NULL)
        {
            //
            // Insufficient space
            //
            return FALSE;
        }

        TempMessage = ExAllocatePoolWithTag(NonPagedPool, PacketChunkSize, POOLTAG);

        if (TempMessage == NULL)
        {
            //
            // Insufficient space
            //
            ExFreePoolWithTag(LogMessage, POOLTAG);
            return FALSE;
        }
    }

    if (ShowCurrentSystemTime)
    {
        //
        // It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
        // it's better to use - 1
        //

        //
        // We won't use this because we can't use in any IRQL
        // Status = RtlStringCchVPrintfA(TempMessage, PacketChunkSize - 1, Fmt, ArgList);
        //
        SprintfResult = vsprintf_s(TempMessage, PacketChunkSize - 1, Fmt, ArgList);

        //
        // Check if the buffer passed the limit
        //
        if (SprintfResult == -1)
        {
            //
            // Probably the buffer is large that we can't store it
            //
            goto FreeBufferAndReturn;
        }

        //
        // Fill the above with timer
        //
        TIME_FIELDS   TimeFields;
        LARGE_INTEGER SystemTime, LocalTime;
        KeQuerySystemTime(&SystemTime);
        ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
        RtlTimeToTimeFields(&LocalTime, &TimeFields);
        //
        // We won't use this because we can't use in any IRQL
        // Status = RtlStringCchPrintfA(TimeBuffer, RTL_NUMBER_OF(TimeBuffer),
        //	"%02hd:%02hd:%02hd.%03hd", TimeFields.Hour,
        //	TimeFields.Minute, TimeFields.Second,
        //	TimeFields.Milliseconds);
        //
        //
        // Append time with previous message
        //
        // Status = RtlStringCchPrintfA(LogMessage, PacketChunkSize - 1, "(%s)\t %s", TimeBuffer, TempMessage);
        //

        //
        // this function probably run without error, so there is no need to check the return value
        //
        sprintf_s(TimeBuffer, RTL_NUMBER_OF(TimeBuffer), "%02hd:%02hd:%02hd.%03hd", TimeFields.Hour, TimeFields.Minute, TimeFields.Second, TimeFields.Milliseconds);

        //
        // Append time with previous message
        //
        SprintfResult = sprintf_s(LogMessage, PacketChunkSize - 1, "(%s - core : %d - vmx-root? %s)\t %s", TimeBuffer, CoreId, IsVmxRootMode ? "yes" : "no", TempMessage);

        //
        // Check if the buffer passed the limit
        //
        if (SprintfResult == -1)
        {
            //
            // Probably the buffer is large that we can't store it
            //
            goto FreeBufferAndReturn;
        }
    }
    else
    {
        //
        // It's actually not necessary to use -1 but because user-mode code might assume a null-terminated buffer so
        // it's better to use - 1
        //

        //
        // We won't use this because we can't use in any IRQL
        // Status = RtlStringCchVPrintfA(LogMessage, PacketChunkSize - 1, Fmt, ArgList);
        //
        SprintfResult = vsprintf_s(LogMessage, PacketChunkSize - 1, Fmt, ArgList);

        //
        // Check if the buffer passed the limit
        //
        if (SprintfResult == -1)
        {
            //
            // Probably the buffer is large that we can't store it
            //
            goto FreeBufferAndReturn;
        }
    }

    //
    // Use std function because they can be run in any IRQL
    // RtlStringCchLengthA(LogMessage, PacketChunkSize - 1, &WrittenSize);
    //
    WrittenSize = strnlen_s(LogMessage, PacketChunkSize - 1);

    if (LogMessage[0] == '\0')
    {
        //
        // nothing to write
        //
        goto FreeBufferAndReturn;
    }

    //
    // Send the prepared buffer (with no priority)
    //
    Result = LogCallbackSendMessageToQueue(OperationCode, IsImmediateMessage, LogMessage, WrittenSize, Priority);

FreeBufferAndReturn:

    if (!IsVmxRootMode)
    {
        ExFreePoolWithTag(LogMessage, POOLTAG);
        ExFreePoolWithTag(TempMessage, POOLTAG);
    }

    return Result;
}

/**
 * @brief Prepare a printf-style message mapping and send string messages
 * and tracing for logging and monitoring
 *
 * @param OperationCode Optional operation code
 * @param IsImmediateMessage Should be sent immediately
 * @param ShowCurrentSystemTime Show system-time
 * @param Priority Whether the message has priority
 * @param Fmt Message format-string
 * @param ...
 * @return BOOLEAN if it was successful then return TRUE, otherwise returns FALSE
 */
BOOLEAN
LogCallbackPrepareAndSendMessageToQueue(UINT32       OperationCode,
                                        BOOLEAN      IsImmediateMessage,
                                        BOOLEAN      ShowCurrentSystemTime,
                                        BOOLEAN      Priority,
                                        const char * Fmt,
                                        ...)
{
    va_list ArgList;
    BOOLEAN Result;

    va_start(ArgList, Fmt);

    Result = LogCallbackPrepareAndSendMessageToQueueWrapper(OperationCode,
                                                            IsImmediateMessage,
                                                            ShowCurrentSystemTime,
                                                            Priority,
                                                            Fmt,
                                                            ArgList);

    va_end(ArgList);
}

/**
 * @brief Send string messages and tracing for logging and monitoring
 *
 * @param OperationCode Optional operation code
 * @param IsImmediateMessage Should be sent immediately
 * @param LogMessage Link of message buffer
 * @param BufferLen Length of buffer
 * @param Priority Whether the buffer has priority
 *
 * @return BOOLEAN if it was successful then return TRUE, otherwise returns FALSE
 */
BOOLEAN
LogCallbackSendMessageToQueue(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority)
{
    BOOLEAN Result;
    UINT32  Index;
    KIRQL   OldIRQL;
    BOOLEAN IsVmxRootMode;

    //
    // Set Vmx State
    //
    IsVmxRootMode = LogCheckVmxOperation();

#if UseWPPTracing

    if (OperationCode == OPERATION_LOG_INFO_MESSAGE)
    {
        HypervisorTraceLevelMessage(
            TRACE_LEVEL_INFORMATION, // ETW Level defined in evntrace.h
            HVFS_LOG_INFO,
            "%s", // Flag defined in WPP_CONTROL_GUIDS
            LogMessage);
    }
    else if (OperationCode == OPERATION_LOG_WARNING_MESSAGE)
    {
        HypervisorTraceLevelMessage(
            TRACE_LEVEL_WARNING, // ETW Level defined in evntrace.h
            HVFS_LOG_WARNING,
            "%s", // Flag defined in WPP_CONTROL_GUIDS
            LogMessage);
    }
    else if (OperationCode == OPERATION_LOG_ERROR_MESSAGE)
    {
        HypervisorTraceLevelMessage(
            TRACE_LEVEL_ERROR, // ETW Level defined in evntrace.h
            HVFS_LOG_ERROR,
            "%s", // Flag defined in WPP_CONTROL_GUIDS
            LogMessage);
    }
    else
    {
        HypervisorTraceLevelMessage(
            TRACE_LEVEL_NONE, // ETW Level defined in evntrace.h
            HVFS_LOG,
            "%s", // Flag defined in WPP_CONTROL_GUIDS
            LogMessage);
    }

#else
    if (IsImmediateMessage)
    {
        return LogCallbackSendBuffer(OperationCode, LogMessage, BufferLen, Priority);
    }
    else
    {
        //
        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //
        if (IsVmxRootMode)
        {
            //
            // Set the index
            //
            Index = 1;
            SpinlockLock(&VmxRootLoggingLockForNonImmBuffers);
        }
        else
        {
            //
            // Set the index
            //
            Index = 0;

            //
            // Acquire the lock
            //
            KeAcquireSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, &OldIRQL);
        }
        //
        // Set the result to True
        //
        Result = TRUE;

        //
        // If log message WrittenSize is above the buffer then we have to send the previous buffer
        //
        if ((MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer + BufferLen) > PacketChunkSize - 1 && MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer != 0)
        {
            //
            // Send the previous buffer (non-immediate message),
            // accumulated messages don't have priority
            //
            Result = LogCallbackSendBuffer(OPERATION_LOG_NON_IMMEDIATE_MESSAGE,
                                           MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage,
                                           MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer,
                                           FALSE);

            //
            // Free the immediate buffer
            //
            MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer = 0;
            RtlZeroMemory(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage, PacketChunkSize);
        }

        //
        // We have to save the message
        //
        RtlCopyBytes(MessageBufferInformation[Index].BufferForMultipleNonImmediateMessage +
                         MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer,
                     LogMessage,
                     BufferLen);

        //
        // add the length
        //
        MessageBufferInformation[Index].CurrentLengthOfNonImmBuffer += BufferLen;

        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //
        if (IsVmxRootMode)
        {
            SpinlockUnlock(&VmxRootLoggingLockForNonImmBuffers);
        }
        else
        {
            //
            // Release the lock
            //
            KeReleaseSpinLock(&MessageBufferInformation[Index].BufferLockForNonImmMessage, OldIRQL);
        }

        return Result;
    }
#endif
}

/**
 * @brief Complete the IRP in IRP Pending state and fill the usermode buffers with pool data
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
LogNotifyUsermodeCallback(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PNOTIFY_RECORD NotifyRecord;
    PIRP           Irp;
    UINT32         Length;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NotifyRecord = DeferredContext;

    ASSERT(NotifyRecord != NULL); // can't be NULL
    _Analysis_assume_(NotifyRecord != NULL);

    switch (NotifyRecord->Type)
    {
    case IRP_BASED:
        Irp = NotifyRecord->Message.PendingIrp;

        if (Irp != NULL)
        {
            PCHAR              OutBuff;       // pointer to output buffer
            ULONG              InBuffLength;  // Input buffer length
            ULONG              OutBuffLength; // Output buffer length
            PIO_STACK_LOCATION IrpSp;

            //
            // Make suree that concurrent calls to notify function never occurs
            //
            if (!(Irp->CurrentLocation <= Irp->StackCount + 1))
            {
                DbgPrint("Err, probably two or more functions called DPC for an object");
                return;
            }

            IrpSp         = IoGetCurrentIrpStackLocation(Irp);
            InBuffLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength || !OutBuffLength)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
            }

            //
            // Check again that SystemBuffer is not null
            //
            if (!Irp->AssociatedIrp.SystemBuffer)
            {
                //
                // Buffer is invalid
                //
                return;
            }

            OutBuff = Irp->AssociatedIrp.SystemBuffer;
            Length  = 0;

            //
            // Read Buffer might be empty (nothing to send)
            //
            if (!LogReadBuffer(NotifyRecord->CheckVmxRootMessagePool, OutBuff, &Length))
            {
                //
                // we have to return here as there is nothing to send here
                //
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
            }

            Irp->IoStatus.Information = Length;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        break;

    case EVENT_BASED:
        //
        // Signal the Event created in user-mode.
        //
        KeSetEvent(NotifyRecord->Message.Event, 0, FALSE);

        //
        // Dereference the object as we are done with it.
        //
        ObDereferenceObject(NotifyRecord->Message.Event);

        break;

    default:
        ASSERT(FALSE);
        break;
    }

    if (NotifyRecord != NULL)
    {
        ExFreePoolWithTag(NotifyRecord, POOLTAG);
    }
}

/**
 * @brief Register a new IRP Pending thread which listens for new buffers
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
LogRegisterIrpBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PNOTIFY_RECORD          NotifyRecord;
    PIO_STACK_LOCATION      IrpStack;
    KIRQL                   OOldIrql;
    PREGISTER_NOTIFY_BUFFER RegisterEvent;

    //
    // check if current core has another thread with pending IRP,
    // if no then put the current thread to pending
    // otherwise return and complete thread with STATUS_SUCCESS as
    // there is another thread waiting for message
    //

    if (g_GlobalNotifyRecord == NULL)
    {
        IrpStack      = IoGetCurrentIrpStackLocation(Irp);
        RegisterEvent = (PREGISTER_NOTIFY_BUFFER)Irp->AssociatedIrp.SystemBuffer;

        //
        // Allocate a record and save all the event context
        //
        NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), POOLTAG);

        if (NULL == NotifyRecord)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NotifyRecord->Type               = IRP_BASED;
        NotifyRecord->Message.PendingIrp = Irp;

        KeInitializeDpc(&NotifyRecord->Dpc,        // Dpc
                        LogNotifyUsermodeCallback, // DeferredRoutine
                        NotifyRecord               // DeferredContext
        );

        IoMarkIrpPending(Irp);

        //
        // check for new message (for both Vmx-root mode or Vmx non root-mode)
        // First, we check for priority messages in both buffers then we check
        // for regular messages
        //
        if (LogCheckForNewMessage(FALSE, TRUE))
        {
            //
            // check vmx non-root (priority buffers)
            //
            NotifyRecord->CheckVmxRootMessagePool = FALSE;

            //
            // Insert dpc to queue
            //
            KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
        }
        else if (LogCheckForNewMessage(TRUE, TRUE))
        {
            //
            // check vmx root (priority buffers)
            //
            NotifyRecord->CheckVmxRootMessagePool = TRUE;
            //
            // Insert dpc to queue
            //
            KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
        }
        else if (LogCheckForNewMessage(FALSE, FALSE))
        {
            //
            // check vmx non-root
            //
            NotifyRecord->CheckVmxRootMessagePool = FALSE;

            //
            // Insert dpc to queue
            //
            KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
        }
        else if (LogCheckForNewMessage(TRUE, FALSE))
        {
            //
            // check vmx root
            //
            NotifyRecord->CheckVmxRootMessagePool = TRUE;
            //
            // Insert dpc to queue
            //
            KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);
        }
        else
        {
            //
            // Set the notify routine to the global structure
            //
            g_GlobalNotifyRecord = NotifyRecord;
        }
        //
        // We will return pending as we have marked the IRP pending
        //
        return STATUS_PENDING;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

/**
 * @brief Create an event-based usermode notifying mechanism
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
LogRegisterEventBasedNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PNOTIFY_RECORD          NotifyRecord;
    NTSTATUS                Status;
    PIO_STACK_LOCATION      IrpStack;
    PREGISTER_NOTIFY_BUFFER RegisterEvent;
    KIRQL                   OldIrql;

    IrpStack      = IoGetCurrentIrpStackLocation(Irp);
    RegisterEvent = (PREGISTER_NOTIFY_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    //
    // Allocate a record and save all the event context
    //
    NotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), POOLTAG);

    if (NULL == NotifyRecord)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NotifyRecord->Type = EVENT_BASED;

    KeInitializeDpc(&NotifyRecord->Dpc,        // Dpc
                    LogNotifyUsermodeCallback, // DeferredRoutine
                    NotifyRecord               // DeferredContext
    );

    //
    // Get the object pointer from the handle
    // Note we must be in the context of the process that created the handle
    //
    Status = ObReferenceObjectByHandle(RegisterEvent->hEvent,
                                       SYNCHRONIZE | EVENT_MODIFY_STATE,
                                       *ExEventObjectType,
                                       Irp->RequestorMode,
                                       &NotifyRecord->Message.Event,
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("Err, unable to reference user mode event object, status = 0x%x", Status);
        ExFreePoolWithTag(NotifyRecord, POOLTAG);
        return Status;
    }

    //
    // Insert dpc to the queue
    //
    KeInsertQueueDpc(&NotifyRecord->Dpc, NotifyRecord, NULL);

    return STATUS_SUCCESS;
}
