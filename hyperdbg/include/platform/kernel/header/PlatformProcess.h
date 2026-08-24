/**
 * @file PlatformProcess.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for process and thread queries
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"

//
// NT's global process object-type pointer (ntddk). Shared code passes
// *PsProcessType to the Se*/Ob* process-handle APIs; Linux has no object
// manager, so this is a placeholder token the (fail-closed) Platform* stubs
// never inspect. Defined in PlatformProcess.c.
//
extern POBJECT_TYPE * PsProcessType;
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

HANDLE
PlatformProcessGetCurrentThreadId(VOID);

HANDLE
PlatformProcessGetCurrentProcessId(VOID);

PVOID
PlatformProcessGetCurrentProcess(VOID);

PVOID
PlatformProcessGetCurrentThread(VOID);

PVOID
PlatformProcessGetCurrentThreadTeb(VOID);

//////////////////////////////////////////////////
//   Cross-platform process/thread APIs         //
//   (Windows -> WDK; Linux arm stubbed, see .c)//
//////////////////////////////////////////////////

PEPROCESS
PlatformProcessGetInitialSystemProcess(VOID);

NTSTATUS
PlatformProcessLookupByProcessId(HANDLE ProcessId, PEPROCESS * Process);

UCHAR *
PlatformProcessGetImageFileName(PEPROCESS Process);

PVOID
PlatformProcessGetSectionBaseAddress(PEPROCESS Process);

VOID
PlatformProcessSetSystemAffinity(KAFFINITY Affinity);

VOID
PlatformProcessRevertToUserAffinity(VOID);

HANDLE
PlatformProcessGetCurrentProcessHandle(VOID);

NTSTATUS
PlatformProcessOpen(PHANDLE            ProcessHandle,
                    ACCESS_MASK        DesiredAccess,
                    POBJECT_ATTRIBUTES ObjectAttributes,
                    PCLIENT_ID         ClientId);

NTSTATUS
PlatformProcessTerminate(HANDLE ProcessHandle, NTSTATUS ExitStatus);

NTSTATUS
PlatformObjectOpenByPointer(PVOID           Object,
                            ULONG           HandleAttributes,
                            PACCESS_STATE   PassedAccessState,
                            ACCESS_MASK     DesiredAccess,
                            POBJECT_TYPE    ObjectType,
                            KPROCESSOR_MODE AccessMode,
                            PHANDLE         Handle);

VOID
PlatformProcessAttach(PEPROCESS Process, PKAPC_STATE ApcState);

VOID
PlatformProcessDetach(PKAPC_STATE ApcState);

//
// Resolve an exported kernel routine by name. Windows: MmGetSystemRoutineAddress().
// Used by the user-debugger attach path to late-bind undocumented Ps*/Zw* routines.
//
PVOID
PlatformGetSystemRoutineAddress(PUNICODE_STRING SystemRoutineName);

//
// Access-state helpers for the undocumented NtOpenProcess path. Windows:
// SeCreateAccessState()/SeDeleteAccessState().
//
NTSTATUS
PlatformSeCreateAccessState(PACCESS_STATE    AccessState,
                            PVOID            AuxData,
                            ACCESS_MASK      Access,
                            PGENERIC_MAPPING GenericMapping);

VOID
PlatformSeDeleteAccessState(PACCESS_STATE AccessState);
