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

#if defined(__linux__)

//
// -------------------------------------------------------------------------
// Linux stand-ins for raw WDK process/thread APIs the shared sources call by
// name. Placeholder stubs. Windows gets these from <ntddk.h>/<ntifs.h>, so the
// whole block is __linux__-only.
// -------------------------------------------------------------------------
//

//
// WDK global: pointer to the System process' EPROCESS. NULL until a real
// process model is wired up. TODO(Linux): point at &init_task's mm/owner.
//
PEPROCESS PsInitialSystemProcess = NULL;

/**
 * @brief WDK stand-in: pseudo-handle for the current process.
 * @details Windows returns (HANDLE)-1; kept identical so callers that pass it
 *          straight back into Zw*VirtualMemory compare/behave the same.
 */
HANDLE
NtCurrentProcess(VOID)
{
    return (HANDLE)(LONG_PTR)-1;
}

/**
 * @brief WDK stand-in: look up an EPROCESS by PID. Stub: always fails.
 * TODO(Linux): find_get_pid()/pid_task() and hang a PEPROCESS shim off it.
 */
NTSTATUS
PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS * Process)
{
    if (Process != NULL)
        *Process = NULL;

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
}

/**
 * @brief WDK stand-in: pin the current thread to one processor. Stub: no-op.
 * TODO(Linux): set_cpus_allowed_ptr() and remember the old mask.
 */
VOID
KeSetSystemAffinityThread(KAFFINITY Affinity)
{
    // no-op
}

/**
 * @brief WDK stand-in: undo KeSetSystemAffinityThread. Stub: no-op.
 */
VOID
KeRevertToUserAffinityThread(VOID)
{
    // no-op
}

#endif // defined(__linux__)
