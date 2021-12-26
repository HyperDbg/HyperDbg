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

    if (g_ZwQueryInformationProcess == NULL)
    {
        UNICODE_STRING RoutineName;

        RtlInitUnicodeString(&RoutineName, L"ZwQueryInformationProcess");

        g_ZwQueryInformationProcess =
            (ZwQueryInformationProcess)MmGetSystemRoutineAddress(&RoutineName);

        if (g_ZwQueryInformationProcess == NULL)
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
            (ZwQueryInformationProcess)MmGetSystemRoutineAddress(&RoutineName);

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

ULONG64
UserAccessGetModuleBasex64(PEPROCESS Proc, UNICODE_STRING ModuleName)
{
    KAPC_STATE     State;
    UNICODE_STRING Name;
    PPEB           Peb = NULL;
    PPEB_LDR_DATA  Ldr = NULL;

    //
    // Process PEB, function is unexported and undocumented
    //
    Peb = (PPEB)g_PsGetProcessPeb(Proc);

    if (!Peb)
    {
        return NULL;
    }

    KeStackAttachProcess(Proc, &State);

    Ldr = (PPEB_LDR_DATA)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return NULL;
    }

    //
    // loop the linked list
    //
    for (PLIST_ENTRY List = (PLIST_ENTRY)Ldr->ModuleListLoadOrder.Flink;
         List != &Ldr->ModuleListLoadOrder;
         List = (PLIST_ENTRY)List->Flink)
    {
        PLDR_DATA_TABLE_ENTRY Entry =
            CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

        LogInfo("%ws  Base: %llx | Entry: %llx", Entry->FullDllName.Buffer, Entry->DllBase, Entry->EntryPoint);

        /*
        if (RtlCompareUnicodeString(&Entry->BaseDllName, &ModuleName, TRUE) == NULL)
        {
            ULONG64 BaseAddr = (ULONG64)Entry->DllBase;
            KeUnstackDetachProcess(&State);
            return BaseAddr;
        }
        */
    }

    KeUnstackDetachProcess(&State);

    //
    // Failed
    //
    return NULL;
}

ULONG
UserAccessGetModuleBasex86(PEPROCESS Proc, UNICODE_STRING ModuleName)
{
    KAPC_STATE      State;
    UNICODE_STRING  Name;
    PPEB32          Peb = NULL;
    PPEB_LDR_DATA32 Ldr = NULL;

    //
    // get process PEB for the x86 part, function is unexported and undocumented
    //
    Peb = (PPEB32)g_PsGetProcessWow64Process(Proc);

    if (!Peb)
    {
        return NULL;
    }

    KeStackAttachProcess(Proc, &State);

    Ldr = (PPEB_LDR_DATA32)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return 0;
    }

    //
    // loop the linked list
    //
    for (PLIST_ENTRY32 List = (PLIST_ENTRY32)Ldr->InLoadOrderModuleList.Flink;
         List != &Ldr->InLoadOrderModuleList;
         List = (PLIST_ENTRY32)List->Flink)
    {
        PLDR_DATA_TABLE_ENTRY32 Entry =
            CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

        //
        // since the PEB is x86, the DLL is x86, and so the base address is in x86 (4 byte as compared to 8 byte)
        // and the UNICODE STRING is in 32 bit(UNICODE_STRING32), and because there is no viable conversion
        // we are just going to force everything in
        //
        UNICODE_STRING DLLname;
        DLLname.Length        = Entry->BaseDllName.Length;
        DLLname.MaximumLength = Entry->BaseDllName.MaximumLength;
        DLLname.Buffer        = (PWCH)Entry->BaseDllName.Buffer;

        LogInfo("%ws  Base: %llx | Entry: %llx", Entry->FullDllName.Buffer, Entry->DllBase, Entry->EntryPoint);

        /*
           if (RtlCompareUnicodeString(&DLLname, &ModuleName, TRUE) == NULL)
        {
            ULONG BaseAddr = Entry->DllBase;
            KeUnstackDetachProcess(&State);
            return BaseAddr;
        }
        */
    }

    KeUnstackDetachProcess(&State);

    //
    // Failed
    //
    return NULL;
}

/**
 * @brief Get the base address of loaded module from process Id
 * @details This function should be called in vmx non-root
 * 
 * @param ProcessId 
 * @param BaseAddress 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessGetBaseOfModuleFromProcessId(HANDLE ProcessId, PUINT64 BaseAddress)
{
    UNICODE_STRING FunctionName;

    PEPROCESS  SourceProcess;
    KAPC_STATE State = {0};

    if (PsLookupProcessByProcessId(ProcessId, &SourceProcess) != STATUS_SUCCESS)
    {
        //
        // if the process not found
        //
        return FALSE;
    }

    ObDereferenceObject(SourceProcess);

    //
    // Find address of PsGetProcessPeb
    //
    if (g_PsGetProcessPeb == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessPeb");
        g_PsGetProcessPeb = (PsGetProcessPeb)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessPeb == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");
            return FALSE;
        }
    }

    //
    // Find address of PsGetProcessWow64Process
    //
    if (g_PsGetProcessWow64Process == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessWow64Process");
        g_PsGetProcessWow64Process = (PsGetProcessWow64Process)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessWow64Process == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");
            return FALSE;
        }
    }

    //
    // check whether the target process is 32-bit or 64-bit
    //
    if (g_PsGetProcessWow64Process(SourceProcess))
    {
        //
        // x86 process, walk x86 module list
        //
        UNICODE_STRING Temp = {0};
        UserAccessGetModuleBasex86(SourceProcess, Temp);
    }
    else if (g_PsGetProcessPeb(SourceProcess))
    {
        //
        // x64 process, walk x64 module list
        //
        UNICODE_STRING Temp = {0};
        UserAccessGetModuleBasex64(SourceProcess, Temp);
    }
    else
    {
        //
        // Wtf?
        //
        return FALSE;
    }

    return TRUE;
}
