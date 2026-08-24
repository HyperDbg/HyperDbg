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

//
// Backing token for the NT global PsProcessType declared in PlatformProcess.h —
// never inspected, it only has to be a dereferenceable address.
//
static POBJECT_TYPE g_LinuxProcessObjectType = NULL;
POBJECT_TYPE *      PsProcessType            = &g_LinuxProcessObjectType;

#endif // defined(__linux__)

#if defined(_WIN32) || defined(_WIN64)
//
// Semi-documented ntoskrnl exports the WDK headers (ntddk.h/ntifs.h) do not
// declare. hyperkd declares them privately in its Common.h, but the wrappers
// below are compiled into every kernel project that pulls PlatformProcess.c
// (hyperhv included), which do not see that header — so declare them here too,
// matching hyperkd/header/common/Common.h.
//
PVOID
PsGetProcessSectionBaseAddress(PEPROCESS Process);

NTKERNELAPI NTSTATUS NTAPI
SeCreateAccessState(
    PACCESS_STATE    AccessState,
    PVOID            AuxData,
    ACCESS_MASK      DesiredAccess,
    PGENERIC_MAPPING Mapping);

NTKERNELAPI VOID NTAPI
SeDeleteAccessState(
    PACCESS_STATE AccessState);
#endif // defined(_WIN32) || defined(_WIN64)

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
 * @brief Base address of a process's main executable image.
 *        Windows: PsGetProcessSectionBaseAddress().
 *        Linux: NULL — callers must tolerate "no image base yet".
 * TODO(Linux): mm->start_code / the first VMA of the task_struct backing Process.
 */
PVOID
PlatformProcessGetSectionBaseAddress(PEPROCESS Process)
{
#if defined(_WIN32) || defined(_WIN64)
    return PsGetProcessSectionBaseAddress(Process);
#elif defined(__linux__)
    (void)Process;
    return NULL;
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

/**
 * @brief Pseudo-handle for the current process. Windows: NtCurrentProcess()
 *        ((HANDLE)-1). Linux: the same (HANDLE)-1 sentinel so callers that pass
 *        it straight into the Zw*VirtualMemory wrappers keep identical semantics.
 */
HANDLE
PlatformProcessGetCurrentProcessHandle(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return NtCurrentProcess();
#elif defined(__linux__)
    return (HANDLE)(LONG_PTR)-1;
#endif
}

/**
 * @brief Open a handle to a process by its CLIENT_ID. Windows: ZwOpenProcess().
 *        Linux: fail-closed stub.
 * TODO(Linux): resolve the pid and hand back a Linux handle shim.
 */
NTSTATUS
PlatformProcessOpen(PHANDLE            ProcessHandle,
                    ACCESS_MASK        DesiredAccess,
                    POBJECT_ATTRIBUTES ObjectAttributes,
                    PCLIENT_ID         ClientId)
{
#if defined(_WIN32) || defined(_WIN64)
    return ZwOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(DesiredAccess);
    UNREFERENCED_PARAMETER(ObjectAttributes);
    UNREFERENCED_PARAMETER(ClientId);

    if (ProcessHandle != NULL)
        *ProcessHandle = NULL;

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}

/**
 * @brief Terminate a process by handle. Windows: ZwTerminateProcess().
 *        Linux: fail-closed stub.
 * TODO(Linux): send SIGKILL to the task backing ProcessHandle.
 */
NTSTATUS
PlatformProcessTerminate(HANDLE ProcessHandle, NTSTATUS ExitStatus)
{
#if defined(_WIN32) || defined(_WIN64)
    return ZwTerminateProcess(ProcessHandle, ExitStatus);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(ProcessHandle);
    UNREFERENCED_PARAMETER(ExitStatus);

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}

/**
 * @brief Open a handle to a kernel object given a pointer to it. Windows:
 *        ObOpenObjectByPointer(). Linux: fail-closed stub.
 * TODO(Linux): map the object pointer to a Linux handle shim.
 */
NTSTATUS
PlatformObjectOpenByPointer(PVOID           Object,
                            ULONG           HandleAttributes,
                            PACCESS_STATE   PassedAccessState,
                            ACCESS_MASK     DesiredAccess,
                            POBJECT_TYPE    ObjectType,
                            KPROCESSOR_MODE AccessMode,
                            PHANDLE         Handle)
{
#if defined(_WIN32) || defined(_WIN64)
    return ObOpenObjectByPointer(Object,
                                 HandleAttributes,
                                 PassedAccessState,
                                 DesiredAccess,
                                 ObjectType,
                                 AccessMode,
                                 Handle);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(Object);
    UNREFERENCED_PARAMETER(HandleAttributes);
    UNREFERENCED_PARAMETER(PassedAccessState);
    UNREFERENCED_PARAMETER(DesiredAccess);
    UNREFERENCED_PARAMETER(ObjectType);
    UNREFERENCED_PARAMETER(AccessMode);

    if (Handle != NULL)
        *Handle = NULL;

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}

/**
 * @brief Attach the current thread to a target process' address space. Windows:
 *        KeStackAttachProcess(). Linux: no-op stub.
 * TODO(Linux): kthread_use_mm() on the mm backing Process, saving state.
 */
VOID
PlatformProcessAttach(PEPROCESS Process, PKAPC_STATE ApcState)
{
#if defined(_WIN32) || defined(_WIN64)
    KeStackAttachProcess(Process, ApcState);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(ApcState);
#endif
}

/**
 * @brief Detach from a process attached via PlatformProcessAttach. Windows:
 *        KeUnstackDetachProcess(). Linux: no-op stub.
 * TODO(Linux): kthread_unuse_mm() restoring the saved state.
 */
VOID
PlatformProcessDetach(PKAPC_STATE ApcState)
{
#if defined(_WIN32) || defined(_WIN64)
    KeUnstackDetachProcess(ApcState);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(ApcState);
#endif
}

/**
 * @brief Resolve an exported kernel routine by name. Windows:
 *        MmGetSystemRoutineAddress(). Linux: fail-closed stub (returns NULL).
 * TODO(Linux): kallsyms_lookup_name()/an allowlist for the handful of routines
 * the attach path late-binds.
 */
PVOID
PlatformGetSystemRoutineAddress(PUNICODE_STRING SystemRoutineName)
{
#if defined(_WIN32) || defined(_WIN64)
    return MmGetSystemRoutineAddress(SystemRoutineName);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(SystemRoutineName);

    return NULL; // TODO(Linux)
#endif
}

/**
 * @brief Build an access state for an object-open. Windows: SeCreateAccessState().
 *        Linux: fail-closed stub.
 * TODO(Linux): no object-manager access model — the caller bails on failure.
 */
NTSTATUS
PlatformSeCreateAccessState(PACCESS_STATE    AccessState,
                            PVOID            AuxData,
                            ACCESS_MASK      Access,
                            PGENERIC_MAPPING GenericMapping)
{
#if defined(_WIN32) || defined(_WIN64)
    return SeCreateAccessState(AccessState, AuxData, Access, GenericMapping);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(AccessState);
    UNREFERENCED_PARAMETER(AuxData);
    UNREFERENCED_PARAMETER(Access);
    UNREFERENCED_PARAMETER(GenericMapping);

    return STATUS_UNSUCCESSFUL; // TODO(Linux)
#endif
}

/**
 * @brief Release an access state from PlatformSeCreateAccessState. Windows:
 *        SeDeleteAccessState(). Linux: no-op stub.
 */
VOID
PlatformSeDeleteAccessState(PACCESS_STATE AccessState)
{
#if defined(_WIN32) || defined(_WIN64)
    SeDeleteAccessState(AccessState);
#elif defined(__linux__)
    UNREFERENCED_PARAMETER(AccessState);
#endif
}
