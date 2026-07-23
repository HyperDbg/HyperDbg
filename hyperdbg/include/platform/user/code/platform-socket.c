/**
 * @file platform-socket.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode cross-platform implementation of the TCP remote-debugging transport
 * @details See platform-socket.h. The few Winsock/POSIX primitives that diverge
 *          (close, shutdown flag, last-error) are mapped onto a common spelling
 *          by the macros below so each wrapper body is written once; only the
 *          startup/cleanup lifecycle, whose structure genuinely differs, keeps a
 *          small in-body guard. The portable socket calls themselves stay at the
 *          tcpclient/tcpserver call sites unchanged.
 *
 * @version 0.21
 * @date 2026-07-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-socket.h"
#    include <errno.h>
#endif // defined(__linux__)

//
// Map the Winsock/POSIX primitives that diverge onto a common spelling.
//
#if defined(_WIN32)
#    define PLATFORM_CLOSE_SOCKET(Socket) closesocket(Socket)
#    define PLATFORM_SHUTDOWN_SEND_FLAG   SD_SEND
#    define PLATFORM_LAST_SOCKET_ERROR    WSAGetLastError()
#elif defined(__linux__)
#    define PLATFORM_CLOSE_SOCKET(Socket) close(Socket)
#    define PLATFORM_SHUTDOWN_SEND_FLAG   SHUT_WR
#    define PLATFORM_LAST_SOCKET_ERROR    errno
#endif

INT
PlatformSocketInitialize(VOID)
{
#if defined(_WIN32)
    WSADATA WsaData;

    //
    // Request Winsock 2.2; the WSADATA is not needed by the caller.
    //
    return WSAStartup(MAKEWORD(2, 2), &WsaData);
#else
    //
    // No global socket-library initialization is needed on Linux.
    //
    return 0;
#endif
}

VOID
PlatformSocketCleanup(VOID)
{
#if defined(_WIN32)
    WSACleanup();
#endif
    //
    // Nothing to tear down on Linux.
    //
}

INT
PlatformCloseSocket(SOCKET Socket)
{
    return PLATFORM_CLOSE_SOCKET(Socket);
}

INT
PlatformShutdownSocketSend(SOCKET Socket)
{
    return shutdown(Socket, PLATFORM_SHUTDOWN_SEND_FLAG);
}

INT
PlatformGetSocketError(VOID)
{
    return PLATFORM_LAST_SOCKET_ERROR;
}
