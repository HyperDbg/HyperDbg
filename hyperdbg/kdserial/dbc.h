/**
 * @file dbc.h
 * @author lava1879 (magicv4@proton.me)
 * @brief xHCI Debug Capability (DbC) interface for the debuggee
 *
 * @version 0.1
 * @date 2026-11-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

//
// Port value that selects DbC instead of a serial port
//
#define SERIAL_PORT_DBC 0xDBC0

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
DbcPrepare(UINT32 PortAddress);

BOOLEAN
DbcIsActive();

VOID
DbcSendByte(UCHAR Byte);

BOOLEAN
DbcRecvByte(PUCHAR RecvByte);

VOID
DbcUnload();

BOOLEAN
KdHyperDbgDbcIsActive();

BOOLEAN
KdHyperDbgDbcIsReady();

VOID
KdHyperDbgDbcSetLongWait(BOOLEAN Allow);

BOOLEAN
KdHyperDbgDbcSetSendWait(BOOLEAN Allow);

VOID
KdHyperDbgDbcSetControlPriority(BOOLEAN Enable);

BOOLEAN
KdHyperDbgDbcCanSendPacket(UINT64 Length);

BOOLEAN
KdHyperDbgDbcWaitForSend(UINT32 Milliseconds);

BOOLEAN
KdHyperDbgDbcWaitForSendEx(UINT32 Milliseconds, BOOLEAN IgnoreLoss);

VOID
KdHyperDbgDbcClearSendLoss();

UINT32
KdHyperDbgDbcGetSendLossCount();
