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
    HANDLE Handle = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        GetCurrentProcessId());

    if (Handle)
    {
        CHAR ModulePath[MAX_PATH] = {0};
        if (GetModuleFileNameEx(Handle, 0, ModulePath, MAX_PATH))
        {
            CloseHandle(Handle);
            strncpy(ProcessNameBuf, PathFindFileNameA(ModulePath), MAX_PATH - 1);
            ProcessNameBuf[MAX_PATH - 1] = '\0';
            return ProcessNameBuf;
        }
        CloseHandle(Handle);
    }
    return NULL;

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
