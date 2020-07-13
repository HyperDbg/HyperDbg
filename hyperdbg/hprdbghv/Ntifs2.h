/**
 * @file Ntifs2.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers from ntifs.h which is not compatible with other parts
 * @details
 * @version 0.1
 * @date 2020-07-13
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

typedef struct _KAPC_STATE
{
    LIST_ENTRY         ApcListHead[MaximumMode];
    struct _KPROCESS * Process;
    union
    {
        UCHAR InProgressFlags;
        struct
        {
            BOOLEAN KernelApcInProgress : 1;
            BOOLEAN SpecialApcInProgress : 1;
        };
    };

    BOOLEAN KernelApcPending;
    union
    {
        BOOLEAN UserApcPendingAll;
        struct
        {
            BOOLEAN SpecialUserApcPending : 1;
            BOOLEAN UserApcPending : 1;
        };
    };
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
    _In_ HANDLE ProcessId,
    _Outptr_ PEPROCESS * Process);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeUnstackDetachProcess(
    _In_ PRKAPC_STATE ApcState);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
KeStackAttachProcess(
    _Inout_ PRKPROCESS PROCESS,
    _Out_ PRKAPC_STATE ApcState);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(return == 0, __drv_allocatesMem(Region))
NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID * BaseAddress,
    _In_ ULONG_PTR  ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG      AllocationType,
    _In_ ULONG      Protect);
