/**
 * @file Configuration.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Configuration interface for hypervisor events
 * @details
 *
 * @version 0.2
 * @date 2023-01-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
ConfigureEnableMovToCr3ExitingOnAllProcessors()
{
    BroadcastEnableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief routines for initializing user-mode, kernel-mode exec trap
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureInitializeExecTrapOnAllProcessors()
{
    return ExecTrapInitialize();
}

/**
 * @brief routines for uninitializing user-mode, kernel-mode exec trap
 *
 * @return VOID
 */
VOID
ConfigureUninitializeExecTrapOnAllProcessors()
{
    ExecTrapUninitialize();
}

/**
 * @brief Apply the MBEC configuration from the kernel side
 * @param CoreId The core id
 *
 * @return VOID
 */
VOID
ConfigureExecTrapApplyMbecConfiguratinFromKernelSide(UINT32 CoreId)
{
    ExecTrapApplyMbecConfiguratinFromKernelSide(&g_GuestState[CoreId]);
}

/**
 * @brief Add the target process to the watching list
 * @param ProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureExecTrapAddProcessToWatchingList(UINT32 ProcessId)
{
    return ExecTrapAddProcessToWatchingList(ProcessId);
}

/**
 * @brief Remove the target process from the watching list
 * @param ProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureExecTrapRemoveProcessFromWatchingList(UINT32 ProcessId)
{
    return ExecTrapRemoveProcessFromWatchingList(ProcessId);
}

/**
 * @brief routines for initializing Mode-based execution hooks
 *
 * @return VOID
 */
VOID
ConfigureModeBasedExecHookUninitializeOnAllProcessors()
{
    ModeBasedExecHookUninitialize();
}

/**
 * @brief routines for initializing dirty logging mechanism
 *
 * @return VOID
 */
VOID
ConfigureDirtyLoggingInitializeOnAllProcessors()
{
    DirtyLoggingInitialize();
}

/**
 * @brief routines for uninitializing dirty logging mechanism
 *
 * @return VOID
 */
VOID
ConfigureDirtyLoggingUninitializeOnAllProcessors()
{
    DirtyLoggingUninitialize();
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
ConfigureDisableMovToCr3ExitingOnAllProcessors()
{
    BroadcastDisableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief routines for enabling syscall hooks on all cores
 *
 * @return VOID
 */
VOID
ConfigureEnableEferSyscallEventsOnAllProcessors()
{
    BroadcastEnableEferSyscallEventsOnAllProcessors();
}

/**
 * @brief routines for disabling syscall hooks on all cores
 *
 * @return VOID
 */
VOID
ConfigureDisableEferSyscallEventsOnAllProcessors()
{
    BroadcastDisableEferSyscallEventsOnAllProcessors();
}

/**
 * @brief Remove all hooks from the hooked pages list using Hooking Tag
 * @details Should be called from vmx non-root
 *
 * @param HookingTag The hooking tag to remove all hooks
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
ConfigureEptHookUnHookAllByHookingTag(UINT64 HookingTag)
{
    return EptHookUnHookAllByHookingTag(HookingTag);
}

/**
 * @brief Remove single hook from the hooked pages by the given hooking tag
 * @details Should be called from Vmx root-mode
 *
 * @param HookingTag The hooking tag to unhook
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
ConfigureEptHookUnHookSingleHookByHookingTagFromVmxRoot(UINT64                              HookingTag,
                                                        EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    return EptHookUnHookSingleHookByHookingTagFromVmxRoot(HookingTag, TargetUnhookingDetails);
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from vmx non-root
 *
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param ProcessId The process id of target process
 * @details in unhooking for some hooks only physical address is availables
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
ConfigureEptHookUnHookSingleAddress(UINT64 VirtualAddress,
                                    UINT64 PhysAddress,
                                    UINT32 ProcessId)
{
    return EptHookUnHookSingleAddress(VirtualAddress, PhysAddress, ProcessId);
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from vmx root-mode and it's the responsibility
 * of caller to broadcast to all cores to remove the target physical address
 * and invalidate EPT and modify exception bitmap (#BPs) if needed
 *
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param TargetUnhookingDetails Target data for the caller to restore EPT entry and
 * invalidate EPT caches. Only when applied in VMX-root mode directly
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
ConfigureEptHookUnHookSingleAddressFromVmxRoot(UINT64                              VirtualAddress,
                                               UINT64                              PhysAddress,
                                               EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    return EptHookUnHookSingleAddressFromVmxRoot(VirtualAddress,
                                                 PhysAddress,
                                                 TargetUnhookingDetails);
}

/**
 * @brief Allocate (reserve) extra pages for storing details of page hooks
 * for memory monitor and regular hidden breakpoit exec EPT hooks
 *
 * @param Count
 *
 * @return VOID
 */
VOID
ConfigureEptHookAllocateExtraHookingPagesForMemoryMonitorsAndExecEptHooks(UINT32 Count)
{
    EptHookAllocateExtraHookingPagesForMemoryMonitorsAndExecEptHooks(Count);
}

/**
 * @brief Allocate (reserve) pages for storing EPT hooks page hooks
 * @param Count
 *
 * @return VOID
 */
VOID
ConfigureEptHookReservePreallocatedPoolsForEptHooks(UINT32 Count)
{
    EptHookReservePreallocatedPoolsForEptHooks(Count);
}

/**
 * @brief This function invokes a VMCALL to set the hook and broadcast the exiting for
 * the breakpoints on exception bitmap
 *
 * @details this command uses hidden breakpoints (0xcc) to hook, THIS FUNCTION SHOULD BE CALLED WHEN THE
 * VMLAUNCH ALREADY EXECUTED, it is because, broadcasting to enable exception bitmap for breakpoint is not
 * clear here, if we want to broadcast to enable exception bitmaps on all cores when vmlaunch is not executed
 * then that's ok but a user might call this function when we didn't configure the vmcs, it's a problem! we
 * can solve it by giving a hint to vmcs configure function to make it ok for future configuration but that
 * sounds stupid, I think it's better to not support this feature. Btw, debugger won't use this function in
 * the above mentioned method, so we won't have any problem with this :)
 *
 * @param TargetAddress The address of function or memory address to be hooked
 * @param ProcessId The process id to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHook(PVOID TargetAddress, UINT32 ProcessId)
{
    return EptHook(TargetAddress, ProcessId);
}

/**
 * @brief This function invokes a direct VMCALL to setup the hook
 *
 * @details the caller of this function should make sure to 1) broadcast to
 * all cores to intercept breakpoints (#BPs) and after calling this function
 * 2) the caller should broadcast to all cores to invalidate their EPTPs
 *
 * @param TargetAddress The address of function or memory address to be hooked
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHookFromVmxRoot(PVOID TargetAddress)
{
    return EptHookFromVmxRoot(TargetAddress);
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook (inline)
 * @details this command uses hidden detours, this NOT be called from vmx-root mode
 *
 * @param CoreId ID of the target core
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHook2(UINT32 CoreId,
                  PVOID  TargetAddress,
                  PVOID  HookFunction,
                  UINT32 ProcessId)
{
    return EptHookInlineHook(&g_GuestState[CoreId],
                             TargetAddress,
                             HookFunction,
                             ProcessId);
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 * @details this command uses hidden detours, this NOT be called from vmx-root mode
 *
 *
 * @param CoreId ID of the target core
 * @param HookingDetails Monitor hooking detail
 * @param ProcessId The process id to translate based on that process's cr3
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHookMonitor(UINT32                                         CoreId,
                        EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * HookingDetails,
                        UINT32                                         ProcessId)
{
    return EptHookMonitorHook(&g_GuestState[CoreId],
                              HookingDetails,
                              ProcessId);
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook (inline EPT hook)
 * @details this command uses hidden detours, this should be called from vmx-root mode
 *
 * @param CoreId ID of the target core
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHook2FromVmxRoot(UINT32 CoreId,
                             PVOID  TargetAddress,
                             PVOID  HookFunction)
{
    return EptHookInlineHookFromVmxRoot(&g_GuestState[CoreId],
                                        TargetAddress,
                                        HookFunction);
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 * @details this command uses hidden detours, this should be called from vmx-root mode
 *
 *
 * @param CoreId ID of the target core
 * @param MemoryAddressDetails Monitor hooking details
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
ConfigureEptHookMonitorFromVmxRoot(UINT32                                         CoreId,
                                   EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * MemoryAddressDetails)
{
    return EptHookMonitorFromVmxRoot(&g_GuestState[CoreId], MemoryAddressDetails);
}

/**
 * @brief Change PML EPT state for execution (execute)
 * @detail should be called from VMX-root
 *
 * @param CoreId Current Core ID
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureEptHookModifyInstructionFetchState(UINT32  CoreId,
                                            PVOID   PhysicalAddress,
                                            BOOLEAN IsUnset)
{
    return EptHookModifyInstructionFetchState(&g_GuestState[CoreId], PhysicalAddress, IsUnset);
}

/**
 * @brief Change PML EPT state for read
 * @detail should be called from VMX-root
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureEptHookModifyPageReadState(UINT32  CoreId,
                                    PVOID   PhysicalAddress,
                                    BOOLEAN IsUnset)
{
    return EptHookModifyPageReadState(&g_GuestState[CoreId], PhysicalAddress, IsUnset);
}

/**
 * @brief Change PML EPT state for write
 * @detail should be called from VMX-root
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
ConfigureEptHookModifyPageWriteState(UINT32  CoreId,
                                     PVOID   PhysicalAddress,
                                     BOOLEAN IsUnset)
{
    return EptHookModifyPageWriteState(&g_GuestState[CoreId], PhysicalAddress, IsUnset);
}

/**
 * @brief routines for enabling EFER syscall hooks on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
ConfigureEnableEferSyscallHookOnSingleCore(UINT32 TargetCoreId)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformEnableEferSyscallHookOnSingleCore, NULL);
}

/**
 * @brief routines for setting EFER syscall or sysret hooks type
 *
 * @param SyscallHookType Type of hook
 *
 * @return VOID
 */
VOID
ConfigureSetEferSyscallOrSysretHookType(DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType)
{
    if (SyscallHookType == DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD)
    {
        g_IsUnsafeSyscallOrSysretHandling = TRUE;
    }
    else if (SyscallHookType == DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY)
    {
        g_IsUnsafeSyscallOrSysretHandling = FALSE;
    }
}

/**
 * @brief set external interrupt exiting on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
ConfigureSetExternalInterruptExitingOnSingleCore(UINT32 TargetCoreId)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformSetExternalInterruptExitingOnSingleCore, NULL);
}

/**
 * @brief enable RDTSC exiting on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
ConfigureEnableRdtscExitingOnSingleCore(UINT32 TargetCoreId)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformEnableRdtscExitingOnSingleCore, NULL);
}

/**
 * @brief enable RDPMC exiting on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
ConfigureEnableRdpmcExitingOnSingleCore(UINT32 TargetCoreId)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformEnableRdpmcExitingOnSingleCore, NULL);
}

/**
 * @brief enable mov 2 debug register exiting on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
ConfigureEnableMovToDebugRegistersExitingOnSingleCore(UINT32 TargetCoreId)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformEnableMovToDebugRegistersExiting, NULL);
}

/**
 * @brief set exception bitmap on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BitMask The bit mask of exception bitmap
 *
 * @return VOID
 */
VOID
ConfigureSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT32 BitMask)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformSetExceptionBitmapOnSingleCore, (PVOID)BitMask);
}

/**
 * @brief enable mov 2 control register on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BroadcastingOption The optional broadcasting fields
 *
 * @return VOID
 */
VOID
ConfigureEnableMovToControlRegisterExitingOnSingleCore(UINT32 TargetCoreId, DEBUGGER_EVENT_OPTIONS * BroadcastingOption)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformEnableMovToControlRegisterExiting, &BroadcastingOption);
}

/**
 * @brief change the mask of msr bitmaps for write on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param MsrMask The ECX in MSR (mask)
 *
 * @return VOID
 */
VOID
ConfigureChangeMsrBitmapWriteOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformChangeMsrBitmapWriteOnSingleCore, (PVOID)MsrMask);
}

/**
 * @brief change the mask of msr bitmaps for read on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param MsrMask The ECX in MSR (mask)
 *
 * @return VOID
 */
VOID
ConfigureChangeMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformChangeMsrBitmapReadOnSingleCore, (PVOID)MsrMask);
}

/**
 * @brief change I/O port bitmap on a single core
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param Port Target port in I/O bitmap
 *
 * @return VOID
 */
VOID
ConfigureChangeIoBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 Port)
{
    DpcRoutineRunTaskOnSingleCore(TargetCoreId, (PVOID)DpcRoutinePerformChangeIoBitmapOnSingleCore, (PVOID)Port);
}
