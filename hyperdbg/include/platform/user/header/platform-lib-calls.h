/**
 * @file platform-lib-calls.h
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode Cross platform APIs for platofrm dependend library calls
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

#include <stdarg.h>
#include <stddef.h>

//
// VSNPRINTF
//
INT
PlatformVsnprintf(char * Buffer, SIZE_T BufferSize, const char * Format, va_list ArgList);

//
// STRDUP
//
char *
PlatformStrDup(const char * Str);

//
// SET MEMORY TO ZERO
//
VOID
PlatformZeroMemory(PVOID Buffer, SIZE_T Size);

//
// SPRINTF
//
INT
PlatformSprintf(char * Buffer, SIZE_T BufferSize, const char * Format, ...);

//
// HIGH-RESOLUTION PERFORMANCE COUNTER
//
BOOLEAN
PlatformQueryPerformanceFrequency(LARGE_INTEGER * Frequency);

BOOLEAN
PlatformQueryPerformanceCounter(LARGE_INTEGER * Count);

//
// EVENT / SYNCHRONIZATION OBJECTS
//
HANDLE
PlatformCreateEvent(BOOLEAN ManualReset, BOOLEAN InitialState);

BOOLEAN
PlatformSetEvent(HANDLE EventHandle);

BOOLEAN
PlatformResetEvent(HANDLE EventHandle);

DWORD
PlatformWaitForSingleObject(HANDLE Handle, DWORD TimeoutMilliseconds);

BOOLEAN
PlatformCloseHandle(HANDLE Handle);

//
// LAST OS ERROR
//
DWORD
PlatformGetLastError(VOID);

//
// PROCESS / THREAD IDENTITY
//
UINT32
PlatformGetCurrentThreadId(VOID);

UINT32
PlatformGetCurrentProcessorNumber(VOID);

UINT32
PlatformGetCurrentProcessId(VOID);

CHAR *
PlatformGetCurrentProcessName(VOID);
