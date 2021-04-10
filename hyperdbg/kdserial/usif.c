/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    usif.c

Abstract:

    Intel Sofia UART serial support.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

#define USIF_FIFO_STAT       0x00000044 // FIFO status register
#define USIF_FIFO_STAT_TXFFS 0x00FF0000 // TX filled FIFO stages
#define USIF_FIFO_STAT_RXFFS 0x000000FF // RX filled FIFO stages
#define USIF_TXD             0x00040000 // Transmisson data register
#define USIF_RXD             0x00080000 // Reception data register

// ------------------------------------------------------------------ Functions

BOOLEAN
UsifInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of an Intel Sofia serial UART.

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
    DbgBreakPoint();
    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    Port->Flags = 0;
    return TRUE;
}

BOOLEAN
UsifSetBaud(
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
    DbgBreakPoint();

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Remember the baud rate.
    //

    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
UsifGetByte(
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
    DbgBreakPoint();

    ULONG Stat;
    ULONG Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Get FIFO status.
    //

    Stat = READ_REGISTER_ULONG((PULONG)(Port->Address + USIF_FIFO_STAT));

    //
    // Is at least one character available?
    //

    if ((Stat & USIF_FIFO_STAT_RXFFS) != 0)
    {
        //
        // Fetch the data byte.
        //

        Value = READ_REGISTER_ULONG((PULONG)(Port->Address + USIF_RXD));
        *Byte = Value & (UCHAR)0xFF;
        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
UsifPutByte(
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
    // Wait for port to be free and FIFO not full.
    //

    if (BusyWait != FALSE)
    {
        while (READ_REGISTER_ULONG((PULONG)(Port->Address + USIF_FIFO_STAT)) &
               (USIF_FIFO_STAT_TXFFS))
            ;
    }
    else if (READ_REGISTER_ULONG((PULONG)(Port->Address + USIF_FIFO_STAT)) &
             (USIF_FIFO_STAT_TXFFS))
    {
        return UartNotReady;
    }

    //
    // Send the byte.
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + USIF_TXD), (ULONG)Byte);
    return UartSuccess;
}

BOOLEAN
UsifRxReady(
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
    ULONG Stat;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Read the STAT Register to determine if there is any pending
    // data to read.
    //

    Stat = READ_REGISTER_ULONG((PULONG)(Port->Address + USIF_FIFO_STAT));
    if ((Stat & USIF_FIFO_STAT_RXFFS) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER UsifHardwareDriver = {
    UsifInitializePort,
    UsifSetBaud,
    UsifGetByte,
    UsifPutByte,
    UsifRxReady};
