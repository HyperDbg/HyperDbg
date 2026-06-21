/**
 * @file platform-ioctl.h
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode cross-platform interface for the local kernel-driver IOCTL transport
 * @details Distinct from the serial transport (platform-serial), which talks to a remote
 *          debuggee. This interface is the LOCAL control channel: the userspace library
 *          drives the HyperDbg kernel driver through device I/O control codes. The command
 *          and packet layers are platform independent; only the device-control primitive
 *          underneath them is OS specific. Windows maps onto Win32 DeviceIoControl over a
 *          \\.\HyperDbgDebuggerDevice handle; Linux will map onto ioctl() against a
 *          /dev/HyperDbg character device exposed by the (future) kernel module (TODO).
 *
 * @version 0.20
 * @date 2026-06-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//
// SEND an I/O control code to the local kernel driver. Mirrors the Win32
// DeviceIoControl signature so the call sites stay verbatim apart from the name.
// The Overlapped parameter is kept as a void * so the shared signature does not
// leak OVERLAPPED into the protocol code; synchronous callers pass NULL.
// Returns TRUE on success; on failure use PlatformGetLastError for the reason.
//
BOOL
PlatformDeviceIoControl(HANDLE  Device,
                        DWORD   IoControlCode,
                        LPVOID  InBuffer,
                        DWORD   InBufferSize,
                        LPVOID  OutBuffer,
                        DWORD   OutBufferSize,
                        LPDWORD BytesReturned,
                        LPVOID  Overlapped);
