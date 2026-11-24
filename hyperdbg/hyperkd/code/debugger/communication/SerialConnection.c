/**
 * @file SerialConnection.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Serial port connection from debuggee to debugger
 * @details
 * @version 0.1
 * @date 2020-12-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief A simple connection test
 *
 * @return VOID
 */
VOID
SerialConnectionTest()
{
    for (SIZE_T i = 0; i < 100; i++)
    {
        KdHyperDbgTest((UINT16)i);
    }
}

/**
 * @brief Send end of buffer packet
 *
 * @return VOID
 */
VOID
SerialConnectionSendEndOfBuffer()
{
    //
    // Send the end buffer
    //
    KdHyperDbgSendByte(SERIAL_END_OF_BUFFER_CHAR_1, TRUE);
    KdHyperDbgSendByte(SERIAL_END_OF_BUFFER_CHAR_2, TRUE);
    KdHyperDbgSendByte(SERIAL_END_OF_BUFFER_CHAR_3, TRUE);
    KdHyperDbgSendByte(SERIAL_END_OF_BUFFER_CHAR_4, TRUE);
}

/**
 * @brief compares the buffer with a string
 *
 * @param CurrentLoopIndex Number of previously read bytes
 * @param Buffer
 * @return BOOLEAN
 */
BOOLEAN
SerialConnectionCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE * Buffer)
{
    UINT32 ActualBufferLength;

    ActualBufferLength = *CurrentLoopIndex;

    //
    // End of buffer is 4 character long
    //
    if (*CurrentLoopIndex <= 3)
    {
        return FALSE;
    }

    if (Buffer[ActualBufferLength] == SERIAL_END_OF_BUFFER_CHAR_4 &&
        Buffer[ActualBufferLength - 1] == SERIAL_END_OF_BUFFER_CHAR_3 &&
        Buffer[ActualBufferLength - 2] == SERIAL_END_OF_BUFFER_CHAR_2 &&
        Buffer[ActualBufferLength - 3] == SERIAL_END_OF_BUFFER_CHAR_1)
    {
        //
        // Clear the end character
        //
        Buffer[ActualBufferLength - 3] = NULL_ZERO;
        Buffer[ActualBufferLength - 2] = NULL_ZERO;
        Buffer[ActualBufferLength - 1] = NULL_ZERO;
        Buffer[ActualBufferLength]     = NULL_ZERO;

        //
        // Set the new length
        //
        *CurrentLoopIndex = ActualBufferLength - 3;

        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Number of pause instructions after a receive poll returns no byte
 *
 */
#define SERIAL_CONNECTION_RECEIVE_POLL_PAUSE 64

/**
 * @brief Try to receive a byte without waiting for the response lock
 *
 * USB send and receive operations share state and use the same lock.
 *
 * @param RecvChar Receives the byte
 * @return BOOLEAN TRUE if a byte is received
 */
static BOOLEAN
SerialConnectionRecvByteUnderResponseLock(PUCHAR RecvChar)
{
    BOOLEAN Result = FALSE;

    if (SpinlockTryLock(&DebuggerResponseLock))
    {
        Result = KdHyperDbgRecvByte(RecvChar);

        SpinlockUnlock(&DebuggerResponseLock);
    }

    if (!Result)
    {
        //
        // Give waiting senders a chance to take the lock
        //
        for (UINT32 i = 0; i < SERIAL_CONNECTION_RECEIVE_POLL_PAUSE; i++)
        {
            CpuPause();
        }
    }

    return Result;
}

/**
 * @brief Discard bytes until the next end of buffer marker to re-align the
 * stream to a frame boundary after a desync
 *
 * @return BOOLEAN TRUE if a marker was found (stream re-aligned), FALSE if too
 * many bytes arrived without one (treat the link as dead)
 */
BOOLEAN
SerialConnectionResyncToNextFrame()
{
    BYTE   Window[4] = {NULL_ZERO, NULL_ZERO, NULL_ZERO, NULL_ZERO};
    UINT32 Discarded = 0;

    while (Discarded < SERIAL_RESYNC_MAX_BYTES)
    {
        UCHAR RecvChar = NULL_ZERO;

        if (!SerialConnectionRecvByteUnderResponseLock(&RecvChar))
        {
            continue;
        }

        Window[0] = Window[1];
        Window[1] = Window[2];
        Window[2] = Window[3];
        Window[3] = RecvChar;
        Discarded++;

        if (Window[0] == SERIAL_END_OF_BUFFER_CHAR_1 &&
            Window[1] == SERIAL_END_OF_BUFFER_CHAR_2 &&
            Window[2] == SERIAL_END_OF_BUFFER_CHAR_3 &&
            Window[3] == SERIAL_END_OF_BUFFER_CHAR_4)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief Receive packet from the debugger
 *
 * @param BufferToSave
 * @param LengthReceived
 *
 * @return BOOLEAN
 */
BOOLEAN
SerialConnectionRecvBuffer(CHAR *   BufferToSave,
                           UINT32 * LengthReceived)
{
    UINT32 Loop = 0;

    //
    // Read data and store in a buffer
    //
    while (TRUE)
    {
        UCHAR RecvChar = NULL_ZERO;

        if (!SerialConnectionRecvByteUnderResponseLock(&RecvChar))
        {
            continue;
        }

        //
        // We already now that the maximum packet size is MaxSerialPacketSize
        // Check to make sure that we don't pass the boundaries
        //
        if (!(MaxSerialPacketSize > Loop))
        {
            //
            // Overflowed without an end of buffer marker, so the stream is
            // desynced (the debugger most likely dropped the link mid-frame
            // without sending the close packet). Returning FALSE here sends the
            // caller straight back into the same desynced stream, which
            // overflows again at once and floods the log. Log once per episode
            // and resync to the next frame boundary instead.
            //
            if (!g_SerialConnectionDesyncReported)
            {
                LogWarning("Warning, serial stream desynced (exceeded the buffer "
                           "limitation with no end marker); resyncing to the next frame");
                g_SerialConnectionDesyncReported = TRUE;
            }

            if (!SerialConnectionResyncToNextFrame())
            {
                //
                // Too many bytes without a marker, treat the link as dead
                //
                return FALSE;
            }

            //
            // Re-aligned to a frame boundary, start a fresh frame
            //
            Loop = 0;
            continue;
        }

        BufferToSave[Loop] = RecvChar;

        if (SerialConnectionCheckForTheEndOfTheBuffer(&Loop, (BYTE *)BufferToSave))
        {
            break;
        }

        Loop++;
    }

    //
    // A full frame arrived, so the stream is back in sync
    //
    g_SerialConnectionDesyncReported = FALSE;

    //
    // Set the length
    //
    *LengthReceived = Loop;

    return TRUE;
}

/**
 * @brief Perform sending buffer over serial
 *
 * @param Buffer buffer to send
 * @param Length length of buffer to send
 * @return BOOLEAN
 */
BOOLEAN
SerialConnectionSend(CHAR * Buffer, UINT32 Length)
{
    //
    // Check if buffer not pass the boundary
    //
    if (Length + SERIAL_END_OF_BUFFER_CHARS_COUNT > MaxSerialPacketSize)
    {
        LogError("Err, buffer is above the maximum buffer size that can be sent to debuggee (%d > %d), "
                 "for more information, please visit https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/increase-communication-buffer-size",
                 Length + SERIAL_END_OF_BUFFER_CHARS_COUNT,
                 MaxSerialPacketSize);
        return FALSE;
    }

    for (SIZE_T i = 0; i < Length; i++)
    {
        KdHyperDbgSendByte(Buffer[i], TRUE);
    }

    //
    // Send the end buffer
    //
    SerialConnectionSendEndOfBuffer();

    return TRUE;
}

/**
 * @brief Perform sending 2 not appended buffers over serial
 *
 * @param Buffer1 buffer to send
 * @param Length1 length of buffer to send
 * @param Buffer2 buffer to send
 * @param Length2 length of buffer to send
 * @return BOOLEAN
 */
BOOLEAN
SerialConnectionSendTwoBuffers(CHAR * Buffer1, UINT32 Length1, CHAR * Buffer2, UINT32 Length2)
{
    //
    // Check if buffer not pass the boundary
    //
    if ((Length1 + Length2 + SERIAL_END_OF_BUFFER_CHARS_COUNT) > MaxSerialPacketSize)
    {
        LogError("Err, buffer is above the maximum buffer size that can be sent to debuggee (%d > %d), "
                 "for more information, please visit https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/increase-communication-buffer-size",
                 Length1 + Length2 + SERIAL_END_OF_BUFFER_CHARS_COUNT,
                 MaxSerialPacketSize);
        return FALSE;
    }

    //
    // Send first buffer
    //
    for (SIZE_T i = 0; i < Length1; i++)
    {
        KdHyperDbgSendByte(Buffer1[i], TRUE);
    }

    //
    // Send second buffer
    //
    for (SIZE_T i = 0; i < Length2; i++)
    {
        KdHyperDbgSendByte(Buffer2[i], TRUE);
    }

    //
    // Send the end buffer
    //
    SerialConnectionSendEndOfBuffer();

    return TRUE;
}

/**
 * @brief Perform sending 3 not appended buffers over serial
 *
 * @param Buffer1 buffer to send
 * @param Length1 length of buffer to send
 * @param Buffer2 buffer to send
 * @param Length2 length of buffer to send
 * @param Buffer3 buffer to send
 * @param Length3 length of buffer to send
 * @return BOOLEAN
 */
BOOLEAN
SerialConnectionSendThreeBuffers(CHAR * Buffer1,
                                 UINT32 Length1,
                                 CHAR * Buffer2,
                                 UINT32 Length2,
                                 CHAR * Buffer3,
                                 UINT32 Length3)
{
    //
    // Check if buffer not pass the boundary
    //
    if ((Length1 + Length2 + Length3 + SERIAL_END_OF_BUFFER_CHARS_COUNT) > MaxSerialPacketSize)
    {
        LogError("Err, buffer is above the maximum buffer size that can be sent to debuggee (%d > %d), "
                 "for more information, please visit https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/increase-communication-buffer-size",
                 Length1 + Length2 + Length3 + SERIAL_END_OF_BUFFER_CHARS_COUNT,
                 MaxSerialPacketSize);
        return FALSE;
    }

    //
    // Send first buffer
    //
    for (SIZE_T i = 0; i < Length1; i++)
    {
        KdHyperDbgSendByte(Buffer1[i], TRUE);
    }

    //
    // Send second buffer
    //
    for (SIZE_T i = 0; i < Length2; i++)
    {
        KdHyperDbgSendByte(Buffer2[i], TRUE);
    }

    //
    // Send third buffer
    //
    for (SIZE_T i = 0; i < Length3; i++)
    {
        KdHyperDbgSendByte(Buffer3[i], TRUE);
    }

    //
    // Send the end buffer
    //
    SerialConnectionSendEndOfBuffer();

    return TRUE;
}

/**
 * @brief Check if baud rate is valid or not
 *
 * @param Baudrate
 * @return BOOLEAN return TRUE if it's correct and returns
 * FALSE if it's not correct
 */
BOOLEAN
SerialConnectionCheckBaudrate(DWORD Baudrate)
{
    if (Baudrate == CBR_110 || Baudrate == CBR_300 || Baudrate == CBR_600 ||
        Baudrate == CBR_1200 || Baudrate == CBR_2400 || Baudrate == CBR_4800 ||
        Baudrate == CBR_9600 || Baudrate == CBR_14400 || Baudrate == CBR_19200 ||
        Baudrate == CBR_38400 || Baudrate == CBR_56000 || Baudrate == CBR_57600 ||
        Baudrate == CBR_115200 || Baudrate == CBR_128000 ||
        Baudrate == CBR_256000)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Check if serial port address
 *
 * @param SerialPort
 * @return BOOLEAN return TRUE if it's correct and returns
 * FALSE if it's not correct
 */
BOOLEAN
SerialConnectionCheckPort(UINT32 SerialPort)
{
    if (SerialPort == COM1_PORT || SerialPort == COM2_PORT || SerialPort == COM3_PORT ||
        SerialPort == COM4_PORT)
    {
        return TRUE;
    }

    if (SerialPort == SERIAL_PORT_DBC)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Perform tasks relating to stepping (step-in & step-out) requests
 *
 * @param DebuggerPrintRequest Request to prepare debuggee
 *
 * @return NTSTATUS
 */
NTSTATUS
SerialConnectionPrepare(PDEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest)
{
    //
    // USB has no baud rate
    //
    if (DebuggeeRequest->PortAddress == SERIAL_PORT_DBC)
    {
        return SerialConnectionPrepareDbc(DebuggeeRequest);
    }

    //
    // Check if baud rate is valid or not
    //
    if (!SerialConnectionCheckBaudrate(DebuggeeRequest->Baudrate))
    {
        //
        // Baud rate is invalid, set the status and return
        //
        DebuggeeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_BAUDRATE;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check if port address is valid or not
    //
    if (!SerialConnectionCheckPort(DebuggeeRequest->PortAddress))
    {
        //
        // Port address is invalid, set the status and return
        //
        DebuggeeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Not in the desync state (reset desync flag)
    //
    g_SerialConnectionDesyncReported = FALSE;

    //
    // Prepare the structures needed for connecting remote port
    //
    KdHyperDbgPrepareDebuggeeConnectionPort(DebuggeeRequest->PortAddress, DebuggeeRequest->Baudrate);

    //
    // Initialize kernel debugger
    //
    KdInitializeKernelDebugger();

    //
    // Send "Start" packet along with Windows Name
    //
    KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                               DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED,
                               (CHAR *)DebuggeeRequest,
                               MAXIMUM_CHARACTER_FOR_OS_NAME);

    //
    // Set status to successful
    //
    DebuggeeRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return STATUS_SUCCESS;
}

/**
 * @brief Prepare the debuggee over a USB debug cable
 *
 * @param DebuggeeRequest Request to prepare the debuggee
 * @return NTSTATUS
 */
NTSTATUS
SerialConnectionPrepareDbc(PDEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest)
{
    if (!SerialConnectionCheckPort(DebuggeeRequest->PortAddress))
    {
        DebuggeeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT;
        return STATUS_UNSUCCESSFUL;
    }

    g_SerialConnectionDesyncReported = FALSE;
    g_SerialConnectionDbcSetupPhase  = FALSE;

    //
    // Keep the link open if the handshake already set it up
    //
    if (!KdHyperDbgDbcIsReady())
    {
        KdHyperDbgPrepareDebuggeeConnectionPort(DebuggeeRequest->PortAddress, DebuggeeRequest->Baudrate);
    }

    if (!KdHyperDbgDbcIsReady())
    {
        LogError("Err, USB debugging is not ready");

        DebuggeeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Clear leftover loss after any fallback prepare and before starting the session
    //
    {
        SERIAL_CONNECTION_DBC_OPERATION Operation = {0};

        Operation.Type = SERIAL_CONNECTION_DBC_OP_CLEAR_SEND_LOSS;
        VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                        (UINT64)&Operation,
                        0,
                        0);
    }

    KdInitializeKernelDebugger();

    //
    // Send USB setup data outside vmx-root until the debugger has the symbol details
    //
    g_SerialConnectionDbcSetupPhase = TRUE;

    //
    // Tell the debugger that the debuggee started and send the OS name
    //
    if (!SerialConnectionDbcSendAndWaitForHost(DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED,
                                               (CHAR *)DebuggeeRequest,
                                               MAXIMUM_CHARACTER_FOR_OS_NAME))
    {
        SerialConnectionDbcEndSetup();

        //
        // Stop the session before the message so that it is not sent over USB
        //
        KdUninitializeKernelDebugger();

        LogError("Err, the debugger did not receive the start packet over USB");

        DebuggeeRequest->Result = DEBUGGER_ERROR_DEBUGGER_DID_NOT_RECEIVE_USB_DATA;
        return STATUS_UNSUCCESSFUL;
    }

    DebuggeeRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return STATUS_SUCCESS;
}

/**
 * @brief Compute the total framed byte length of a debuggee-to-debugger packet
 *
 * @param Buffer Optional payload buffer
 * @param Length Length of the payload buffer
 * @return UINT64 Total framed length including the header and end marker
 */
static UINT64
SerialConnectionDbcFramedPacketLength(CHAR * Buffer, UINT32 Length)
{
    if (Buffer == NULL || Length == 0)
    {
        return sizeof(DEBUGGER_REMOTE_PACKET) + SERIAL_END_OF_BUFFER_CHARS_COUNT;
    }

    return (UINT64)sizeof(DEBUGGER_REMOTE_PACKET) + Length + SERIAL_END_OF_BUFFER_CHARS_COUNT;
}

/**
 * @brief Frame and send one debuggee-to-debugger packet while DebuggerResponseLock is held
 *
 * @param Response Requested action of the packet
 * @param Buffer Optional payload buffer
 * @param Length Length of the payload buffer
 * @return BOOLEAN Result from the serial or USB transport
 */
static BOOLEAN
SerialConnectionDbcSendFramedPacket(DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
                                    CHAR *                                  Buffer,
                                    UINT32                                  Length)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};

    Packet.Indicator                  = INDICATOR_OF_HYPERDBG_PACKET;
    Packet.TypeOfThePacket            = DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER;
    Packet.RequestedActionOfThePacket = Response;
    Packet.Checksum                   = KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                                              sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    if (Buffer == NULL || Length == 0)
    {
        return SerialConnectionSend((CHAR *)&Packet, sizeof(DEBUGGER_REMOTE_PACKET));
    }

    Packet.Checksum += KdComputeDataChecksum((PVOID)Buffer, Length);

    return SerialConnectionSendTwoBuffers((CHAR *)&Packet,
                                          sizeof(DEBUGGER_REMOTE_PACKET),
                                          Buffer,
                                          Length);
}

/**
 * @brief Execute one short USB control or non-blocking send step in vmx-root
 * @details Runs inside DEBUGGER_VMCALL_DBC_OPERATION so DebuggerResponseLock is
 * never held in vmx non-root.
 *
 * @param Operation Operation parameters on the caller stack
 * @return VOID
 */
VOID
SerialConnectionDbcOperationFromVmxRoot(PSERIAL_CONNECTION_DBC_OPERATION Operation)
{
    switch (Operation->Type)
    {
    case SERIAL_CONNECTION_DBC_OP_CLEAR_SEND_LOSS:
        SpinlockLock(&DebuggerResponseLock);
        KdHyperDbgDbcClearSendLoss();
        SpinlockUnlock(&DebuggerResponseLock);
        break;

    case SERIAL_CONNECTION_DBC_OP_POLL_SEND:
        //
        // Poll once without blocking, read the loss count, and release priority when drained
        //
        SpinlockLock(&DebuggerResponseLock);
        Operation->Result    = KdHyperDbgDbcWaitForSendEx(0, TRUE);
        Operation->LossCount = KdHyperDbgDbcGetSendLossCount();
        if (Operation->Result && Operation->PriorityHeld)
        {
            KdHyperDbgDbcSetControlPriority(FALSE);
            Operation->PriorityHeld = FALSE;
        }
        SpinlockUnlock(&DebuggerResponseLock);
        break;

    case SERIAL_CONNECTION_DBC_OP_SET_CONTROL_PRIORITY:
        SpinlockLock(&DebuggerResponseLock);
        KdHyperDbgDbcSetControlPriority(Operation->Flag);
        SpinlockUnlock(&DebuggerResponseLock);
        break;

    case SERIAL_CONNECTION_DBC_OP_SET_LONG_WAIT:
        SpinlockLock(&DebuggerResponseLock);
        KdHyperDbgDbcSetLongWait(Operation->Flag);
        SpinlockUnlock(&DebuggerResponseLock);
        break;

    case SERIAL_CONNECTION_DBC_OP_BEGIN_SEND_AND_WAIT:
    {
        BOOLEAN PreviousLongWait;

        //
        // Keep priority during confirmation without clearing earlier loss
        // and check whether earlier transfers already drained
        //
        SpinlockLock(&DebuggerResponseLock);
        KdHyperDbgDbcSetControlPriority(TRUE);
        PreviousLongWait = KdHyperDbgDbcSetSendWait(TRUE);
        KdHyperDbgDbcSetLongWait(TRUE);
        KdHyperDbgDbcSetSendWait(PreviousLongWait);
        Operation->PriorityHeld = TRUE;
        Operation->Result       = KdHyperDbgDbcWaitForSendEx(0, TRUE);
        SpinlockUnlock(&DebuggerResponseLock);
        break;
    }

    case SERIAL_CONNECTION_DBC_OP_TRY_SEND_SYMBOL:
    {
        BOOLEAN PreviousLongWait;
        BOOLEAN Usb;
        BOOLEAN CanFit;
        BOOLEAN Sent = FALSE;
        BOOLEAN Lost;
        UINT64  TotalLength;

        TotalLength = SerialConnectionDbcFramedPacketLength(Operation->Buffer, Operation->Length);

        //
        // Check earlier loss, try to queue without waiting, and manage retry priority under one lock
        //
        SpinlockLock(&DebuggerResponseLock);

        Usb  = KdHyperDbgDbcIsActive();
        Lost = Usb && KdHyperDbgDbcGetSendLossCount() != 0;

        if (!Lost)
        {
            if (!Operation->PriorityHeld)
            {
                KdHyperDbgDbcSetControlPriority(TRUE);
                Operation->PriorityHeld = TRUE;
            }

            PreviousLongWait = KdHyperDbgDbcSetSendWait(TRUE);

            CanFit = (!Usb || KdHyperDbgDbcIsReady()) &&
                     (!Usb || KdHyperDbgDbcCanSendPacket(TotalLength));

            if (CanFit)
            {
                Sent = SerialConnectionDbcSendFramedPacket(
                    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_UPDATE_SYMBOL_INFO,
                    Operation->Buffer,
                    Operation->Length);

                if (Usb)
                {
                    Sent = Sent && KdHyperDbgDbcIsReady() && KdHyperDbgDbcGetSendLossCount() == 0;
                }

                if (!Sent && Usb)
                {
                    Operation->Abort = TRUE;
                }
            }

            KdHyperDbgDbcSetSendWait(PreviousLongWait);
            Lost = Usb && KdHyperDbgDbcGetSendLossCount() != 0;
        }

        if ((Sent || Lost || Operation->Abort) && Operation->PriorityHeld)
        {
            KdHyperDbgDbcSetControlPriority(FALSE);
            Operation->PriorityHeld = FALSE;
        }

        Operation->Result = Sent;
        Operation->Lost   = Lost;

        SpinlockUnlock(&DebuggerResponseLock);

        if (Sent &&
            g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent &&
            g_IgnoreBreaksToDebugger.SpeialEventResponse == DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_UPDATE_SYMBOL_INFO)
        {
            RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));
            ScopedSpinlock(DebuggerResponseLock, KdHyperDbgDbcSetLongWait(FALSE));
        }

        break;
    }

    case SERIAL_CONNECTION_DBC_OP_TRY_SEND_CONFIRMED:
    {
        PDEBUGGEE_SYMBOL_UPDATE_RESULT FinishPacket = NULL;
        BOOLEAN                        PreviousLongWait;
        BOOLEAN                        Usb;
        BOOLEAN                        CanFit;
        BOOLEAN                        CanSend;
        BOOLEAN                        Sent = FALSE;
        UINT32                         LossBeforeSend;
        UINT64                         TotalLength;

        if (Operation->IsFinishPacket)
        {
            if (Operation->Buffer == NULL || Operation->Length < sizeof(DEBUGGEE_SYMBOL_UPDATE_RESULT))
            {
                Operation->Abort = TRUE;
                break;
            }

            FinishPacket = (PDEBUGGEE_SYMBOL_UPDATE_RESULT)Operation->Buffer;
        }

        TotalLength = SerialConnectionDbcFramedPacketLength(Operation->Buffer, Operation->Length);

        //
        // Check earlier loss, update finish status, and queue without waiting under one lock
        //
        SpinlockLock(&DebuggerResponseLock);

        KdHyperDbgDbcSetControlPriority(TRUE);
        PreviousLongWait = KdHyperDbgDbcSetSendWait(TRUE);
        Usb              = KdHyperDbgDbcIsActive();

        CanFit = (!Usb || KdHyperDbgDbcIsReady()) &&
                 (!Usb || KdHyperDbgDbcCanSendPacket(TotalLength));

        LossBeforeSend = Usb ? KdHyperDbgDbcGetSendLossCount() : 0;
        CanSend        = LossBeforeSend == 0 || (FinishPacket != NULL && !g_SerialConnectionDbcSetupPhase);

        if (!CanSend)
        {
            Operation->Abort = TRUE;
        }
        else if (CanFit)
        {
            if (FinishPacket != NULL && LossBeforeSend != 0)
            {
                FinishPacket->KernelStatus = DEBUGGER_ERROR_DEBUGGER_DID_NOT_RECEIVE_USB_DATA;
            }

            Sent = SerialConnectionDbcSendFramedPacket(Operation->Response,
                                                       Operation->Buffer,
                                                       Operation->Length);

            if (Usb)
            {
                Sent = Sent && KdHyperDbgDbcIsReady() && KdHyperDbgDbcGetSendLossCount() == LossBeforeSend;
            }

            Operation->Result    = Sent;
            Operation->LossCount = LossBeforeSend;

            if (!Sent && Usb)
            {
                Operation->Abort = TRUE;
            }
        }

        KdHyperDbgDbcSetSendWait(PreviousLongWait);
        KdHyperDbgDbcSetControlPriority(FALSE);

        SpinlockUnlock(&DebuggerResponseLock);

        if (Sent &&
            g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent &&
            g_IgnoreBreaksToDebugger.SpeialEventResponse == Operation->Response)
        {
            RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));
            ScopedSpinlock(DebuggerResponseLock, KdHyperDbgDbcSetLongWait(FALSE));
        }

        break;
    }

    default:
        break;
    }
}

/**
 * @brief Pause briefly in vmx non-root between short vmcall polls
 *
 * @return VOID
 */
static VOID
SerialConnectionDbcPause()
{
    for (UINT32 i = 0; i < SERIAL_CONNECTION_DBC_BUSY_POLL_PAUSES; i++)
    {
        CpuPause();
    }
}

/**
 * @brief Check whether a short PASSIVE_LEVEL vmcall poll burst expired
 *
 * @param StartCounter Initial performance counter reading
 * @param Frequency Performance counter frequency
 * @param Microseconds Maximum busy-poll duration in microseconds
 * @param Polls Poll iteration counter to advance
 * @return BOOLEAN TRUE when the short poll window ended
 */
static BOOLEAN
SerialConnectionDbcShortPollExpired(LARGE_INTEGER StartCounter,
                                    LARGE_INTEGER Frequency,
                                    UINT32        Microseconds,
                                    UINT32 *      Polls)
{
    LARGE_INTEGER Now;
    UINT64        ElapsedTicks;
    UINT64        LimitTicks;
    UINT64        Freq;

    (*Polls)++;

    if (*Polls >= SERIAL_CONNECTION_DBC_BUSY_POLL_MAX_ITERS || Frequency.QuadPart <= 0)
    {
        return TRUE;
    }

    Now = PlatformTimeQueryPerformanceCounter(NULL);
    if (Now.QuadPart < StartCounter.QuadPart)
    {
        return TRUE;
    }

    Freq         = (UINT64)Frequency.QuadPart;
    ElapsedTicks = (UINT64)(Now.QuadPart - StartCounter.QuadPart);
    LimitTicks   = (Freq / 1000000ULL) * Microseconds + ((Freq % 1000000ULL) * Microseconds) / 1000000ULL;

    return ElapsedTicks >= LimitTicks;
}

/**
 * @brief Toggle protected USB send priority from vmx non-root through a vmcall
 *
 * @param Enable TRUE to raise priority, FALSE to release it
 * @return VOID
 */
static VOID
SerialConnectionDbcSetControlPriority(BOOLEAN Enable)
{
    SERIAL_CONNECTION_DBC_OPERATION Operation = {0};

    Operation.Type = SERIAL_CONNECTION_DBC_OP_SET_CONTROL_PRIORITY;
    Operation.Flag = Enable;

    VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                    (UINT64)&Operation,
                    0,
                    0);
}

/**
 * @brief Update the USB long-wait mode from vmx non-root through a vmcall
 *
 * @param Allow TRUE to allow long USB send waits
 * @return VOID
 */
VOID
SerialConnectionDbcSetLongWait(BOOLEAN Allow)
{
    SERIAL_CONNECTION_DBC_OPERATION Operation = {0};

    if (!KdHyperDbgDbcIsActive())
    {
        return;
    }

    Operation.Type = SERIAL_CONNECTION_DBC_OP_SET_LONG_WAIT;
    Operation.Flag = Allow;

    VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                    (UINT64)&Operation,
                    0,
                    0);
}

/**
 * @brief Wait outside vmx-root until queued USB transfers complete
 * @details Runs at PASSIVE_LEVEL. Polls through short non-blocking vmcalls for
 * up to SERIAL_CONNECTION_DBC_BUSY_POLL_US before sleeping between retries.
 *
 * @param TimeoutMs Time to wait for the host
 * @param LossCount Optional output for the loss count observed on completion
 * @param PriorityHeld Optional in/out flag to release control priority when drained
 * @return BOOLEAN TRUE when all queued USB transfers completed
 */
static BOOLEAN
SerialConnectionDbcWaitForHostInternal(UINT32    TimeoutMs,
                                       UINT32 *  LossCount,
                                       BOOLEAN * PriorityHeld)
{
    UINT64                          StartMs    = PlatformTimeGetMonotonicMilliseconds();
    LARGE_INTEGER                   Frequency  = {0};
    LARGE_INTEGER                   PollStart  = {0};
    SERIAL_CONNECTION_DBC_OPERATION Operation;
    BOOLEAN                         BusyPolled = FALSE;
    UINT32                          PollCount  = 0;

    while (TRUE)
    {
        RtlZeroMemory(&Operation, sizeof(Operation));
        Operation.Type         = SERIAL_CONNECTION_DBC_OP_POLL_SEND;
        Operation.PriorityHeld = PriorityHeld != NULL ? *PriorityHeld : FALSE;

        VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                        (UINT64)&Operation,
                        0,
                        0);

        if (PriorityHeld != NULL)
        {
            *PriorityHeld = Operation.PriorityHeld;
        }

        if (LossCount != NULL)
        {
            *LossCount = Operation.LossCount;
        }

        if (Operation.Result)
        {
            return TRUE;
        }

        if (TimeoutMs > 0 && !BusyPolled)
        {
            if (PollCount == 0)
            {
                PollStart = PlatformTimeQueryPerformanceCounter(&Frequency);
            }

            if (!SerialConnectionDbcShortPollExpired(PollStart, Frequency, SERIAL_CONNECTION_DBC_BUSY_POLL_US, &PollCount))
            {
                SerialConnectionDbcPause();
                continue;
            }

            BusyPolled = TRUE;
        }

        if (PlatformTimeGetMonotonicMilliseconds() - StartMs >= TimeoutMs)
        {
            return FALSE;
        }

        PlatformTimeDelayMilliseconds(SERIAL_CONNECTION_DBC_POLL_MS);
    }
}

/**
 * @brief Wait outside vmx-root until queued USB transfers complete
 * @details The caller runs at PASSIVE_LEVEL. Each short poll runs in vmx-root
 * through a vmcall and retries sleep only after the short poll burst ends.
 *
 * @param TimeoutMs Time to wait for the host
 * @return BOOLEAN TRUE when all queued USB transfers completed
 */
BOOLEAN
SerialConnectionDbcWaitForHost(UINT32 TimeoutMs)
{
    return SerialConnectionDbcWaitForHostInternal(TimeoutMs, NULL, NULL);
}

/**
 * @brief Queue a USB symbol update and wait at PASSIVE_LEVEL if space is needed
 * @details When the packet fits, returns immediately after one vmcall without sleeping.
 * When the send ring is full, polls briefly through vmcalls before sleeping.
 *
 * @param Buffer Symbol update packet
 * @param Length Length of the packet
 * @return BOOLEAN TRUE if the packet was queued and no delivery loss was recorded
 */
BOOLEAN
SerialConnectionDbcSendSymbolPacket(CHAR * Buffer, UINT32 Length)
{
    UINT64                          StartMs;
    UINT64                          ElapsedMs;
    LARGE_INTEGER                   Frequency    = {0};
    LARGE_INTEGER                   PollStart    = {0};
    SERIAL_CONNECTION_DBC_OPERATION Operation;
    BOOLEAN                         Result       = FALSE;
    BOOLEAN                         PriorityHeld = FALSE;
    BOOLEAN                         BusyPolled   = FALSE;
    UINT32                          PollCount    = 0;

    if (!KdHyperDbgDbcIsActive())
    {
        return FALSE;
    }

    StartMs = PlatformTimeGetMonotonicMilliseconds();

    while (TRUE)
    {
        RtlZeroMemory(&Operation, sizeof(Operation));
        Operation.Type         = SERIAL_CONNECTION_DBC_OP_TRY_SEND_SYMBOL;
        Operation.Buffer       = Buffer;
        Operation.Length       = Length;
        Operation.PriorityHeld = PriorityHeld;

        VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                        (UINT64)&Operation,
                        0,
                        0);

        PriorityHeld = Operation.PriorityHeld;

        if (Operation.Result || Operation.Lost || Operation.Abort)
        {
            Result = Operation.Result && !Operation.Lost && !Operation.Abort;
            break;
        }

        //
        // Poll briefly before sleeping when the ring is full
        //
        if (!BusyPolled)
        {
            if (PollCount == 0)
            {
                PollStart = PlatformTimeQueryPerformanceCounter(&Frequency);
            }

            if (!SerialConnectionDbcShortPollExpired(PollStart, Frequency, SERIAL_CONNECTION_DBC_BUSY_POLL_US, &PollCount))
            {
                SerialConnectionDbcPause();
                continue;
            }

            BusyPolled = TRUE;
        }

        ElapsedMs = PlatformTimeGetMonotonicMilliseconds() - StartMs;
        if (ElapsedMs >= SERIAL_CONNECTION_DBC_SETUP_TIMEOUT_MS)
        {
            break;
        }

        //
        // Yield before retrying if no packet can be queued after the short poll burst
        //
        PlatformTimeDelayMilliseconds(SERIAL_CONNECTION_DBC_POLL_MS);
    }

    if (PriorityHeld)
    {
        SerialConnectionDbcSetControlPriority(FALSE);
    }

    return Result;
}

/**
 * @brief Send a USB packet outside vmx-root and wait for the host to take it
 * @details The caller runs at PASSIVE_LEVEL. Each locked step runs in vmx-root
 * through a vmcall and retries poll briefly before sleeping.
 *
 * @param Response Requested action of the packet
 * @param Buffer Buffer to send
 * @param Length Length of the buffer
 * @return BOOLEAN TRUE if this packet completed without a new delivery loss
 */
BOOLEAN
SerialConnectionDbcSendAndWaitForHost(DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
                                      CHAR *                                  Buffer,
                                      UINT32                                  Length)
{
    SERIAL_CONNECTION_DBC_OPERATION Operation;
    UINT64                          StartMs;
    UINT64                          ElapsedMs;
    LARGE_INTEGER                   Frequency      = {0};
    LARGE_INTEGER                   PollStart      = {0};
    BOOLEAN                         Result         = FALSE;
    BOOLEAN                         IsFinishPacket = FALSE;
    BOOLEAN                         PriorityHeld   = FALSE;
    BOOLEAN                         BusyPolled     = FALSE;
    UINT32                          PollCount      = 0;
    UINT32                          LossBeforeSend = 0;
    UINT32                          LossAfterSend  = 0;

    if (Response == DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SYMBOL_FINISHED)
    {
        if (Buffer == NULL || Length < sizeof(DEBUGGEE_SYMBOL_UPDATE_RESULT))
        {
            return FALSE;
        }

        IsFinishPacket = TRUE;
    }

    RtlZeroMemory(&Operation, sizeof(Operation));
    Operation.Type = SERIAL_CONNECTION_DBC_OP_BEGIN_SEND_AND_WAIT;
    VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                    (UINT64)&Operation,
                    0,
                    0);

    PriorityHeld = Operation.PriorityHeld;

    //
    // Wait for earlier USB bytes before sending this packet
    //
    if (Operation.Result ||
        SerialConnectionDbcWaitForHostInternal(SERIAL_CONNECTION_DBC_SETUP_TIMEOUT_MS, NULL, NULL))
    {
        StartMs = PlatformTimeGetMonotonicMilliseconds();

        while (TRUE)
        {
            RtlZeroMemory(&Operation, sizeof(Operation));
            Operation.Type           = SERIAL_CONNECTION_DBC_OP_TRY_SEND_CONFIRMED;
            Operation.Response       = Response;
            Operation.Buffer         = Buffer;
            Operation.Length         = Length;
            Operation.IsFinishPacket = IsFinishPacket;

            VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_OPERATION,
                            (UINT64)&Operation,
                            0,
                            0);

            if (Operation.Result)
            {
                Result         = TRUE;
                LossBeforeSend = Operation.LossCount;
                break;
            }

            if (Operation.Abort)
            {
                break;
            }

            //
            // Poll briefly before sleeping if space is not immediately ready
            //
            if (!BusyPolled)
            {
                if (PollCount == 0)
                {
                    PollStart = PlatformTimeQueryPerformanceCounter(&Frequency);
                }

                if (!SerialConnectionDbcShortPollExpired(PollStart, Frequency, SERIAL_CONNECTION_DBC_BUSY_POLL_US, &PollCount))
                {
                    SerialConnectionDbcPause();
                    continue;
                }

                BusyPolled = TRUE;
            }

            ElapsedMs = PlatformTimeGetMonotonicMilliseconds() - StartMs;
            if (ElapsedMs >= SERIAL_CONNECTION_DBC_SETUP_TIMEOUT_MS)
            {
                break;
            }

            //
            // Yield before retrying if no packet can be queued after the short poll burst
            //
            PlatformTimeDelayMilliseconds(SERIAL_CONNECTION_DBC_POLL_MS);
        }

        if (Result)
        {
            Result = SerialConnectionDbcWaitForHostInternal(SERIAL_CONNECTION_DBC_SETUP_TIMEOUT_MS,
                                                            &LossAfterSend,
                                                            &PriorityHeld) &&
                     LossAfterSend == LossBeforeSend;
        }
    }

    if (PriorityHeld)
    {
        SerialConnectionDbcSetControlPriority(FALSE);
    }

    return Result;
}

/**
 * @brief End the USB setup phase and let sends drop bytes again
 *
 * @return VOID
 */
VOID
SerialConnectionDbcEndSetup()
{
    g_SerialConnectionDbcSetupPhase = FALSE;

    SerialConnectionDbcSetLongWait(FALSE);
}

/**
 * @brief Send a buffer of the USB session setup to the debugger
 * @details Runs outside vmx-root at PASSIVE_LEVEL. The setup ends after the
 * end of the symbol details or after an error.
 *
 * @param DebuggeeBufferRequest Request to buffer that will be sent to the debugger
 * @return UINT32 Result to return to user mode
 */
UINT32
SerialConnectionDbcSendSetupBuffer(PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest)
{
    BOOLEAN Sent;

    if (DebuggeeBufferRequest->RequestedAction == DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_UPDATE_SYMBOL_INFO)
    {
        Sent = SerialConnectionDbcSendSymbolPacket(
            (CHAR *)((UINT64)DebuggeeBufferRequest + SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER),
            DebuggeeBufferRequest->LengthOfBuffer);
    }
    else
    {
        Sent = SerialConnectionDbcSendAndWaitForHost(DebuggeeBufferRequest->RequestedAction,
                                                    (CHAR *)((UINT64)DebuggeeBufferRequest + SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER),
                                                    DebuggeeBufferRequest->LengthOfBuffer);
    }

    if (!Sent)
    {
        SerialConnectionDbcEndSetup();

        //
        // Stop the session before the message so that it is not sent over USB
        //
        KdUninitializeKernelDebugger();

        LogError("Err, the debugger did not receive the symbol details over USB");

        return DEBUGGER_ERROR_DEBUGGER_DID_NOT_RECEIVE_USB_DATA;
    }

    if (DebuggeeBufferRequest->RequestedAction == DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SYMBOL_FINISHED)
    {
        SerialConnectionDbcEndSetup();
    }

    return DEBUGGER_OPERATION_WAS_SUCCESSFUL;
}

/**
 * @brief Send or receive handshake bytes over USB
 * @details Runs before the session starts. A request with no bytes to send
 * reads available bytes without waiting.
 *
 * @param DbcHandshakeRequest Input/output packet in the non-paged system buffer
 * @return NTSTATUS
 */
NTSTATUS
SerialConnectionDbcHandshake(PDEBUGGEE_DBC_HANDSHAKE_PACKET DbcHandshakeRequest)
{
    UCHAR RecvChar = NULL_ZERO;

    DbcHandshakeRequest->ReceiveLength = 0;

    //
    // After the session starts, receive bytes in vmx-root
    //
    if (g_KernelDebuggerState || DbcHandshakeRequest->SendLength > DEBUGGEE_DBC_RECEIVE_BUFFER_SIZE)
    {
        DbcHandshakeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Reset DbC to discard transfers from the previous session
    //
    if (DbcHandshakeRequest->ResetConnection)
    {
        KdHyperDbgPrepareDebuggeeConnectionPort(SERIAL_PORT_DBC, 0);
    }

    if (!KdHyperDbgDbcIsReady())
    {
        LogError("Err, USB debugging is not ready");

        DbcHandshakeRequest->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT;
        return STATUS_UNSUCCESSFUL;
    }

    if (DbcHandshakeRequest->SendLength != 0)
    {
        for (UINT32 i = 0; i < DbcHandshakeRequest->SendLength; i++)
        {
            KdHyperDbgSendByte(DbcHandshakeRequest->SendBuffer[i], TRUE);
        }
    }
    else
    {
        while (DbcHandshakeRequest->ReceiveLength < DEBUGGEE_DBC_RECEIVE_BUFFER_SIZE && KdHyperDbgRecvByte(&RecvChar))
        {
            DbcHandshakeRequest->ReceiveBuffer[DbcHandshakeRequest->ReceiveLength] = RecvChar;
            DbcHandshakeRequest->ReceiveLength++;
        }
    }

    DbcHandshakeRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return STATUS_SUCCESS;
}

/**
 * @brief Read debugger bytes over USB in vmx-root
 * @details Skip the read if a core is halting the system or the response lock is busy.
 *
 * @param DbcReceiveRequest Output packet in the non-paged system buffer
 * @return VOID
 */
VOID
SerialConnectionDbcReceiveFromVmxRoot(PDEBUGGEE_DBC_RECEIVE_PACKET DbcReceiveRequest)
{
    UINT32 Length   = 0;
    UCHAR  RecvChar = NULL_ZERO;

    DbcReceiveRequest->Length = 0;

    if (!g_KernelDebuggerState)
    {
        return;
    }

    if (SpinlockCheckLock(&DebuggerHandleBreakpointLock) || !SpinlockTryLock(&DebuggerResponseLock))
    {
        return;
    }

    while (Length < DEBUGGEE_DBC_RECEIVE_BUFFER_SIZE && KdHyperDbgRecvByte(&RecvChar))
    {
        DbcReceiveRequest->Buffer[Length] = RecvChar;
        Length++;

        if (Length >= SERIAL_END_OF_BUFFER_CHARS_COUNT &&
            DbcReceiveRequest->Buffer[Length - 4] == SERIAL_END_OF_BUFFER_CHAR_1 &&
            DbcReceiveRequest->Buffer[Length - 3] == SERIAL_END_OF_BUFFER_CHAR_2 &&
            DbcReceiveRequest->Buffer[Length - 2] == SERIAL_END_OF_BUFFER_CHAR_3 &&
            DbcReceiveRequest->Buffer[Length - 1] == SERIAL_END_OF_BUFFER_CHAR_4)
        {
            break;
        }
    }

    SpinlockUnlock(&DebuggerResponseLock);

    DbcReceiveRequest->Length = Length;
}

/**
 * @brief Handle a user-mode request to read debugger bytes over USB
 *
 * @param DbcReceiveRequest Input/output packet in the non-paged system buffer
 * @return NTSTATUS
 */
NTSTATUS
SerialConnectionDbcReceive(PDEBUGGEE_DBC_RECEIVE_PACKET DbcReceiveRequest)
{
    DbcReceiveRequest->Length = 0;

    if (g_KernelDebuggerState && KdHyperDbgDbcIsActive())
    {
        VmFuncVmxVmcall(DEBUGGER_VMCALL_DBC_RECEIVE,
                        (UINT64)DbcReceiveRequest,
                        0,
                        0);
    }

    DbcReceiveRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return STATUS_SUCCESS;
}
