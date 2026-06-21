/**
 * @file platform-serial.h
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode cross-platform interface for the kernel-debugger serial transport
 * @details The kernel-debugging *protocol* in kd.cpp is platform independent; only
 *          the byte transport underneath it (serial COM port / named pipe) is OS
 *          specific. This interface isolates those primitives so the protocol layer
 *          can stay shared. Windows maps onto Win32 (CreateFile / Comm* / overlapped
 *          ReadFile/WriteFile); Linux will map onto termios over /dev/tty* (TODO).
 *
 * @version 0.20
 * @date 2026-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//
// Distinguishes the two I/O modes the transport uses. On Windows the debugger
// side opens the port for overlapped (async) I/O while the debuggee side uses
// blocking I/O; this enum lets the platform layer pick the right mechanism
// without leaking OVERLAPPED into the protocol code.
//
typedef enum _PLATFORM_SERIAL_IO_ROLE
{
    PLATFORM_SERIAL_IO_DEBUGGER, // overlapped / async reads
    PLATFORM_SERIAL_IO_DEBUGGEE, // blocking reads (with comm timeout)

} PLATFORM_SERIAL_IO_ROLE;

//
// OPEN a serial COM port by name for the given role.
// Returns a transport handle, or NULL on failure.
//
HANDLE
PlatformSerialOpen(const char * PortName, PLATFORM_SERIAL_IO_ROLE Role);

//
// CONFIGURE baud rate and the fixed 8-N-1 framing the protocol expects.
//
BOOLEAN
PlatformSerialConfigure(HANDLE Handle, DWORD BaudRate);

//
// READ a single byte. *BytesRead is set to the number of bytes actually read.
// The caller (protocol layer) owns the packet-assembly loop.
//
BOOLEAN
PlatformSerialReadByte(HANDLE                  Handle,
                       CHAR *                  OutByte,
                       DWORD *                 BytesRead,
                       PLATFORM_SERIAL_IO_ROLE Role);

//
// WRITE a buffer. Synchronous selects blocking write (debuggee/handshaking)
// versus overlapped write (debugger).
//
BOOLEAN
PlatformSerialWrite(HANDLE Handle, const void * Buffer, UINT32 Length, BOOLEAN Synchronous);

//
// CLOSE the transport handle and release any associated OS resources.
//
BOOLEAN
PlatformSerialClose(HANDLE Handle);
