/**
 * @file PlatformProcess.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for process and thread queries
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformProcess.h"
#endif // defined(__linux__)

/**
 * @brief Get the current thread ID
 *
 * @return HANDLE
 */
HANDLE
PlatformProcessGetCurrentThreadId(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentThreadId();

#elif defined(__linux__)

    return (HANDLE)(uintptr_t)task_pid_nr(current); // tsk->pid == the thread id (userspace TID)

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current process ID
 *
 * @return HANDLE
 */
HANDLE
PlatformProcessGetCurrentProcessId(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentProcessId();

#elif defined(__linux__)

    return (HANDLE)(uintptr_t)task_tgid_nr(current); // tsk->tgid == the process id (userspace PID)

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current process (PEPROCESS)
 *
 * @return PVOID Pointer to the EPROCESS structure for the current process
 */
PVOID
PlatformProcessGetCurrentProcess(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return (PVOID)PsGetCurrentProcess();

#elif defined(__linux__)

    //
    // PsGetCurrentProcess returns the per-PROCESS object (identical for every
    // thread in the process). current is the per-THREAD task_struct, so use its
    // thread-group leader — stable across all threads, and the correct source
    // for the process ->comm name.
    //
    return (PVOID)current->group_leader;

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current thread (PETHREAD)
 *
 * @return PVOID Pointer to the ETHREAD structure for the current thread
 */
PVOID
PlatformProcessGetCurrentThread(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return (PVOID)PsGetCurrentThread();

#elif defined(__linux__)

    //
    // On Linux a task IS a thread, so the current task_struct is the thread object.
    //
    return (PVOID)current;

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the TEB (Thread Environment Block) of the current thread
 *
 * @return PVOID Pointer to the TEB of the current thread
 */
PVOID
PlatformProcessGetCurrentThreadTeb(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentThreadTeb();

#elif defined(__linux__)

    //
    // STUB: the TEB is a Windows user-mode structure with no Linux equivalent
    // (cf. $peb, likewise stubbed). $teb reads NULL on Linux.
    //
    return NULL;

#else

#    error "Unsupported platform"

#endif
}

//
// -------------------------------------------------------------------------
// Cross-platform wrappers for the process/thread APIs the shared sources use.
// Windows forwards to the WDK; the Linux arm is a placeholder stub for now.
// -------------------------------------------------------------------------
//

/**
 * @brief The System process' EPROCESS. Windows: PsInitialSystemProcess. Linux:
 *        NULL stub. TODO(Linux): a PEPROCESS shim over &init_task.
 */
PEPROCESS
PlatformProcessGetInitialSystemProcess(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return PsInitialSystemProcess;
#elif defined(__linux__)
    return NULL; // TODO(Linux)
#endif
}

/**
 * @brief Look up an EPROCESS by PID. Windows: PsLookupProcessByProcessId().
 *        Linux: stub that always fails.
 * TODO(Linux): find_get_pid()/pid_task() and hang a PEPROCESS shim off it.
 */
NTSTATUS
PlatformProcessLookupByProcessId(HANDLE ProcessId, PEPROCESS * Process)
{
#if defined(_WIN32) || defined(_WIN64)
    return PsLookupProcessByProcessId(ProcessId, Process);
#elif defined(__linux__)
    if (Process != NULL)
        *Process = NULL;

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}

/**
 * @brief The short image name of a process. Windows: PsGetProcessImageFileName().
 *        Linux: a static "" so callers doing string ops don't dereference NULL.
 * TODO(Linux): get_task_comm() on the task_struct backing Process.
 */
UCHAR *
PlatformProcessGetImageFileName(PEPROCESS Process)
{
#if defined(_WIN32) || defined(_WIN64)
    return PsGetProcessImageFileName(Process);
#elif defined(__linux__)
    static UCHAR Unknown[] = "";
    (void)Process;
    return Unknown;
#endif
}

/**
 * @brief Pin the current thread to one processor. Windows:
 *        KeSetSystemAffinityThread(). Linux: no-op stub.
 * TODO(Linux): set_cpus_allowed_ptr() and remember the old mask.
 */
VOID
PlatformProcessSetSystemAffinity(KAFFINITY Affinity)
{
#if defined(_WIN32) || defined(_WIN64)
    KeSetSystemAffinityThread(Affinity);
#elif defined(__linux__)
    // no-op
#endif
}

/**
 * @brief Undo PlatformProcessSetSystemAffinity. Windows:
 *        KeRevertToUserAffinityThread(). Linux: no-op stub.
 */
VOID
PlatformProcessRevertToUserAffinity(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    KeRevertToUserAffinityThread();
#elif defined(__linux__)
    // no-op
#endif
}
