/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ioaccess.c

Abstract:

    This module provides a function table consumed by the UART library.

--*/

#include <ntddk.h>
#include <uart.h>

#if defined(_X86_)

UCHAR
ReadPort8(
    PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);
}

USHORT
ReadPort16(
    PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}

ULONG
ReadPort32(
    PULONG Port)
{
    return READ_PORT_ULONG(Port);
}

VOID
WritePort8(
    PUCHAR      Port,
    const UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

VOID
WritePort16(
    PUSHORT      Port,
    const USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}

VOID
WritePort32(
    PULONG      Port,
    const ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}

UCHAR
ReadRegister8(
    PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}

USHORT
ReadRegister16(
    PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}

ULONG
ReadRegister32(
    PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}

VOID
WriteRegister8(
    PUCHAR      Register,
    const UCHAR Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

VOID
WriteRegister16(
    PUSHORT      Register,
    const USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
WriteRegister32(
    PULONG      Register,
    const ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}

#endif

UART_HARDWARE_ACCESS UartHardwareAccess = {

#if defined(_X86_)

    ReadPort8,
    WritePort8,
    ReadPort16,
    WritePort16,
    ReadPort32,
    WritePort32,

#elif defined(_AMD64_)

    READ_PORT_UCHAR,
    WRITE_PORT_UCHAR,
    READ_PORT_USHORT,
    WRITE_PORT_USHORT,
    READ_PORT_ULONG,
    WRITE_PORT_ULONG,

#else

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

#endif

#if defined(_X86_)

    ReadRegister8,
    WriteRegister8,
    ReadRegister16,
    WriteRegister16,
    ReadRegister32,
    WriteRegister32,

#else

    READ_REGISTER_UCHAR,
    WRITE_REGISTER_UCHAR,
    READ_REGISTER_USHORT,
    WRITE_REGISTER_USHORT,
    READ_REGISTER_ULONG,
    WRITE_REGISTER_ULONG,

#endif

#if defined(_WIN64)

    READ_REGISTER_ULONG64,
    WRITE_REGISTER_ULONG64

#else

    NULL,
    NULL

#endif

};