/**
 * @file UserAccess.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Access and parse user-mode components of binaries
 * @details Access to Portable Executables
 *
 * @version 0.1
 * @date 2021-12-24
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

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
        return FALSE;
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

    if (g_ZwQueryInformationProcess == NULL)
    {
        return FALSE;
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

/**
 * @brief If the target process's main module is loaded, it fills
 * the Entrypoint and the BaseAddress
 * @details This function is safe to be called in vmx non-root
 * 
 * @param PebAddress 
 * @param Is32Bit 
 * @param BaseAddress 
 * @param Entrypoint 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessGetBaseAndEntrypointOfMainModuleIfLoadedInVmxRoot(PPEB PebAddress, BOOLEAN Is32Bit, PUINT64 BaseAddress, PUINT64 Entrypoint)
{
    if (Is32Bit)
    {
        UNICODE_STRING  Name;
        PEB_LDR_DATA32  Ldr32        = {0};
        PEB32           Peb32        = {0};
        PPEB_LDR_DATA32 LdrAddress32 = NULL;

        MemoryMapperReadMemorySafeOnTargetProcess(PebAddress, &Peb32, sizeof(PEB32));

        LdrAddress32 = (PPEB_LDR_DATA32)Peb32.Ldr;

        if (!LdrAddress32)
        {
            return FALSE;
        }

        MemoryMapperReadMemorySafeOnTargetProcess(LdrAddress32, &Ldr32, sizeof(PEB_LDR_DATA32));

        PLIST_ENTRY32 List = (PLIST_ENTRY32)Ldr32.InLoadOrderModuleList.Flink;

        PLDR_DATA_TABLE_ENTRY32 EntryAddress = CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);
        LDR_DATA_TABLE_ENTRY32  Entry        = {0};

        MemoryMapperReadMemorySafeOnTargetProcess(EntryAddress, &Entry, sizeof(LDR_DATA_TABLE_ENTRY32));

        if (Entry.DllBase == NULL || Entry.EntryPoint == NULL)
        {
            return FALSE;
        }
        else
        {
            *BaseAddress = Entry.DllBase;
            *Entrypoint  = Entry.EntryPoint;

            return TRUE;
        }
    }
    else
    {
        UNICODE_STRING Name;
        PPEB_LDR_DATA  LdrAddress = NULL;
        PEB_LDR_DATA   Ldr        = {0};

        PEB Peb = {0};

        MemoryMapperReadMemorySafeOnTargetProcess(PebAddress, &Peb, sizeof(PEB));

        LdrAddress = (PPEB_LDR_DATA)Peb.Ldr;

        if (!LdrAddress)
        {
            return FALSE;
        }

        MemoryMapperReadMemorySafeOnTargetProcess(LdrAddress, &Ldr, sizeof(PEB_LDR_DATA));

        PLIST_ENTRY List = (PLIST_ENTRY)Ldr.ModuleListLoadOrder.Flink;

        PLDR_DATA_TABLE_ENTRY EntryAddress = CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
        LDR_DATA_TABLE_ENTRY  Entry        = {0};

        MemoryMapperReadMemorySafeOnTargetProcess(EntryAddress, &Entry, sizeof(LDR_DATA_TABLE_ENTRY));

        // LogInfo("base: %llx | entry: %llx", Entry.DllBase, Entry.EntryPoint);

        if (Entry.DllBase == NULL || Entry.EntryPoint == NULL)
        {
            return FALSE;
        }
        else
        {
            *BaseAddress = Entry.DllBase;
            *Entrypoint  = Entry.EntryPoint;

            return TRUE;
        }
    }
}

/**
 * @brief Gets the loaded modules details from PEB
 * @details This function should be called in vmx non-root
 * 
 * @param Proc 
 * @param OnlyCountModules 
 * @param ModulesCount 
 * @param ModulesList 
 * @param SizeOfBufferForModulesList 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessPrintLoadedModulesX64(PEPROCESS                       Proc,
                                BOOLEAN                         OnlyCountModules,
                                PUINT32                         ModulesCount,
                                PUSERMODE_LOADED_MODULE_SYMBOLS ModulesList,
                                UINT32                          SizeOfBufferForModulesList)
{
    KAPC_STATE     State;
    UNICODE_STRING Name;
    PPEB           Peb                 = NULL;
    PPEB_LDR_DATA  Ldr                 = NULL;
    UINT32         CountOfModules      = 0;
    UINT32         CurrentSavedModules = 0;
    UINT32         TempSize            = 0;

    if (g_PsGetProcessPeb == NULL)
    {
        return FALSE;
    }

    //
    // Process PEB, function is unexported and undocumented
    //
    Peb = (PPEB)g_PsGetProcessPeb(Proc);

    if (!Peb)
    {
        return FALSE;
    }

    KeStackAttachProcess(Proc, &State);

    Ldr = (PPEB_LDR_DATA)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return FALSE;
    }

    if (OnlyCountModules)
    {
        //
        // loop the linked list (Computer the size)
        //
        for (PLIST_ENTRY List = (PLIST_ENTRY)Ldr->ModuleListLoadOrder.Flink;
             List != &Ldr->ModuleListLoadOrder;
             List = (PLIST_ENTRY)List->Flink)
        {
            PLDR_DATA_TABLE_ENTRY Entry =
                CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

            //
            // Calculate count of modules
            //
            CountOfModules++;

            /*
        Log("Base: %016llx\tEntryPoint: %016llx\tModule: %ws\tPath: %ws\n",
            Entry->DllBase,
            Entry->EntryPoint,
            Entry->BaseDllName.Buffer,
            Entry->FullDllName.Buffer);
        */
        }

        *ModulesCount = CountOfModules;

        KeUnstackDetachProcess(&State);
        return TRUE;
    }
    else
    {
        //
        // It's not counting the modules, so we compute the number of modules
        // that can be stored in the buffer by using the size of the buffer
        //
        CountOfModules = SizeOfBufferForModulesList / sizeof(USERMODE_LOADED_MODULE_SYMBOLS);
    }

    //
    // Walk again to save the buffer
    //
    Ldr = (PPEB_LDR_DATA)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return FALSE;
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

        if (CountOfModules == CurrentSavedModules)
        {
            //
            // Won't continue as the buffer is now full
            // Generally, we shouldn't be at this stage, only when
            // a module is just loaded and we didn't allocate enough
            // memory for it, so it's better to continue
            //
            KeUnstackDetachProcess(&State);
            return TRUE;
        }

        //
        // Save the details into the storage
        //
        ModulesList[CurrentSavedModules].Entrypoint  = Entry->EntryPoint;
        ModulesList[CurrentSavedModules].BaseAddress = Entry->DllBase;

        //
        // Copy the path
        //
        TempSize = Entry->FullDllName.Length;
        if (TempSize >= MAX_PATH)
        {
            TempSize = MAX_PATH;
        }

        TempSize = TempSize * 2;
        memcpy(&ModulesList[CurrentSavedModules].FilePath, Entry->FullDllName.Buffer, TempSize);

        CurrentSavedModules++;
    }

    KeUnstackDetachProcess(&State);

    return TRUE;
}

/**
 * @brief Gets the loaded modules details from PEB (x86)
 * @details This function should be called in vmx non-root
 * 
 * @param Proc 
 * @param OnlyCountModules 
 * @param ModulesCount 
 * @param ModulesList 
 * @param SizeOfBufferForModulesList 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessPrintLoadedModulesX86(PEPROCESS                       Proc,
                                BOOLEAN                         OnlyCountModules,
                                PUINT32                         ModulesCount,
                                PUSERMODE_LOADED_MODULE_SYMBOLS ModulesList,
                                UINT32                          SizeOfBufferForModulesList)
{
    KAPC_STATE      State;
    UNICODE_STRING  Name;
    PPEB32          Peb                 = NULL;
    PPEB_LDR_DATA32 Ldr                 = NULL;
    UINT32          CountOfModules      = 0;
    UINT32          CurrentSavedModules = 0;
    UINT32          TempSize            = 0;

    if (g_PsGetProcessWow64Process == NULL)
    {
        return FALSE;
    }

    //
    // Process PEB, function is unexported and undocumented
    //
    Peb = (PPEB32)g_PsGetProcessWow64Process(Proc);

    if (!Peb)
    {
        return FALSE;
    }

    KeStackAttachProcess(Proc, &State);

    Ldr = (PPEB_LDR_DATA32)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return FALSE;
    }

    if (OnlyCountModules)
    {
        //
        // loop the linked list (Computer the size)
        //
        for (PLIST_ENTRY32 List = (PLIST_ENTRY32)Ldr->InLoadOrderModuleList.Flink;
             List != &Ldr->InLoadOrderModuleList;
             List = (PLIST_ENTRY32)List->Flink)
        {
            PLDR_DATA_TABLE_ENTRY32 Entry =
                CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

            //
            // Calculate count of modules
            //
            CountOfModules++;
        }

        *ModulesCount = CountOfModules;

        KeUnstackDetachProcess(&State);
        return TRUE;
    }
    else
    {
        //
        // It's not counting the modules, so we compute the number of modules
        // that can be stored in the buffer by using the size of the buffer
        //
        CountOfModules = SizeOfBufferForModulesList / sizeof(USERMODE_LOADED_MODULE_SYMBOLS);
    }

    //
    // Walk again to save the buffer
    //
    Ldr = (PPEB_LDR_DATA32)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return FALSE;
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

        if (CountOfModules == CurrentSavedModules)
        {
            //
            // Won't continue as the buffer is now full
            // Generally, we shouldn't be at this stage, only when
            // a module is just loaded and we didn't allocate enough
            // memory for it, so it's better to continue
            //
            KeUnstackDetachProcess(&State);
            return TRUE;
        }

        //
        // Save the details into the storage
        //
        ModulesList[CurrentSavedModules].Entrypoint  = Entry->EntryPoint;
        ModulesList[CurrentSavedModules].BaseAddress = Entry->DllBase;

        //
        // Copy the path
        //
        TempSize = Entry->FullDllName.Length;
        if (TempSize >= MAX_PATH)
        {
            TempSize = MAX_PATH;
        }

        TempSize = TempSize * 2;
        memcpy(&ModulesList[CurrentSavedModules].FilePath, Entry->FullDllName.Buffer, TempSize);

        CurrentSavedModules++;
    }

    KeUnstackDetachProcess(&State);

    return TRUE;
}

/**
 * @brief Print loaded modules details from PEB
 * @details This function should be called in vmx non-root
 * 
 * @param Proc 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessPrintLoadedModulesX86_2(PEPROCESS Proc)
{
    KAPC_STATE      State;
    UNICODE_STRING  Name;
    PPEB32          Peb = NULL;
    PPEB_LDR_DATA32 Ldr = NULL;

    if (g_PsGetProcessWow64Process == NULL)
    {
        return FALSE;
    }

    //
    // get process PEB for the x86 part, function is unexported and undocumented
    //
    Peb = (PPEB32)g_PsGetProcessWow64Process(Proc);

    if (!Peb)
    {
        return FALSE;
    }

    KeStackAttachProcess(Proc, &State);

    Ldr = (PPEB_LDR_DATA32)Peb->Ldr;

    if (!Ldr)
    {
        KeUnstackDetachProcess(&State);
        return FALSE;
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
        UNICODE_STRING ModuleName;
        UNICODE_STRING ModulePath;
        UINT64         BaseAddr          = NULL;
        UINT64         EntrypointAddress = NULL;

        BaseAddr          = Entry->DllBase;
        EntrypointAddress = Entry->EntryPoint;

        ModuleName.Length        = Entry->BaseDllName.Length;
        ModuleName.MaximumLength = Entry->BaseDllName.MaximumLength;
        ModuleName.Buffer        = (PWCH)Entry->BaseDllName.Buffer;

        ModulePath.Length        = Entry->FullDllName.Length;
        ModulePath.MaximumLength = Entry->FullDllName.MaximumLength;
        ModulePath.Buffer        = (PWCH)Entry->FullDllName.Buffer;

        Log("Base: %016llx\tEntryPoint: %016llx\tModule: %ws\tPath: %ws\n",
            BaseAddr,
            EntrypointAddress,
            ModuleName.Buffer,
            ModulePath.Buffer);
    }

    KeUnstackDetachProcess(&State);

    return TRUE;
}

/**
 * @brief Detects whether process is 32-bit or 64-bit
 * @details This function should be called in vmx non-root
 * 
 * @param ProcessId 
 * @param Is32Bit 
 * 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessIsWow64Process(HANDLE ProcessId, PBOOLEAN Is32Bit)
{
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

    if (g_PsGetProcessWow64Process == NULL || g_PsGetProcessPeb == NULL)
    {
        return FALSE;
    }

    if (g_PsGetProcessWow64Process(SourceProcess))
    {
        //
        // x86 process, walk x86 module list
        //

        *Is32Bit = TRUE;

        return TRUE;
    }
    else if (g_PsGetProcessPeb(SourceProcess))
    {
        //
        // x64 process, walk x64 module list
        //
        *Is32Bit = FALSE;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Get details about loaded modules
 * @details This function should be called in vmx non-root
 * 
 * @param ProcessLoadedModuleRequest
 * @param BufferSize
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessGetLoadedModules(PUSERMODE_LOADED_MODULE_DETAILS ProcessLoadedModuleRequest, UINT32 BufferSize)
{
    PEPROCESS SourceProcess;
    BOOLEAN   Is32Bit;

    if (PsLookupProcessByProcessId(ProcessLoadedModuleRequest->ProcessId, &SourceProcess) != STATUS_SUCCESS)
    {
        //
        // if the process not found
        //
        ProcessLoadedModuleRequest->Result = DEBUGGER_ERROR_INVALID_PROCESS_ID;
        return FALSE;
    }

    ObDereferenceObject(SourceProcess);

    //
    // check whether the target process is 32-bit or 64-bit
    //
    if (!UserAccessIsWow64Process(ProcessLoadedModuleRequest->ProcessId, &Is32Bit))
    {
        //
        // Unable to detect whether it's 32-bit or 64-bit
        //
        ProcessLoadedModuleRequest->Result = DEBUGGER_ERROR_UNABLE_TO_GET_MODULES_OF_THE_PROCESS;
        return FALSE;
    }

    if (Is32Bit)
    {
        //
        // x86 process, walk x86 module list
        //
        if (UserAccessPrintLoadedModulesX86(SourceProcess,
                                            ProcessLoadedModuleRequest->OnlyCountModules,
                                            &ProcessLoadedModuleRequest->ModulesCount,
                                            (UINT64)ProcessLoadedModuleRequest + sizeof(USERMODE_LOADED_MODULE_DETAILS),
                                            BufferSize - sizeof(USERMODE_LOADED_MODULE_DETAILS)))
        {
            ProcessLoadedModuleRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            return TRUE;
        }
    }
    else
    {
        //
        // x64 process, walk x64 module list
        //
        if (UserAccessPrintLoadedModulesX64(SourceProcess,
                                            ProcessLoadedModuleRequest->OnlyCountModules,
                                            &ProcessLoadedModuleRequest->ModulesCount,
                                            (UINT64)ProcessLoadedModuleRequest + sizeof(USERMODE_LOADED_MODULE_DETAILS),
                                            BufferSize - sizeof(USERMODE_LOADED_MODULE_DETAILS)))
        {
            ProcessLoadedModuleRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            return TRUE;
        }
    }

    ProcessLoadedModuleRequest->Result = DEBUGGER_ERROR_UNABLE_TO_GET_MODULES_OF_THE_PROCESS;
    return FALSE;
}

/**
 * @brief Checks whether the loaded module is available or not
 * 
 * @return BOOLEAN 
 */
BOOLEAN
UserAccessCheckForLoadedModuleDetails()
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;
    UINT64                              BaseAddress = NULL;
    UINT64                              Entrypoint  = NULL;

    //
    // Find the thread debugging detail structure
    //
    ProcessDebuggingDetail =
        AttachingFindProcessDebuggingDetailsByProcessId(PsGetCurrentProcessId());

    //
    // Check if we find the debugging detail of the thread or not
    //
    if (ProcessDebuggingDetail == NULL)
    {
        return FALSE;
    }

    if (ProcessDebuggingDetail->PebAddressToMonitor != NULL &&
        UserAccessGetBaseAndEntrypointOfMainModuleIfLoadedInVmxRoot(ProcessDebuggingDetail->PebAddressToMonitor,
                                                                    ProcessDebuggingDetail->Is32Bit,
                                                                    &BaseAddress,
                                                                    &Entrypoint))
    {
        ProcessDebuggingDetail->BaseAddressOfMainModule = BaseAddress;
        ProcessDebuggingDetail->EntrypointOfMainModule  = Entrypoint;

        //
        // Set debug register to get the entrypoint of user-mode processs
        //
        DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                          BREAK_ON_INSTRUCTION_FETCH,
                          FALSE,
                          Entrypoint);

        // LogInfo("Base: %016llx \t EntryPoint: %016llx", BaseAddress, Entrypoint);

        return TRUE;
    }
    else
    {
        //
        // Not available
        //
        return FALSE;
    }
}
