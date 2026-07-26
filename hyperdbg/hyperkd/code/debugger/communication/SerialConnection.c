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

//
// Set when the serial stream desyncs so the warning is logged once per episode
// (cleared on the next good frame) instead of on every overflow
//
static BOOLEAN g_SerialConnectionDesyncReported = FALSE;

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

        if (!KdHyperDbgRecvByte(&RecvChar))
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

        if (!KdHyperDbgRecvByte(&RecvChar))
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
