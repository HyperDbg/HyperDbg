/**
 * @file platform-socket.h
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode cross-platform interface for the TCP remote-debugging transport
 * @details The remote-debugging command/result exchange in tcpclient.cpp /
 *          tcpserver.cpp is written against the BSD-socket API, which Winsock and
 *          POSIX share almost verbatim (socket / connect / bind / listen / accept /
 *          send / recv / shutdown / getaddrinfo). Only a handful of things diverge:
 *          Winsock's startup/cleanup lifecycle, closesocket vs close(2), the
 *          SD_SEND vs SHUT_WR shutdown flag, WSAGetLastError vs errno, and the
 *          address-length out-parameter type of accept() (int vs socklen_t). Those
 *          are isolated behind the Platform* wrappers / typedef below so the socket
 *          call sites stay shared. On Linux this header also pulls in the POSIX
 *          socket headers that back the portable calls; on Windows they come from
 *          <winsock2.h>/<ws2tcpip.h> (included by pch.h).
 *
 * @version 0.21
 * @date 2026-07-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#    include <sys/socket.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <unistd.h>
#endif // defined(__linux__)

//
// Length type for the address-size out-parameter of accept() (and friends).
// Winsock uses int; POSIX uses socklen_t. Kept as a platform typedef so the
// call sites declare the right type without an inline #ifdef.
//
#if defined(_WIN32)
typedef int PLATFORM_SOCKLEN;
#elif defined(__linux__)
typedef socklen_t PLATFORM_SOCKLEN;
#endif

//
// INITIALIZE the socket subsystem before any socket call. Mirrors WSAStartup:
// returns 0 on success and non-zero on failure. The Winsock version request is
// kept internal. No-op on Linux (always returns 0).
//
INT
PlatformSocketInitialize(VOID);

//
// CLEAN UP the socket subsystem (Winsock WSACleanup; no-op on Linux).
//
VOID
PlatformSocketCleanup(VOID);

//
// CLOSE a socket (Winsock closesocket; POSIX close(2)).
//
INT
PlatformCloseSocket(SOCKET Socket);

//
// SHUT DOWN the sending side of a socket (Winsock shutdown(.., SD_SEND);
// POSIX shutdown(.., SHUT_WR)). Returns 0 on success, SOCKET_ERROR on failure.
//
INT
PlatformShutdownSocketSend(SOCKET Socket);

//
// LAST socket error for the calling thread (Winsock WSAGetLastError; POSIX
// errno). See the last-error caveat in platform-lib-calls.h: the numeric code
// spaces still differ; callers that only log or check non-zero are fine.
//
INT
PlatformGetSocketError(VOID);
