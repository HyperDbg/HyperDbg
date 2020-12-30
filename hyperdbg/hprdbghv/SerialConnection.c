/**
 * @file SerialConnection.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
    for (size_t i = 0; i < 100; i++)
    {
        KdHyperDbgTest((UINT16)i);
    }
}

/**
 * @brief Perform sending buffer over serial
 * 
 * @param Buffer buffer to send
 * @param Length length of buffer to send
 * @return VOID 
 */
VOID
SerialConnectionSend(CHAR * Buffer, PVOID Length)
{
    for (size_t i = 0; i < Length; i++)
    {
        KdHyperDbgSendByte(Buffer[i], TRUE);
    }

    //
    // Send the end buffer
    //
    KdHyperDbgSendByte(0x00, TRUE);
    KdHyperDbgSendByte(0x80, TRUE);
    KdHyperDbgSendByte(0xee, TRUE);
    KdHyperDbgSendByte(0xff, TRUE);
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
    // Send "Start" packet
    //
    SerialConnectionSend("Start", 5);

    //
    // Send Windows Name
    //
    SerialConnectionSend(DebuggeeRequest->OsName, MAXIMUM_CHARACTER_FOR_OS_NAME);

    //
    // Set status to successful
    //
    DebuggeeRequest->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}
