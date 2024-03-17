/**
 * @file HaltedBroadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcasting functions in case of halted cores
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function broadcasts MSR (READ) changes to all cores
 * @details Should be called from VMX root-mode
 *
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedBroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_READ;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = BitmapMask;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts MSR (WRITE) changes to all cores
 * @details Should be called from VMX root-mode
 *
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedBroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_WRITE;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = BitmapMask;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts IO changes to all cores
 * @details Should be called from VMX root-mode
 *
 * @param Port
 *
 * @return VOID
 */
VOID
HaltedBroadcastChangeAllIoBitmapAllCores(UINT64 Port)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_CHANGE_IO_BITMAP;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = Port;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable RDPMC exiting to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableRdpmcExitingAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_SET_RDPMC_EXITING;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable rdtsc/rdtscp exiting to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableRdtscExitingAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_SET_RDTSC_EXITING;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable mov to debug registers exiting to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableMov2DebugRegsExitingAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_DEBUG_REGS_EXITING;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable external interrupt exiting to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableExternalInterruptExitingAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_ENABLE_EXTERNAL_INTERRUPT_EXITING;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts set exception bitmap to all cores
 * @details Should be called from VMX root-mode
 *
 * @param ExceptionIndex
 *
 * @return VOID
 */
VOID
HaltedBroadcastSetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_SET_EXCEPTION_BITMAP;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = ExceptionIndex;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts unset exception bitmap on VMCS to all cores
 * @details Should be called from VMX root-mode
 *
 * @param ExceptionIndex
 *
 * @return VOID
 */
VOID
HaltedBroadcastUnSetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_UNSET_EXCEPTION_BITMAP;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = ExceptionIndex;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable mov to CR exiting to all cores
 * @details Should be called from VMX root-mode
 *
 * @param BroadcastingOption
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableMovToCrExitingAllCores(DEBUGGER_EVENT_OPTIONS * BroadcastingOption)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_CONTROL_REGS_EXITING;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = BroadcastingOption->OptionalParam1;
    DirectVmcallOptions.OptionalParam2 = BroadcastingOption->OptionalParam2;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts enable syscall hook using EFER SCE bit to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastEnableEferSyscallHookAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_ENABLE_SYSCALL_HOOK_EFER;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts invalidate EPT (All Contexts) to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastInvalidateEptAllContextsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_INVEPT_ALL_CONTEXTS;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts invalidate EPT (A Single Context) to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastInvalidateSingleContextAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_INVEPT_SINGLE_CONTEXT;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts restore a single EPT entry and invalidate EPT cache to all cores
 * @details Should be called from VMX root-mode
 *
 * @param UnhookingDetail
 *
 * @return VOID
 */
VOID
HaltedBroadcastUnhookSinglePageAllCores(EPT_SINGLE_HOOK_UNHOOKING_DETAILS * UnhookingDetail)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_UNHOOK_SINGLE_PAGE;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = UnhookingDetail->PhysicalAddress;
    DirectVmcallOptions.OptionalParam2 = UnhookingDetail->OriginalEntry;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts disable external interrupt exiting only to clear !interrupt commands to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastSetDisableExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts reset MSR Bitmap Read to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastResetMsrBitmapReadAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_READ;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts reset MSR Bitmap Write to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastResetMsrBitmapWriteAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_WRITE;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts reset exception bitmap on VMCS to all cores
 * @details Should be called from VMX root-mode
 * THIS VMCALL SHOULD BE USED ONLY IN RESETTING (CLEARING) EXCEPTION EVENTS
 *
 * @return VOID
 */
VOID
HaltedBroadcastResetExceptionBitmapOnlyOnClearingExceptionEventsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts reset I/O Bitmaps (A & B) to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastResetIoBitmapAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_RESET_IO_BITMAP;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts clear rdtsc exiting bit ONLY in the case of disabling
 * the events for !tsc command to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastDisableRdtscExitingForClearingTscEventsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts disable rdpmc exiting in primary cpu-based
 * controls to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastDisableRdpmcExitingAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_UNSET_RDPMC_EXITING;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts disable syscall hook using EFER SCE bit
 * controls to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastDisableEferSyscallEventsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_DISABLE_SYSCALL_HOOK_EFER;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts clear mov 2 hw dr exiting bit ONLY in the case of
 * disabling the events for !dr command to all cores
 * @details Should be called from VMX root-mode
 *
 * @return VOID
 */
VOID
HaltedBroadcastDisableMov2DrExitingForClearingDrEventsAllCores()
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}

/**
 * @brief This function broadcasts clear mov 2 cr exiting bit ONLY in the case of disabling
 * the events for !crwrite command to all cores
 * @details Should be called from VMX root-mode
 *
 * @param BroadcastingOption
 *
 * @return VOID
 */
VOID
HaltedBroadcastDisableMov2CrExitingForClearingCrEventsAllCores(DEBUGGER_EVENT_OPTIONS * BroadcastingOption)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = (UINT64)NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = BroadcastingOption->OptionalParam1;
    DirectVmcallOptions.OptionalParam2 = BroadcastingOption->OptionalParam2;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}
