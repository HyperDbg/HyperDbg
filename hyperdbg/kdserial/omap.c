/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    omap.c

Abstract:

    This module contains support for the TI OMAP serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"
#include "kdcom.h"

// ---------------------------------------------------------------- Definitions

#undef CLOCK_RATE
#define CLOCK_RATE   2995200ul
#define COM_EFR      0x2
#define COM_MDR1     0x8
#define LC_DATA_SIZE 0x03

// ----------------------------------------------------------------- Prototypes

UART_STATUS
Uart16550GetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR    Byte);

UART_STATUS
Uart16550PutByte(
    _Inout_ PCPPORT Port,
    UCHAR           Byte,
    BOOLEAN         BusyWait);

BOOLEAN
Uart16550RxReady(
    _Inout_ PCPPORT Port);

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
OmapSetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

// ------------------------------------------------------------------ Functions

BOOLEAN
OmapInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of a TI OMAP serial UART.

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

    Port->Flags = 0;

    //
    // Set the Read / Write function pointers for this serial port.
    //

    UartpSetAccess(Port, MemoryMapped, AcpiGenericAccessSizeByte, 32);

    //
    // Set baud Rate.
    //

    OmapSetBaud(Port, Port->BaudRate);

    //
    // Put in operational mode.
    //

    Port->Write(Port, COM_LCR, Port->Read(Port, COM_LCR) & ~LC_DLAB);

    //
    // Disable device interrupts.
    //

    Port->Write(Port, COM_IEN, 0);

    //
    // Reset and disable the FIFO queue.
    //

    Port->Write(Port, COM_FCR, (FC_CLEAR_TRANSMIT | FC_CLEAR_RECEIVE));

    //
    // Configure the Modem Control Register. Disable device interrupts and
    // turn off loopback.
    //

    Port->Write(Port, COM_MCR, Port->Read(Port, COM_MCR) & MC_DTRRTS);

    //
    // Initialize the Modem Control Register. Indicate to the device that
    // we are able to send and receive data.
    //

    Port->Write(Port, COM_MCR, MC_DTRRTS);

    //
    // Enable the FIFO queues.
    //

    Port->Write(Port, COM_FCR, FC_ENABLE);
    return TRUE;
}

BOOLEAN
OmapSetBaud(
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
    ULONG DivisorLatch;
    UCHAR Enhanced;

    UNREFERENCED_PARAMETER(Rate);

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Disable UART.
    //

    Port->Write(Port, COM_MDR1, 0x7);

    //
    // Set register configuration mode B.
    //

    Port->Write(Port, COM_LCR, 0xBF);

    //
    // Set enhanced mode.
    //

    Enhanced = Port->Read(Port, COM_EFR);
    Port->Write(Port, COM_EFR, (Enhanced | (1 << 4)));

    //
    // Switch to operational mode.
    //

    Port->Write(Port, COM_LCR, 0);

    //
    // Clear sleep mode.
    //

    Port->Write(Port, COM_IEN, 0);

    //
    // Set register configuration mode B.
    //

    Port->Write(Port, COM_LCR, 0xBF);

    //
    // Compute the divsor.
    //

    DivisorLatch = CLOCK_RATE / 115200;

    //
    // Write the divisor latch value to DLL and DLM.
    //

    Port->Write(Port, COM_DLM, (UCHAR)((DivisorLatch >> 8) & 0xFF));
    Port->Write(Port, COM_DLL, (UCHAR)(DivisorLatch & 0xFF));

    //
    // Restore enhanced mode.
    //

    Port->Write(Port, COM_EFR, Enhanced);

    //
    // Reset the Line Control Register.
    //

    Port->Write(Port, COM_LCR, LC_DATA_SIZE);

    //
    // Enable UART.
    //

    Port->Write(Port, COM_MDR1, 0);
    return TRUE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER OmapHardwareDriver = {
    OmapInitializePort,
    OmapSetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady};
