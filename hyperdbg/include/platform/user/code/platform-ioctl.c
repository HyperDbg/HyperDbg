/**
 * @file platform-ioctl.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode cross-platform implementation of the local kernel-driver IOCTL transport
 * @details See platform-ioctl.h. The Windows branch forwards directly to Win32
 *          DeviceIoControl / CreateFileA. The Linux branch is currently stubbed and is
 *          the home where the ioctl()-based implementation against the /dev/HyperDbg
 *          character device will live once the kernel module exists.
 *
 * @version 0.20
 * @date 2026-06-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-ioctl.h"
#endif // defined(__linux__)

//
// SEND an I/O control code to the local kernel driver.
//
// TODO(Linux): implement the local driver transport using ioctl() against a
// /dev/HyperDbg character device exposed by the kernel module:
//   - open the device once (in the library init path) -> file descriptor stored
//     in g_DeviceHandle (see PlatformOpenDevice below)
//   - ioctl(fd, IoControlCode, ...) with the in/out buffer marshalling the driver
//     expects (likely a single in-out buffer)
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
#if defined(_WIN32)
    return DeviceIoControl(Device,
                           IoControlCode,
                           InBuffer,
                           InBufferSize,
                           OutBuffer,
                           OutBufferSize,
                           BytesReturned,
                           (LPOVERLAPPED)Overlapped);
#elif defined(__linux__)
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
#else
#    error "Unsupported platform"
#endif
}

//
// OPEN the local kernel-driver device and return a handle to it.
//
// TODO(Linux): open the device with open("/dev/HyperDbg", O_RDWR), returning the
// resulting file descriptor as the g_DeviceHandle. The /dev/HyperDbg character device
// is exposed by the (future) kernel module; until it exists there is no device to open,
// so this returns INVALID_HANDLE_VALUE. The caller checks against INVALID_HANDLE_VALUE
// exactly as it does for the Win32 CreateFile it replaces, so the library init path
// fails cleanly (no driver) instead of crashing.
//
HANDLE
PlatformOpenDevice(LPCSTR DeviceName)
{
#if defined(_WIN32)
    return CreateFileA(
        DeviceName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, /// lpSecurityAttirbutes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL); /// lpTemplateFile
#elif defined(__linux__)
    (void)DeviceName;
    return INVALID_HANDLE_VALUE;
#else
#    error "Unsupported platform"
#endif
}
