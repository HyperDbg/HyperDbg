/**
 * @file platform-serial.c
 * @author Max Raulea (max.raulea@gmail.com)
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
// Per-direction overlapped I/O state, owned by the transport layer.
//
static OVERLAPPED g_PlatformOverlappedReadDebugger  = {0};
static OVERLAPPED g_PlatformOverlappedReadDebuggee  = {0};
static OVERLAPPED g_PlatformOverlappedWriteDebugger = {0};

HANDLE
PlatformSerialOpen(const char * PortName, PLATFORM_SERIAL_IO_ROLE Role)
{
    HANDLE Comm;
    char   PortNo[24] = {0};

    //
    // Append name to make a Windows-understandable format
    //
    sprintf_s(PortNo, sizeof(PortNo), "\\\\.\\%s", PortName);

    if (Role == PLATFORM_SERIAL_IO_DEBUGGEE)
    {
        //
        // Debuggee uses non-overlapped (blocking) I/O
        //
        Comm = CreateFile(PortNo,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL);

        g_PlatformOverlappedReadDebuggee.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    else
    {
        //
        // Debugger uses overlapped (async) I/O
        //
        Comm = CreateFile(PortNo,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_OVERLAPPED,
                          NULL);

        g_PlatformOverlappedReadDebugger.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
        g_PlatformOverlappedWriteDebugger.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    if (Comm == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    //
    // Purge the serial port
    //
    PurgeComm(Comm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    return Comm;
}

BOOLEAN
PlatformSerialConfigure(HANDLE Handle, DWORD BaudRate)
{
    DCB SerialParams       = {0};
    SerialParams.DCBlength = sizeof(SerialParams);

    if (GetCommState(Handle, &SerialParams) == FALSE)
    {
        return FALSE;
    }

    SerialParams.BaudRate = BaudRate;
    SerialParams.ByteSize = 8;
    SerialParams.StopBits = ONESTOPBIT;
    SerialParams.Parity   = NOPARITY;

    if (SetCommState(Handle, &SerialParams) == FALSE)
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
PlatformSerialReadByte(HANDLE                  Handle,
                       CHAR *                  OutByte,
                       DWORD *                 BytesRead,
                       PLATFORM_SERIAL_IO_ROLE Role)
{
    OVERLAPPED * Ovl = (Role == PLATFORM_SERIAL_IO_DEBUGGEE)
                           ? &g_PlatformOverlappedReadDebuggee
                           : &g_PlatformOverlappedReadDebugger;

    if (Role == PLATFORM_SERIAL_IO_DEBUGGEE)
    {
        //
        // Apply a read timeout for the debuggee side
        //
        COMMTIMEOUTS Timeouts;
        GetCommTimeouts(Handle, &Timeouts);
        Timeouts.ReadIntervalTimeout         = MAXDWORD;
        Timeouts.ReadTotalTimeoutConstant    = 5000;
        Timeouts.ReadTotalTimeoutMultiplier  = 0;
        Timeouts.WriteTotalTimeoutConstant   = 0;
        Timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(Handle, &Timeouts);
    }

    if (!ReadFile(Handle, OutByte, sizeof(CHAR), NULL, Ovl))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            return FALSE;
        }
    }

    WaitForSingleObject(Ovl->hEvent, INFINITE);
    GetOverlappedResult(Handle, Ovl, BytesRead, FALSE);
    ResetEvent(Ovl->hEvent);

    return TRUE;
}

BOOLEAN
PlatformSerialWrite(HANDLE Handle, const void * Buffer, UINT32 Length, BOOLEAN Synchronous)
{
    if (Synchronous)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(Handle, Buffer, Length, &BytesWritten, NULL) == FALSE)
        {
            return FALSE;
        }
        return (BytesWritten == Length);
    }
    else
    {
        if (WriteFile(Handle, Buffer, Length, NULL, &g_PlatformOverlappedWriteDebugger))
        {
            return TRUE;
        }

        if (GetLastError() != ERROR_IO_PENDING)
        {
            return FALSE;
        }

        if (WaitForSingleObject(g_PlatformOverlappedWriteDebugger.hEvent, INFINITE) != WAIT_OBJECT_0)
        {
            return FALSE;
        }

        ResetEvent(g_PlatformOverlappedWriteDebugger.hEvent);
        return TRUE;
    }
}

BOOLEAN
PlatformSerialClose(HANDLE Handle)
{
    if (g_PlatformOverlappedReadDebugger.hEvent)
        CloseHandle(g_PlatformOverlappedReadDebugger.hEvent);
    if (g_PlatformOverlappedReadDebuggee.hEvent)
        CloseHandle(g_PlatformOverlappedReadDebuggee.hEvent);
    if (g_PlatformOverlappedWriteDebugger.hEvent)
        CloseHandle(g_PlatformOverlappedWriteDebugger.hEvent);

    g_PlatformOverlappedReadDebugger.hEvent  = NULL;
    g_PlatformOverlappedReadDebuggee.hEvent  = NULL;
    g_PlatformOverlappedWriteDebugger.hEvent = NULL;

    if (Handle)
        return (BOOLEAN)CloseHandle(Handle);

    return TRUE;
}

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

BOOLEAN
PlatformSerialClose(HANDLE Handle)
{
    (void)Handle;
    return TRUE;
}

#else
#    error "Unsupported platform"
#endif
