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

/**
 * @brief Report no USB selection without a USB transport
 *
 * @return BOOLEAN FALSE
 */
BOOLEAN
KdHyperDbgDbcIsActive()
{
    return FALSE;
}

BOOLEAN
KdHyperDbgDbcIsReady()
{
    return FALSE;
}

/**
 * @brief Ignore the USB send wait mode without a USB transport
 *
 * @param Allow Requested wait mode
 * @return VOID
 */
VOID
KdHyperDbgDbcSetLongWait(BOOLEAN Allow)
{
    UNREFERENCED_PARAMETER(Allow);
}

/**
 * @brief Leave the send wait mode unchanged without a USB transport
 *
 * @param Allow Requested wait mode
 * @return BOOLEAN FALSE
 */
BOOLEAN
KdHyperDbgDbcSetSendWait(BOOLEAN Allow)
{
    UNREFERENCED_PARAMETER(Allow);

    return FALSE;
}

/**
 * @brief Ignore protected send priority without a USB transport
 *
 * @param Enable Requested priority scope change
 * @return VOID
 */
VOID
KdHyperDbgDbcSetControlPriority(BOOLEAN Enable)
{
    UNREFERENCED_PARAMETER(Enable);
}

/**
 * @brief Keep event output unchanged without a USB transport
 *
 * @param Length Total packet length
 * @return BOOLEAN FALSE
 */
BOOLEAN
KdHyperDbgDbcDropEventPacket(UINT32 Length)
{
    UNREFERENCED_PARAMETER(Length);

    return FALSE;
}

/**
 * @brief Leave packet space checks unchanged without a USB transport
 *
 * @param Length Total packet length
 * @return BOOLEAN TRUE
 */
BOOLEAN
KdHyperDbgDbcCanSendPacket(UINT64 Length)
{
    UNREFERENCED_PARAMETER(Length);

    return TRUE;
}

/**
 * @brief Report no pending USB sends without a USB transport
 *
 * @param Milliseconds Requested wait time
 * @return BOOLEAN TRUE
 */
BOOLEAN
KdHyperDbgDbcWaitForSend(UINT32 Milliseconds)
{
    UNREFERENCED_PARAMETER(Milliseconds);

    return TRUE;
}

/**
 * @brief Report no pending USB transfers without a USB transport
 *
 * @param Milliseconds Requested wait time
 * @param IgnoreLoss Requested loss check
 * @return BOOLEAN TRUE
 */
BOOLEAN
KdHyperDbgDbcWaitForSendEx(UINT32 Milliseconds, BOOLEAN IgnoreLoss)
{
    UNREFERENCED_PARAMETER(Milliseconds);
    UNREFERENCED_PARAMETER(IgnoreLoss);

    return TRUE;
}

/**
 * @brief Ignore a USB delivery loss reset without a USB transport
 *
 * @return VOID
 */
VOID
KdHyperDbgDbcClearSendLoss()
{
}

/**
 * @brief Report no USB delivery loss without a USB transport
 *
 * @return UINT32 Zero
 */
UINT32
KdHyperDbgDbcGetSendLossCount()
{
    return 0;
}
