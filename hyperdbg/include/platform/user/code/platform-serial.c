/**
 * @file platform-serial.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode cross-platform implementation of the kernel-debugger serial transport
 * @details See platform-serial.h. The Windows branch wraps the Win32 serial primitives
 *          (CreateFile / Comm* / overlapped ReadFile/WriteFile) and owns the per-direction
 *          OVERLAPPED state internally so the protocol layer never sees it. The Linux
 *          branch is currently stubbed (returns FALSE/NULL) and is the home where the
 *          termios-based implementation over /dev/tty* will live.
 *
 * @version 0.20
 * @date 2026-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-serial.h"
#endif // defined(__linux__)

#if defined(_WIN32)

//
// Not implemented here
//

#elif defined(__linux__)

//
// TODO: implement the serial transport on Linux using termios over /dev/tty*:
//   - PlatformSerialOpen      -> open(PortName, O_RDWR | O_NOCTTY)
//   - PlatformSerialConfigure -> tcgetattr/cfsetspeed/tcsetattr (raw, 8-N-1)
//   - PlatformSerialReadByte  -> read() (with VTIME/VMIN or poll() for the timeout role)
//   - PlatformSerialWrite     -> write()
//   - PlatformSerialClose     -> close()
// Named-pipe transport would map onto a UNIX domain socket / FIFO.
//
// Until then these return failure so the kernel-debugger connection simply
// reports "not supported on Linux yet" rather than crashing.
//

HANDLE
PlatformSerialOpen(const char * PortName, PLATFORM_SERIAL_IO_ROLE Role)
{
    (void)PortName;
    (void)Role;
    return NULL;
}

BOOLEAN
PlatformSerialConfigure(HANDLE Handle, DWORD BaudRate)
{
    (void)Handle;
    (void)BaudRate;
    return FALSE;
}

BOOLEAN
PlatformSerialReadByte(HANDLE                  Handle,
                       CHAR *                  OutByte,
                       DWORD *                 BytesRead,
                       PLATFORM_SERIAL_IO_ROLE Role)
{
    (void)Handle;
    (void)OutByte;
    (void)Role;
    if (BytesRead)
        *BytesRead = 0;
    return FALSE;
}

BOOLEAN
PlatformSerialWrite(HANDLE Handle, const void * Buffer, UINT32 Length, BOOLEAN Synchronous)
{
    (void)Handle;
    (void)Buffer;
    (void)Length;
    (void)Synchronous;
    return FALSE;
}

#else
#    error "Unsupported platform"
#endif
