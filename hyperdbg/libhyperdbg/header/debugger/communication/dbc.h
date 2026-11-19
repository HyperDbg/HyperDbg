/**
 * @file dbc.h
 * @author lava1879 (magicv4@proton.me)
 * @brief USB debugger transport declarations
 * @version 0.1
 * @date 2026-11-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

////////////////////////////////////////////////////////////////////////////
//                               Constants                                //
////////////////////////////////////////////////////////////////////////////

#define DBC_USB_VID             0x0525
#define DBC_USB_PID             0x127A
#define DBC_USB_MAX_PACKET_SIZE 1024
#define DBC_USB_PIPE_OUT        0x01
#define DBC_USB_PIPE_IN         0x81
#define DBC_USB_REOPEN_DELAY_MS 500

//
// USB handshake timing on the debuggee
//
#define DBC_DEBUGGEE_HANDSHAKE_TIMEOUT_SECONDS      60
#define DBC_DEBUGGEE_HANDSHAKE_READ_TIMEOUT_SECONDS 5
#define DBC_DEBUGGEE_POLL_INTERVAL_MS               5

////////////////////////////////////////////////////////////////////////////
//                               Functions                                //
////////////////////////////////////////////////////////////////////////////

HANDLE
DbcConnect();

BOOLEAN
DbcReadByte(HANDLE Handle, CHAR * OutByte, DWORD * BytesRead);

BOOLEAN
DbcWrite(HANDLE Handle, const VOID * Buffer, UINT32 Length);

BOOLEAN
DbcClose(HANDLE Handle);
