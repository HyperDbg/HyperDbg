/**
 * @file platform-ioctl.c
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode cross-platform implementation of the local kernel-driver IOCTL transport
 * @details See platform-ioctl.h. The Windows branch forwards directly to Win32
 *          DeviceIoControl. The Linux branch is currently stubbed (returns FALSE) and is
 *          the home where the ioctl()-based implementation against the /dev/HyperDbg
 *          character device will live once the kernel module exists.
 *
 * @version 0.1
 * @date 2026-06-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-ioctl.h"
#endif // defined(__linux__)

#if defined(_WIN32)

BOOL
PlatformDeviceIoControl(HANDLE  Device,
                        DWORD   IoControlCode,
                        LPVOID  InBuffer,
                        DWORD   InBufferSize,
                        LPVOID  OutBuffer,
                        DWORD   OutBufferSize,
                        LPDWORD BytesReturned,
                        LPVOID  Overlapped)
{
    return DeviceIoControl(Device,
                           IoControlCode,
                           InBuffer,
                           InBufferSize,
                           OutBuffer,
                           OutBufferSize,
                           BytesReturned,
                           (LPOVERLAPPED)Overlapped);
}

#elif defined(__linux__)

//
// TODO: implement the local driver transport on Linux using ioctl() against a
// /dev/HyperDbg character device exposed by the kernel module:
//   - open the device once (in the library init path) -> file descriptor stored
//     in g_DeviceHandle
//   - PlatformDeviceIoControl -> ioctl(fd, IoControlCode, ...) with the in/out
//     buffer marshalling the driver expects (likely a single in-out buffer)
//   - close on teardown
// The kernel module does not exist yet, so this returns failure for now: callers
// that have already asserted g_DeviceHandle will simply report the IOCTL failed
// rather than crashing.
//

BOOL
PlatformDeviceIoControl(HANDLE  Device,
                        DWORD   IoControlCode,
                        LPVOID  InBuffer,
                        DWORD   InBufferSize,
                        LPVOID  OutBuffer,
                        DWORD   OutBufferSize,
                        LPDWORD BytesReturned,
                        LPVOID  Overlapped)
{
    (void)Device;
    (void)IoControlCode;
    (void)InBuffer;
    (void)InBufferSize;
    (void)OutBuffer;
    (void)OutBufferSize;
    (void)Overlapped;
    if (BytesReturned)
        *BytesReturned = 0;
    return FALSE;
}

#else
#    error "Unsupported platform"
#endif
