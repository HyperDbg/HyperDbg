/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    uartio.c

Abstract:

    This file defines I/O functions and the function pointers used by the serial
    drivers to access the device hardware.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

//
// Maximum addressable register width varies with platform architecture.
//

#if defined(_WIN64)

#    define MAX_REGISTER_WIDTH 64

#else

#    define MAX_REGISTER_WIDTH 32

#endif

// ----------------------------------------------------------------- Date Types

// ------------------------------------------------------------------ Functions

//
// Memory-mapped I/O Routines.
//

static VOID
WriteRegisterWithIndex8(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PUCHAR Pointer;

    Pointer = (PUCHAR)(Port->Address + Index * Port->ByteWidth);
    WRITE_REGISTER_UCHAR(Pointer, Value);
    return;
}

static UCHAR
ReadRegisterWithIndex8(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PUCHAR Pointer;

    Pointer = (PUCHAR)(Port->Address + Index * Port->ByteWidth);
    return READ_REGISTER_UCHAR(Pointer);
}

static VOID
WriteRegisterWithIndex16(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PUSHORT Pointer;

    Pointer = (PUSHORT)(Port->Address + Index * Port->ByteWidth);
    WRITE_REGISTER_USHORT(Pointer, Value);
    return;
}

static UCHAR
ReadRegisterWithIndex16(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PUSHORT Pointer;

    Pointer = (PUSHORT)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_REGISTER_USHORT(Pointer);
}

static VOID
WriteRegisterWithIndex32(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PULONG Pointer;

    Pointer = (PULONG)(Port->Address + Index * Port->ByteWidth);
    WRITE_REGISTER_ULONG(Pointer, Value);
    return;
}

static UCHAR
ReadRegisterWithIndex32(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PULONG Pointer;

    Pointer = (PULONG)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_REGISTER_ULONG(Pointer);
}

#if defined(_WIN64)

static VOID
WriteRegisterWithIndex64(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PULONG64 Pointer;

    Pointer = (PULONG64)(Port->Address + Index * Port->ByteWidth);
    WRITE_REGISTER_ULONG64(Pointer, Value);
    return;
}

static UCHAR
ReadRegisterWithIndex64(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PULONG64 Pointer;

    Pointer = (PULONG64)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_REGISTER_ULONG64(Pointer);
}

#endif

//
// Port I/O Functions. (Only available on architectures that have I/O ports.)
//

#if defined(_X86_) || defined(_AMD64_)

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

static VOID
WritePortWithIndex16(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PUSHORT Pointer;

    Pointer = (PUSHORT)(Port->Address + Index * Port->ByteWidth);
    WRITE_PORT_USHORT(Pointer, Value);
    return;
}

static UCHAR
ReadPortWithIndex16(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PUSHORT Pointer;

    Pointer = (PUSHORT)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_PORT_USHORT(Pointer);
}

static VOID
WritePortWithIndex32(
    _In_ PCPPORT Port,
    const UCHAR  Index,
    const UCHAR  Value)
{
    PULONG Pointer;

    Pointer = (PULONG)(Port->Address + Index * Port->ByteWidth);
    WRITE_PORT_ULONG(Pointer, Value);
    return;
}

static UCHAR
ReadPortWithIndex32(
    _In_ PCPPORT Port,
    const UCHAR  Index)
{
    PULONG Pointer;

    Pointer = (PULONG)(Port->Address + Index * Port->ByteWidth);
    return (UCHAR)READ_PORT_ULONG(Pointer);
}

#endif

BOOLEAN
UartpSetAccess(
    _Inout_ PCPPORT Port,
    const BOOLEAN   MemoryMapped,
    const UCHAR     AccessSize,
    const UCHAR     BitWidth)

/*++

Routine Description:

    This routine sets the access type (port I/O or memory mapped I/O), access
    size and bit width for the given port. This routine must be called before
    attempting to access the UART hardware.

Arguments:

    Port - Supplies a pointer to the structure that holds the port's state.

    MemoryMapped - TRUE if the port uses MMIO, FALSE if it uses legacy port I/O.

    AccessSize - Supplies the ACPI Generic Access Size of the register's bus.

    BitWidth - Supplies the size in bits of the device's registers.

Return value:

    TRUE if the pointers were assigned, FALSE if a parameter was not valid.

--*/

{
    UCHAR                             MinRegisterWidth;
    BOOLEAN                           PowerOfTwo;
    UART_HARDWARE_READ_INDEXED_UCHAR  ReadFunction;
    UART_HARDWARE_WRITE_INDEXED_UCHAR WriteFunction;

    MinRegisterWidth = 8;

    //
    // Select the appropriate port access routines depending upon whether this
    // serial port is mapped into memory or I/O space.
    //

    if (MemoryMapped == FALSE)
    {
        switch ((ACPI_GENERIC_ACCESS_SIZE)AccessSize)
        {
#if defined(_X86_) || defined(_AMD64_)

        case AcpiGenericAccessSizeLegacy:
            __fallthrough;

        case AcpiGenericAccessSizeByte:
            WriteFunction = WritePortWithIndex8;
            ReadFunction  = ReadPortWithIndex8;
            break;

        case AcpiGenericAccessSizeWord:
            WriteFunction    = WritePortWithIndex16;
            ReadFunction     = ReadPortWithIndex16;
            MinRegisterWidth = 16;
            break;

        case AcpiGenericAccessSizeDWord:
            WriteFunction    = WritePortWithIndex32;
            ReadFunction     = ReadPortWithIndex32;
            MinRegisterWidth = 32;
            break;

            //
            // The quad word access size isn't supported for port based I/O.
            //

#endif

        default:
            return FALSE;
        }
    }
    else
    {
        switch ((ACPI_GENERIC_ACCESS_SIZE)AccessSize)
        {
        case AcpiGenericAccessSizeLegacy:
            __fallthrough;

        case AcpiGenericAccessSizeByte:
            WriteFunction = WriteRegisterWithIndex8;
            ReadFunction  = ReadRegisterWithIndex8;
            break;

        case AcpiGenericAccessSizeWord:
            WriteFunction    = WriteRegisterWithIndex16;
            ReadFunction     = ReadRegisterWithIndex16;
            MinRegisterWidth = 16;
            break;

        case AcpiGenericAccessSizeDWord:
            WriteFunction    = WriteRegisterWithIndex32;
            ReadFunction     = ReadRegisterWithIndex32;
            MinRegisterWidth = 32;
            break;

#if defined(_WIN64)

        case AcpiGenericAccessSizeQWord:
            WriteFunction    = WriteRegisterWithIndex64;
            ReadFunction     = ReadRegisterWithIndex64;
            MinRegisterWidth = 64;
            break;

#endif

        default:
            return FALSE;
        }
    }

    //
    // Validate BitWidth parameter.
    //

    PowerOfTwo = ((BitWidth & (BitWidth - 1)) == 0);
    if ((PowerOfTwo == FALSE) ||
        (BitWidth < MinRegisterWidth) ||
        (BitWidth > MAX_REGISTER_WIDTH))
    {
        return FALSE;
    }

    Port->ByteWidth = BitWidth / 8;
    Port->Write     = WriteFunction;
    Port->Read      = ReadFunction;
    return TRUE;
}
