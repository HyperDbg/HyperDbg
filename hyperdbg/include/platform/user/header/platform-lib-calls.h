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
// TODO (linux, correctness): the wrappers below only unify the success/failure
// *boolean* convention. The actual error semantics still differ across platforms
// and must be reconciled later:
//   - PlatformGetLastError returns raw Linux errno (EACCES=13, ...) which does NOT
//     match the Win32 ERROR_* code space (ERROR_ACCESS_DENIED=5, ...). Code that
//     merely logs/checks-nonzero is fine; code that compares against ERROR_* needs
//     an errno -> ERROR_* mapping here.
//   - Failure sentinels are not uniform across the Win32 calls we wrap: file opens
//     fail with INVALID_HANDLE_VALUE (not NULL), most other handle calls fail with
//     NULL. Callers must test the right one.
// For now: getting the project to compile is step 1; correctness comes next.
//
DWORD
PlatformGetLastError(VOID);

//
// CONSOLE OUTPUT (raw bytes to stdout)
//
BOOLEAN
PlatformWriteConsole(const VOID * Buffer, DWORD NumberOfBytes);

//
// FILE I/O
//
HANDLE
PlatformOpenFileForWriting(const wchar_t * Path);

BOOLEAN
PlatformWriteFile(HANDLE FileHandle, const VOID * Buffer, DWORD NumberOfBytes);

BOOLEAN
PlatformCloseFile(HANDLE FileHandle);

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
