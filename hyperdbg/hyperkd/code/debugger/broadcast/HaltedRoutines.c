/**
 * @file HaltedRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief All single core broadcasting functions in case of halted core
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function performs running MSR changes (RDMSR) on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedRoutineChangeAllMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 BitmapMask)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running MSR changes (WRMSR) on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedRoutineChangeAllMsrBitmapWriteOnSingleCore(UINT32 TargetCoreId, UINT64 BitmapMask)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running changes to I/O bitmap on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param Port
 *
 * @return VOID
 */
VOID
HaltedRoutineChangeIoBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 Port)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable RDPMC exiting on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableRdpmcExitingOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable rdtsc/rdtscp exiting on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableRdtscExitingOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable mov to debug registers exiting on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableMov2DebugRegsExitingOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable external interrupt exiting on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableExternalInterruptExiting(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running set exception bitmap on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param ExceptionIndex
 *
 * @return VOID
 */
VOID
HaltedRoutineSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 ExceptionIndex)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running unset exception bitmap on VMCS on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param ExceptionIndex
 *
 * @return VOID
 */
VOID
HaltedRoutineUnSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 ExceptionIndex)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable mov to CR exiting on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BroadcastingOption
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableMovToCrExitingOnSingleCore(UINT32 TargetCoreId, DEBUGGER_EVENT_OPTIONS * BroadcastingOption)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running enable syscall hook using EFER SCE bit on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineEnableEferSyscallHookOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running invalidate EPT (All Contexts) on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineInvalidateEptAllContextsOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}

/**
 * @brief This function performs running invalidate EPT (A Single Context) on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 *
 * @return VOID
 */
VOID
HaltedRoutineInvalidateSingleContextOnSingleCore(UINT32 TargetCoreId)
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
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  &DirectVmcallOptions);
}
