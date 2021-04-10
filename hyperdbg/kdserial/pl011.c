/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pl011.c

Abstract:

    This module contains support for the PL011/SBSA serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

#define UART_DR    0x00 // Data Register
#define UART_RSR   0x04 // Receive status register (read)
#define UART_ECR   0x04 // Error clear register (write)
#define UART_FR    0x18 // Flag register (read only)
#define UART_ILPR  0x20 // IrDA low-power counter register
#define UART_IBRD  0x24 // Integer baud rate divisor register
#define UART_FBRD  0x28 // Fractional baud rate divisor register
#define UART_LCRH  0x2C // Line Control register, HIGH byte
#define UART_CR    0x30 // Control register
#define UART_IFLS  0x34 // Interrupt FIFO level select register
#define UART_IMSC  0x38 // Interrupt mask set/clear
#define UART_RIS   0x3C // Raw Interrrupt status
#define UART_MIS   0x40 // Masked interrupt status
#define UART_ICR   0x44 // Interrupt clear register
#define UART_DMACR 0x48 // DMA control register

#define TOTAL_UART_REGISTER_SIZE 0x4C

//
// Register Masks
//

#define UART_FR_TXFE 0x80 // UART_FR flag, xmit buffer empty
#define UART_FR_RXFF 0x40 // UART_FR flag, receive buffer full
#define UART_FR_TXFF 0x20 // UART_FR flag, xmit buffer full
#define UART_FR_RXFE 0x10 // UART_FR flag, receive buffer empty
#define UART_FR_BUSY 0x08 // UART_FR flag, UART busy

#define UART_LCRH_SPS    0x80
#define UART_LCRH_WLEN_8 0x60
#define UART_LCRH_WLEN_7 0x40
#define UART_LCRH_WLEN_6 0x20
#define UART_LCRH_WLEN_5 0x00
#define UART_LCRH_FEN    0x10
#define UART_LCRH_STP2   0x08
#define UART_LCRH_EPS    0x04
#define UART_LCRH_PEN    0x02
#define UART_LCRH_BRK    0x01

#define UART_CR_CTSEn  0x8000
#define UART_CR_RTSEn  0x4000
#define UART_CR_OUT2   0x2000
#define UART_CR_OUT1   0x1000
#define UART_CR_RTS    0x0800
#define UART_CR_DTR    0x0400
#define UART_CR_RXE    0x0200
#define UART_CR_TXE    0x0100
#define UART_CR_LBE    0x0080
#define UART_CR_SIRLP  0x0004
#define UART_CR_SIREN  0x0002
#define UART_CR_UARTEN 0x0001

#define UART_DR_OE 0x800 // UART_DR flag, overrun error
#define UART_DR_BE 0x400 // UART_DR flag, break error
#define UART_DR_PE 0x200 // UART_DR flag, parity error
#define UART_DR_FE 0x100 // UART_DR flag, framing error

// --------------------------------------------------------------------- Macros

#define PL011_READ_REGISTER_UCHAR(a, f) \
    (UCHAR)((f) ? READ_REGISTER_ULONG((PULONG)(a)) : READ_REGISTER_UCHAR(a))

#define PL011_READ_REGISTER_USHORT(a, f) \
    (USHORT)((f) ? READ_REGISTER_ULONG((PULONG)(a)) : READ_REGISTER_USHORT(a))

#define PL011_WRITE_REGISTER_UCHAR(a, d, f) \
    ((f) ? WRITE_REGISTER_ULONG((PULONG)(a), d) : WRITE_REGISTER_UCHAR(a, d))

#define PL011_WRITE_REGISTER_USHORT(a, d, f) \
    ((f) ? WRITE_REGISTER_ULONG((PULONG)(a), d) : WRITE_REGISTER_USHORT(a, d))

// ------------------------------------------------------------------ Functions

BOOLEAN
PL011InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of a PL011 serial UART.

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
    UINT16  Cr;
    BOOLEAN Force32Bit;
    UINT16  Fsr;

    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    if (BitWidth == 32)
    {
        Port->Flags = PORT_FORCE_32BIT_IO;
        Force32Bit  = TRUE;
    }
    else
    {
        Port->Flags = 0;
        Force32Bit  = FALSE;
    }

    //
    // Disable UART.
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                0,
                                Force32Bit);

    //
    // Flush the FIFO by disabling the FIFO.
    // FIFO need to be flushed before re-enabling the UART.
    //

    do
    {
        PL011_WRITE_REGISTER_UCHAR((PUCHAR)(Port->Address + UART_LCRH), 0, FALSE);
        Fsr = PL011_READ_REGISTER_USHORT((PUSHORT)(Port->Address + UART_FR), FALSE);

    } while ((Fsr & (UART_FR_RXFE | UART_FR_TXFE)) != (UART_FR_RXFE | UART_FR_TXFE));

    //
    // Set word length to 8 bits, enable the FIFO, disable parity.
    //

    PL011_WRITE_REGISTER_UCHAR(Port->Address + UART_LCRH,
                               (UART_LCRH_WLEN_8 | UART_LCRH_FEN),
                               Force32Bit);

    //
    // Clear all interrupts
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_IMSC),
                                0x0000,
                                Force32Bit);

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_ICR),
                                0x07FF,
                                Force32Bit);

    //
    // Enable UART, Enable Transmit and Receive, Enable RTS.
    //
    // The RTS hardware flow control is required to ensure RX FIFO won't be
    // overflowed on simulator.
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                (UART_CR_RTSEn | UART_CR_RXE | UART_CR_TXE),
                                Force32Bit);

    Cr = PL011_READ_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                    Force32Bit);

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                (Cr | UART_CR_UARTEN),
                                Force32Bit);

    return TRUE;
}

BOOLEAN
SBSAInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of an SBSA serial UART.

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
    UINT16  Cr;
    BOOLEAN Force32Bit;

    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    if (BitWidth == 32)
    {
        Port->Flags = PORT_FORCE_32BIT_IO;
        Force32Bit  = TRUE;
    }
    else
    {
        Port->Flags = 0;
        Force32Bit  = FALSE;
    }

    //
    // Disable UART.
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                0,
                                Force32Bit);

    //
    // Set word length to 8 bits, enable the FIFO, disable parity.
    //

    PL011_WRITE_REGISTER_UCHAR(Port->Address + UART_LCRH,
                               (UART_LCRH_WLEN_8 | UART_LCRH_FEN),
                               Force32Bit);

    //
    // Clear all interrupts
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_IMSC),
                                0x0000,
                                Force32Bit);

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_ICR),
                                0x07FF,
                                Force32Bit);

    //
    // Enable UART, Enable Transmit and Receive, Enable RTS.
    //
    // The RTS hardware flow control is required to ensure RX FIFO won't be
    // overflowed on simulator.
    //

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                (UART_CR_RTSEn | UART_CR_RXE | UART_CR_TXE),
                                Force32Bit);

    Cr = PL011_READ_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                    Force32Bit);

    PL011_WRITE_REGISTER_USHORT((PUSHORT)(Port->Address + UART_CR),
                                (Cr | UART_CR_UARTEN),
                                Force32Bit);

    return TRUE;
}

BOOLEAN
SBSA32InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of an SBSA32 serial UART.

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
    UNREFERENCED_PARAMETER(BitWidth);

    return SBSAInitializePort(LoadOptions, Port, MemoryMapped, AccessSize, 32);
}

BOOLEAN
PL011SetBaud(
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
    // Remember the baud rate.
    //

    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
PL011GetByte(
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
    BOOLEAN Force32Bit;
    USHORT  Fsr;
    USHORT  Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    Force32Bit = ((Port->Flags & PORT_FORCE_32BIT_IO) != 0);

    //
    // Get FIFO status.
    //

    Fsr = PL011_READ_REGISTER_USHORT((PUSHORT)(Port->Address + UART_FR),
                                     Force32Bit);

    //
    // Is at least one character available?
    //

    if ((Fsr & UART_FR_RXFE) == 0)
    {
        //
        // Fetch the data byte and associated error information.
        //

        Value =
            PL011_READ_REGISTER_USHORT((PUSHORT)(Port->Address + UART_DR),
                                       Force32Bit);

        //
        // Check for errors. Deliberately don't treat overrun as an error.
        //

        if ((Value & (UART_DR_PE | UART_DR_FE | UART_DR_BE)) != 0)
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
PL011PutByte(
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
    BOOLEAN Force32Bit;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    Force32Bit = ((Port->Flags & PORT_FORCE_32BIT_IO) != 0);

    //
    // Wait for port to be free and FIFO not full.
    //
    // _ARM_WORKAROUND_ modem control is not supported
    //

    if (BusyWait != FALSE)
    {
        while (PL011_READ_REGISTER_USHORT(
                   (PUSHORT)(Port->Address + UART_FR),
                   Force32Bit) &
               (UART_FR_TXFF))
            ;
    }
    else
    {
        if (PL011_READ_REGISTER_USHORT(
                (PUSHORT)(Port->Address + UART_FR),
                Force32Bit) &
            (UART_FR_TXFF))
        {
            return UartNotReady;
        }
    }

    //
    // Send the byte.
    //

    PL011_WRITE_REGISTER_UCHAR(Port->Address + UART_DR, Byte, Force32Bit);
    return UartSuccess;
}

BOOLEAN
PL011RxReady(
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
    PUCHAR  BaseAddress;
    USHORT  Flags;
    BOOLEAN Force32Bit;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Read the Flag Register to determine if there is any pending
    // data to read.
    //

    BaseAddress = Port->Address;
    Force32Bit  = ((Port->Flags & PORT_FORCE_32BIT_IO) != 0);
    Flags       = PL011_READ_REGISTER_USHORT((PUSHORT)(BaseAddress + UART_FR),
                                       Force32Bit);

    //
    // Check the "receive FIFO empty" flag. If it is clear, then at least one
    // byte is available.
    //

    if (CHECK_FLAG(Flags, UART_FR_RXFE) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER PL011HardwareDriver = {
    PL011InitializePort,
    PL011SetBaud,
    PL011GetByte,
    PL011PutByte,
    PL011RxReady};

UART_HARDWARE_DRIVER SBSAHardwareDriver = {
    SBSAInitializePort,
    PL011SetBaud,
    PL011GetByte,
    PL011PutByte,
    PL011RxReady};

UART_HARDWARE_DRIVER SBSA32HardwareDriver = {
    SBSA32InitializePort,
    PL011SetBaud,
    PL011GetByte,
    PL011PutByte,
    PL011RxReady};
