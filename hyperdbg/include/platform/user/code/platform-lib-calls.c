/**
 * @file platform-lib-calls.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode Cross platform APIs for platofrm dependend library calls
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-lib-calls.h"
#    include <unistd.h>
#    include <sched.h>
#    include <sys/syscall.h>
#    include <errno.h>
#    include <stdint.h>
#    include <string.h>
#    include <strings.h>
#    include <signal.h>
#    include <dlfcn.h>
#    include <time.h> // clock_gettime / CLOCK_MONOTONIC (PlatformQueryPerformanceCounter)
#endif // defined(__linux__)

/**
 * @brief Platform independent wrapper for vsprintf_s / vsnprintf
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @param ArgList variadic argument list
 * @return INT number of characters written, or -1 on error
 */
INT
PlatformVsnprintf(char * Buffer, SIZE_T BufferSize, const char * Format, va_list ArgList)
{
#if defined(_WIN32)
    return vsprintf_s(Buffer, BufferSize, Format, ArgList);
#elif defined(__linux__)
    return vsnprintf(Buffer, BufferSize, Format, ArgList);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for _strdup / strdup
 *
 * @param Str string to duplicate
 * @return char * pointer to the duplicated string, or NULL on failure
 */
char *
PlatformStrDup(const char * Str)
{
#if defined(_WIN32)
    return _strdup(Str);
#elif defined(__linux__)
    return strdup(Str);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for RtlZeroMemory / memset
 *
 * @param Buffer pointer to the memory region to zero
 * @param Size number of bytes to zero
 */
VOID
PlatformZeroMemory(PVOID Buffer, SIZE_T Size)
{
#if defined(_WIN32)
    RtlZeroMemory(Buffer, Size);
#elif defined(__linux__)
    memset(Buffer, 0, Size);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for RtlCopyMemory / memcpy
 *
 * @param Destination pointer to the destination buffer
 * @param Source pointer to the source buffer
 * @param Size number of bytes to copy
 */
VOID
PlatformCopyMemory(PVOID Destination, const VOID * Source, SIZE_T Size)
{
#if defined(_WIN32)
    RtlCopyMemory(Destination, Source, Size);
#elif defined(__linux__)
    memcpy(Destination, Source, Size);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for QueryPerformanceFrequency
 *
 * @param Frequency output — ticks per second
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformQueryPerformanceFrequency(LARGE_INTEGER * Frequency)
{
#if defined(_WIN32)
    return (BOOLEAN)QueryPerformanceFrequency((LARGE_INTEGER *)Frequency);
#elif defined(__linux__)
    Frequency->QuadPart = 1000000000LL; // clock_gettime gives nanosecond resolution
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for QueryPerformanceCounter
 *
 * @param Count output — current tick count
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformQueryPerformanceCounter(LARGE_INTEGER * Count)
{
#if defined(_WIN32)
    return (BOOLEAN)QueryPerformanceCounter((LARGE_INTEGER *)Count);
#elif defined(__linux__)
    struct timespec Ts;
    clock_gettime(CLOCK_MONOTONIC, &Ts);
    Count->QuadPart = (INT64)Ts.tv_sec * 1000000000LL + Ts.tv_nsec;
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for sprintf_s / snprintf
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @return INT number of characters written, or -1 on error
 */
INT
PlatformSprintf(char * Buffer, SIZE_T BufferSize, const char * Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    INT Result;
#if defined(_WIN32)
    Result = vsprintf_s(Buffer, BufferSize, Format, Args);
#elif defined(__linux__)
    Result = vsnprintf(Buffer, BufferSize, Format, Args);
#else
#    error "Unsupported platform"
#endif
    va_end(Args);
    return Result;
}

/**
 * @brief Platform independent wrapper for _snprintf_s(..., _TRUNCATE, ...)
 *
 * Writes a formatted string into Buffer. If the output is larger than the
 * buffer, it is truncated and always null-terminated.
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @return INT number of characters written, or -1 if truncation or an error occurred.
 */
INT
PlatformSnprintf(char * Buffer, SIZE_T BufferSize, const char * Format, ...)
{
    va_list Args;
    INT     Result;

    va_start(Args, Format);

#if defined(_WIN32)
    Result = _vsnprintf_s(Buffer, BufferSize, _TRUNCATE, Format, Args);
#elif defined(__linux__)
    Result = vsnprintf(Buffer, BufferSize, Format, Args);

    /* Match Windows _TRUNCATE behavior: return -1 on truncation. */
    if (Result >= 0 && (SIZE_T)Result >= BufferSize)
    {
        Result = -1;
    }
#else
#    error "Unsupported platform"
#endif

    va_end(Args);
    return Result;
}

/**
 * @brief Platform independent wrapper for strnlen_s / strnlen
 *
 * @param Str string to measure (must not be NULL)
 * @param MaxLength maximum number of characters to examine
 * @return SIZE_T length of the string, capped at MaxLength
 */
SIZE_T
PlatformStrnlen(const char * Str, SIZE_T MaxLength)
{
#if defined(_WIN32)
    return strnlen_s(Str, MaxLength);
#elif defined(__linux__)
    return strnlen(Str, MaxLength);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for strcpy_s
 *
 * @details Copies Src into the DestSize-byte Dest buffer, including the null
 * terminator. On Linux there is no standard strcpy_s, so this performs the same
 * bounds check: if Src (plus terminator) does not fit, Dest is set to an empty
 * string and a non-zero error is returned, matching strcpy_s's behavior.
 *
 * @param Dest destination buffer
 * @param DestSize size of the destination buffer in bytes
 * @param Src source string
 * @return INT 0 on success, non-zero on failure
 */
INT
PlatformStrCpy(char * Dest, SIZE_T DestSize, const char * Src)
{
#if defined(_WIN32)
    return strcpy_s(Dest, DestSize, Src);
#elif defined(__linux__)
    // NOT YET TESTED!! So needs some testing to see if it actually behaves the same as strcpy_S on windows
    SIZE_T Length;

    if (Dest == NULL || DestSize == 0 || Src == NULL)
    {
        if (Dest != NULL && DestSize != 0)
        {
            Dest[0] = '\0';
        }
        return -1;
    }

    Length = strlen(Src);

    if (Length >= DestSize)
    {
        //
        // Source does not fit (need room for the null terminator too)
        //
        Dest[0] = '\0';
        return -1;
    }

    memcpy(Dest, Src, Length + 1);
    return 0;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for strncpy_s
 *
 * @details Copies the first D characters of Src into the DestSize-byte Dest
 * buffer and appends a null terminator, where D is the lesser of Count and the
 * length of Src. If those characters do not fit while still leaving room for the
 * terminator, Dest is set to an empty string and a non-zero error is returned.
 * Passing _TRUNCATE as Count instead copies as much of Src as fits, returning
 * STRUNCATE when anything had to be dropped. On Linux there is no standard
 * strncpy_s, so this reproduces those same rules.
 *
 * @param Dest destination buffer
 * @param DestSize size of the destination buffer in bytes
 * @param Src source string
 * @param Count maximum characters to copy, or _TRUNCATE
 * @return INT 0 on success, STRUNCATE if truncated, non-zero on failure
 */
INT
PlatformStrNCpy(char * Dest, SIZE_T DestSize, const char * Src, SIZE_T Count)
{
#if defined(_WIN32)
    return strncpy_s(Dest, DestSize, Src, Count);
#elif defined(__linux__)
    // NOT YET TESTED!! So needs some testing to see if it actually behaves the same as strncpy_s on windows
    SIZE_T Length;

    if (Dest == NULL || DestSize == 0 || Src == NULL)
    {
        if (Dest != NULL && DestSize != 0)
        {
            Dest[0] = '\0';
        }
        return -1;
    }

    //
    // Never read past Count characters of Src; it need not be null-terminated
    // within that span
    //
    Length = PlatformStrnlen(Src, Count == _TRUNCATE ? DestSize : Count);

    if (Count == _TRUNCATE)
    {
        //
        // Copy as much as fits and report whether anything was dropped
        //
        if (Length >= DestSize)
        {
            memcpy(Dest, Src, DestSize - 1);
            Dest[DestSize - 1] = '\0';
            return STRUNCATE;
        }
    }
    else if (Length >= DestSize)
    {
        //
        // Source does not fit (need room for the null terminator too)
        //
        Dest[0] = '\0';
        return -1;
    }

    memcpy(Dest, Src, Length);
    Dest[Length] = '\0';
    return 0;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for _stricmp
 *
 * @details Compares two strings ignoring case. Returns 0 when equal; a value
 * less/greater than zero otherwise. Linux uses strcasecmp, which has the same
 * semantics as the Win32 CRT's _stricmp.
 *
 * @param Str1 first string
 * @param Str2 second string
 * @return INT 0 if equal (case-insensitively), non-zero otherwise
 */
INT
PlatformStrCaseCmp(const char * Str1, const char * Str2)
{
#if defined(_WIN32)
    return _stricmp(Str1, Str2);
#elif defined(__linux__)
    return strcasecmp(Str1, Str2);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for Sleep / usleep
 *
 * @param Milliseconds number of milliseconds to suspend the calling thread
 */
VOID
PlatformSleep(DWORD Milliseconds)
{
#if defined(_WIN32)
    Sleep(Milliseconds);
#elif defined(__linux__)
    usleep((useconds_t)Milliseconds * 1000);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for DebugBreak / raise(SIGTRAP)
 *
 * @details Delivers a breakpoint trap to the calling process.
 */
VOID
PlatformDebugBreak(VOID)
{
#if defined(_WIN32)
    DebugBreak();
#elif defined(__linux__)
    raise(SIGTRAP);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetCurrentThreadId / gettid
 *
 * @return UINT32 thread ID of the calling thread
 */
UINT32
PlatformGetCurrentThreadId(VOID)
{
#if defined(_WIN32)
    return (UINT32)GetCurrentThreadId();
#elif defined(__linux__)
    return (UINT32)syscall(SYS_gettid);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetCurrentProcessorNumber / sched_getcpu
 *
 * @return UINT32 logical processor index the calling thread is running on
 */
UINT32
PlatformGetCurrentProcessorNumber(VOID)
{
#if defined(_WIN32)
    return (UINT32)GetCurrentProcessorNumber();
#elif defined(__linux__)
    return (UINT32)sched_getcpu();
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent count of online logical processors
 *
 * @return SIZE_T number of logical processors online, or 0 if unknown
 */
SIZE_T
PlatformGetActiveProcessorCount(VOID)
{
#if defined(_WIN32)
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    return (SIZE_T)SysInfo.dwNumberOfProcessors;
#elif defined(__linux__)
    // Not yet tested!!
    long Count = sysconf(_SC_NPROCESSORS_ONLN);
    return Count > 0 ? (SIZE_T)Count : 0;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetCurrentProcessId / getpid
 *
 * @return UINT32 PID of the calling process
 */
UINT32
PlatformGetCurrentProcessId(VOID)
{
#if defined(_WIN32)
    return (UINT32)GetCurrentProcessId();
#elif defined(__linux__)
    return (UINT32)getpid();
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to get the current process name
 *
 * @return CHAR* pointer to a static buffer holding the process name, or NULL on failure
 */
CHAR *
PlatformGetCurrentProcessName(VOID)
{
    static CHAR ProcessNameBuf[MAX_PATH] = {0};

#if defined(_WIN32)
    //
    // Use base kernel32 only (no psapi/shlwapi) so this compiles in every
    // project that builds platform-lib-calls.c (e.g. script-engine, which has
    // a minimal include set). GetModuleFileNameA(NULL, ...) returns the full
    // path of the current process image.
    //
    if (GetModuleFileNameA(NULL, ProcessNameBuf, MAX_PATH) == 0)
    {
        return NULL;
    }

    //
    // Return the basename (strip the directory part)
    //
    char * LastSeparator = strrchr(ProcessNameBuf, '\\');
    if (LastSeparator)
    {
        return LastSeparator + 1;
    }

    return ProcessNameBuf;

#elif defined(__linux__)
    FILE * f = fopen("/proc/self/comm", "r");
    if (f)
    {
        if (fgets(ProcessNameBuf, sizeof(ProcessNameBuf), f))
        {
            size_t Len = strlen(ProcessNameBuf);
            if (Len > 0 && ProcessNameBuf[Len - 1] == '\n')
                ProcessNameBuf[Len - 1] = '\0';
        }
        fclose(f);
        return ProcessNameBuf;
    }
    return NULL;

#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for CreateEvent
 *
 * @param ManualReset  TRUE for a manual-reset event, FALSE for auto-reset
 * @param InitialState TRUE if the event starts signaled
 * @return HANDLE to the event, or NULL on failure
 */
HANDLE
PlatformCreateEvent(BOOLEAN ManualReset, BOOLEAN InitialState)
{
#if defined(_WIN32)
    return CreateEvent(NULL, ManualReset, InitialState, NULL);
#elif defined(__linux__)
    //
    // TODO: back this with a pthread mutex+cond (or eventfd) when the Linux
    //       kernel-debugger transport is implemented. For now return a dummy
    //       non-NULL handle so existing NULL-checks treat creation as success.
    //
    (void)ManualReset;
    (void)InitialState;
    return (HANDLE)(uintptr_t)1;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for SetEvent
 */
BOOLEAN
PlatformSetEvent(HANDLE EventHandle)
{
#if defined(_WIN32)
    return (BOOLEAN)SetEvent(EventHandle);
#elif defined(__linux__)
    (void)EventHandle; // TODO: signal the underlying cond/eventfd
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for ResetEvent
 */
BOOLEAN
PlatformResetEvent(HANDLE EventHandle)
{
#if defined(_WIN32)
    return (BOOLEAN)ResetEvent(EventHandle);
#elif defined(__linux__)
    (void)EventHandle; // TODO: clear the underlying cond/eventfd
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for WaitForSingleObject
 *
 * @return 0 (WAIT_OBJECT_0) on success
 */
DWORD
PlatformWaitForSingleObject(HANDLE Handle, DWORD TimeoutMilliseconds)
{
#if defined(_WIN32)
    return WaitForSingleObject(Handle, TimeoutMilliseconds);
#elif defined(__linux__)
    //
    // TODO: wait on the underlying cond/eventfd. For now return immediately as
    //       success — no real transport exists yet to wait on.
    //
    (void)Handle;
    (void)TimeoutMilliseconds;
    return 0;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for CloseHandle
 */
BOOLEAN
PlatformCloseHandle(HANDLE Handle)
{
#if defined(_WIN32)
    return (BOOLEAN)CloseHandle(Handle);
#elif defined(__linux__)
    (void)Handle; // TODO: free the underlying cond/eventfd or close the fd
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for CreateThread
 *
 * @param Routine thread entry point
 * @param Param   parameter passed to the thread routine
 * @return HANDLE to the new thread, or NULL on failure
 */
HANDLE
PlatformCreateThread(PLATFORM_THREAD_ROUTINE Routine, PVOID Param)
{
#if defined(_WIN32)
    return CreateThread(NULL, 0, Routine, Param, 0, NULL);
#elif defined(__linux__)
    //
    // TODO: back this with pthread_create when the Linux kernel-debugger
    //       transport is implemented. Returning NULL leaves the listening
    //       thread unstarted, which is all the callers check for.
    //
    (void)Routine;
    (void)Param;
    return NULL;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for TerminateThread
 *
 * @details There is no POSIX equivalent by design: no call forcibly kills a
 *          thread without unwinding, since doing so never releases the
 *          target's locks. pthread_cancel is the nearest primitive but has
 *          different semantics (deferred by default, and glibc implements it
 *          as a forced unwind that runs destructors and cleanup handlers).
 *
 *          TODO (linux): the only caller is disconnect.cpp, whose listening
 *          thread blocks in recv() inside a
 *          `while (g_IsConnectedToRemoteDebuggee)` loop and breaks on any
 *          receive error. The correct teardown there is to clear the flag,
 *          shutdown(fd, SHUT_RDWR) to kick the thread out of recv, then
 *          pthread_join it — letting it exit through its own error path, with
 *          no cancellation primitive involved. That needs a call-site reorder
 *          (teardown before thread-kill, and SD_SEND -> SHUT_RDWR to wake a
 *          blocked reader), which the port's no-logic-changes rule defers.
 *
 *          Returning TRUE is consistent with PlatformCreateThread, which
 *          returns NULL on Linux — the thread is never started there, so
 *          there is nothing to terminate yet.
 *
 * @param Thread handle to the thread to terminate
 * @param ExitCode exit code for the terminated thread
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformTerminateThread(HANDLE Thread, DWORD ExitCode)
{
#if defined(_WIN32)
    return (BOOLEAN)TerminateThread(Thread, ExitCode);
#elif defined(__linux__)
    (void)Thread;
    (void)ExitCode;
    return TRUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetLastError
 */
DWORD
PlatformGetLastError(VOID)
{
#if defined(_WIN32)
    return GetLastError();
#elif defined(__linux__)
    return (DWORD)errno;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to write raw bytes to the console
 *
 * @details Used to emit pre-encoded UTF-8 byte sequences (e.g. box-drawing
 *          characters) directly to standard output. On Windows this goes
 *          through WriteConsoleA so the console code page is bypassed; on
 *          Linux the terminal is UTF-8 native so the bytes are written as-is.
 *
 * @param Buffer pointer to the bytes to write
 * @param NumberOfBytes number of bytes to write
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformWriteConsole(const VOID * Buffer, DWORD NumberOfBytes)
{
#if defined(_WIN32)
    return (BOOLEAN)WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, NumberOfBytes, NULL, NULL);
#elif defined(__linux__)
    return (BOOLEAN)(fwrite(Buffer, 1, NumberOfBytes, stdout) == NumberOfBytes);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to create/open a file for writing
 *
 * @param Path wide path of the file to create (truncated if it exists)
 * @return HANDLE to the opened file, or INVALID_HANDLE_VALUE on failure
 */
HANDLE
PlatformOpenFileForWriting(const WCHAR * Path)
{
#if defined(_WIN32)
    return CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#elif defined(__linux__)
    //
    // TODO: handle this later. The path arrives as a std::wstring (4-byte
    //       wchar_t on Linux) and must be narrowed to a UTF-8 char* before it
    //       can be handed to fopen. Until that conversion is wired up, fail the
    //       open so callers (e.g. dump.cpp) bail out cleanly instead of writing
    //       to a bogus handle.
    //
    (void)Path;
    return INVALID_HANDLE_VALUE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to open a file for writing with
 *        OPEN_ALWAYS semantics (open existing without truncating, else create),
 *        taking a narrow (char*) path
 *
 * @details Unlike PlatformOpenFileForWriting (wide path, CREATE_ALWAYS/truncate)
 *          this keeps any existing file content. The Linux handle is a FILE* so
 *          it works with PlatformWriteFile / PlatformCloseFile.
 *
 * @param Path narrow path of the file to open or create
 * @return HANDLE to the opened file, or INVALID_HANDLE_VALUE on failure
 */
HANDLE
PlatformOpenFileForWritingNarrow(const CHAR * Path)
{
#if defined(_WIN32)
    return CreateFileA(Path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#elif defined(__linux__)
    //
    // NOT YET TESTED!!
    // "r+b" opens an existing file at offset 0 without truncating (matching
    // OPEN_ALWAYS on an existing file); if it does not exist, create it with
    // "w+b". Return the FILE* as the HANDLE (PlatformWriteFile/PlatformCloseFile
    // treat the Linux handle as a FILE*).
    //
    FILE * File = fopen(Path, "r+b");
    if (File == NULL)
    {
        File = fopen(Path, "w+b");
    }
    if (File == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)File;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to write a buffer to an open file
 *
 * @param FileHandle handle returned by PlatformOpenFileForWriting
 * @param Buffer pointer to the bytes to write
 * @param NumberOfBytes number of bytes to write
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformWriteFile(HANDLE FileHandle, const VOID * Buffer, DWORD NumberOfBytes)
{
#if defined(_WIN32)
    DWORD BytesWritten;
    return (BOOLEAN)WriteFile(FileHandle, Buffer, NumberOfBytes, &BytesWritten, NULL);
#elif defined(__linux__)
    return (BOOLEAN)(fwrite(Buffer, 1, NumberOfBytes, (FILE *)FileHandle) == NumberOfBytes);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to close a file opened by
 *        PlatformOpenFileForWriting
 *
 * @param FileHandle handle to close
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformCloseFile(HANDLE FileHandle)
{
#if defined(_WIN32)
    return (BOOLEAN)CloseHandle(FileHandle);
#elif defined(__linux__)
    return (BOOLEAN)(fclose((FILE *)FileHandle) == 0);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to map an entire file read-only into memory
 *
 * @details The returned pointer stays valid until released with PlatformUnmapFile;
 *          the underlying file/descriptor is closed before returning (the mapping
 *          outlives it on both platforms).
 *
 * @param Path wide path of the file to map
 * @param OutFileSize output — size of the file in bytes (0 on failure)
 * @return VOID* base address of the mapped file, or NULL on failure
 */
VOID *
PlatformMapFileReadOnly(const WCHAR * Path, PSIZE_T OutFileSize, PHANDLE OutFileHandle)
{
#if defined(_WIN32)
    HANDLE        FileHandle;
    HANDLE        MapObjectHandle;
    VOID *        BaseAddr;
    LARGE_INTEGER FileSize;

    *OutFileSize   = 0;
    *OutFileHandle = INVALID_HANDLE_VALUE;

    FileHandle = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    if (!GetFileSizeEx(FileHandle, &FileSize))
    {
        CloseHandle(FileHandle);
        return NULL;
    }

    MapObjectHandle = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (MapObjectHandle == NULL)
    {
        CloseHandle(FileHandle);
        return NULL;
    }

    BaseAddr = MapViewOfFile(MapObjectHandle, FILE_MAP_READ, 0, 0, 0);

    //
    // The view stays valid after the mapping object handle is closed. The file
    // handle is kept open and handed back so the caller can still issue raw
    // reads (PlatformReadFileAtOffset); it is closed by PlatformUnmapFile.
    //
    CloseHandle(MapObjectHandle);

    if (BaseAddr == NULL)
    {
        CloseHandle(FileHandle);
        return NULL;
    }

    *OutFileSize   = (SIZE_T)FileSize.QuadPart;
    *OutFileHandle = FileHandle;
    return BaseAddr;
#elif defined(__linux__)
    //
    // TODO (linux): implement the real mapping. Expected contract:
    //   1. Narrow the 4-byte wchar_t 'Path' to a UTF-8 char* (the project still
    //      lacks a wchar_t->UTF-8 helper; the same one is needed by
    //      PlatformOpenFileForWriting for the dump.cpp write path).
    //   2. fd = open(narrowed_path, O_RDONLY);                  // fail -> NULL
    //   3. fstat(fd, &st) to get the file size.
    //   4. base = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //   5. *OutFileHandle = (HANDLE)(intptr_t)fd;  // keep fd open for raw reads
    //   6. *OutFileSize = st.st_size; return base;  (return NULL on any failure)
    // PlatformUnmapFile must then munmap(base, size) and close the fd — which is
    // why both the size and the handle are passed back in on unmap. The raw-read
    // path (PlatformReadFileAtOffset) would pread() from that same fd.
    //
    // Until implemented, fail the map so PE-parser callers print "could not open
    // the file" and bail out cleanly instead of dereferencing a bogus pointer.
    //
    (void)Path;
    *OutFileSize   = 0;
    *OutFileHandle = INVALID_HANDLE_VALUE;
    return NULL;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for a positioned (seek + read) file read
 *
 * @param FileHandle handle handed back by PlatformMapFileReadOnly
 * @param Offset byte offset to read from (absolute, from start of file)
 * @param Buffer destination buffer
 * @param NumberOfBytes number of bytes to read
 * @param BytesRead output — number of bytes actually read
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformReadFileAtOffset(HANDLE FileHandle, UINT64 Offset, VOID * Buffer, DWORD NumberOfBytes, LPDWORD BytesRead)
{
#if defined(_WIN32)
    LARGE_INTEGER Distance;
    Distance.QuadPart = (LONGLONG)Offset;

    if (!SetFilePointerEx(FileHandle, Distance, NULL, FILE_BEGIN))
    {
        return FALSE;
    }

    return (BOOLEAN)ReadFile(FileHandle, Buffer, NumberOfBytes, BytesRead, NULL);
#elif defined(__linux__)
    //
    // TODO (linux): pread((int)(intptr_t)FileHandle, Buffer, NumberOfBytes, Offset)
    //               once PlatformMapFileReadOnly wraps a real fd. Unreached today
    //               because the map returns NULL on Linux, so callers bail first.
    //
    (void)FileHandle;
    (void)Offset;
    (void)Buffer;
    (void)NumberOfBytes;
    if (BytesRead != NULL)
    {
        *BytesRead = 0;
    }
    return FALSE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper to release a mapping from PlatformMapFileReadOnly
 *
 * @param BaseAddress base address returned by PlatformMapFileReadOnly
 * @param FileSize size that was reported by PlatformMapFileReadOnly (needed by munmap)
 * @param FileHandle file handle handed back by PlatformMapFileReadOnly
 */
VOID
PlatformUnmapFile(VOID * BaseAddress, SIZE_T FileSize, HANDLE FileHandle)
{
#if defined(_WIN32)
    (void)FileSize; // not needed by UnmapViewOfFile
    if (BaseAddress != NULL)
    {
        UnmapViewOfFile(BaseAddress);
    }
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FileHandle);
    }
#elif defined(__linux__)
    //
    // TODO (linux): munmap(BaseAddress, FileSize) and close the fd behind
    //               FileHandle once PlatformMapFileReadOnly is implemented.
    //               No-op for now since the map always returns NULL.
    //
    (void)BaseAddress;
    (void)FileSize;
    (void)FileHandle;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for CreateProcessW
 *
 * @details Creates a process with the given creation flags. STARTUPINFO is kept
 * internal so it never leaks to callers. The command line is passed writable to
 * match CreateProcessW's contract.
 *
 * @param FileName application name
 * @param CommandLine command line
 * @param CreationFlags process creation flags
 * @param ProcessInformation out-param receiving process/thread handles and ids
 * @return BOOLEAN TRUE on success, FALSE on failure
 */
BOOLEAN
PlatformCreateProcess(const WCHAR * FileName, const WCHAR * CommandLine, DWORD CreationFlags, PPROCESS_INFORMATION ProcessInformation)
{
#if defined(_WIN32)
    STARTUPINFOW StartupInfo;

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);

    return (BOOLEAN)CreateProcessW(FileName,
                                   (WCHAR *)CommandLine,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   CreationFlags,
                                   NULL,
                                   NULL,
                                   &StartupInfo,
                                   ProcessInformation);
#elif defined(__linux__)
    //
    // TODO (linux): back with fork()+execve() (and CREATE_SUSPENDED via a
    //               ptrace/stop) when the Linux user-debugger backend lands.
    //
    (void)FileName;
    (void)CommandLine;
    (void)CreationFlags;
    (void)ProcessInformation;
    return FALSE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for OpenProcess
 *
 * @param DesiredAccess desired access rights
 * @param InheritHandle whether the handle is inheritable
 * @param ProcessId target process id
 * @return HANDLE process handle, or NULL on failure
 */
HANDLE
PlatformOpenProcess(DWORD DesiredAccess, BOOL InheritHandle, DWORD ProcessId)
{
#if defined(_WIN32)
    return OpenProcess(DesiredAccess, InheritHandle, ProcessId);
#elif defined(__linux__)
    //
    // TODO (linux): resolve a /proc/<pid> handle or ptrace-attach when the
    //               Linux user-debugger backend lands.
    //
    (void)DesiredAccess;
    (void)InheritHandle;
    (void)ProcessId;
    return NULL;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for TerminateProcess
 *
 * @param Process process handle
 * @param ExitCode exit code to set
 * @return BOOL non-zero on success, zero on failure
 */
BOOL
PlatformTerminateProcess(HANDLE Process, UINT ExitCode)
{
#if defined(_WIN32)
    return TerminateProcess(Process, ExitCode);
#elif defined(__linux__)
    //
    // TODO (linux): kill(pid, SIGKILL) once process handles are real.
    //
    (void)Process;
    (void)ExitCode;
    return FALSE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for ResumeThread
 *
 * @param Thread thread handle
 * @return DWORD previous suspend count, or (DWORD)-1 on failure
 */
DWORD
PlatformResumeThread(HANDLE Thread)
{
#if defined(_WIN32)
    return ResumeThread(Thread);
#elif defined(__linux__)
    //
    // TODO (linux): PTRACE_CONT / SIGCONT once thread handles are real.
    //
    (void)Thread;
    return (DWORD)-1;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetExitCodeProcess
 *
 * @param Process process handle
 * @param ExitCode out-param receiving the exit code
 * @return BOOL non-zero on success, zero on failure
 */
BOOL
PlatformGetExitCodeProcess(HANDLE Process, LPDWORD ExitCode)
{
#if defined(_WIN32)
    return GetExitCodeProcess(Process, ExitCode);
#elif defined(__linux__)
    //
    // TODO (linux): waitpid(WNOHANG)/read /proc state once handles are real.
    //
    (void)Process;
    (void)ExitCode;
    return FALSE;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for LoadLibrary
 *
 * @param ModulePath narrow path of the shared module to load
 * @return HMODULE handle to the loaded module, or NULL on failure
 */
HMODULE
PlatformLoadLibrary(const CHAR * ModulePath)
{
#if defined(_WIN32)
    return LoadLibraryA(ModulePath);
#elif defined(__linux__)
    // NOT YET TESTED!!
    return (HMODULE)dlopen(ModulePath, RTLD_NOW | RTLD_LOCAL);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for GetProcAddress
 *
 * @param Module module handle returned by PlatformLoadLibrary
 * @param ProcName name of the exported symbol to resolve
 * @return PVOID address of the symbol, or NULL if not found
 */
PVOID
PlatformGetProcAddress(HMODULE Module, const CHAR * ProcName)
{
#if defined(_WIN32)
    return (PVOID)GetProcAddress(Module, ProcName);
#elif defined(__linux__)
    // NOT YET TESTED!!
    return dlsym((void *)Module, ProcName);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for FreeLibrary
 *
 * @param Module module handle returned by PlatformLoadLibrary
 * @return BOOL non-zero on success, zero on failure
 */
BOOL
PlatformFreeLibrary(HMODULE Module)
{
#if defined(_WIN32)
    return FreeLibrary(Module);
#elif defined(__linux__)
    //
    // NOT YET TESTED!!
    // dlclose returns 0 on success (opposite of FreeLibrary), so invert it to
    // preserve the "non-zero == success" contract.
    //
    return (BOOL)(dlclose((void *)Module) == 0);
#else
#    error "Unsupported platform"
#endif
}
