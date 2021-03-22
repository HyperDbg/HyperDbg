/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    bcm2835.c

Abstract:

    This module contains Broadcom BCM2835 mini UART serial support, for devices
    such as the Raspberry Pi 2 and 3.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

#define AUX_MU_IO_REG   0x40 // Data register
#define AUX_MU_IER_REG  0x44 // Interrupt Enable register
#define AUX_MU_LCR_REG  0x4C // Line Control register
#define AUX_MU_STAT_REG 0x64 // Line status register

#define AUX_MU_IER_TXE  0x00000001 // TX FIFO empty interrupt
#define AUX_MU_IER_RXNE 0x00000002 // RX FIFO not empty interrupt

//
// 8-bit mode: There is BCM2635 datasheet errata:
// http://elinux.org/BCM2835_datasheet_errata
// To get 8 bits we need to set the value of 0x03, not 0x01.
//

#define AUX_MU_LCR_8BIT 0x00000003

#define AUX_MU_STAT_RXNE 0x00000001 // RX FIFO not empty
#define AUX_MU_STAT_TXNF 0x00000002 // TX FIFO not full

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
Bcm2835RxReady(
    _Inout_ PCPPORT Port);

// ------------------------------------------------------------------ Functions

BOOLEAN
Bcm2835InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of a BCM2835 serial UART.

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

    ULONG IntEnable;

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    Port->Flags = 0;

    //
    // We cannot set the baud rate since we do not have the UART's clock value.
    // Moreover with Raspberry Pi 2 and 3, the UEFI firmware sets the baud rate
    // to 921600 bps, while BCDEdit limits it to 115200 bps.
    //

    Port->BaudRate = 0;

    //
    // Disable interrupts
    //

    IntEnable = READ_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_IER_REG));
    IntEnable &= ~(AUX_MU_IER_TXE | AUX_MU_IER_RXNE);
    WRITE_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_IER_REG), IntEnable);

    //
    // Set 8 bit mode
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_LCR_REG),
                         AUX_MU_LCR_8BIT);

    //
    // Mini UART RX/TX are always enabled!
    //

    return TRUE;
}

BOOLEAN
Bcm2835SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate)

/*++

Routine Description:

    The input clock frequency to the BCM2835 UART isn't discoverable, so don't
    attempt to update the hardware.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Rate - Unused

Return Value:

    FALSE always.

--*/

{
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
Bcm2835GetByte(
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
    ULONG Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Check to see if a byte is available.
    //

    if (Bcm2835RxReady(Port) != FALSE)
    {
        //
        // Fetch the data byte and associated error information.
        //

        Value = READ_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_IO_REG));

        //
        // Cannot check for errors as the mini UART does not provide this
        // information.
        //

        *Byte = Value & (UCHAR)0xFF;
        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
Bcm2835PutByte(
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
    ULONG StatusReg;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // If BusyWait is set, wait for FIFO to be not full. Otherwise, only check
    // one time.
    //

    if (BusyWait != FALSE)
    {
        do
        {
            StatusReg =
                READ_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_STAT_REG));

        } while ((StatusReg & AUX_MU_STAT_TXNF) == 0);
    }
    else
    {
        StatusReg =
            READ_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_STAT_REG));

        if ((StatusReg & AUX_MU_STAT_TXNF) == 0)
        {
            return UartNotReady;
        }
    }

    //
    // Send the byte
    //

    WRITE_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_IO_REG), (ULONG)Byte);
    return UartSuccess;
}

BOOLEAN
Bcm2835RxReady(
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
    ULONG StatusReg;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Read the Flag Register to determine if there is any pending
    // data to read.
    //

    StatusReg = READ_REGISTER_ULONG((PULONG)(Port->Address + AUX_MU_STAT_REG));

    //
    // Check the "receive FIFO not-empty" flag.  If it is set, then at least
    // one byte is available.
    //

    if ((StatusReg & AUX_MU_STAT_RXNE) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER Bcm2835HardwareDriver = {
    Bcm2835InitializePort,
    Bcm2835SetBaud,
    Bcm2835GetByte,
    Bcm2835PutByte,
    Bcm2835RxReady};
