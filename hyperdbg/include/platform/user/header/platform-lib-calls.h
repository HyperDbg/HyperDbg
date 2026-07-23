/**
 * @file platform-lib-calls.h
 * @author Max Raulea (max.raulea@hyperdbg.org)
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
// COPY MEMORY
//
VOID
PlatformCopyMemory(PVOID Destination, const VOID * Source, SIZE_T Size);

//
// SPRINTF
//
INT
PlatformSprintf(char * Buffer, SIZE_T BufferSize, const char * Format, ...);

//
// SNPRINTF
//
INT
PlatformSnprintf(char * Buffer, SIZE_T BufferSize, const char * Format, ...);

//
// BOUNDED STRING LENGTH
//
SIZE_T
PlatformStrnlen(const char * Str, SIZE_T MaxLength);

//
// BOUNDED STRING COPY
//
// Mirrors strcpy_s: copies Src into Dest (of DestSize bytes, including the null
// terminator). Returns 0 on success, non-zero if the arguments are invalid or
// Src does not fit (in which case Dest is left as an empty string).
//
INT
PlatformStrCpy(char * Dest, SIZE_T DestSize, const char * Src);

//
// CASE-INSENSITIVE STRING COMPARE
//
// Mirrors _stricmp: returns 0 when the strings are equal ignoring case, and a
// negative/positive value otherwise. Linux uses strcasecmp (same semantics).
//
INT
PlatformStrCaseCmp(const char * Str1, const char * Str2);

//
// SLEEP (milliseconds)
//
VOID
PlatformSleep(DWORD Milliseconds);

//
// DEBUG BREAK (raise a breakpoint trap in the calling process)
//
VOID
    PlatformDebugBreak(VOID);

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
// THREADS
//
// Mirrors the Win32 thread routine signature so the thread functions
// themselves stay unchanged; the default attributes/stack size and the
// thread-id out-param of CreateThread are not used by any caller.
//
typedef DWORD(WINAPI * PLATFORM_THREAD_ROUTINE)(PVOID Param);

HANDLE
PlatformCreateThread(PLATFORM_THREAD_ROUTINE Routine, PVOID Param);

BOOLEAN
PlatformTerminateThread(HANDLE Thread, DWORD ExitCode);

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
PlatformOpenFileForWriting(const WCHAR * Path);

//
// Narrow (char*) variant with OPEN_ALWAYS semantics (open existing, else
// create; no truncate), as opposed to the wide PlatformOpenFileForWriting above
// which truncates (CREATE_ALWAYS). Used by the event-forwarding file sink, whose
// path is already a narrow std::string, so it sidesteps the wide-char issue and
// works on Linux.
//
HANDLE
PlatformOpenFileForWritingNarrow(const CHAR * Path);

BOOLEAN
PlatformWriteFile(HANDLE FileHandle, const VOID * Buffer, DWORD NumberOfBytes);

BOOLEAN
PlatformCloseFile(HANDLE FileHandle);

//
// READ-ONLY FILE MAPPING
//
// PlatformMapFileReadOnly also hands back the still-open file handle in
// *OutFileHandle, so callers can issue supplementary raw reads via
// PlatformReadFileAtOffset; release everything with PlatformUnmapFile.
//
VOID *
PlatformMapFileReadOnly(const WCHAR * Path, PSIZE_T OutFileSize, PHANDLE OutFileHandle);

BOOLEAN
PlatformReadFileAtOffset(HANDLE FileHandle, UINT64 Offset, VOID * Buffer, DWORD NumberOfBytes, LPDWORD BytesRead);

VOID
PlatformUnmapFile(VOID * BaseAddress, SIZE_T FileSize, HANDLE FileHandle);

//
// PROCESS / THREAD IDENTITY
//
UINT32
PlatformGetCurrentThreadId(VOID);

UINT32
PlatformGetCurrentProcessorNumber(VOID);

//
// Number of logical processors currently online. Windows uses the classic
// GetSystemInfo count; Linux uses sysconf(_SC_NPROCESSORS_ONLN). Returns 0 if
// the count cannot be determined.
//
// NOTE: rdmsr.cpp keeps its own NUMA-aware GetLogicalProcessorInformationEx
// chain on Windows and only falls back to this wrapper on Linux, so the
// Windows core count there is unchanged.
//
SIZE_T
PlatformGetActiveProcessorCount(VOID);

UINT32
PlatformGetCurrentProcessId(VOID);

CHAR *
    PlatformGetCurrentProcessName(VOID);

//
// PROCESS / THREAD LIFECYCLE (spawn / open / terminate / query)
//
// Thin wrappers over the Win32 process-management calls used by the
// user-debugger. Each is real on Windows and stubbed on Linux (there is no
// Linux process/ptrace backend yet). STARTUPINFO stays inside
// PlatformCreateProcess so it never leaks to callers.
//
BOOLEAN
PlatformCreateProcess(const WCHAR * FileName, const WCHAR * CommandLine, DWORD CreationFlags, PPROCESS_INFORMATION ProcessInformation);

HANDLE
PlatformOpenProcess(DWORD DesiredAccess, BOOL InheritHandle, DWORD ProcessId);

BOOL
PlatformTerminateProcess(HANDLE Process, UINT ExitCode);

DWORD
PlatformResumeThread(HANDLE Thread);

BOOL
PlatformGetExitCodeProcess(HANDLE Process, LPDWORD ExitCode);

//
// DYNAMIC LIBRARY LOADING
//
// Thin wrappers over LoadLibrary/GetProcAddress/FreeLibrary, used by the
// event-forwarding "module" sink (loads a plugin exporting
// hyperdbg_event_forwarding). Windows = the Win32 calls; Linux = dlopen/dlsym/
// dlclose (exact 1:1 semantic map).
//
HMODULE
PlatformLoadLibrary(const CHAR * ModulePath);

PVOID
PlatformGetProcAddress(HMODULE Module, const CHAR * ProcName);

BOOL
PlatformFreeLibrary(HMODULE Module);
