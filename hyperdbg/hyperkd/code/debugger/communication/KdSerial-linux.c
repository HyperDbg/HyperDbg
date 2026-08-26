/**
 * @file KdSerial-linux.c
 * @brief Linux placeholder stubs for the kdserial transport entry points.
 *
 * @details On Windows the KdHyperDbg* serial routines live in the kdserial KMDF
 * driver, which is dropped from the Linux module (Windows-only serial transport;
 * a Linux kdserial is planned as separate work). hyperkd's SerialConnection.c
 * still references them, so this TU provides no-op stubs purely so the single
 * .ko links. This is Kbuild-only; there is no Windows counterpart.
 *
 * TODO(Linux): implement the serial/remote-debugging transport (8250/16550 UART
 * or a virtio/socket channel) and drop this stub.
 */
#include "pch.h"

VOID
KdHyperDbgTest(UINT16 Byte)
{
    UNREFERENCED_PARAMETER(Byte);
}

VOID
KdHyperDbgPrepareDebuggeeConnectionPort(UINT32 PortAddress, UINT32 Baudrate)
{
    UNREFERENCED_PARAMETER(PortAddress);
    UNREFERENCED_PARAMETER(Baudrate);
}

VOID
KdHyperDbgSendByte(UCHAR Byte, BOOLEAN BusyWait)
{
    UNREFERENCED_PARAMETER(Byte);
    UNREFERENCED_PARAMETER(BusyWait);
}

BOOLEAN
KdHyperDbgRecvByte(PUCHAR RecvByte)
{
    UNREFERENCED_PARAMETER(RecvByte);

    return FALSE;
}
