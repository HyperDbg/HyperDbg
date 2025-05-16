/**
 * @file Transparency.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief try to hide the debugger from anti-debugging and anti-hypervisor methods
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Hide debugger on transparent-mode (activate transparent-mode)
 *
 * @param TransparentModeRequest
 * @return BOOLEAN
 */
BOOLEAN
TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE TransparentModeRequest)
{
    //
    // Check whether the transparent-mode was already initialized or not
    //
    if (!g_TransparentMode)
{

    //
    // Insert EPT memory page hook for Windows system call handler, KiSystemCall64()
    //
    MSR Msr = {0};
    Msr.Flags = __readmsr(IA32_LSTAR);

    if (!EptHook((PVOID)(Msr.Flags + 3), (UINT32)(ULONG_PTR)PsGetCurrentProcessId())) {
        LogInfo("Error while inserting EPT page hook for Windows system call handler at address 0x%p+3", Msr.Flags);
        
        TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
        return FALSE;
    }
    else {
        LogInfo("EPT page hook inserted");
    }

    //
    // Allocate buffer for the transparent-mode trap flag state
    //
    g_TransparentModeTrapFlagState = (TRANSPARENT_MODE_TRAP_FLAG_STATE *)PlatformMemAllocateZeroedNonPagedPool(sizeof(TRANSPARENT_MODE_TRAP_FLAG_STATE));

    //
    // Intercept trap flags #DBs and #BPs for the transparent-mode
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Enable the transparent-mode
    //
    g_TransparentMode                    = TRUE;
    TransparentModeRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    
    //
    // Successfully enabled the transparent-mode
    //
    return TRUE;
}
    else
    {
        TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_DEBUGGER_ALREADY_HIDE;
        return FALSE;
    }
}

/**
 * @brief Deactivate transparent-mode
 * @param TransparentModeRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentUnhideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE TransparentModeRequest)
{
    if (g_TransparentMode)
    {
        //
        // Disable the transparent-mode
        //
        g_TransparentMode = FALSE;
    
        //
        // Unset the trap flags #DBs and #BPs for the transparent-mode
        //
        BroadcastDisableDbAndBpExitingAllCores();
    
        //
        // Free the buffer for the transparent-mode trap flag state
        //
        PlatformMemFreePool(g_TransparentModeTrapFlagState);
    
    
        MSR Msr = {0};
        Msr.Flags = __readmsr(IA32_LSTAR);
    
        
        if (!EptHookUnHookSingleAddress((UINT64)(Msr.Flags + 3), (UINT64)NULL, (UINT32)(ULONG_PTR)PsGetCurrentProcessId())) {
            LogInfo("Error while removing the EPT hook from windows syscall handler at address 0x%p+3", Msr.Flags);
    
            TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
            return FALSE;
        }
    
        TransparentModeRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
        return TRUE;
    }
    else
    {
        TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_DEBUGGER_ALREADY_UNHIDE;
        return FALSE;
    }
}

/**
 * @brief Handle Cpuid Vmexits when the Transparent mode is enabled
 *
 * @param CpuInfo The temporary logical processor registers
 * @param Regs Vcpu's GP registers
 * @return VOID
 */
VOID
TransparentCPUID(INT32 CpuInfo[], PGUEST_REGS Regs)
{
    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        //
        // Unset the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        CpuInfo[2] &= ~HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS || Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // When transparent, all CPUID leaves in the 0x40000000+ range should contain no usable data
        //
        CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0;
    }
}

/**
 * @brief Handle The triggered hook on KiSystemCall64 system call handler
 * when the Transparency mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleSystemCallHook(VIRTUAL_MACHINE_STATE* VCpu)
{

    switch(VCpu->Regs->rax & 0x00000000ffffffff)
    {
    case SysNtQuerySystemInformation:
    case SysNtQuerySystemInformationEx:
    {
        //
        // Handle the NtQuerySystemInformation System call
        //

        TransparentHandleNtQuerySystemInformationSyscall(VCpu);
        
        break;
    }
    case SysNtQueryVolumeInformationFile:
    {
        //
        // Handle the NtQueryVolumeInformationFile System call
        //
        //TransparentHandleNtQueryVolumeInformationFileSyscall(VCpu);

        break;
    }
    case SysNtSystemDebugControl:
    {
        //
        // Handle the NtSystemDebugControl System call
        //
        TransparentHandleNtSystemDebugControlSyscall(VCpu);

        break;
    }
    case SysNtQueryAttributesFile:
    {
        //
        // Handle the NtQueryAttributesFile System call
        //
        
        TransparentHandleNtQueryAttributesFileSyscall(VCpu);

        break;
    }
    case SysNtOpenDirectoryObject:
    {
        //
        // Handle the NtOpenDirectoryObject System call
        //
        TransparentHandleNtOpenDirectoryObjectSyscall(VCpu);

        break;
    }
    case SysNtQueryDirectoryObject:
    {
        //
        // Handle the NtQueryDirectoryObject System call
        //
        //TransparentHandleNtQueryDirectoryObjectSyscall(VCpu);

        break;
    }
    case SysNtQueryInformationProcess:
    {
        //
        // Handle the NtQueryInformationProcess System call
        //
        TransparentHandleNtQueryInformationProcessSyscall(VCpu);

        break;
    }
    case SysNtQueryInformationThread:
    {
        //
        // Handle the NtQueryInformationThread System call
        //
        //TransparentHandleNtQueryInformationThreadSyscall(VCpu);

        break;
    }
    case SysNtOpenFile:
    {
        //
        // Handle the NtOpenFile System call
        //
        TransparentHandleNtOpenFileSyscall(VCpu);

        break;
    }
    case SysNtOpenKeyTransacted:
    case SysNtOpenKeyEx:
    case SysNtOpenKey:
    {
        //
        // Handle the NtOpenKey System call
        //
        TransparentHandleNtOpenKeySyscall(VCpu);
        break;
    }
    case SysNtQueryValueKey:
    {
        //
        // Handle the NtQueryValueKey System call
        //
        TransparentHandleNtQueryValueKeySyscall(VCpu);
        break;
    }
    case SysNtEnumerateKey:
    {
        //
        // Handle the NtEnumerateKey System call
        //
        TransparentHandleNtEnumerateKeySyscall(VCpu);
        break;
    }
    default:
    {
        break;
    }
    }
}

/**
 * @brief Handle The NtQuerySystemInformation system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtQuerySystemInformationSyscall(VIRTUAL_MACHINE_STATE * VCpu)
{
    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};

    switch (VCpu->Regs->r10) 
    {
    case SystemProcessInformation:
    case SystemExtendedProcessInformation:
    {

        ContextParams.OptionalParam1                  = SystemProcessInformation;
        ContextParams.OptionalParam2                  = VCpu->Regs->rdx;
        ContextParams.OptionalParam3                  = VCpu->Regs->r8 - 0x400;

        TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);

        break;
    }
    case SystemModuleInformation:
    {

        ContextParams.OptionalParam1                  = SystemModuleInformation;
        ContextParams.OptionalParam2                  = VCpu->Regs->rdx;
        ContextParams.OptionalParam3                  = VCpu->Regs->r8;

        TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);

        break;
    }
    case SystemKernelDebuggerInformation:
    {

        ContextParams.OptionalParam1                  = SystemKernelDebuggerInformation;
        ContextParams.OptionalParam2                  = VCpu->Regs->rdx;
        ContextParams.OptionalParam3                  = VCpu->Regs->r8;

        TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);
        break;
    }
    case SystemCodeIntegrityInformation:
    {

        ContextParams.OptionalParam1                  = SystemCodeIntegrityInformation;
        ContextParams.OptionalParam2                  = VCpu->Regs->rdx;
        ContextParams.OptionalParam3                  = 0x8;

        TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);
        break;
    }
    case SystemFirmwareTableInformation:
    {

        ContextParams.OptionalParam1                  = SystemFirmwareTableInformation;
        ContextParams.OptionalParam2                  = VCpu->Regs->rdx;
        ContextParams.OptionalParam3                  = VCpu->Regs->r8;

        TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);
        break;
    }
    default:
    {
        return;
    }
    }
}

/**
 * @brief Obtain a copy of the PWCHAR wide character string from a OBJECT_ATTRIBUTES structure at a guest virtual address
 * 
 * @details Returns an allocated tagged memory pointer which needs to be freed with PlatformMemFreePool()
 *
 * @param virtPtr A pointer to a guest virutal memory address, containing a OBJECT_ATTRIBUTES structure
 * @return PVOID Pointer to an allocated tagged memory pool, which needs to be freed with PlatformMemFreePool()
 */
PVOID
TransparentGetObjectNameFromAttributesVirtualPointer(UINT64 virtPtr) 
{
    //PVOID buf = PlatformMemAllocateZeroedNonPagedPool(sizeof(OBJECT_ATTRIBUTES));
    OBJECT_ATTRIBUTES Buf = { 0 };
    
    //
    // Read the OBJECT_ATTRIBUTES structure from the virtual address pointer
    //
    if (MemoryMapperReadMemorySafeOnTargetProcess(virtPtr, &Buf, sizeof(OBJECT_ATTRIBUTES))) {

        //PVOID Namebuf = PlatformMemAllocateZeroedNonPagedPool(sizeof(UNICODE_STRING));
        UNICODE_STRING NameBuf = { 0 };

        //
        // Read the UNICODE_STRING structure from a virtual address pointer, pointed to by OBJECT_ATTRIBUTES.ObjectName struct entry
        //
        if (!MemoryMapperReadMemorySafeOnTargetProcess((UINT64)Buf.ObjectName, &NameBuf, sizeof(UNICODE_STRING)))
        {
            LogInfo("BadRead");
            return NULL;
        }

        //
        // The OBJECT_ATTRIBUTES structure contains a PUNICODE_STRING pointer to a guest virtual address which contains this UNICODE_STRING
        // This in turn will contain another pointer, this time a PWCHAR, to another virtual address, which will contain the wide char string we need
        //
        PVOID ObjectNameBuf = PlatformMemAllocateZeroedNonPagedPool(NameBuf.Length + sizeof(WCHAR));

        if (ObjectNameBuf == NULL)
        {
            LogInfo("Error allocating ImageName memory buffer");

            return NULL;
        }

        //
        // Read the PWCHAR string from a virtual address pointer, pointed to by OBJECT_ATTRIBUTES.ObjectName.Buffer struct entry
        //
        if (!MemoryMapperReadMemorySafeOnTargetProcess((UINT64)NameBuf.Buffer, ObjectNameBuf, NameBuf.Length + sizeof(WCHAR)))
        {
            LogInfo("BadRead");
            PlatformMemFreePool(ObjectNameBuf);

            return NULL;
        }

        //
        // The caller is responsible for freeing the memory buffer, using PlatformMemFreePool()
        //
        return ObjectNameBuf;
    }
    return NULL;
}

/**
 * @brief Handle The NtQueryAttributesFile system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtQueryAttributesFileSyscall(VIRTUAL_MACHINE_STATE* VCpu)
{

    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1 = VCpu->Regs->rdx;

    //
    // Check if the pointer given as the 3rd argument to the system call with type POBJECT_ATTRIBUTES is valid
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->r10, sizeof(OBJECT_ATTRIBUTES))) 
    {
        //
        // From the POBJECT_ATTRIBUTES structure obtain the wide character string of the requested file path
        //   
        PVOID PathBuf = TransparentGetObjectNameFromAttributesVirtualPointer(VCpu->Regs->r10);
        PWCH FilePath = (PWCH)PathBuf;
                
        //
        // If the file Attributes request is for a listed file, insert the SYSCALL trap flag and continue execution
        //
        for (UINT16 j = 0; j < (sizeof(HV_FILES) / sizeof(HV_FILES[0])); j++)
        {

            if (wcsstr(FilePath, HV_FILES[j]))
            {
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                                    HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                    VCpu->Regs->rax,
                                                    &ContextParams);

                break;
            }
        }

        //
        // Free the allocated copy of the directory path, obtained from TransparentGetObjectNameFromAttributesVirtualPointer()
        // 
        PlatformMemFreePool(PathBuf);
            
    }   
}

/**
 * @brief Handle The NtOpenDirectoryObject system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtOpenDirectoryObjectSyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Set up the context data for the callback after SYSRET
    //
    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1 = VCpu->Regs->r10;

    //
    // Check if the pointer given as the 3rd argument to the system call with type POBJECT_ATTRIBUTES is valid
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->r8, sizeof(OBJECT_ATTRIBUTES))) 
    {

        //
        // From the POBJECT_ATTRIBUTES structure obtain the wide character string of the requested directory path
        //
        PVOID PathBuf = TransparentGetObjectNameFromAttributesVirtualPointer(VCpu->Regs->r8);
        PWCH DirPath = (PWCH)PathBuf;

        if (DirPath == NULL)
        {
            return;
        }

        //
        // If the directory object request is for a listed directory, insert the SYSCALL trap flag and continue execution
        //
        for (UINT16 j = 0; j < (sizeof(HV_DIRS) / sizeof(HV_DIRS[0])); j++)
        {
            if (wcsstr(DirPath, HV_DIRS[j]))
            {
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                    HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                    VCpu->Regs->rax,
                                    &ContextParams);

                break;
            }
        }

        //
        // Free the allocated copy of the directory path, obtained from TransparentGetObjectNameFromAttributesVirtualPointer()
        // 
        PlatformMemFreePool(PathBuf);
    }   
}

/**
 * @brief Handle The NtSystemDebugControl system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtSystemDebugControlSyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    VCpu->Regs->rdx = 0x0;
    VCpu->Regs->r9 = 0x0;

    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};

    TransparentSetTrapFlagAfterSyscall(VCpu,
                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                           VCpu->Regs->rax,
                                           &ContextParams);
}

/**
 * @brief Handle The NtQueryInformationProcess system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtQueryInformationProcessSyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Set up the context parameters for the interception callback
    //
    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1 = VCpu->Regs->rdx; // ProcessInformationClass
    ContextParams.OptionalParam2 = VCpu->Regs->r8;  // BufferPtr
    ContextParams.OptionalParam2 = VCpu->Regs->r9;  // BufferSize

    //
    // Set the trap flag to intercept the SYSRET instruction
    //
    TransparentSetTrapFlagAfterSyscall(VCpu,
                                       HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                       VCpu->Regs->rax,
                                       &ContextParams);
}

/**
 * @brief Handle The NtOpenFile system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtOpenFileSyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Check if the user-mode pointer in R8 to a OBJECT_ATTRIBUTES struct is valid
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->r8, sizeof(OBJECT_ATTRIBUTES))) 
    {
        //
        // From the OBJECT_ATTRIBUTES struct pointer extract the file path for which this syscall is called
        //
        PVOID NameBuf = TransparentGetObjectNameFromAttributesVirtualPointer(VCpu->Regs->r8);
        PWCH FileName = (PWCH)NameBuf;
        if (FileName == NULL)
        {
            return;
        }

        //
        // Check if the requested file includes any hypervisor specific strings
        // This also checks parent directory names of the requested file
        //
        for (UINT16 j = 0; j < (sizeof(HV_FILES) / sizeof(HV_FILES[0])); j++)
        {
            if (wcsstr(FileName, HV_FILES[j]))
            {
                LogInfo("A call to NtOpenFile systemcall for a hypervisor specific file was made");

                //
                //If a match was found, corrupt the user-mode pointers in CPU registers, so that, when the kernel-mode execution continues, it would fail.
                //
                VCpu->Regs->r8 = 0x0;
                VCpu->Regs->r10 = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                       HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                       VCpu->Regs->rax,
                                       &ContextParams);

                break;
            }
        }
        //
        //Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }   
}

/**
 * @brief Handle The NtOpenKey system call 
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtOpenKeySyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Check if the user-mode pointer in R8 to a OBJECT_ATTRIBUTES struct is valid
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->r8, sizeof(OBJECT_ATTRIBUTES))) 
    {

        //
        // From the OBJECT_ATTRIBUTES struct pointer extract the registry key path for which this syscall is called
        //
        PVOID NameBuf = TransparentGetObjectNameFromAttributesVirtualPointer(VCpu->Regs->r8);
        PWCH KeyName = (PWCH)NameBuf;

        if (KeyName == NULL)
        {
            LogInfo("BADRET");
            return;
        }

        //
        // Check if the requested registry entry path includes any hypervisor specific strings
        //
        for (UINT16 j = 0; j < (sizeof(HV_REGKEYS) / sizeof(HV_REGKEYS[0])); j++)
        {
            if (wcsstr(KeyName, HV_REGKEYS[j]) > 0)
            {
                //
                // If a match was found, corrupt the user-mode pointer in CPU registers, so that, when the kernel-mode execution continues, it would fail.
                //
                VCpu->Regs->r8 = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = { 0 };
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                                    HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                    VCpu->Regs->rax,
                                                    &ContextParams);

                break;
            }
        }

        //
        //Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }
}

/**
 * @brief Handle The NtQueryValueKey system call
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtQueryValueKeySyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Check if the user-mode pointer in RDX to a UNICODE_STRING struct is valid
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->rdx, sizeof(UNICODE_STRING)))
    {

        UNICODE_STRING NameUString = { 0 };


        //
        // Read the UNICODE_STRING structure from a virtual address pointer, pointed to by RDX struct entry
        //
        if (!MemoryMapperReadMemorySafeOnTargetProcess(VCpu->Regs->rdx, &NameUString, sizeof(UNICODE_STRING))) return;


        //
        // Read the PWCH wide char string from the address pointer in the UNICODE_STRING
        //
        PVOID NameBuf = PlatformMemAllocateZeroedNonPagedPool(NameUString.Length + sizeof(WCHAR));
        if (NameBuf == NULL)
        {
            return;
        }

        if (!MemoryMapperReadMemorySafeOnTargetProcess((UINT64)NameUString.Buffer, NameBuf, NameUString.Length + sizeof(WCHAR)))
        {
            PlatformMemFreePool(NameBuf);
            return;
        }

        PWCH KeyName = (PWCH)NameBuf;



        //
        // If the registry key request was for kay that could contain hypervisor specific information in its data, 
        // the return buffer(%R9) needs to be modified, but the buffer length is in the user mode stack
        //
        for (ULONG i = 0; i < (sizeof(TRANSPARENT_DETECTABLE_REGISTRY_KEYS) / sizeof(TRANSPARENT_DETECTABLE_REGISTRY_KEYS[0])); i++)
        {
            if (!wcscmp(KeyName, TRANSPARENT_DETECTABLE_REGISTRY_KEYS[i]))
            {
                //
                // If a match is found, set up the context values and set the trap flag for the SYSRET callback
                //

                //LogInfo("Regs: R10: %llx, RDX: %llx, R8: %llx, R9: %llx", VCpu->Regs->r10, VCpu->Regs->rdx, VCpu->Regs->r8, VCpu->Regs->r9);

                TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};

                ContextParams.OptionalParam1 = VCpu->Regs->r8;
                ContextParams.OptionalParam2 = VCpu->Regs->r9;

                //
                // Read the 5th argument of the system call from the stack at location %RSP + 0x28
                //
                if (CheckAccessValidityAndSafety(VCpu->Regs->rsp + 0x28, sizeof(UINT64)))
                {
                    MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(VCpu->Regs->rsp + 0x28), &ContextParams.OptionalParam3, sizeof(ULONG));
                }
                else
                {
                    LogInfo("Process 0x%llx on thread %llx executed NtQueryValueKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                                                                                     HANDLE_TO_UINT32(PsGetCurrentThreadId()));
                    
                    PlatformMemFreePool(NameBuf);
                    return;
                }

                //
                // Read the 6th argument of the system call from the stack at location %RSP + 0x30
                //
                if (CheckAccessValidityAndSafety(VCpu->Regs->rsp + 0x30, sizeof(UINT64)))
                {
                    MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(VCpu->Regs->rsp + 0x30), &ContextParams.OptionalParam4, sizeof(UINT64));
                }
                else
                {
                    LogInfo("Process 0x%llx on thread %llx executed NtQueryValueKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                                                                                     HANDLE_TO_UINT32(PsGetCurrentThreadId()));
                    
                    PlatformMemFreePool(NameBuf);
                    return;
                }

                
                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                                    HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                    VCpu->Regs->rax,
                                                    &ContextParams);

                //
                // Clean-up and return to guest exection
                //
                PlatformMemFreePool(NameBuf);
                return;
            }
        }

        //
        // If the call was for a registry key that contains a hypervisor specific string,
        // The user-mode caller should just receive an error return code not a modified data buffer
        //
        for (UINT16 j = 1; j < (sizeof(HV_REGKEYS) / sizeof(HV_REGKEYS[0])); j++)
        {
            if (wcsstr(KeyName, HV_REGKEYS[j]) > 0)
            {
                //
                // When the match is found, corrupt the buffer pointers in the registers
                // and set the SYSRET callback trap flag
                //
                TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = {0};

                VCpu->Regs->rdx = 0x0;
                VCpu->Regs->r9 = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                TransparentSetTrapFlagAfterSyscall(VCpu,
                                                    HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                    VCpu->Regs->rax,
                                                    &ContextParams);
                
                break;
            }
        }

        //
        //Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }  
}

/**
 * @brief Handle The NtEnumerateKey system call
 * when the Transparent mode is enabled
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
TransparentHandleNtEnumerateKeySyscall(VIRTUAL_MACHINE_STATE* VCpu)
{
    //
    // Set up the context parameters for the interception callback
    //
    TRANSPARENT_MODE_CONTEXT_PARAMS ContextParams = { 0 };
    ContextParams.OptionalParam1 = VCpu->Regs->r8;
    ContextParams.OptionalParam2 = VCpu->Regs->r9;

    //
    // Read the 5th argument of the system call from the stack at location %RSP + 0x28
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->rsp + 0x28, sizeof(UINT64)))
    {
        MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(VCpu->Regs->rsp + 0x28), &ContextParams.OptionalParam3, sizeof(ULONG));
    }
    else
    {
        LogInfo("Process 0x%llx on thread %llx executed NtEnumerateKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                                                                        HANDLE_TO_UINT32(PsGetCurrentThreadId()));
        return;
    }

    //
    // Read the 6th argument of the system call from the stack at location %RSP + 0x30
    //
    if (CheckAccessValidityAndSafety(VCpu->Regs->rsp + 0x30, sizeof(UINT64)))
    {
        MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(VCpu->Regs->rsp + 0x30), &ContextParams.OptionalParam4, sizeof(UINT64));
    }
    else
    {
        LogInfo("Process 0x%llx on thread %llx executed NtEnumerateKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                                                                        HANDLE_TO_UINT32(PsGetCurrentThreadId()));
        return;
    }

    //
    // If the call was made without an allocated buffer (with size 0)
    // we have no need to intercept it
    //
    if (ContextParams.OptionalParam3 == 0)
    {
        return;
    }

    //
    // Set the trap flag to intercept the SYSRET instruction
    //
    TransparentSetTrapFlagAfterSyscall(VCpu,
                                        HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                        HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                        VCpu->Regs->rax,
                                        &ContextParams);
}

/**
 * @brief This function makes sure to unset the RFLAGS.TF on next trigger of #DB
 * on the target process/thread
 * @param ProcessId
 * @param ThreadId
 * @param Context
 * @param Params
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentStoreProcessInformation(UINT32                            ProcessId,
                                   UINT32                            ThreadId,
                                   UINT64                            Context,
                                   TRANSPARENT_MODE_CONTEXT_PARAMS * Params)
{
    UINT32                                      Index;
    BOOLEAN                                     Result;
    BOOLEAN                                     SuccessfullyStored;
    TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&TransparentModeTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_TransparentModeTrapFlagState->ThreadInformation[0],
                                           g_TransparentModeTrapFlagState->NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    if (Result)
    {
        //
        // It means that we already find this entry in the stored list
        // so, just imply that the addition was successful (no need for extra addition)
        //
        SuccessfullyStored = TRUE;
        goto Return;
    }
    else
    {
        //
        // Insert the thread into the list as the item is not already present
        //
        SuccessfullyStored = InsertionSortInsertItem((UINT64 *)&g_TransparentModeTrapFlagState->ThreadInformation[0],
                                                     &g_TransparentModeTrapFlagState->NumberOfItems,
                                                     MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS,
                                                     &Index,
                                                     ProcThrdInfo.asUInt);

        if (SuccessfullyStored)
        {
            //
            // Successfully inserted the thread/process into the list
            // Now let's store the context of the caller along with parameters
            //
            g_TransparentModeTrapFlagState->Context[Index] = Context;
            memcpy(&g_TransparentModeTrapFlagState->Params[Index], Params, sizeof(TRANSPARENT_MODE_CONTEXT_PARAMS));
        }

        goto Return;
    }

Return:
    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&TransparentModeTrapListLock);

    return SuccessfullyStored;
}

/**
 * @brief Set the trap flag in the guest after a syscall
 *
 * @param VCpu The virtual processor's state
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 * @param Context The context of the caller
 * @param Params The (optional) parameters of the caller
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentSetTrapFlagAfterSyscall(VIRTUAL_MACHINE_STATE *           VCpu,
                                   UINT32                            ProcessId,
                                   UINT32                            ThreadId,
                                   UINT64                            Context,
                                   TRANSPARENT_MODE_CONTEXT_PARAMS * Params)
{
    //
    // Do not add anything to the list if the transparent-mode is not enabled (or disabled by the user)
    //
    if (!g_TransparentMode)
    {
        //
        // Transparent-mode is not enabled
        //
        return FALSE;
    }

    //
    // Insert the thread/process into the list of processes/threads
    //
    if (!TransparentStoreProcessInformation(ProcessId, ThreadId, Context, Params))
    {
        //
        // Failed to store the process/thread information
        //
        return FALSE;
    }

    //
    // *** Successfully stored the process/thread information ***
    //

    //
    // Set the trap flag to TRUE because we want to intercept the thread again
    // once it returns to the user-mode (SYSRET) instruction
    //
    // Here the RFLAGS is in the R11 register (See Intel manual about the SYSCALL register)
    //
    VCpu->Regs->r11 |= X86_FLAGS_TF;

    //
    // Create log message for the syscall
    //
    // LogInfo("Transparent set trap flag for process: %x, thread: %x\n", ProcessId, ThreadId);

    return TRUE;
}

/**
 * @brief Handle the trap flags as the result of interception of the return of the
 * system-call
 *
 * @param VCpu The virtual processor's state
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentCheckAndHandleAfterSyscallTrapFlags(VIRTUAL_MACHINE_STATE * VCpu,
                                               UINT32                  ProcessId,
                                               UINT32                  ThreadId)
{
    RFLAGS                                      Rflags = {0};
    UINT32                                      Index;
    UINT64                                      Context = NULL64_ZERO;
    TRANSPARENT_MODE_CONTEXT_PARAMS             Params;
    TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};
    BOOLEAN                                     Result;
    BOOLEAN                                     ResultToReturn;

    //
    // Read the trap flag
    //
    Rflags.AsUInt = HvGetRflags();

    if (!Rflags.TrapFlag)
    {
        //
        // The trap flag is not set, so we don't need to do anything
        //
        return FALSE;
    }

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&TransparentModeTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_TransparentModeTrapFlagState->ThreadInformation[0],
                                           g_TransparentModeTrapFlagState->NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    //
    // Check whether this thread is expected to have trap flag
    // by the transparent-mode or not
    //
    if (Result)
    {
        //
        // Read the context of the caller
        //
        Context = g_TransparentModeTrapFlagState->Context[Index];

        //
        // Read the (optional) parameters of the caller
        //
        memcpy(&Params, &g_TransparentModeTrapFlagState->Params[Index], sizeof(TRANSPARENT_MODE_CONTEXT_PARAMS));

        //
        // Clear the trap flag from the RFLAGS register
        //
        HvSetRflagTrapFlag(FALSE);

        //
        // Remove the thread/process from the list of processes/threads
        //
        InsertionSortDeleteItem((UINT64 *)&g_TransparentModeTrapFlagState->ThreadInformation[0],
                                &g_TransparentModeTrapFlagState->NumberOfItems,
                                Index);

        //
        // Handled by the transparent-mode
        //
        ResultToReturn = TRUE;

        goto ReturnResult;
    }
    else
    {
        //
        // Not related to the transparent-mode
        //
        ResultToReturn = FALSE;

        goto ReturnResult;
    }

ReturnResult:

    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&TransparentModeTrapListLock);

    //
    // Call the callback function to handle the trap flag if its needed
    // Note that we call it here so we already unlocked the list lock
    // to optimize the performance (avoid holding the lock for a long time)
    //
    if (ResultToReturn)
    {
        TransparentCallbackHandleAfterSyscall(VCpu, ProcessId, ThreadId, Context, &Params);
    }

    return ResultToReturn;
}

/**
 * @brief Handle the request for SystemModuleInformation
 * 
 * @details This function removes entries from a list of system drivers that could reveal the presence of hypervisors
 *          This depends on an incomplete list HV_DRIVER, of known hypervisor drivers
 *          The revealing list entries are removed and overwritten, but the memory buffer is not reallocated, so
 *          it is possible to still detect that some tampering was done from the user space
 *
 * @param ptr The pointer to a valid read/writable SYSTEM_MODULE_INFORMATION memory buffer 
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHandleModuleInformationQuery(PVOID ptr, UINT64 virtualAddress, UINT32 bufferSize)
{
    PSYSTEM_MODULE_INFORMATION StructBuf = (PSYSTEM_MODULE_INFORMATION)ptr;
    PSYSTEM_MODULE_ENTRY ModuleList = StructBuf->Module;

    //
    // Traverse the list of system modules and remove the system drivers
    // matching a known list of hypervisor drivers based on their filename
    //
    for (UINT16 i = 0; i < StructBuf->Count; i++)
    {
        PCHAR path = (PCHAR)ModuleList[i].FullPathName;

        for (UINT16 j = 0; j < (sizeof(HV_DRIVER) / sizeof(HV_DRIVER[0])); j++)
        {
            if (strstr(path, HV_DRIVER[j]))
            {
                //
                // If a module file name matches, remove the entry from the list by shifting it forward by one entry
                //
                for (UINT16 k = i; k < StructBuf->Count - 1; k++)
                {
                    ModuleList[k] = ModuleList[k + 1];
                }

                //
                // Decrement the list size as one entry has been removed
                //
                i--;
                StructBuf->Count--;

                break;
            }
        }

    }
    if(!MemoryMapperWriteMemorySafeOnTargetProcess(virtualAddress, ptr, bufferSize))
    { 
        return FALSE;
    }
    return TRUE;
}


/**
 * @brief Handle the request for SystemProcessInformation
 * 
 * @details This function removes entries from a list of active system processes that could reveal the presence of hypervisors
 *          Currently, the revealing system processes only have their Image Name renamed and some detectable trails are still left
 *
 * @param ptr The pointer to a valid read/writable SYSTEM_PROCESS_INFORMATION memory buffer 
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHandleProcessInformationQuery(PVOID ptr) 
{

    PSYSTEM_PROCESS_INFORMATION StructBuf = (PSYSTEM_PROCESS_INFORMATION)ptr;

    //
    // Loop through all the entries and filter out the offending ones
    //
    do 
    {
        if(StructBuf->ImageName.Length != 0)
        {

            //
            // We need to modify the Image name of the process which requires extra allocation
            //
            PVOID StringBuf = PlatformMemAllocateZeroedNonPagedPool(StructBuf->ImageName.Length + sizeof(WCHAR));

            if (StringBuf == NULL)
            {
                LogInfo("Error allocating ImageName memory buffer");

                return FALSE;
            }

            if (!MemoryMapperReadMemorySafeOnTargetProcess((UINT64)StructBuf->ImageName.Buffer, StringBuf, StructBuf->ImageName.Length + sizeof(WCHAR)))
            {
                PlatformMemFreePool(StringBuf);

                return FALSE;
            }
            
            PWCH ImageName = (PWCH)StringBuf;

            if (ImageName == NULL)
            {
                PlatformMemFreePool(StringBuf);

                return FALSE;
            }

            //
            // Loop through the known list of identifiable hypervisor related processes
            //
            for (UINT16 i = 0; i < (sizeof(HV_Processes) / sizeof(HV_Processes[0])); i++) 
            {
                if (!_wcsnicmp(ImageName, HV_Processes[i], (StructBuf->ImageName.Length) / 2))
                {
                    //
                    // If the name matches, randomize it
                    //
                    for (UINT16 j = 0; j < ((StructBuf->ImageName.Length / sizeof(WCHAR)) - 4); j++)
                    {
                        UINT32 r = (TransparentGetRand() % 26) + 97;
                        if (ImageName[j] && r)
                        {
                            ImageName[j] = (WCHAR)r;
                        }
                        
                    }

                    break;
                }
            }

            //
            // Write the modified name back to the usermode buffer
            //
            MemoryMapperWriteMemorySafeOnTargetProcess((UINT64)StructBuf->ImageName.Buffer, StringBuf, StructBuf->ImageName.Length + sizeof(WCHAR));

            PlatformMemFreePool(StringBuf);
        }

        //
        // Move to the next process entry
        //
        StructBuf = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)StructBuf + StructBuf->NextEntryOffset);

        //
        // Some internal Windows calls to this system call use different offsetting/entry structure layout and causes errors 
        //
        if (!CheckAccessValidityAndSafety((UINT64)StructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
        {
            return FALSE;
        }
    } while (StructBuf->NextEntryOffset != 0);

    return TRUE;
}

/**
 * @brief Handle the request for SystemFirmwareTableInformation
 * 
 * @param ptr The pointer to a valid read/writable SYSTEM_FIRMWARE_TABLE_INFORMATION memory buffer 
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHandleFirmwareInformationQuery(UINT64 ptr, UINT32 size)
{
    //
    // From the user-mode pointer, read the SYSTEM_FIRMWARE_TABLE_INFORMATION struct
    //
    PVOID buf = PlatformMemAllocateZeroedNonPagedPool(size + 1);
    if (buf == NULL) {
        return FALSE;
    }
 
    if (!MemoryMapperReadMemorySafeOnTargetProcess(ptr, buf, size))
    {
        PlatformMemFreePool(buf);
        return FALSE;
    }

    PSYSTEM_FIRMWARE_TABLE_INFORMATION temp = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)buf;

    //
    // The request needs to be a "get" request for an existing table
    // with 'RSMB', 'ACPI' or 'FIRM' table providers
    //
    if (temp->Action == SystemFirmwareTable_Get &&
        temp->TableID != 0                      &&
        (temp->ProviderSignature == 0x52534D42 ||
        temp->ProviderSignature == 0x41435049  ||
        temp->ProviderSignature == 0x4649524D))
    {
        for (ULONG i = 0; i < (sizeof(HV_FIRM_NAMES) / sizeof(HV_FIRM_NAMES[0])); i++)
        {
            ULONGLONG nameLen = strlen(HV_FIRM_NAMES[i]);
            if (temp->TableBufferLength < nameLen) continue;
            
            //
            // Walk the buffer to find the matching firmware strings
            //
            for (ULONG pos = 0; pos < (temp->TableBufferLength - nameLen); pos++)
            {

                if (!memcmp((PCHAR)(temp->TableBuffer + pos), HV_FIRM_NAMES[i], nameLen)) {
                    LogInfo("Query for system firmware was called, no mitigation was performed");
                    LogInfo("Query matched a known hypervisor: %s", HV_FIRM_NAMES[i]);

                    // 
                    // A temporary solution that just changes a single character in the firmware string to a random value
                    // This bypasses current firmware data checks, but can easily be detected anyway
                    //
                    UINT32 r = (TransparentGetRand() % 26) + 97;
                    (temp->TableBuffer + pos)[pos % nameLen] = (CHAR)r;

                    
                }                         
            }

        }

        //
        // Write the modified struct back to the user-mode buffer pointer
        //
        if (!MemoryMapperWriteMemorySafeOnTargetProcess(ptr, buf, size))
        {
            PlatformMemFreePool(buf);
            return FALSE;
        }
    }
    PlatformMemFreePool(buf);
    return TRUE;
}

/**
 * @brief Replace occurances of a hypervisor specific strings with legitimate vendor strings in a provided buffer
 * 
 * @param Params        Set transparent callback params that contain:
                        in OptionalParam2 a pointer to a valid read/writable memory buffer that contains both, a WCHAR string and its length in bytes
                        in OptionalParam3 max size in bytes of the allocatec buffer
                        in OptionalParam4 a pointer to a ULONG containing current size of the buffer
 *
 * @param DataOffset    Offset in bytes from OptionalParam2 to the start of the WCHAR data string
 * 
 * @param DataLenOffset Offset in bytes from OptionalParam2 to a ULONG containing the string length(in bytes)
 * 
 * @return UINT64
 */
UINT64
TransparentReplaceVendorStringFromBufferWChar(TRANSPARENT_MODE_CONTEXT_PARAMS* Params, ULONG DataOffset, ULONG DataLenOffset)
{
    PVOID Buf = NULL;
    BOOL PoolAlloc = FALSE;

    //
    // Check that the user provided pointers are safe to read from
    //
    if (CheckAccessValidityAndSafety(Params->OptionalParam4, sizeof(ULONG)))
    {
        //
        // Read the size of the data that the kernel wrote to the buffer
        //
        ULONG BufSize = 0;
        if (!MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG)))
        {
            goto ReturnWithError;
        }

        //
        // If the data the kernel wanted to write is bigger than what was allocated by the user
        //
        if (BufSize > Params->OptionalParam3 || BufSize == 0)
        {
            //
            // NOTE: might need zeroing the memory if the kernel did infact write to the pointer
            //

            return 0;
        }

        //
        // Check that the user provided pointers are safe to read from
        //
        if (!CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            goto ReturnWithError;
        }

        //
        // If the buffer is small, e.g. for just a single word, store it on the stack
        // else, allocate a nonpaged memory buffer
        //
        CHAR StackBuf[MAX_PATH] = { 0 };

        if (Params->OptionalParam3 + sizeof(WCHAR) > MAX_PATH)
        {
            Buf = PlatformMemAllocateZeroedNonPagedPool(Params->OptionalParam3 + sizeof(WCHAR));
            PoolAlloc = TRUE;
        }
        else
        {
            Buf = &StackBuf;
        }
     
        if (!Buf || !MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3))
        {
            goto ReturnWithError;
        }
        
        //
        // Get the actual data we are trying to modify(in wide char form)
        //
        PWCH StringBuf = (PWCH)((PBYTE)Buf + DataOffset);

        //
        // Traverse the list of registry key names and vendor strings that are specific to common hypervisors
        // if a match is found perform the modification
        //
        for (UINT16 i = 0; i < (sizeof(HV_REGKEYS) / sizeof(HV_REGKEYS[0])); i++)
        {
            PWCH MatchStart = wcsstr(StringBuf, HV_REGKEYS[i]);
            if (MatchStart != 0)
            {
                PWCH NewVendorString = NULL;

                //
                // If the match was for a device id, the replacement should be with a different ID string not vendor name
                //
                if (i < 3)
                {
                    //
                    // SPOOFS PCI device ID's(in the registry), This might be implemented in other ways that are not part of this implementation
                    //
                    
                    NewVendorString = TRANSPARENT_LEGIT_DEVICE_ID_VENDOR_STRINGS_WCHAR[1];
                }
                else
                {
                    //
                    // Obtain the replacement vendor name string, randomized when the transparency mode was enabled
                    //
                    NewVendorString = TRANSPARENT_LEGIT_VENDOR_STRINGS_WCHAR[1]; 
                }

                //
                // Obtain the lengths of all the strings and substring
                //
                ULONG tempSize = (ULONG)wcslen(NewVendorString) * sizeof(WCHAR);

                ULONG MatchedStringLen = (ULONG)wcslen(HV_REGKEYS[i]) * sizeof(WCHAR);
                ULONG oldLength = *((PBYTE)Buf + DataLenOffset);

                ULONG NewStringSize = oldLength - MatchedStringLen + tempSize;

                //
                // Check if the buffer size allows the modification, in case of expansion
                //
                if (BufSize - MatchedStringLen + tempSize > Params->OptionalParam3)
                {
                    //
                    // If adding the new string exceeds the user allocated size,
                    // zero out the buffer
                    //
                    memset(Buf, 0x0, Params->OptionalParam3);
                    MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3);

                    //
                    // Update the required buffer size for the next call
                    //
                    BufSize = (tempSize - MatchedStringLen) + oldLength;
                    MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG));
                    
                    //
                    // And return STATUS_BUFFER_OVERFLOW error
                    //

                    if(PoolAlloc) PlatformMemFreePool(Buf);
                    return (UINT64)(UINT32)STATUS_BUFFER_OVERFLOW;
                }

                //
                // Calculate the positions of the replacement
                //
                ULONG MatchOffset = (ULONG)((MatchStart - StringBuf));
                PWCH MatchEnd = StringBuf + MatchOffset + (MatchedStringLen / sizeof(WCHAR));

                //
                // Move the data after the matched string forward
                // and replace the identified hypervisor string with the genuine one
                //
                memmove((PVOID)(StringBuf + MatchOffset + (tempSize / sizeof(WCHAR))), (PVOID)MatchEnd, oldLength - MatchedStringLen - (MatchOffset * sizeof(WCHAR)));
                memcpy((PVOID)MatchStart, (PVOID)NewVendorString, tempSize);
                        
                *(PULONG)((PBYTE)Buf + DataLenOffset) = NewStringSize;
                BufSize = BufSize - MatchedStringLen + tempSize;

                //
                // Write the changes back to the user buffers
                //
                if (!MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, BufSize)) 
                {
                    goto ReturnWithError;
                }

                if (!MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG)))
                {
                    goto ReturnWithError;
                }

                //
                // Cleanup
                //
                if(PoolAlloc) PlatformMemFreePool(Buf);

                return 0;
            }
   
        }

        //
        // The data buffer contained no detectable strings
        //
        if(PoolAlloc) PlatformMemFreePool(Buf);
        return 0;
    }

//
// An error occured while performing the mitigations, the user buffer might be left unmodified
//
ReturnWithError:
    LogInfo("A call for to read a registry entry, which could contain hypervisor specific data, was intercepted but the mitigations failed");
    LogInfo("The caller process recieved the results in this virtual address: %llx", Params->OptionalParam2);

    if (Buf != NULL) {
        if(PoolAlloc) PlatformMemFreePool(Buf);
    }

    return 0;
}

/**
 * @brief Callback function to handle the returns from the NtQueryValueKey syscall
 *
 * @param Params        The set transparent callback params that contain:
                        in OptionalParam1 the KEY_VALUE_INFORMATION_CLASS enum value 
                        in OptionalParam2 a pointer to a valid read/writable memory buffer that contains both, a WCHAR string and its length in bytes
                        in OptionalParam3 max size in bytes of the allocatec buffer
                        in OptionalParam4 a pointer to a ULONG containing current size of the buffer
 *
 * @return UINT64 
 */
UINT64
TransparentCallbackHandleAfterNtQueryValueKeySyscall(TRANSPARENT_MODE_CONTEXT_PARAMS* Params)
{
    ULONG LenOffset = 0;
    ULONG BufOffset = 0;

    //
    // Based on the KEY_VALUE_INFORMATION_CLASS given, set the struct offsets for the data buffer and its length fields
    //
    switch (Params->OptionalParam1)
    {

    //
    // KeyValueBasicInformation and queries for a key that has a hypervisor specific name
    //
    case 0x0:
    {
        
        if (Params->OptionalParam2 != 0) return 0;

        //
        // If the key query was for a registry key that has a hypervisor specific name, return an error code
        //
        return (UINT64)(UINT32)STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // KeyValuePartialInformation
    //
    case 0x2:
    {
        LenOffset = sizeof(ULONG) * 2;
        BufOffset = sizeof(ULONG) * 3;

        break;
    }

    //
    // KeyValueFullInformation
    //
    case 0x3:
    case 0x1:
    {
        LenOffset = sizeof(ULONG) * 3;
        BufOffset = sizeof(ULONG) * 4; //Name offset, Data is after it

        break;
    }

    //
    // KeyValuePartialInformationAlign64
    //
    case 0x4:
    {
        LenOffset = sizeof(ULONG) * 1;
        BufOffset = sizeof(ULONG) * 2;

        break;
    }   
    default:
    {
        LogInfo("NtQueryValueKey was called with KeyValueInformationClass 0x%x, a handler for which has not been implemented", Params->OptionalParam1);
        return 0;
    }
    }

    //
    // Given the user buffer, read and exchange any Hypervisor vendor strings in the registry key data
    //
    return TransparentReplaceVendorStringFromBufferWChar(Params, BufOffset, LenOffset);

}

/**
 * @brief Callback function to handle the returns from the NtQueryValueKey syscall
 *
 * @param Params        The set transparent callback params that contain:
                        in OptionalParam1 the KEY_VALUE_INFORMATION_CLASS enum value 
                        in OptionalParam2 a pointer to a valid read/writable memory buffer that contains both, a WCHAR string and its length in bytes
                        in OptionalParam3 max size in bytes of the allocatec buffer
                        in OptionalParam4 a pointer to a ULONG containing current size of the buffer
 *
 * @return UINT64 
 */
UINT64
TransparentCallbackHandleAfterNtEnumerateKeySyscall(TRANSPARENT_MODE_CONTEXT_PARAMS* Params)
{
    ULONG LenOffset = 0;
    ULONG BufOffset = 0;

    //
    // Based on the KEY_INFORMATION_CLASS given, set the struct offsets for the data buffer and its length fields
    //
    switch (Params->OptionalParam1)
    {

    //
    // KeyBasicInformation
    //
    case 0x0:
    {

        LenOffset = sizeof(LARGE_INTEGER) + sizeof(ULONG);
        BufOffset = sizeof(LARGE_INTEGER) + (sizeof(ULONG) * 2);

        break;
    }

    //
    // KeyNodeInformation
    //
    case 0x1:
    {
        return 0;
    }

    //
    // KeyNameInformation
    //
    case 0x3:
    {
        LenOffset = 0;
        BufOffset = sizeof(ULONG);

        break;
    }   
    default:
    {
        LogInfo("NtEnumerateKey was called with KeyInformationClass 0x%x, a handler for which has not been implemented", Params->OptionalParam1);
        return 0;
    }
    }

    //
    // Given the user buffer, read and exchange any Hypervisor vendor strings in the registry key names
    //
    return TransparentReplaceVendorStringFromBufferWChar(Params, BufOffset, LenOffset);

}

/**
 * @brief Callback function to handle the returns from the NtQuerySystemInformation syscall
 *
 * @param VCpu The virtual processor's state
 * @param Params The (optional) parameters of the caller
 *
 * @return VOID
 */
VOID
TransparentCallbackHandleAfterNtQuerySystemInformationSyscall(TRANSPARENT_MODE_CONTEXT_PARAMS * Params)
{
    //
    // Handle each defined SYSTEM_INFORMATION_CLASS
    //
    switch (Params->OptionalParam1)
    {
    case SystemCodeIntegrityInformation:
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3)) {

            SYSTEM_CODEINTEGRITY_INFORMATION Temp = { 0 };

            //
            // Read data from the saved pointer of the now filled information buffer
            //
            MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, Params->OptionalParam3);

            //
            // Modify the data and write it back to the information buffer to be passed to user mode
            //
            Temp.CodeIntegrityOptions = 0x01;
            MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, Params->OptionalParam3);

        }
        else
        {
            LogInfo("A call for the NtQuerySystemInformation system call requesting SystemCodeIntegrityInformation structure was made, but the usermode buffer was not captured");
        }

        break;
    }
    case SystemProcessInformation:
    case SystemModuleInformation:
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (Params->OptionalParam2 != 0x0 &&
            Params->OptionalParam3 != 0x0 &&
            CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3)) {

            //
            // Allocate a buffer to copy user buffer data to for modification
            //
            PVOID Buf = PlatformMemAllocateZeroedNonPagedPool(Params->OptionalParam3);
            if (Buf == NULL)
            {
                LogError("Err, insufficient memory");
                break;
            }

            //
            // Copy over the data and perform the modifications
            //
            if (!MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3)) {
                LogInfo("Error reading memory buffer given by the usermode call");
            }
            else
            {
                if ((Params->OptionalParam1 == SystemProcessInformation && !TransparentHandleProcessInformationQuery(Buf)) ||
                    (Params->OptionalParam1 == SystemModuleInformation && !TransparentHandleModuleInformationQuery(Buf, Params->OptionalParam2, (UINT32)Params->OptionalParam3)))
                {
                    //
                    // Some internal Windows calls to these system calls use different offsetting/entry structure layout and causes errors 
                    // 

                    // LogInfo("Error while modifying the buffer for data query 0x02x", Params->OptionalParam1);
                }
            }

            PlatformMemFreePool(Buf);

        }
        break;
    }
    case SystemKernelDebuggerInformation:
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3)) {

            //
            // Write to the output buffer 0x0001 for "Debugger not present"
            //
            WORD Temp = 0x0100;
            MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, 2);
        }
    }
    case SystemFirmwareTableInformation:
    {
        if (CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3) &&
            !TransparentHandleFirmwareInformationQuery(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            LogInfo("A query for SystemFirmwareTableInformation was made, but the transparent mitigation failed");
        }
        break;
    }

    default:
    {
        break;
    }
    }
}

/**
 * @brief Callback function to handle returns from the syscall
 *
 * @param VCpu The virtual processor's state
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 * @param Context The context of the caller
 * @param Params The (optional) parameters of the caller
 *
 * @return VOID
 */
VOID
TransparentCallbackHandleAfterSyscall(VIRTUAL_MACHINE_STATE * VCpu,
                                      UINT32                  ProcessId,
                                      UINT32                  ThreadId,
                                      UINT64                  Context,
                                      TRANSPARENT_MODE_CONTEXT_PARAMS * Params)
{
    //
    // Handle each defined system call separately, after the kernel execution has finished(at the SYSRET instruction)
    //
    switch (Context)
    {

    //
    // Handle the memory buffer and return code modification after NtQuerySystemInformation system call
    //
    case SysNtQuerySystemInformation:
    {

        TransparentCallbackHandleAfterNtQuerySystemInformationSyscall(Params);

        break;
    }

    //
    // Handle the memory buffer and return code modification after NtQueryAttributesFile system call
    //
    case SysNtQueryAttributesFile:
    {

        //
        // Check if the obtained buffer pointer is valid
        //
        if (CheckAccessValidityAndSafety(Params->OptionalParam1, sizeof(FILE_BASIC_INFORMATION)))
        {

            FILE_BASIC_INFORMATION Buf = { 0 };
            //
            // Copy over the data from the output buffer pointer
            //
            if (!MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam1, &Buf, sizeof(FILE_BASIC_INFORMATION)))
            {
                LogError("Err, Virtual memory read failed");
                break;
            }

            //
            // Modify the file attribute to INVALID_FILE_ATTRIBUTES and write it back to the pointer
            //
            Buf.FileAttributes = ((DWORD)-1);

            if (!MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam1, &Buf, sizeof(FILE_BASIC_INFORMATION)))
            {
                LogError("Err, Virtual memory write failed");
            }

        }
        else {
            LogInfo("A call for the NtQueryAttributeFile system call for a marked file was made, but the output buffer was not captured");
        }
        break;
    }

    //
    // Handle the memory buffer and return code modification after NtOpenDirectoryObject system call.
    // 
    // NOTE: No transparent mitigations of this call have been implemented
    //
    case SysNtOpenDirectoryObject:
    {
        LogInfo("A NtOpenDirectoryObject system call was made for a known directory that reveals hypervisor presence. process: %x, thread: %x, rip: %llx \n",
            ProcessId,
            ThreadId,
            VCpu->LastVmexitRip);
        LogInfo("No action to mitigate this was made as a handler for NtOpenDirectoryObject has not been implemented");

        break;
    }

    //
    // Handle the memory buffer modification after NtQueryInformationProcess system call
    //
    case SysNtQueryInformationProcess:
    {

        switch (Params->OptionalParam1)
        {
        case 0x07:
        {

            DWORD_PTR notDebugged = 0x0;
            MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &notDebugged, sizeof(DWORD_PTR));
            break;
        }
        case 0x1f:
        {

            DWORD notDebugged = 0x0;
            MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &notDebugged, sizeof(DWORD));
            break;
        }
        case 0x1e:
        {

            if (CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
            {
                LogInfo("ProcessDebugObject");
            }
            break;
        }
        default:
        {
            break;
        }
        }

        break;
    }

    //
    // Handle the return code modification after NtSystemDebugControl system call
    //
    case SysNtSystemDebugControl:
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call, after the SYSRET, change the return code to STATUS_DEBUGGER_INACTIVE
        //
        VCpu->Regs->rax = (UINT64)(UINT32)STATUS_DEBUGGER_INACTIVE;
        break;
    }

    //
    // Handle the return code modification after SysNtOpenFile system call
    //
    case SysNtOpenFile:
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call if the request was for a known hypervisor file
        // after the SYSRET, change the return code to STATUS_OBJECT_NAME_NOT_FOUND
        //
        VCpu->Regs->rax = (UINT64)(UINT32)STATUS_OBJECT_NAME_NOT_FOUND;
        break;
    }

    //
    // Handle the return code modification after NtNtQueryValueKey system call
    // 
    // NOTE: The transparent mitigation will replace all occurances of a hypervisor vendor string in the registry
    // key data to a randomized real hardware vendor string, no matter the meaning of the key, 
    // This can cause some keys to illogical data, for example, 
    // a disk drive ID having a vendor string of ASUS even though(as far as I know) ASUS doesnt produce storage devices.
    // or a case where only a part of a string(the matching part) is replaced and cases like ASUS_Virtual are returned to the user-mode
    //
    case SysNtQueryValueKey:
    {
        UINT64 RetVal;

        //
        // Call the handler of NtQueryValueKey syscall callback
        //
        RetVal = TransparentCallbackHandleAfterNtQueryValueKeySyscall(Params);

        //
        // If a custom(Specific to transparency) error code should be returned,
        // set it to %RAX
        // Else leave it to what the kernel already set it to
        //
        if (RetVal != 0)
        {
            VCpu->Regs->rax = RetVal;
        }

        break;
    }

    //
    // Handle the memory buffer modification after NtOpenKey system call and its derivatives
    //
    case SysNtOpenKey:
    case SysNtOpenKeyEx:
    case SysNtOpenKeyTransacted:
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call if the request was for a known hypervisor registry key
        // after the SYSRET, change the return code to STATUS_OBJECT_NAME_NOT_FOUND
        //
        VCpu->Regs->rax = (UINT64)(UINT32)STATUS_OBJECT_NAME_NOT_FOUND;

        break;
    }

    case SysNtEnumerateKey:
    {
        
        UINT64 RetVal;

        //
        // Call the handler of NtEnumerateKey syscall callback
        //
        RetVal = TransparentCallbackHandleAfterNtEnumerateKeySyscall(Params);

        //
        // If a custom(Specific to transparency) error code should be returned,
        // set it to %RAX
        // Else leave it to what the kernel already set it to
        //
        if (RetVal != 0)
        {
            VCpu->Regs->rax = RetVal;
        }

        break;
    }
    default:

        //
        // A SYSRET trap flag was inserted for a System call that does not have a transparency handler implemented
        //
        LogInfo("Transparent callback  for an unimplemented system call handle with the trap flag for process: %x, thread: %x, rip: %llx, context: %llx RAX: %llx (p1: %llx, p2: %llx, p3: %llx, p4: %llx) \n",
                        ProcessId,
                        ThreadId,
                        VCpu->LastVmexitRip,
                        Context,
                        VCpu->Regs->rax,
                        Params->OptionalParam1,
                        Params->OptionalParam2,
                        Params->OptionalParam3,
                        Params->OptionalParam4);
        break;
    }

}

/**
 * @brief Generate a random number by utilizing RDTSC instruction.
 *
 * Masking 16 LSB of the measured clock time.
 * @return UINT32
 */
UINT32
TransparentGetRand()
{
    UINT64 Tsc;
    UINT32 Rand;

    Tsc  = __rdtsc();
    Rand = Tsc & 0xffff;

    return Rand;
}
//
// /**
//  * @brief maximum random value
//  */
// #define MY_RAND_MAX 32768
//
//     /**
//      * @brief pre-defined log result
//      * @details we used this because we want to avoid using floating-points in
//      * kernel
//      */
//     int TransparentTableLog[] =
//     {
//         0,
//         69,
//         110,
//         139,
//         161,
//         179,
//         195,
//         208,
//         220,
//         230,
//         240,
//         248,
//         256,
//         264,
//         271,
//         277,
//         283,
//         289,
//         294,
//         300,
//         304,
//         309,
//         314,
//         318,
//         322,
//         326,
//         330,
//         333,
//         337,
//         340,
//         343,
//         347,
//         350,
//         353,
//         356,
//         358,
//         361,
//         364,
//         366,
//         369,
//         371,
//         374,
//         376,
//         378,
//         381,
//         383,
//         385,
//         387,
//         389,
//         391,
//         393,
//         395,
//         397,
//         399,
//         401,
//         403,
//         404,
//         406,
//         408,
//         409,
//         411,
//         413,
//         414,
//         416,
//         417,
//         419,
//         420,
//         422,
//         423,
//         425,
//         426,
//         428,
//         429,
//         430,
//         432,
//         433,
//         434,
//         436,
//         437,
//         438,
//         439,
//         441,
//         442,
//         443,
//         444,
//         445,
//         447,
//         448,
//         449,
//         450,
//         451,
//         452,
//         453,
//         454,
//         455,
//         456,
//         457,
//         458,
//         460,
//         461};
//
//
// /**
//  * @brief Integer power function definition.
//  *
//  * @params x Base Value
//  * @params p Power Value
//  * @return int
//  */
// int
// TransparentPow(int x, int p)
// {
//     int Res = 1;
//     for (int i = 0; i < p; i++)
//     {
//         Res = Res * x;
//     }
//     return Res;
// }
//
// /**
//  * @brief Integer Natural Logarithm function estimation.
//  *
//  * @params x input value
//  * @return int
//  */
// int
// TransparentLog(int x)
// {
//     int n     = x;
//     int Digit = 0;
//
//     while (n >= 100)
//     {
//         n = n / 10;
//         Digit++;
//     }
//
//     //
//     // Use pre-defined values of logarithms and estimate the total value
//     //
//     return TransparentTableLog[n] / 100 + (Digit * 23) / 10;
// }
// /**
//  * @brief Integer root function estimation.
//  *
//  * @params x input value
//  * @return int
//  */
// int
// TransparentSqrt(int x)
// {
//     int Res = 0;
//     int Bit;
//
//     //
//     // The second-to-top bit is set.
//     //
//     Bit = 1 << 30;
//
//     //
//     // "Bit" starts at the highest power of four <= the argument.
//     //
//     while (Bit > x)
//         Bit >>= 2;
//
//     while (Bit != 0)
//     {
//         if (x >= Res + Bit)
//         {
//             x -= Res + Bit;
//             Res = (Res >> 1) + Bit;
//         }
//         else
//             Res >>= 1;
//         Bit >>= 2;
//     }
//     return Res;
// }
//
// /**
//  * @brief Integer Gaussian Random Number Generator(GRNG) based on Box-Muller method. A Float to Integer
//  * mapping is used in the function.
//  *
//  * @params Average Mean
//  * @parans Sigma Standard Deviation of the targeted Gaussian Distribution
//  * @return int
//  */
// int
// TransparentRandn(int Average, int Sigma)
// {
//     int U1, r1, U2, r2, W, Mult;
//     int X1, X2 = 0, XS1;
//     int LogTemp = 0;
//
//     do
//     {
//         r1 = TransparentGetRand();
//         r2 = TransparentGetRand();
//
//         U1 = (r1 % MY_RAND_MAX) - (MY_RAND_MAX / 2);
//
//         U2 = (r2 % MY_RAND_MAX) - (MY_RAND_MAX / 2);
//
//         W = U1 * U1 + U2 * U2;
//     } while (W >= MY_RAND_MAX * MY_RAND_MAX / 2 || W == 0);
//
//     LogTemp = (TransparentLog(W) - TransparentLog(MY_RAND_MAX * MY_RAND_MAX));
//
//     Mult = TransparentSqrt((-2 * LogTemp) * (MY_RAND_MAX * MY_RAND_MAX / W));
//
//     X1  = U1 * Mult / MY_RAND_MAX;
//     XS1 = U1 * Mult;
//
//     X2 = U2 * Mult / MY_RAND_MAX;
//
//     return (Average + (Sigma * XS1) / MY_RAND_MAX);
// }
//
// /**
//  * @brief Add name or process id of the target process to the list
//  * of processes that HyperDbg should apply transparent-mode on them
//  *
//  * @param Measurements
//  * @return BOOLEAN
//  */
// BOOLEAN
// TransparentAddNameOrProcessIdToTheList(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements)
// {
//     SIZE_T                SizeOfBuffer;
//     PTRANSPARENCY_PROCESS PidAndNameBuffer;
//
//     //
//     // Check whether it's a process id or it's a process name
//     //
//     if (Measurements->TrueIfProcessIdAndFalseIfProcessName)
//     {
//         //
//         // It's a process Id
//         //
//         SizeOfBuffer = sizeof(TRANSPARENCY_PROCESS);
//     }
//     else
//     {
//         //
//         // It's a process name
//         //
//         SizeOfBuffer = sizeof(TRANSPARENCY_PROCESS) + Measurements->LengthOfProcessName;
//     }
//
//     //
//     // Allocate the Buffer
//     //
//     PidAndNameBuffer = PlatformMemAllocateZeroedNonPagedPool(SizeOfBuffer);
//
//     if (PidAndNameBuffer == NULL)
//     {
//         return FALSE;
//     }
//
//     //
//     // Save the address of the buffer for future de-allocation
//     //
//     PidAndNameBuffer->BufferAddress = PidAndNameBuffer;
//
//     //
//     // Check again whether it's a process id or it's a process name
//     // then fill the structure
//     //
//     if (Measurements->TrueIfProcessIdAndFalseIfProcessName)
//     {
//         //
//         // It's a process Id
//         //
//         PidAndNameBuffer->ProcessId                            = Measurements->ProcId;
//         PidAndNameBuffer->TrueIfProcessIdAndFalseIfProcessName = TRUE;
//     }
//     else
//     {
//         //
//         // It's a process name
//         //
//         PidAndNameBuffer->TrueIfProcessIdAndFalseIfProcessName = FALSE;
//
//         //
//         // Move the process name string to the end of the buffer
//         //
//         RtlCopyBytes((void *)((UINT64)PidAndNameBuffer + sizeof(TRANSPARENCY_PROCESS)),
//                      (const void *)((UINT64)Measurements + sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE)),
//                      Measurements->LengthOfProcessName);
//
//         //
//         // Set the process name location
//         //
//         PidAndNameBuffer->ProcessName = (PVOID)((UINT64)PidAndNameBuffer + sizeof(TRANSPARENCY_PROCESS));
//     }
//
//     //
//     // Link it to the list of process that we need to transparent
//     // vm-exits for them
//     //
//     InsertHeadList(&g_TransparentModeMeasurements->ProcessList, &(PidAndNameBuffer->OtherProcesses));
//
//     return TRUE;
// }
//
// /**
//  * @brief Hide debugger on transparent-mode (activate transparent-mode)
//  *
//  * @param Measurements
//  * @return NTSTATUS
//  */
// NTSTATUS
// TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements)
// {
//     //
//     // Check whether the transparent-mode was already initialized or not
//     //
//     if (!g_TransparentMode)
//     {
//         //
//         // Allocate the measurements buffer
//         //
//         g_TransparentModeMeasurements = (PTRANSPARENCY_MEASUREMENTS)PlatformMemAllocateZeroedNonPagedPool(sizeof(TRANSPARENCY_MEASUREMENTS));
//
//         if (!g_TransparentModeMeasurements)
//         {
//             return STATUS_INSUFFICIENT_RESOURCES;
//         }
//
//         //
//         // Initialize the lists
//         //
//         InitializeListHead(&g_TransparentModeMeasurements->ProcessList);
//
//         //
//         // Fill the transparency details CPUID
//         //
//         g_TransparentModeMeasurements->CpuidAverage           = Measurements->CpuidAverage;
//         g_TransparentModeMeasurements->CpuidMedian            = Measurements->CpuidMedian;
//         g_TransparentModeMeasurements->CpuidStandardDeviation = Measurements->CpuidStandardDeviation;
//
//         //
//         // Fill the transparency details RDTSC
//         //
//         g_TransparentModeMeasurements->RdtscAverage           = Measurements->RdtscAverage;
//         g_TransparentModeMeasurements->RdtscMedian            = Measurements->RdtscMedian;
//         g_TransparentModeMeasurements->RdtscStandardDeviation = Measurements->RdtscStandardDeviation;
//
//         //
//         // add the new process name or Id to the list
//         //
//         TransparentAddNameOrProcessIdToTheList(Measurements);
//
//         //
//         // Enable RDTSC and RDTSCP exiting on all cores
//         //
//         BroadcastEnableRdtscExitingAllCores();
//
//         //
//         // Finally, enable the transparent-mode
//         //
//         g_TransparentMode = TRUE;
//     }
//     else
//     {
//         //
//         // It's already initialized, we just need to
//         // add the new process name or Id to the list
//         //
//         TransparentAddNameOrProcessIdToTheList(Measurements);
//     }
//
//     return STATUS_SUCCESS;
// }
//
// /**
//  * @brief Deactivate transparent-mode
//  *
//  * @return NTSTATUS
//  */
// NTSTATUS
// TransparentUnhideDebugger()
// {
//     PLIST_ENTRY TempList           = 0;
//     PVOID       BufferToDeAllocate = 0;
//
//     if (g_TransparentMode)
//     {
//         //
//         // Disable the transparent-mode
//         //
//         g_TransparentMode = FALSE;
//
//         //
//         // Disable RDTSC and RDTSCP emulation
//         //
//         BroadcastDisableRdtscExitingAllCores();
//
//         //
//         // Free list of allocated buffers
//         //
//         // Check for process id and process name, if not match then we don't emulate it
//         //
//         TempList = &g_TransparentModeMeasurements->ProcessList;
//         while (&g_TransparentModeMeasurements->ProcessList != TempList->Flink)
//         {
//             TempList                             = TempList->Flink;
//             PTRANSPARENCY_PROCESS ProcessDetails = (PTRANSPARENCY_PROCESS)CONTAINING_RECORD(TempList, TRANSPARENCY_PROCESS, OtherProcesses);
//
//             //
//             // Save the buffer so we can de-allocate it
//             //
//             BufferToDeAllocate = ProcessDetails->BufferAddress;
//
//             //
//             // We have to remove the event from the list
//             //
//             RemoveEntryList(&ProcessDetails->OtherProcesses);
//
//             //
//             // Free the buffer
//             //
//             PlatformMemFreePool(BufferToDeAllocate);
//         }
//
//         //
//         // Deallocate the measurements buffer
//         //
//         PlatformMemFreePool(g_TransparentModeMeasurements);
//         g_TransparentModeMeasurements = NULL;
//
//         return STATUS_SUCCESS;
//     }
//     else
//     {
//         return STATUS_UNSUCCESSFUL;
//     }
// }
//
// /**
//  * @brief VM-Exit handler for different exit reasons
//  * @details Should be called from vmx-root
//  *
//  * @param VCpu The virtual processor's state
//  * @param ExitReason Exit Reason
//  * @return BOOLEAN Return True we should emulate RDTSCP
//  *  or return false if we should not emulate RDTSCP
//  */
// BOOLEAN
// TransparentModeStart(VIRTUAL_MACHINE_STATE * VCpu, UINT32 ExitReason)
// {
//     UINT32      Aux                = 0;
//     PLIST_ENTRY TempList           = 0;
//     PCHAR       CurrentProcessName = 0;
//     UINT32      CurrentProcessId;
//     UINT64      CurrrentTime;
//     HANDLE      CurrentThreadId;
//     BOOLEAN     Result                      = TRUE;
//     BOOLEAN     IsProcessOnTransparencyList = FALSE;
//
//     //
//     // Save the current time
//     //
//     CurrrentTime = __rdtscp(&Aux);
//
//     //
//     // Save time of vm-exit on each logical processor separately
//     //
//     VCpu->TransparencyState.PreviousTimeStampCounter = CurrrentTime;
//
//     //
//     // Find the current process id and name
//     //
//     CurrentProcessId   = HANDLE_TO_UINT32(PsGetCurrentProcessId());
//     CurrentProcessName = CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess());
//
//     //
//     // Check for process id and process name, if not match then we don't emulate it
//     //
//     TempList = &g_TransparentModeMeasurements->ProcessList;
//     while (&g_TransparentModeMeasurements->ProcessList != TempList->Flink)
//     {
//         TempList                             = TempList->Flink;
//         PTRANSPARENCY_PROCESS ProcessDetails = (PTRANSPARENCY_PROCESS)CONTAINING_RECORD(TempList, TRANSPARENCY_PROCESS, OtherProcesses);
//         if (ProcessDetails->TrueIfProcessIdAndFalseIfProcessName)
//         {
//             //
//             // This entry is process id
//             //
//             if (ProcessDetails->ProcessId == CurrentProcessId)
//             {
//                 //
//                 // Let the transparency handler to handle it
//                 //
//                 IsProcessOnTransparencyList = TRUE;
//                 break;
//             }
//         }
//         else
//         {
//             //
//             // This entry is a process name
//             //
//             if (CurrentProcessName != NULL && CommonIsStringStartsWith(CurrentProcessName, ProcessDetails->ProcessName))
//             {
//                 //
//                 // Let the transparency handler to handle it
//                 //
//                 IsProcessOnTransparencyList = TRUE;
//                 break;
//             }
//         }
//     }
//
//     //
//     // Check whether we find this process on transparency list or not
//     //
//     if (!IsProcessOnTransparencyList)
//     {
//         //
//         // No, we didn't let's do the normal tasks
//         //
//         return TRUE;
//     }
//
//     //
//     // Get current thread Id
//     //
//     CurrentThreadId = PsGetCurrentThreadId();
//
//     //
//     // Check whether we are in new thread or in previous thread
//     //
//     if (VCpu->TransparencyState.ThreadId != CurrentThreadId)
//     {
//         //
//         // It's a new thread Id reset everything
//         //
//         VCpu->TransparencyState.ThreadId                        = CurrentThreadId;
//         VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc = NULL64_ZERO;
//         VCpu->TransparencyState.CpuidAfterRdtscDetected         = FALSE;
//     }
//
//     //
//     // Now, it's time to check and play with RDTSC/P and CPUID
//     //
//
//     if (ExitReason == VMX_EXIT_REASON_EXECUTE_RDTSC || ExitReason == VMX_EXIT_REASON_EXECUTE_RDTSCP)
//     {
//         if (VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc == NULL64_ZERO)
//         {
//             //
//             // It's a timing and the previous time for the thread is null
//             // so we need to save the time (maybe) for future use
//             //
//             VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc = CurrrentTime;
//         }
//         else if (VCpu->TransparencyState.CpuidAfterRdtscDetected == TRUE)
//         {
//             //
//             // Someone tries to know about the hypervisor
//             // let's play with them
//             //
//
//             // LogInfo("Possible RDTSC+CPUID+RDTSC");
//         }
//         else if (VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc != NULL64_ZERO &&
//                  VCpu->TransparencyState.CpuidAfterRdtscDetected == FALSE)
//         {
//             //
//             // It's a new rdtscp, let's save the new value
//             //
//             VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc +=
//                 TransparentRandn((UINT32)g_TransparentModeMeasurements->CpuidAverage,
//                                  (UINT32)g_TransparentModeMeasurements->CpuidStandardDeviation);
//         }
//
//         //
//         // Adjust the rdtsc based on RevealedTimeStampCounterByRdtsc
//         //
//         VCpu->Regs->rax = 0x00000000ffffffff &
//                           VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc;
//
//         VCpu->Regs->rdx = 0x00000000ffffffff &
//                           (VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc >> 32);
//
//         //
//         // Check if we need to adjust rcx as a result of rdtscp
//         //
//         if (ExitReason == VMX_EXIT_REASON_EXECUTE_RDTSCP)
//         {
//             VCpu->Regs->rcx = 0x00000000ffffffff & Aux;
//         }
//         //
//         // Shows that vm-exit handler should not emulate the RDTSC/P
//         //
//         Result = FALSE;
//     }
//     else if (ExitReason == VMX_EXIT_REASON_EXECUTE_CPUID &&
//              VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc != NULL64_ZERO)
//     {
//         //
//         // The guy executed one or more CPUIDs after an rdtscp so we
//         //  need to add new cpuid value to previous timer and also
//         //  we need to store it somewhere to remember this behavior
//         //
//         VCpu->TransparencyState.RevealedTimeStampCounterByRdtsc +=
//             TransparentRandn((UINT32)g_TransparentModeMeasurements->CpuidAverage,
//                              (UINT32)g_TransparentModeMeasurements->CpuidStandardDeviation);
//
//         VCpu->TransparencyState.CpuidAfterRdtscDetected = TRUE;
//     }
//
//     return Result;
// }
//
