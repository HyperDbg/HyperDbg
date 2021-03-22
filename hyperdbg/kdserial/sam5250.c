/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sam5250.c

Abstract:

    This module contains support for the Samsung 5250 serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

#define ULCON   0x00
#define UCON    0x04
#define UFCON   0x08
#define UTRSTAT 0x10
#define UERSTAT 0x14
#define UFSTAT  0x18
#define UTXH    0x20
#define URXH    0x24
#define UINTP   0x30
#define UINTM   0x38

#define UFSTAT_TXFE  (1 << 24)
#define UTRSTAT_RXFE (1 << 0)

#define UERSTAT_OE (1 << 0)
#define UERSTAT_PE (1 << 1)
#define UERSTAT_FE (1 << 2)
#define UERSTAT_BE (1 << 3)

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
Sam5250SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

// ------------------------------------------------------------------ Functions

BOOLEAN
Sam5250InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of a Samsung 5250 serial UART.

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
    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    Port->Flags = 0;

    //
    // Disable UART.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UCON), 0);

    //
    // Set word length to 8 bits and disable parity.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + ULCON), 0x3);

    //
    // Enable the FIFO.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UFCON), 0x1);

    //
    // Mask all interrupts.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UINTM), 0xF);

    //
    // Clear all interrupts.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UINTP), 0xF);

    //
    // Enable UART, enable Transmit (mode: 01) and Receive (mode: 01).
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UCON), 0x5);
    return TRUE;
}

BOOLEAN
Sam5250SetBaud(
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
    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // There is no way to determine reference clock frequency for this UART so
    // just remember the desired baud rate but don't do anything else.
    //

    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
Sam5250GetByte(
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
    ULONG Fsr;
    ULONG Error;
    ULONG Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Get FIFO status.
    //

    Fsr = READ_REGISTER_ULONG((PULONG)(Port->Address + UTRSTAT));

    //
    // Is at least one character available?
    //

    if ((Fsr & UTRSTAT_RXFE) != 0)
    {
        //
        // Fetch the data byte and associated error information.
        //

        Value = READ_REGISTER_ULONG((PULONG)(Port->Address + URXH));

        //
        // Check for errors.  Deliberately don't treat overrun as an error.
        //

        Error = READ_REGISTER_ULONG((PULONG)(Port->Address + UERSTAT));
        if ((Error & (UERSTAT_PE | UERSTAT_FE | UERSTAT_BE)) != 0)
        {
            *Byte = 0;
            return UartError;
        }

        *Byte = Value & (UCHAR)0xFF;
        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
Sam5250PutByte(
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

Return Value:

    UART_STATUS code.

--*/

{
    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    //  Wait for port to be free and FIFO not full.
    //
    // _ARM_WORKAROUND_ Modem control is not supported.
    //

    if (BusyWait != FALSE)
    {
        while (READ_REGISTER_ULONG((PULONG)(Port->Address + UFSTAT)) & (UFSTAT_TXFE))
            ;
    }
    else
    {
        if (READ_REGISTER_ULONG((PULONG)(Port->Address + UFSTAT)) & (UFSTAT_TXFE))
        {
            return UartNotReady;
        }
    }

    //
    // Send the byte.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + UTXH), (ULONG)Byte);
    return UartSuccess;
}

BOOLEAN
Sam5250RxReady(
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
    ULONG Flags;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Read the Flag Register to determine if there is any pending data to read.
    //

    Flags = READ_REGISTER_ULONG((PULONG)(Port->Address + UTRSTAT));
    if ((Flags & UTRSTAT_RXFE) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER Sam5250HardwareDriver = {
    Sam5250InitializePort,
    Sam5250SetBaud,
    Sam5250GetByte,
    Sam5250PutByte,
    Sam5250RxReady};
