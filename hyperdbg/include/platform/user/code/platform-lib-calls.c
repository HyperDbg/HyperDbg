/**
 * @file platform-lib-calls.c
 * @author Max Raulea (max.raulea@gmail.com) 
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
PlatformOpenFileForWriting(const wchar_t * Path)
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
PlatformMapFileReadOnly(const wchar_t * Path, PSIZE_T OutFileSize, PHANDLE OutFileHandle)
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
