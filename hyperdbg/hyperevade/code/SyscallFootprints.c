/**
 * @file SyscallFootprints.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Try to hide SYSCALL methods from anti-debugging and anti-hypervisor
 * @details
 * @version 0.14
 * @date 2025-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handle The triggered hook on KiSystemCall64 system call handler
 * when the Transparency mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleSystemCallHook(GUEST_REGS * Regs)
{
    //
    // If the transparent mode is not enabled, do nothing
    //
    if (!g_TransparentMode)
    {
        return;
    }

    PCHAR  CallingProcess = g_Callbacks.CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess());
    UINT64 Context        = Regs->rax;

    //
    // Skip the transparent mitigations of system calls when the caller process
    // is a Windows process that should receive unmodified data
    //
    for (ULONG i = 0; i < (sizeof(TRANSPARENT_WIN_PROCESS_IGNORE) / sizeof(TRANSPARENT_WIN_PROCESS_IGNORE[0])); i++)
    {
        if (strstr(CallingProcess, TRANSPARENT_WIN_PROCESS_IGNORE[i]))
        {
            return;
        }
    }

    if (Context == g_SystemCallNumbersInformation.SysNtQuerySystemInformation ||
        Context == g_SystemCallNumbersInformation.SysNtQuerySystemInformationEx)
    {
        //
        // Handle the NtQuerySystemInformation System call
        //

        TransparentHandleNtQuerySystemInformationSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtSystemDebugControl)
    {
        //
        // Handle the NtSystemDebugControl System call
        //
        TransparentHandleNtSystemDebugControlSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryAttributesFile)
    {
        //
        // Handle the NtQueryAttributesFile System call
        //
        TransparentHandleNtQueryAttributesFileSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenDirectoryObject)
    {
        //
        // Handle the NtOpenDirectoryObject System call
        //
        TransparentHandleNtOpenDirectoryObjectSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryDirectoryObject)
    {
        //
        // Handle the NtQueryDirectoryObject System call
        //
        // TransparentHandleNtQueryDirectoryObjectSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryInformationProcess)
    {
        //
        // Handle the NtQueryInformationProcess System call
        //
        TransparentHandleNtQueryInformationProcessSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryInformationThread)
    {
        //
        // Handle the NtQueryInformationThread System call
        //
        // TransparentHandleNtQueryInformationThreadSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenFile)
    {
        //
        // Handle the NtOpenFile System call
        //
        TransparentHandleNtOpenFileSyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenKeyEx || Context == g_SystemCallNumbersInformation.SysNtOpenKey)
    {
        //
        // Handle the NtOpenKey System call
        //
        TransparentHandleNtOpenKeySyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryValueKey)
    {
        //
        // Handle the NtQueryValueKey System call
        //
        TransparentHandleNtQueryValueKeySyscall(Regs);
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtEnumerateKey)
    {
        //
        // Handle the NtEnumerateKey System call
        //
        TransparentHandleNtEnumerateKeySyscall(Regs);
    }
    else
    {
        //
        // The syscall is not important to us
        //
    }
}

/**
 * @brief Handle The NtQuerySystemInformation system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtQuerySystemInformationSyscall(GUEST_REGS * Regs)
{
    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};

    switch (Regs->r10)
    {
    case SystemProcessInformation:
    case SystemExtendedProcessInformation:
    {
        ContextParams.OptionalParam1 = SystemProcessInformation;
        ContextParams.OptionalParam2 = Regs->rdx;
        ContextParams.OptionalParam3 = Regs->r8 - 0x400;

        g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                           Regs->rax,
                                                           &ContextParams);

        break;
    }
    case SystemModuleInformation:
    {
        ContextParams.OptionalParam1 = SystemModuleInformation;
        ContextParams.OptionalParam2 = Regs->rdx;
        ContextParams.OptionalParam3 = Regs->r8;

        g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                           Regs->rax,
                                                           &ContextParams);

        break;
    }
    case SystemKernelDebuggerInformation:
    {
        ContextParams.OptionalParam1 = SystemKernelDebuggerInformation;
        ContextParams.OptionalParam2 = Regs->rdx;
        ContextParams.OptionalParam3 = Regs->r8;

        g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                           Regs->rax,
                                                           &ContextParams);
        break;
    }
    case SystemCodeIntegrityInformation:
    {
        ContextParams.OptionalParam1 = SystemCodeIntegrityInformation;
        ContextParams.OptionalParam2 = Regs->rdx;
        ContextParams.OptionalParam3 = 0x8;

        g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                           Regs->rax,
                                                           &ContextParams);
        break;
    }

    //
    // Currently SystemFirmwareTableInformation transparent handler is not implemented
    // As the queries produce a data buffer too large to safely copy and modify in root-mode
    //

    //    case SystemFirmwareTableInformation:
    //    {
    //
    //        ContextParams.OptionalParam1                  = SystemFirmwareTableInformation;
    //        ContextParams.OptionalParam2                  = Regs->rdx;
    //        ContextParams.OptionalParam3                  = Regs->r8;
    //        ContextParams.OptionalParam4                  = Regs->r9;
    //
    //        g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
    //                                           HANDLE_TO_UINT32(PsGetCurrentProcessId()),
    //                                           HANDLE_TO_UINT32(PsGetCurrentThreadId()),
    //                                           Regs->rax,
    //                                           &ContextParams);
    //        break;
    //    }
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
    // PVOID buf = PlatformMemAllocateZeroedNonPagedPool(sizeof(OBJECT_ATTRIBUTES));
    OBJECT_ATTRIBUTES Buf = {0};

    //
    // Read the OBJECT_ATTRIBUTES structure from the virtual address pointer
    //
    if (g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(virtPtr, &Buf, sizeof(OBJECT_ATTRIBUTES)))
    {
        // PVOID Namebuf = PlatformMemAllocateZeroedNonPagedPool(sizeof(UNICODE_STRING));
        UNICODE_STRING NameBuf = {0};

        //
        // Read the UNICODE_STRING structure from a virtual address pointer, pointed to by OBJECT_ATTRIBUTES.ObjectName struct entry
        //
        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)Buf.ObjectName, &NameBuf, sizeof(UNICODE_STRING)))
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
        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)NameBuf.Buffer, ObjectNameBuf, NameBuf.Length + sizeof(WCHAR)))
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
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtQueryAttributesFileSyscall(GUEST_REGS * Regs)
{
    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1                  = Regs->rdx;

    //
    // Check if the pointer given as the 3rd argument to the system call with type POBJECT_ATTRIBUTES is valid
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->r10, sizeof(OBJECT_ATTRIBUTES)))
    {
        //
        // From the POBJECT_ATTRIBUTES structure obtain the wide character string of the requested file path
        //
        PVOID PathBuf  = TransparentGetObjectNameFromAttributesVirtualPointer(Regs->r10);
        PWCH  FilePath = (PWCH)PathBuf;

        //
        // If the file Attributes request is for a listed file, insert the SYSCALL trap flag and continue execution
        //
        for (UINT16 j = 0; j < (sizeof(HV_FILES) / sizeof(HV_FILES[0])); j++)
        {
            if (wcsstr(FilePath, HV_FILES[j]))
            {
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
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
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtOpenDirectoryObjectSyscall(GUEST_REGS * Regs)
{
    //
    // Set up the context data for the callback after SYSRET
    //
    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1                  = Regs->r10;

    //
    // Check if the pointer given as the 3rd argument to the system call with type POBJECT_ATTRIBUTES is valid
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->r8, sizeof(OBJECT_ATTRIBUTES)))
    {
        //
        // From the POBJECT_ATTRIBUTES structure obtain the wide character string of the requested directory path
        //
        PVOID PathBuf = TransparentGetObjectNameFromAttributesVirtualPointer(Regs->r8);
        PWCH  DirPath = (PWCH)PathBuf;

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
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
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
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtSystemDebugControlSyscall(GUEST_REGS * Regs)
{
    //
    // Corrupt the system call arguments, to cause the kernel to return an error
    //
    Regs->r9 = 0x0;

    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};

    //
    // Set the trap flag to intercept the SYSRET instruction
    //
    g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                       HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                       Regs->rax,
                                                       &ContextParams);
}

/**
 * @brief Handle The NtQueryInformationProcess system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtQueryInformationProcessSyscall(GUEST_REGS * Regs)
{
    //
    // Set up the context parameters for the interception callback
    //
    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1                  = Regs->rdx; // ProcessInformationClass
    ContextParams.OptionalParam2                  = Regs->r8;  // BufferPtr
    ContextParams.OptionalParam3                  = Regs->r9;  // BufferSize

    //
    // Set the trap flag to intercept the SYSRET instruction
    //
    g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                       HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                       Regs->rax,
                                                       &ContextParams);
}

/**
 * @brief Handle The NtOpenFile system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtOpenFileSyscall(GUEST_REGS * Regs)
{
    //
    // Check if the user-mode pointer in R8 to a OBJECT_ATTRIBUTES struct is valid
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->r8, sizeof(OBJECT_ATTRIBUTES)))
    {
        //
        // From the OBJECT_ATTRIBUTES struct pointer extract the file path for which this syscall is called
        //
        PVOID NameBuf  = TransparentGetObjectNameFromAttributesVirtualPointer(Regs->r8);
        PWCH  FileName = (PWCH)NameBuf;
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
                // If a match was found, corrupt the user-mode pointers in CPU registers, so that, when the kernel-mode execution continues, it would fail.
                //
                Regs->r8  = 0x0;
                Regs->r10 = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
                                                                   &ContextParams);

                break;
            }
        }
        //
        // Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }
}

/**
 * @brief Handle The NtOpenKey system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtOpenKeySyscall(GUEST_REGS * Regs)
{
    //
    // Check if the user-mode pointer in R8 to a OBJECT_ATTRIBUTES struct is valid
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->r8, sizeof(OBJECT_ATTRIBUTES)))
    {
        //
        // From the OBJECT_ATTRIBUTES struct pointer extract the registry key path for which this syscall is called
        //
        PVOID NameBuf = TransparentGetObjectNameFromAttributesVirtualPointer(Regs->r8);
        PWCH  KeyName = (PWCH)NameBuf;

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
                Regs->r8 = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
                                                                   &ContextParams);

                break;
            }
        }

        //
        // Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }
}

/**
 * @brief Handle The NtQueryValueKey system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtQueryValueKeySyscall(GUEST_REGS * Regs)
{
    //
    // Check if the user-mode pointer in RDX to a UNICODE_STRING struct is valid
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->rdx, sizeof(UNICODE_STRING)))
    {
        UNICODE_STRING NameUString = {0};

        //
        // Read the UNICODE_STRING structure from a virtual address pointer, pointed to by RDX struct entry
        //
        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Regs->rdx, &NameUString, sizeof(UNICODE_STRING)))
            return;

        //
        // Read the PWCH wide char string from the address pointer in the UNICODE_STRING
        //
        PVOID NameBuf = PlatformMemAllocateZeroedNonPagedPool(NameUString.Length + sizeof(WCHAR));
        if (NameBuf == NULL)
        {
            return;
        }

        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)NameUString.Buffer, NameBuf, NameUString.Length + sizeof(WCHAR)))
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

                SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};

                ContextParams.OptionalParam1 = Regs->r8;
                ContextParams.OptionalParam2 = Regs->r9;

                //
                // Read the 5th argument of the system call from the stack at location %RSP + 0x28
                //
                if (g_Callbacks.CheckAccessValidityAndSafety(Regs->rsp + 0x28, sizeof(UINT64)))
                {
                    g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(Regs->rsp + 0x28), &ContextParams.OptionalParam3, sizeof(ULONG));
                }
                else
                {
                    LogInfo("Process 0x%llx on thread %llx executed NtQueryValueKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId()));

                    PlatformMemFreePool(NameBuf);
                    return;
                }

                //
                // Read the 6th argument of the system call from the stack at location %RSP + 0x30
                //
                if (g_Callbacks.CheckAccessValidityAndSafety(Regs->rsp + 0x30, sizeof(UINT64)))
                {
                    g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(Regs->rsp + 0x30), &ContextParams.OptionalParam4, sizeof(UINT64));
                }
                else
                {
                    LogInfo("Process 0x%llx on thread %llx executed NtQueryValueKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId()));

                    PlatformMemFreePool(NameBuf);
                    return;
                }

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
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
                SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};

                Regs->rdx = 0x0;
                Regs->r9  = 0x0;

                //
                // Set the trap flag to intercept the SYSRET instruction
                //
                g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                                   HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                   HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                                   Regs->rax,
                                                                   &ContextParams);

                break;
            }
        }

        //
        // Clean up the allocated memory
        //
        PlatformMemFreePool(NameBuf);
    }
}

/**
 * @brief Handle The NtEnumerateKey system call
 * when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @return VOID
 */
VOID
TransparentHandleNtEnumerateKeySyscall(GUEST_REGS * Regs)
{
    //
    // Set up the context parameters for the interception callback
    //
    SYSCALL_CALLBACK_CONTEXT_PARAMS ContextParams = {0};
    ContextParams.OptionalParam1                  = Regs->r8;
    ContextParams.OptionalParam2                  = Regs->r9;

    //
    // Read the 5th argument of the system call from the stack at location %RSP + 0x28
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->rsp + 0x28, sizeof(UINT64)))
    {
        g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(Regs->rsp + 0x28), &ContextParams.OptionalParam3, sizeof(ULONG));
    }
    else
    {
        LogInfo("Process 0x%llx on thread %llx executed NtEnumerateKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId()));
        return;
    }

    //
    // Read the 6th argument of the system call from the stack at location %RSP + 0x30
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Regs->rsp + 0x30, sizeof(UINT64)))
    {
        g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)(Regs->rsp + 0x30), &ContextParams.OptionalParam4, sizeof(UINT64));
    }
    else
    {
        LogInfo("Process 0x%llx on thread %llx executed NtEnumerateKey systemcall but reading the provided arguments from %RSP failed", HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId()));
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
    g_Callbacks.SyscallCallbackSetTrapFlagAfterSyscall(Regs,
                                                       HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                       Regs->rax,
                                                       &ContextParams);
}

/**
 * @brief Handle the request for SystemModuleInformation
 *
 * @details This function removes entries from a list of system drivers that could reveal the presence of hypervisors
 *          This depends on an incomplete list HV_DRIVER, of known hypervisor drivers
 *          The revealing list entries are removed and overwritten, but the memory buffer is not reallocated, so
 *          it is possible to still detect that some tampering was done from the user space
 *
 * @param Ptr The pointer to a valid read/writable SYSTEM_MODULE_INFORMATION memory buffer
 * @param VirualAddress A pointer to a user-mode virual address
 * @param BufferSize Size of the user-mode buffer
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHandleModuleInformationQuery(PVOID Ptr, UINT64 VirtualAddress, UINT32 BufferSize)
{
    PSYSTEM_MODULE_INFORMATION StructBuf  = (PSYSTEM_MODULE_INFORMATION)Ptr;
    PSYSTEM_MODULE_ENTRY       ModuleList = StructBuf->Module;

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
    if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(VirtualAddress, Ptr, BufferSize))
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Handle the request for SystemProcessInformation
 *
 * @details This function removes entries from a list of active system processes that could reveal the presence of hypervisors
 *
 * @param Params        Preset transparent callback params that contain:
                        in OptionalParam2 a pointer to a valid read/writable memory buffer that contains a SYSTEM_PROCESS_INFORMATION structure
                        in OptionalParam3 max size in bytes of the allocated buffer
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHandleProcessInformationQuery(SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
{
    SYSTEM_PROCESS_INFORMATION PrevStructBuf = {0};
    SYSTEM_PROCESS_INFORMATION CurStructBuf  = {0};

    ULONG   ReadOffset, WriteOffset;
    BOOLEAN MatchFound = FALSE;
    ULONG   PrevOffset = 0;

    if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, &PrevStructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
    {
        return FALSE;
    }

    ReadOffset  = PrevStructBuf.NextEntryOffset;
    WriteOffset = 0;

    //
    // The first entry will always be System Idle Process, which can be skipped
    //
    if (PrevStructBuf.NextEntryOffset == 0 ||
        !g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2 + ReadOffset, &CurStructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
    {
        return FALSE;
    }

    //
    // Loop through all the entries and filter out the offending ones
    //
    do
    {
        MatchFound = FALSE;

        if (CurStructBuf.ImageName.Length != 0)
        {
            //
            // We need to search for the Image name of the process which requires extra allocation
            //
            PVOID StringBuf = PlatformMemAllocateZeroedNonPagedPool(CurStructBuf.ImageName.Length + sizeof(WCHAR));

            if (StringBuf == NULL)
            {
                LogInfo("Error allocating ImageName memory buffer");

                return FALSE;
            }

            //
            // Read the WCHAR process image name from the user-mode pointer
            //
            if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess((UINT64)CurStructBuf.ImageName.Buffer, StringBuf, CurStructBuf.ImageName.Length + sizeof(WCHAR)))
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
                if (!_wcsnicmp(ImageName, HV_Processes[i], (CurStructBuf.ImageName.Length) / sizeof(WCHAR)))
                {
                    //
                    // If the name matches, bypass it by increasing the previous entries .nextEntryOffset value
                    //

                    //
                    // The offset to this matching entry need to preserved for zeroing later
                    //
                    PrevOffset = PrevStructBuf.NextEntryOffset;

                    PrevStructBuf.NextEntryOffset = PrevStructBuf.NextEntryOffset + CurStructBuf.NextEntryOffset;

                    MatchFound = TRUE;

                    //
                    // Write the modified offset back to the usermode buffer
                    //
                    if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess((UINT64)(Params->OptionalParam2 + WriteOffset), &PrevStructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
                    {
                        LogError("Failed to modify memory buffer for the SystemProcessInformation query system call");
                    }

                    //
                    // The entry gets bypassed, but since the Image name is a pointer in the struct, to completely clear any presence of these processes
                    // zero out the name buffer as well
                    //
                    memset(StringBuf, 0x0, CurStructBuf.ImageName.Length);
                    ULONG BufOffset = (ULONG)((PBYTE)&CurStructBuf.ImageName.Length - (PBYTE)&CurStructBuf) + sizeof(USHORT);

                    if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess((UINT64)(Params->OptionalParam2 + WriteOffset + PrevOffset + BufOffset), StringBuf, CurStructBuf.ImageName.Length))
                    {
                        LogError("Failed to modify memory buffer for the SystemProcessInformation query system call");
                    }

                    break;
                }
            }

            PlatformMemFreePool(StringBuf);
        }

        //
        // If the last entry been reached, exit
        //
        if (CurStructBuf.NextEntryOffset == 0)
        {
            return TRUE;
        }

        //
        // If the current entry didnt match any process names, move forward
        //
        if (!MatchFound)
        {
            WriteOffset += PrevStructBuf.NextEntryOffset;

            PrevStructBuf = CurStructBuf;
        }

        //
        // Move over to the next entry
        //
        ReadOffset += CurStructBuf.NextEntryOffset;

        //
        // Some internal Windows calls to this system call use different offsetting/entry structure layout and causes errors
        //
        if (!g_Callbacks.CheckAccessValidityAndSafety((UINT64)(Params->OptionalParam2 + ReadOffset), sizeof(SYSTEM_PROCESS_INFORMATION)))
        {
            return FALSE;
        }

        //
        // Zero out the matching entry, so that its data doesnt remain in memory
        //
        if (MatchFound)
        {
            CurStructBuf = (SYSTEM_PROCESS_INFORMATION) {0};
            if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess((UINT64)(Params->OptionalParam2 + WriteOffset + PrevOffset), &CurStructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
            {
                return FALSE;
            }
        }

        //
        // Read from the user buffer the next process entry
        //
        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2 + ReadOffset, &CurStructBuf, sizeof(SYSTEM_PROCESS_INFORMATION)))
        {
            return FALSE;
        }

    } while (TRUE);

    return TRUE;
}

/**
 * @brief Handle the request for SystemFirmwareTableInformation
 *
 * @param ptr The pointer to a valid read/writable SYSTEM_FIRMWARE_TABLE_INFORMATION memory buffer
 * @param BufMaxSize The size of the allocated user-mode buffer
 * @param BufSizePtr A pointer to a ULONG field containing the size of the written data
 *
 * @return BOOLEAN
 */
UINT64
TransparentHandleFirmwareInformationQuery(UINT64 Ptr, UINT32 BufMaxSize, UINT64 BufSizePtr)
{
    ULONG BufSize = 0;

    //
    // Read the size of the data the kernel wrote in the buffer
    //
    if (!g_Callbacks.CheckAccessValidityAndSafety(BufSizePtr, sizeof(ULONG)) ||
        !g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(BufSizePtr, &BufSize, sizeof(ULONG)))
    {
        return 0;
    }

    //
    // If the memory buffer was too small for the kernel to write the needed information, bypass the mitigations
    //
    if (BufSize > BufMaxSize)
    {
        //
        // NOTE: might need zeroing the memory if the kernel did infact write to the pointer
        //
        return 0;
    }

    //
    // If the buffer size is too big, we cant do any mitigations with the current implementation an it exceeds
    // the size of an EPT page, risking curruption when allocating the copy buffer space in root-mode.
    //
    if (BufMaxSize > PAGE_SIZE / 2)
    {
        LogInfo("The intercepted data buffer was too large for modification, in total 0x%x bytes", BufMaxSize);
        LogInfo("The system call return value was set to STATUS_INVALID_INFO_CLASS, but this could be detected as hypervisor intervention");

        return (UINT64)(UINT32)STATUS_INVALID_INFO_CLASS;
    }

    if (BufSize == 0)
    {
        BufSize = BufMaxSize;
    }
    //
    // From the user-mode pointer, read the SYSTEM_FIRMWARE_TABLE_INFORMATION struct
    //
    PVOID Buf = PlatformMemAllocateZeroedNonPagedPool(BufMaxSize + 1);
    if (Buf == NULL)
    {
        return 0;
    }

    if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Ptr, Buf, BufSize))
    {
        PlatformMemFreePool(Buf);
        return 0;
    }

    PSYSTEM_FIRMWARE_TABLE_INFORMATION StructBuf = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)Buf;

    //
    // The request needs to be a "get" request for an existing table
    // with 'RSMB', 'ACPI' or 'FIRM' table providers
    //
    if (StructBuf->Action == SystemFirmwareTable_Get &&
        StructBuf->TableID != 0 &&
        (StructBuf->ProviderSignature == 0x52534D42 ||
         StructBuf->ProviderSignature == 0x41435049 ||
         StructBuf->ProviderSignature == 0x4649524D))
    {
        PCHAR StringBuf = (PCHAR)StructBuf->TableBuffer;

        for (ULONG i = 0; i < (sizeof(HV_FIRM_NAMES) / sizeof(HV_FIRM_NAMES[0])); i++)
        {
            for (ULONG j = 0; j < StructBuf->TableBufferLength; j++)
            {
                WORD  Count      = 0;
                PCHAR MatchStart = strstr(StringBuf + j, HV_FIRM_NAMES[i]);
                if (MatchStart != 0)
                {
                    LogInfo("Found Match for %s", HV_FIRM_NAMES[i]);

                    PCHAR NewVendorString  = NULL;
                    ULONG NewSubstringSize = 0;

                    //
                    // Replace the first occurace of the vendor string with AMERICAN MEGATRENDS INC.
                    // The rest with To Be Filled By O.E.M.
                    //
                    if (Count == 0)
                    {
                        NewVendorString  = "AMERICAN MEGATRENDS INC.";
                        NewSubstringSize = 24 * sizeof(CHAR);
                    }
                    else
                    {
                        NewVendorString  = "To Be Filled By O.E.M.";
                        NewSubstringSize = 22 * sizeof(CHAR);
                    }

                    //
                    // Obtain the lengths of all the strings and substring
                    //

                    ULONG MatchedStringLen = (ULONG)strlen(HV_FIRM_NAMES[i]);
                    ULONG oldLength        = StructBuf->TableBufferLength;

                    ULONG NewStringSize = oldLength - MatchedStringLen + NewSubstringSize;

                    //
                    // Check if the buffer size allows the modification, in case of expansion
                    //
                    if (BufSize - MatchedStringLen + NewSubstringSize > BufMaxSize)
                    {
                        //
                        // If adding the new string exceeds the user allocated size,
                        // zero out the buffer
                        //
                        memset(Buf, 0x0, BufMaxSize);
                        g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Ptr, Buf, BufMaxSize);

                        //
                        // Update the required buffer size for the next call
                        //
                        BufSize = (BufSize - oldLength) + NewStringSize;
                        g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(BufSizePtr, &BufSize, sizeof(ULONG));

                        //
                        // And return STATUS_BUFFER_TOO_SMALL error
                        //

                        PlatformMemFreePool(Buf);
                        return (UINT64)(UINT32)STATUS_BUFFER_TOO_SMALL;
                    }

                    //
                    // Calculate the positions of the replacement
                    //
                    ULONG MatchOffset = (ULONG)((MatchStart - StringBuf));
                    PCHAR MatchEnd    = StringBuf + MatchOffset + MatchedStringLen;

                    //
                    // Move the data after the matched string forward
                    // and replace the identified hypervisor string with the genuine one
                    //
                    memmove((PVOID)(StringBuf + MatchOffset + NewSubstringSize), (PVOID)MatchEnd, oldLength - MatchedStringLen - MatchOffset);
                    memcpy((PVOID)MatchStart, (PVOID)NewVendorString, NewSubstringSize);

                    StructBuf->TableBufferLength = NewStringSize;
                    BufSize                      = BufSize - MatchedStringLen + NewSubstringSize;

                    //
                    // Write the changes back to the user buffers
                    //
                    if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Ptr, Buf, BufSize))
                    {
                        LogInfo("Error writing to user-mode buffer: %llx", Ptr);
                    }

                    if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(BufSizePtr, &BufSize, sizeof(ULONG)))
                    {
                        LogInfo("Error writing to user-mode buffer: %llx", BufSizePtr);
                    }
                }
            }
        }
    }

    PlatformMemFreePool(Buf);
    return 1;
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
TransparentReplaceVendorStringFromBufferWChar(SYSCALL_CALLBACK_CONTEXT_PARAMS * Params, ULONG DataOffset, ULONG DataLenOffset)
{
    PVOID Buf       = NULL;
    BOOL  PoolAlloc = FALSE;

    //
    // Check that the user provided pointers are safe to read from
    //
    if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam4, sizeof(ULONG)))
    {
        //
        // Read the size of the data that the kernel wrote to the buffer
        //
        ULONG BufSize = 0;
        if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG)))
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
        // Check that the user provided pointers are safe to read from and the buffer is not too large
        //
        if (Params->OptionalParam3 >= 0xC00 || !g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            goto ReturnWithError;
        }

        //
        // If the buffer is small, e.g. for just a single word, store it on the stack
        // else, allocate a nonpaged memory buffer
        //
        CHAR StackBuf[MAX_PATH] = {0};

        if (Params->OptionalParam3 + sizeof(WCHAR) > MAX_PATH)
        {
            Buf       = PlatformMemAllocateZeroedNonPagedPool(Params->OptionalParam3 + sizeof(WCHAR));
            PoolAlloc = TRUE;
        }
        else
        {
            Buf = &StackBuf;
        }

        if (!Buf || !g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3))
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

            while (MatchStart != 0)
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
                    WORD Idx        = TRANSPARENT_GENUINE_VENDOR_STRING_INDEX % (sizeof(TRANSPARENT_LEGIT_DEVICE_ID_VENDOR_STRINGS_WCHAR) / sizeof(TRANSPARENT_LEGIT_DEVICE_ID_VENDOR_STRINGS_WCHAR[0]));
                    NewVendorString = TRANSPARENT_LEGIT_DEVICE_ID_VENDOR_STRINGS_WCHAR[Idx];
                }

                //
                // Remove common VM strings from the data
                //
                else if (i < 9)
                {
                    NewVendorString = L" ";
                }
                else
                {
                    //
                    // Obtain the replacement vendor name string, randomized when the transparency mode was enabled
                    //
                    NewVendorString = TRANSPARENT_LEGIT_VENDOR_STRINGS_WCHAR[TRANSPARENT_GENUINE_VENDOR_STRING_INDEX];
                }

                //
                // Obtain the lengths of all the strings and substring
                //
                ULONG tempSize = (ULONG)wcslen(NewVendorString) * sizeof(WCHAR);

                ULONG MatchedStringLen = (ULONG)wcslen(HV_REGKEYS[i]) * sizeof(WCHAR);
                ULONG oldLength        = *((PBYTE)Buf + DataLenOffset);

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
                    g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3);

                    //
                    // Update the required buffer size for the next call
                    //
                    BufSize = (tempSize - MatchedStringLen) + oldLength;
                    g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG));

                    //
                    // And return STATUS_BUFFER_OVERFLOW error
                    //

                    if (PoolAlloc)
                        PlatformMemFreePool(Buf);
                    return (UINT64)(UINT32)STATUS_BUFFER_OVERFLOW;
                }

                //
                // Calculate the positions of the replacement
                //
                ULONG MatchOffset = (ULONG)((MatchStart - StringBuf));
                PWCH  MatchEnd    = StringBuf + MatchOffset + (MatchedStringLen / sizeof(WCHAR));

                //
                // Move the data after the matched string forward
                //
                memmove((PVOID)(StringBuf + MatchOffset + (tempSize / sizeof(WCHAR))), (PVOID)MatchEnd, oldLength - MatchedStringLen - (MatchOffset * sizeof(WCHAR)));

                //
                // Replace the identified hypervisor string with the genuine one, if needed
                //
                memcpy((PVOID)MatchStart, (PVOID)NewVendorString, tempSize);

                *(PULONG)((PBYTE)Buf + DataLenOffset) = NewStringSize;
                BufSize                               = BufSize - MatchedStringLen + tempSize;

                //
                // Write the changes back to the user buffers
                //
                if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, BufSize))
                {
                    goto ReturnWithError;
                }

                if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam4, &BufSize, sizeof(ULONG)))
                {
                    goto ReturnWithError;
                }

                //
                // Cleanup
                //

                MatchStart = wcsstr(StringBuf, HV_REGKEYS[i]);

                if (!MatchStart)
                    i = 0;
            }
        }

        //
        // The data buffer contained no detectable strings
        //
        if (PoolAlloc)
            PlatformMemFreePool(Buf);
        return 0;
    }

    //
    // An error occured while performing the mitigations, the user buffer might be left unmodified
    //
ReturnWithError:
    LogInfo("A call for to read a registry entry, which could contain hypervisor specific data, was intercepted but the mitigations failed");
    LogInfo("The caller process recieved the results in this virtual address: %llx", Params->OptionalParam2);

    if (Buf != NULL)
    {
        if (PoolAlloc)
            PlatformMemFreePool(Buf);
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
TransparentCallbackHandleAfterNtQueryValueKeySyscall(SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
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
        if (Params->OptionalParam2 != 0)
            return 0;

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
        BufOffset = sizeof(ULONG) * 4; // Name offset, Data is after it

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
TransparentCallbackHandleAfterNtEnumerateKeySyscall(SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
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
 * @param Regs The virtual processor's state of registers
 * @param Params The (optional) parameters of the caller
 *
 * @return VOID
 */
VOID
TransparentCallbackHandleAfterNtQuerySystemInformationSyscall(GUEST_REGS * Regs, SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
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
        if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            SYSTEM_CODEINTEGRITY_INFORMATION Temp = {0};

            //
            // Read data from the saved pointer of the now filled information buffer
            //
            g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, Params->OptionalParam3);

            //
            // Modify the data and write it back to the information buffer to be passed to user mode
            //
            Temp.CodeIntegrityOptions = 0x01;
            g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, Params->OptionalParam3);
        }
        else
        {
            LogInfo("A call for the NtQuerySystemInformation system call requesting SystemCodeIntegrityInformation structure was made, but the usermode buffer was not captured");
        }

        break;
    }
    case SystemProcessInformation:
    case SystemExtendedProcessInformation:
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (Params->OptionalParam2 != 0x0 &&
            Params->OptionalParam3 != 0x0 &&
            g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            if (!TransparentHandleProcessInformationQuery(Params))
            {
                //
                // Some internal Windows calls to these system calls use different offsetting/entry structure layout and causes errors
                //

                // LogInfo("Error while modifying the buffer for data query 0x02x", Params->OptionalParam1);
            }
        }
        break;
    }

    case SystemModuleInformation:
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (Params->OptionalParam2 != 0x0 &&
            Params->OptionalParam3 != 0x0 &&
            g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
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
            if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam2, Buf, Params->OptionalParam3))
            {
                LogInfo("Error reading memory buffer given by the usermode call");
            }
            else
            {
                if (!TransparentHandleModuleInformationQuery(Buf, Params->OptionalParam2, (UINT32)Params->OptionalParam3))
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
        if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            //
            // Write to the output buffer 0x0001 for "Debugger not present"
            //
            WORD Temp = 0x0100;
            g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &Temp, 2);
        }
    }
    case SystemFirmwareTableInformation:
    {
        if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
        {
            UINT64 RetVal = TransparentHandleFirmwareInformationQuery(Params->OptionalParam2, (UINT32)Params->OptionalParam3, Params->OptionalParam4);
            if (RetVal == 0x0)
            {
                LogInfo("A query for SystemFirmwareTableInformation was made, but the transparent mitigation failed");
            }
            else if (RetVal != 0x1)
            {
                LogInfo("Changing to %llx", RetVal);
                Regs->rax = RetVal;
            }
        }
        else
        {
            LogInfo("A query for SystemFirmwareTableInformation was made, but the user-mode buffer was not captured");
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
 * @param Regs The virtual processor's state of registers
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 * @param Context The context of the caller
 * @param Params The (optional) parameters of the caller
 *
 * @return VOID
 */
VOID
TransparentCallbackHandleAfterSyscall(GUEST_REGS *                      Regs,
                                      UINT32                            ProcessId,
                                      UINT32                            ThreadId,
                                      UINT64                            Context,
                                      SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
{
    //
    // Handle each defined system call separately, after the kernel execution has finished(at the SYSRET instruction)
    //

    //
    // Handle the memory buffer and return code modification after NtQuerySystemInformation system call
    //
    if (Context == g_SystemCallNumbersInformation.SysNtQuerySystemInformation)
    {
        TransparentCallbackHandleAfterNtQuerySystemInformationSyscall(Regs, Params);
    }
    //
    // Handle the memory buffer and return code modification after NtQueryAttributesFile system call
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryAttributesFile)
    {
        //
        // Check if the obtained buffer pointer is valid
        //
        if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam1, sizeof(FILE_BASIC_INFORMATION)))
        {
            FILE_BASIC_INFORMATION Buf = {0};
            //
            // Copy over the data from the output buffer pointer
            //
            if (!g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(Params->OptionalParam1, &Buf, sizeof(FILE_BASIC_INFORMATION)))
            {
                LogError("Err, Virtual memory read failed");
            }
            else
            {
                //
                // Modify the file attribute to INVALID_FILE_ATTRIBUTES and write it back to the pointer
                //
                Buf.FileAttributes = ((DWORD)-1);

                if (!g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam1, &Buf, sizeof(FILE_BASIC_INFORMATION)))
                {
                    LogError("Err, Virtual memory write failed");
                }
            }
        }
        else
        {
            LogInfo("A call for the NtQueryAttributeFile system call for a marked file was made, but the output buffer was not captured");
        }
    }

    //
    // Handle the memory buffer and return code modification after NtOpenDirectoryObject system call.
    //
    // NOTE: No transparent mitigations of this call have been implemented
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenDirectoryObject)
    {
        LogInfo("A NtOpenDirectoryObject system call was made for a known directory that reveals hypervisor presence. process: %x, thread: %x\n",
                ProcessId,
                ThreadId);
        LogInfo("No action to mitigate this was made as a handler for NtOpenDirectoryObject has not been implemented");
    }
    //
    // Handle the memory buffer modification after NtQueryInformationProcess system call
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryInformationProcess)
    {
        switch (Params->OptionalParam1)
        {
        case 0x07:
        {
            if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, sizeof(DWORD_PTR)))
            {
                //
                // Zero out the return buffer to user-mode
                //
                DWORD_PTR NoDebugPort = 0x0;

                g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &NoDebugPort, sizeof(DWORD_PTR));
            }
            break;
        }
        case 0x1f:
        {
            //
            // Zero out the return buffer to user-mode
            //
            ULONG notDebugged = 0x0;
            g_Callbacks.MemoryMapperWriteMemorySafeOnTargetProcess(Params->OptionalParam2, &notDebugged, sizeof(ULONG));
            break;
        }
        case 0x1e:
        {
            if (g_Callbacks.CheckAccessValidityAndSafety(Params->OptionalParam2, (UINT32)Params->OptionalParam3))
            {
                LogInfo("Process %llx called the NtQueryInformationProcess system call with the ProcessDebugObject class, no transparent mitigations were performed", ProcessId);
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }
    //
    // Handle the return code modification after NtSystemDebugControl system call
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtSystemDebugControl)
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call, after the SYSRET, change the return code to STATUS_DEBUGGER_INACTIVE
        //
        Regs->rax = (UINT64)(UINT32)STATUS_DEBUGGER_INACTIVE;
    }

    //
    // Handle the return code modification after SysNtOpenFile system call
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenFile)
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call if the request was for a known hypervisor file
        // after the SYSRET, change the return code to STATUS_OBJECT_NAME_NOT_FOUND
        //
        Regs->rax = (UINT64)(UINT32)STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Handle the return code modification after NtNtQueryValueKey system call
    //
    // NOTE: The transparent mitigation will replace all occurances of a hypervisor vendor string in the registry
    // key data to a randomized real hardware vendor string, no matter the meaning of the key,
    // This can cause some keys to illogical data, for example,
    // a disk drive ID having a vendor string of ASUS even though(as far as I know) ASUS doesnt produce storage devices.
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtQueryValueKey)
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
            Regs->rax = RetVal;
        }
    }
    //
    // Handle the memory buffer modification after NtOpenKey system call and its derivatives
    //
    else if (Context == g_SystemCallNumbersInformation.SysNtOpenKey || Context == g_SystemCallNumbersInformation.SysNtOpenKeyEx)
    {
        //
        // In the entry handler, the Syscall number was changed to corrupt this call if the request was for a known hypervisor registry key
        // after the SYSRET, change the return code to STATUS_OBJECT_NAME_NOT_FOUND
        //
        Regs->rax = (UINT64)(UINT32)STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else if (Context == g_SystemCallNumbersInformation.SysNtEnumerateKey)
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
            Regs->rax = RetVal;
        }
    }
    else
    {
        //
        // A SYSRET trap flag was inserted for a System call that does not have a transparency handler implemented
        //
        LogInfo("Transparent callback  for an unimplemented system call handle with the trap flag for process: %x, thread: %x, context: %llx RAX: %llx (p1: %llx, p2: %llx, p3: %llx, p4: %llx) \n",
                ProcessId,
                ThreadId,
                Context,
                Regs->rax,
                Params->OptionalParam1,
                Params->OptionalParam2,
                Params->OptionalParam3,
                Params->OptionalParam4);
    }
}
