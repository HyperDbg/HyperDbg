/**
 * @file Common.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for common tasks in debugger
 * @details
 * @version 0.2
 * @date 2023-01-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Checks whether the process with ProcId exists or not
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param UINT32 ProcId
 * @return BOOLEAN Returns true if the process
 * exists and false if it the process doesn't exist
 */
BOOLEAN
CommonIsProcessExist(UINT32 ProcId)
{
    PEPROCESS TargetEprocess;

    if (PsLookupProcessByProcessId((HANDLE)ProcId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return FALSE;
    }
    else
    {
        ObDereferenceObject(TargetEprocess);

        return TRUE;
    }
}

/**
 * @brief Get handle from Process Id
 * @param Handle
 * @param ProcessId
 *
 * @return NTSTATUS
 */
_Use_decl_annotations_
NTSTATUS
CommonGetHandleFromProcess(UINT32 ProcessId, PHANDLE Handle)
{
    NTSTATUS Status;
    Status                    = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjAttr = {0};
    CLIENT_ID         Cid     = {0};
    InitializeObjectAttributes(&ObjAttr, NULL, 0, NULL, NULL);

    Cid.UniqueProcess = (HANDLE)ProcessId;
    Cid.UniqueThread  = (HANDLE)0;

    Status = ZwOpenProcess(Handle, PROCESS_ALL_ACCESS, &ObjAttr, &Cid);

    return Status;
}

/**
 * @brief Get process name by eprocess
 *
 * @param Eprocess Process eprocess
 * @return PCHAR Returns a pointer to the process name
 */
PCHAR
CommonGetProcessNameFromProcessControlBlock(PEPROCESS Eprocess)
{
    PCHAR Result = 0;

    //
    // We can't use PsLookupProcessByProcessId as in pageable and not
    // work on vmx-root
    //
    Result = (CHAR *)PsGetProcessImageFileName(Eprocess);

    return Result;
}

/**
 * @brief The undocumented way of NtOpenProcess
 * @param ProcessHandle
 * @param DesiredAccess
 * @param ProcessId
 * @param AccessMode
 *
 * @return NTSTATUS
 */
NTSTATUS
CommonUndocumentedNtOpenProcess(
    PHANDLE         ProcessHandle,
    ACCESS_MASK     DesiredAccess,
    HANDLE          ProcessId,
    KPROCESSOR_MODE AccessMode)
{
    NTSTATUS     Status = STATUS_SUCCESS;
    ACCESS_STATE AccessState;
    CHAR         AuxData[0x200] = {0};
    PEPROCESS    ProcessObject  = NULL;
    HANDLE       ProcHandle     = NULL;

    Status = SeCreateAccessState(
        &AccessState,
        AuxData,
        DesiredAccess,
        (PGENERIC_MAPPING)((PCHAR)*PsProcessType + 52));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    AccessState.PreviouslyGrantedAccess |= AccessState.RemainingDesiredAccess;
    AccessState.RemainingDesiredAccess = 0;

    Status = PsLookupProcessByProcessId(ProcessId, &ProcessObject);

    if (!NT_SUCCESS(Status))
    {
        SeDeleteAccessState(&AccessState);
        return Status;
    }
    Status = ObOpenObjectByPointer(
        ProcessObject,
        0,
        &AccessState,
        0,
        *PsProcessType,
        AccessMode,
        &ProcHandle);

    SeDeleteAccessState(&AccessState);

    ObDereferenceObject(ProcessObject);

    if (NT_SUCCESS(Status))
        *ProcessHandle = ProcHandle;

    return Status;
}

/**
 * @brief Kill a user-mode process with different methods
 * @param ProcessId
 * @param KillingMethod
 *
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
CommonKillProcess(UINT32 ProcessId, PROCESS_KILL_METHODS KillingMethod)
{
    NTSTATUS  Status        = STATUS_SUCCESS;
    HANDLE    ProcessHandle = NULL;
    PEPROCESS Process       = NULL;

    if (ProcessId == NULL_ZERO)
    {
        return FALSE;
    }

    switch (KillingMethod)
    {
    case PROCESS_KILL_METHOD_1:

        Status = CommonGetHandleFromProcess(ProcessId, &ProcessHandle);

        if (!NT_SUCCESS(Status) || ProcessHandle == NULL)
        {
            return FALSE;
        }

        //
        // Call ZwTerminateProcess with NULL handle
        //
        Status = ZwTerminateProcess(ProcessHandle, 0);

        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        break;

    case PROCESS_KILL_METHOD_2:

        CommonUndocumentedNtOpenProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            (HANDLE)ProcessId,
            KernelMode);

        if (ProcessHandle == NULL)
        {
            return FALSE;
        }

        //
        // Call ZwTerminateProcess with NULL handle
        //
        Status = ZwTerminateProcess(ProcessHandle, 0);

        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        break;

    case PROCESS_KILL_METHOD_3:

        //
        // Get the base address of process's executable image and unmap it
        //
        Status = MmUnmapViewOfSection(Process, PsGetProcessSectionBaseAddress(Process));

        //
        // Dereference the target process
        //
        ObDereferenceObject(Process);

        break;

    default:

        //
        // Unknown killing method
        //
        return FALSE;
        break;
    }

    //
    // If we reached here, it means the functionality of
    // the above codes was successful
    //
    return TRUE;
}

/**
 * @brief Validate core number
 * @param CoreNumber
 *
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
CommonValidateCoreNumber(UINT32 CoreNumber)
{
    ULONG ProcessorsCount;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    if (CoreNumber >= ProcessorsCount)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
