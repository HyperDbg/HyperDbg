/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    uart16550.c

Abstract:

    This module contains support for the standard 16550 serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"
#include "kdcom.h"

// ----------------------------------------------- Function Test

//
// Global Variables
//
CPPORT g_PortDetails = {0};

/*

F8 02 00 00 00 00 00 00  00 C2 01 00 00 00 01 00  ................
70 35 83 0F 04 F8 FF FF  40 35 83 0F 04 F8 FF FF  p5......@5......

*/

static VOID
WritePortWithIndex8(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PUCHAR Pointer;

    Pointer = (PUCHAR)(Port->Address + Index * Port->ByteWidth);
    WRITE_PORT_UCHAR(Pointer, Value);
    return;
}

static UCHAR
ReadPortWithIndex8(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PUCHAR Pointer;

    Pointer = (PUCHAR)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_PORT_UCHAR(Pointer);
}

UINT64
KdHyperDbgTest(UINT16 Byte)
{
    //
    // *** This function is for internal use and test
    // don't use it ***
    //

    CPPORT TempPort    = {0};
    TempPort.Address   = 0x2f8;
    TempPort.BaudRate  = 0x01c200; //115200
    TempPort.Flags     = 0;
    TempPort.ByteWidth = 1;

    TempPort.Write = WritePortWithIndex8;
    TempPort.Read  = ReadPortWithIndex8;

    Uart16550PutByte(&TempPort, 0x42, TRUE);

    for (size_t i = 0; i < 100; i++)
    {
        char RecvByte = 0;
        // Uart16550GetByte(&TempPort,&RecvByte);
    }
}

VOID
KdHyperDbgPrepareDebuggeeConnectionPort(UINT32 PortAddress, UINT32 Baudrate)
{
    g_PortDetails.Address   = PortAddress;
    g_PortDetails.BaudRate  = Baudrate;
    g_PortDetails.Flags     = 0;
    g_PortDetails.ByteWidth = 1;

    g_PortDetails.Write = WritePortWithIndex8;
    g_PortDetails.Read  = ReadPortWithIndex8;
}

VOID
KdHyperDbgSendByte(UCHAR Byte, BOOLEAN BusyWait)
{
    Uart16550PutByte(&g_PortDetails, Byte, BusyWait);
}

BOOLEAN
KdHyperDbgRecvByte(PUCHAR RecvByte)
{
    if (Uart16550GetByte(&g_PortDetails, RecvByte) == UartSuccess)
    {
        return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
Uart16550SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

// ------------------------------------------------------------------ Functions

BOOLEAN
Uart16550InitializePortCommon(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the common initialization of a 16550 serial UART.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Supplies a boolean which indicates if the UART is accessed
        through memory-mapped registers or legacy port I/O.

    AccessSize - Supplies an ACPI Generic Access Size value which indicates the
        type of bus access that should be performed when accessing the UART.

    BitWidth - Supplies a number indicating how wide the UART's registers are.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{
    UCHAR RegisterValue;

    UNREFERENCED_PARAMETER(LoadOptions);

    //
    // Set the Read / Write function pointers for this serial port.
    //

    UartpSetAccess(Port, MemoryMapped, AccessSize, BitWidth);

    //
    // Set DLAB to zero. The DLAB controls the meaning of the first two
    // registers. When zero, the first register is used for all byte transfers
    // and the second register controls device interrupts.
    //

    RegisterValue = Port->Read(Port, COM_LCR);
    RegisterValue &= ~LC_DLAB;
    Port->Write(Port, COM_LCR, RegisterValue);

    //
    // Disable device interrupts.  This implementation will handle state
    // transitions by request only.
    //

    Port->Write(Port, COM_IEN, 0);

    //
    // Reset and disable the FIFO queue.
    // N.B. FIFO will be reenabled before returning from this routine.
    //

    Port->Write(Port, COM_FCR, FC_CLEAR_TRANSMIT | FC_CLEAR_RECEIVE);

    //
    // Configure the baud rate and mode.
    //

    Uart16550SetBaud(Port, Port->BaudRate);

    //
    // Enable the FIFO.
    //

    Port->Write(Port, COM_FCR, FC_ENABLE);

    //
    // Assert DTR, RTS. Disable loopback. Indicate to the device that
    // we are able to send and receive data.
    //

    Port->Write(Port, COM_MCR, MC_DTRRTS);

    //
    // Initialize ring indicator bit based on hardware state.
    //

    RegisterValue = Port->Read(Port, COM_MSR);
    if (CHECK_FLAG(RegisterValue, SERIAL_MSR_RI))
    {
        Port->Flags |= PORT_RING_INDICATOR;
    }

    return TRUE;
}

BOOLEAN
Uart16550LegacyInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine initializes the 16550 UART as a legacy serial port. In this
    case, legacy refers to the fact that the serial port is bound to one of the
    four well-known PC I/O port addresses (selected with a COM port number),
    has an access size of one byte, and a register width of 8 bits. This
    routine is intended for use with manually (user) specified COM ports.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Unused.

    AccessSize - Unused.

    BitWidth - Unused.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);
    UNREFERENCED_PARAMETER(MemoryMapped);

    switch ((ULONG_PTR)Port->Address)
    {
    case 1:
        Port->Address = (PUCHAR)COM1_PORT;
        break;

    case 2:
        Port->Address = (PUCHAR)COM2_PORT;
        break;

    case 3:
        Port->Address = (PUCHAR)COM3_PORT;
        break;

    case 4:
        Port->Address = (PUCHAR)COM4_PORT;
        break;

    default:
        return FALSE;
    }

    Port->Flags = 0;
    return Uart16550InitializePortCommon(LoadOptions,
                                         Port,
                                         FALSE,
                                         AcpiGenericAccessSizeByte,
                                         8);
}

BOOLEAN
Uart16550InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine initializes the 16550 serial UART using a base address and
    address space ID that originated in an ACPI Generic Address Structure. This
    routine is intended for use with DBGP-compatible serial ports.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Supplies a boolean which indicates if the UART is accessed
        through memory-mapped registers or legacy port I/O.

    AccessSize - Unused.

    BitWidth - Unused.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{
    UCHAR RegisterBitWidth;

    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

#if defined(_ARM_) || defined(_ARM64_)

    //
    // ARM-based systems are by definition MMIO and tend to space the ports
    // apart at 4-byte intervals. Expand to DWORD boundaries for ARM devices.
    //

    RegisterBitWidth = 32;
    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    //
    // ARM-based systems typically have non-standard dividends for baud rate.
    // It is deemed safest to assume the UEFI firmware has already set up
    // the serial port properly and just inherit the baud rate that is already
    // set.
    //

    Port->Flags = PORT_DEFAULT_RATE;

#else

    RegisterBitWidth = 8;
    Port->Flags      = 0;

#endif

    return Uart16550InitializePortCommon(LoadOptions,
                                         Port,
                                         MemoryMapped,
                                         AcpiGenericAccessSizeByte,
                                         RegisterBitWidth);
}

BOOLEAN
Uart16550MmInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine initializes the 16550 UART as a memory-mapped serial port.
    The base address, access size, and bit width are all respected by this
    type of serial port. This routine is intended for use with DBG2-compatible
    serial ports.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Supplies a boolean which indicates if the UART is accessed
        through memory-mapped registers or legacy port I/O.

    AccessSize - Supplies an ACPI Generic Access Size value which indicates the
        type of bus access that should be performed when accessing the UART.

    BitWidth - Supplies a number indicating how wide the UART's registers are.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{
    Port->Flags = PORT_DEFAULT_RATE;
    return Uart16550InitializePortCommon(LoadOptions,
                                         Port,
                                         MemoryMapped,
                                         AccessSize,
                                         BitWidth);
}

BOOLEAN
Uart16550SetBaudCommon(
    _Inout_ PCPPORT Port,
    ULONG           Rate,
    ULONG           Clock)

/*++

Routine Description:

    Set the baud rate for the UART hardware and record it in the port object.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Rate - Supplies the desired baud rate in bits per second.

    Clock - Supplies the base clock frequency of the UART in Hz.

Return Value:

    TRUE if the baud rate was programmed, FALSE if it was not.

--*/

{
    UCHAR Lcr;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    if ((Rate == 0) || (Clock == 0))
    {
        return FALSE;
    }

    //
    // A device's baud rate is written to DLL and DLM.  The values of these
    // registers are the resultant when the max rate (clock) is divided by the
    // device's desired operating rate.
    //

    const ULONG DivisorLatch = Clock / Rate;

    //
    // Set the divisor latch access bit (DLAB) in the line control register.
    // When non-zero, the first two registers become DLL and DLM.
    //

    Lcr = Port->Read(Port, COM_LCR);
    Lcr |= LC_DLAB;
    Port->Write(Port, COM_LCR, Lcr);

    //
    // Set the divisor latch value (MSB first, then LSB).
    //

    Port->Write(Port, COM_DLM, (UCHAR)((DivisorLatch >> 8) & 0xFF));
    Port->Write(Port, COM_DLL, (UCHAR)(DivisorLatch & 0xFF));

    //
    // Set Line Control Register to 8N1 (no parity, 8 data bits, 1 stop bit).
    // This also sets the DLAB back to zero.
    //

    Port->Write(Port, COM_LCR, 3);

    //
    // Remember the baud rate.
    //

    Port->BaudRate = Rate;
    return TRUE;
}

BOOLEAN
Uart16550SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate)

/*++

Routine Description:

    Set the baud rate for the UART hardware and record it in the port object.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Rate - Supplies the desired baud rate in bits per second.

Return Value:

    TRUE if the baud rate was programmed, FALSE if it was not.

--*/

{
    if (CHECK_FLAG(Port->Flags, PORT_DEFAULT_RATE))
    {
        return FALSE;
    }

    return Uart16550SetBaudCommon(Port, Rate, CLOCK_RATE);
}

UART_STATUS
Uart16550GetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR    Byte)

/*++

Routine Description:

    Fetch a data byte from the UART device and return it.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the address of variable to hold the result.

Return Value:

    UART_STATUS code.

--*/

{
    UCHAR Data;
    UCHAR Lsr;
    UCHAR Msr;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Check to see if all bits are set in LSR. If this is the case, it means
    // the port I/O address is invalid as 0xFF is nonsense for LSR.
    //

    Lsr = Port->Read(Port, COM_LSR);
    if (Lsr == SERIAL_LSR_NOT_PRESENT)
    {
        return UartNotReady;
    }

    if (CHECK_FLAG(Lsr, COM_DATRDY))
    {
        //
        // Return unsuccessfully if any errors are indicated by the
        // LSR.
        //

        if (CHECK_FLAG(Lsr, COM_PE) ||
            CHECK_FLAG(Lsr, COM_FE) ||
            CHECK_FLAG(Lsr, COM_OE))
        {
            return UartError;
        }

        Data = Port->Read(Port, COM_DAT);

        //
        // When using modem control, ignore any bytes that don't have
        // the carrier detect flag set.
        //

        if (CHECK_FLAG(Port->Flags, PORT_MODEM_CONTROL))
        {
            Msr = Port->Read(Port, COM_MSR);
            if (CHECK_FLAG(Msr, MS_CD) == FALSE)
            {
                return UartNoData;
            }
        }

        *Byte = Data;
        return UartSuccess;
    }
    else
    {
        //
        // Data is not available. Determine if the ring indicator has toggled.
        // If so, enable modem control.
        //

        Msr = Port->Read(Port, COM_MSR);
        if ((CHECK_FLAG(Port->Flags, PORT_RING_INDICATOR) &&
             !CHECK_FLAG(Msr, SERIAL_MSR_RI)) ||
            (!CHECK_FLAG(Port->Flags, PORT_RING_INDICATOR) &&
             CHECK_FLAG(Msr, SERIAL_MSR_RI)))
        {
            Port->Flags |= PORT_MODEM_CONTROL;
        }

        return UartNoData;
    }
}

UART_STATUS
Uart16550PutByte(
    _Inout_ PCPPORT Port,
    UCHAR           Byte,
    BOOLEAN         BusyWait)

/*++

Routine Description:

    Write a data byte out to the UART device.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the data to emit.

    BusyWait - Supplies a flag to control whether this routine will busy
        wait (spin) for the UART hardware to be ready to transmit.

--*/

{
    UCHAR Lsr;
    UCHAR Msr;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // When using modem control, DSR, CTS, and CD flags must all be set before
    // sending any data.
    //

    if (CHECK_FLAG(Port->Flags, PORT_MODEM_CONTROL))
    {
        Msr = Port->Read(Port, COM_MSR);
        while ((Msr & MS_DSRCTSCD) != MS_DSRCTSCD)
        {
            //
            // If there's a byte ready, discard it from the input queue.
            //

            if (!CHECK_FLAG(Msr, MS_CD))
            {
                Lsr = Port->Read(Port, COM_LSR);
                if (CHECK_FLAG(Port->Flags, COM_DATRDY))
                {
                    Port->Read(Port, COM_DAT);
                }
            }

            Msr = Port->Read(Port, COM_MSR);
        }
    }

    //
    // Check to see if all bits are set in LSR. If this is the case, it means
    // the port I/O address is invalid as 0xFF is nonsense for LSR. This
    // prevents writing a byte to non-existent hardware.
    //

    Lsr = Port->Read(Port, COM_LSR);
    if (Lsr == SERIAL_LSR_NOT_PRESENT)
    {
        return UartNotReady;
    }

    //
    // The port must be ready to accept a byte for output before continuing.
    //

    while (!CHECK_FLAG(Lsr, COM_OUTRDY))
    {
        //
        // Determine if the ring indicator has toggled.
        // If so, enable modem control.
        //

        Msr = Port->Read(Port, COM_MSR);
        if ((CHECK_FLAG(Port->Flags, PORT_RING_INDICATOR) &&
             !CHECK_FLAG(Msr, SERIAL_MSR_RI)) ||
            (!CHECK_FLAG(Port->Flags, PORT_RING_INDICATOR) &&
             CHECK_FLAG(Msr, SERIAL_MSR_RI)))
        {
            Port->Flags |= PORT_MODEM_CONTROL;
        }

        if (BusyWait == FALSE)
        {
            return UartNotReady;
        }

        Lsr = Port->Read(Port, COM_LSR);
    }

    //
    // Transmitter holding register is empty. Send the byte.
    //

    Port->Write(Port, COM_DAT, Byte);
    return UartSuccess;
}

BOOLEAN
Uart16550RxReady(
    _Inout_ PCPPORT Port)

/*++

Routine Description:

    This routine determines if there is data pending in the UART.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

Return Value:

    TRUE if data is available, FALSE otherwise.

--*/

{
    UCHAR Lsr;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Check to see if all bits are set in LSR. If this is the case, it means
    // the port I/O address is invalid as 0xFF is nonsense for LSR. This
    // prevents the DATRDY check below from returning TRUE, which could cause
    // a caller to think that data is pending when in actuality there is no
    // UART present.
    //

    Lsr = Port->Read(Port, COM_LSR);
    if (Lsr == SERIAL_LSR_NOT_PRESENT)
    {
        return FALSE;
    }

    //
    // Look at the Line Status Register to determine if there is pending data.
    //

    if (CHECK_FLAG(Lsr, COM_DATRDY))
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER Legacy16550HardwareDriver = {
    Uart16550LegacyInitializePort,
    Uart16550SetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady};

UART_HARDWARE_DRIVER Uart16550HardwareDriver = {
    Uart16550InitializePort,
    Uart16550SetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady};

UART_HARDWARE_DRIVER MM16550HardwareDriver = {
    Uart16550MmInitializePort,
    Uart16550SetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady};
