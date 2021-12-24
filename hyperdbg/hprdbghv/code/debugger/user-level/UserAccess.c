/**
 * @file UserAccess.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Access and parse user-mode components of binaries
 * @details Access to Portable Executables
 *
 * @version 0.1
 * @date 2021-12-24
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Get the image path from process Id
 * @details This function should be called in vmx non-root
 * for size 512 is enough, if the size is not enough it 
 * returns FALSE
 * it's up to the user to deallocate ProcessImageName.Buffer
 * 
 * @param ProcessId 
 * @param ProcessImageName 
 * @param SizeOfImageNameToBeAllocated 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessAllocateAndGetImagePathFromProcessId(HANDLE          ProcessId,
                                               PUNICODE_STRING ProcessImageName,
                                               UINT32          SizeOfImageNameToBeAllocated)
{
    NTSTATUS        Status;
    ULONG           ReturnedLength;
    ULONG           BufferLength;
    HANDLE          ProcessHandle;
    PVOID           Buffer;
    PEPROCESS       EProcess;
    PUNICODE_STRING ImageName;

    //
    // This eliminates the possibility of the IDLE Thread/Process
    //
    PAGED_CODE();

    Status = PsLookupProcessByProcessId(ProcessId, &EProcess);

    if (NT_SUCCESS(Status))
    {
        Status = ObOpenObjectByPointer(EProcess, 0, NULL, 0, 0, KernelMode, &ProcessHandle);

        if (!NT_SUCCESS(Status))
        {
            LogError("Err, cannot get the process object (%08x)", Status);
            return FALSE;
        }

        ObDereferenceObject(EProcess);
    }
    else
    {
        //
        // Probably, the process id is wrong!
        //
        return FALSE;
    }

    if (NULL == g_ZwQueryInformationProcess)
    {
        UNICODE_STRING RoutineName;

        RtlInitUnicodeString(&RoutineName, L"ZwQueryInformationProcess");

        g_ZwQueryInformationProcess =
            (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&RoutineName);

        if (NULL == g_ZwQueryInformationProcess)
        {
            LogError("Err, cannot resolve ZwQueryInformationProcess");
            return FALSE;
        }
    }

    //
    // Query the actual size of the process path
    //
    Status = g_ZwQueryInformationProcess(ProcessHandle,
                                         ProcessImageFileName,
                                         NULL, // Buffer
                                         0,    // Buffer size
                                         &ReturnedLength);

    if (Status != STATUS_INFO_LENGTH_MISMATCH)
    {
        //
        // ZwQueryInformationProcess failed
        //
        return FALSE;
    }

    //
    // Check there is enough space to store the actual process path when it is found
    // If not return FALSE
    //
    BufferLength = ReturnedLength - sizeof(UNICODE_STRING);

    if (SizeOfImageNameToBeAllocated < BufferLength)
    {
        return FALSE;
    }

    //
    // Allocate a temporary buffer to store the path name
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, ReturnedLength, POOLTAG);

    if (Buffer == NULL)
    {
        return FALSE;
    }

    //
    // Retrieve the process path from the handle to the process
    //
    Status = g_ZwQueryInformationProcess(ProcessHandle,
                                         ProcessImageFileName,
                                         Buffer,
                                         ReturnedLength,
                                         &ReturnedLength);

    if (NT_SUCCESS(Status))
    {
        //
        // Copy the path name
        //
        ImageName = (PUNICODE_STRING)Buffer;

        //
        // Alloate UNICODE_STRING
        //
        ProcessImageName->Length        = 0;
        ProcessImageName->MaximumLength = SizeOfImageNameToBeAllocated;
        ProcessImageName->Buffer        = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, SizeOfImageNameToBeAllocated, POOLTAG);

        if (ProcessImageName->Buffer == NULL)
        {
            return FALSE;
        }

        RtlZeroMemory(ProcessImageName->Buffer, SizeOfImageNameToBeAllocated);

        //
        // Copy path to the buffer
        //
        RtlCopyUnicodeString(ProcessImageName, ImageName);

        //
        // Free the temp buffer which stored the path
        //
        ExFreePoolWithTag(Buffer, POOLTAG);

        return TRUE;
    }
    else
    {
        //
        // There was an error in ZwQueryInformationProcess
        // Free the temp buffer which stored the path
        //
        ExFreePoolWithTag(Buffer, POOLTAG);
        return FALSE;
    }
}

/**
 * @brief Get the process's PEB from process Id
 * @details This function should be called in vmx non-root
 * 
 * @param ProcessId 
 * @param Peb 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessGetPebFromProcessId(HANDLE ProcessId, PUINT64 Peb)
{
    NTSTATUS                  Status;
    ULONG                     ReturnedLength;
    HANDLE                    ProcessHandle;
    PEPROCESS                 EProcess;
    PPEB                      ProcessPeb;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo = {0};

    //
    // This eliminates the possibility of the IDLE Thread/Process
    //
    PAGED_CODE();

    Status = PsLookupProcessByProcessId(ProcessId, &EProcess);

    if (NT_SUCCESS(Status))
    {
        Status = ObOpenObjectByPointer(EProcess, 0, NULL, 0, 0, KernelMode, &ProcessHandle);

        if (!NT_SUCCESS(Status))
        {
            LogError("Err, cannot get the process object (%08x)", Status);
            return FALSE;
        }

        ObDereferenceObject(EProcess);
    }
    else
    {
        //
        // Probably, the process id is wrong!
        //
        return FALSE;
    }

    if (NULL == g_ZwQueryInformationProcess)
    {
        UNICODE_STRING RoutineName;

        RtlInitUnicodeString(&RoutineName, L"ZwQueryInformationProcess");

        g_ZwQueryInformationProcess =
            (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&RoutineName);

        if (NULL == g_ZwQueryInformationProcess)
        {
            LogError("Err, cannot resolve ZwQueryInformationProcess");
            return FALSE;
        }
    }

    //
    //  Retrieve the process path from the handle to the process
    //
    Status = g_ZwQueryInformationProcess(ProcessHandle,
                                         ProcessBasicInformation,
                                         &ProcessBasicInfo,
                                         sizeof(PROCESS_BASIC_INFORMATION),
                                         &ReturnedLength);

    if (NT_SUCCESS(Status))
    {
        ProcessPeb = ProcessBasicInfo.PebBaseAddress;

        *Peb = ProcessPeb;
        return TRUE;
    }

    return FALSE;
}
