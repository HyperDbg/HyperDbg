/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    nvidia.c

Abstract:

    This code supports the Nvidia 16550 variation found in their ARM SoCs.
    It differs from the standard 16550 in these ways:
      * this implementation does not configure the baud rate register
      * register alignment - registers are aligned on 32 bit boundaries

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ----------------------------------------------------------------- Prototypes

BOOLEAN
Uart16550InitializePortCommon(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth);

BOOLEAN
Uart16550SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

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

// ------------------------------------------------------------------ Functions

BOOLEAN
NvidiaInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of an Nvidia 16550 serial UART.

Arguments:

    LoadOptions - Optional load option string.

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
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    //
    // Initialize the flags and set the bit to disable baud rate setting.
    //

    Port->Flags = PORT_DEFAULT_RATE;
    return Uart16550InitializePortCommon(LoadOptions,
                                         Port,
                                         MemoryMapped,
                                         AcpiGenericAccessSizeByte,
                                         32);
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER NvidiaHardwareDriver = {
    NvidiaInitializePort,
    Uart16550SetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady};
