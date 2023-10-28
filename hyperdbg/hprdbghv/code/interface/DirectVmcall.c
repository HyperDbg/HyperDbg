/**
 * @file DirectVmcall.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Direct VMCALL routines
 * @details
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines for test direct VMCALL
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallTest(UINT32                     CoreId,
                 DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_TEST, DirectVmcallOptions);
}

/**
 * @brief routines for performing a direct VMCALL
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param VmcallNumber
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallPerformVmcall(UINT32                     CoreId,
                          UINT64                     VmcallNumber,
                          DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VmcallNumber, DirectVmcallOptions);
}

/**
 * @brief routines for changing MSR Bitmap (Read)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallChangeMsrBitmapRead(UINT32                     CoreId,
                                DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_CHANGE_MSR_BITMAP_READ, DirectVmcallOptions);
}

/**
 * @brief routines for changing MSR Bitmap (Write)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallChangeMsrBitmapWrite(UINT32                     CoreId,
                                 DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_CHANGE_MSR_BITMAP_WRITE, DirectVmcallOptions);
}

/**
 * @brief routines for changing IO Bitmap
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallChangeIoBitmap(UINT32                     CoreId,
                           DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_CHANGE_IO_BITMAP, DirectVmcallOptions);
}

/**
 * @brief routines for enabling rdpmc exiting
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableRdpmcExiting(UINT32                     CoreId,
                               DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_SET_RDPMC_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for enabling rdtsc/rdtscp exiting
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableRdtscpExiting(UINT32                     CoreId,
                                DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_SET_RDTSC_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for enabling mov to debug registers exiting
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableMov2DebugRegsExiting(UINT32                     CoreId,
                                       DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for setting exception bitmap
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallSetExceptionBitmap(UINT32                     CoreId,
                               DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_SET_EXCEPTION_BITMAP, DirectVmcallOptions);
}

/**
 * @brief routines for enabling external interrupt exiting
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableExternalInterruptExiting(UINT32                     CoreId,
                                           DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for enabling mov to CR exiting
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableMovToCrExiting(UINT32                     CoreId,
                                 DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for enabling syscall hook using EFER SCE bit
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallEnableEferSyscall(UINT32                     CoreId,
                              DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_ENABLE_SYSCALL_HOOK_EFER, DirectVmcallOptions);
}

/**
 * @brief routines for putting hidden breakpoints (using EPT)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallSetHiddenBreakpointHook(UINT32                     CoreId,
                                    DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_SET_HIDDEN_CC_BREAKPOINT, DirectVmcallOptions);
}

/**
 * @brief routines for invalidating EPT (All Contexts)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallInvalidateEptAllContexts(UINT32                     CoreId,
                                     DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_INVEPT_ALL_CONTEXTS, DirectVmcallOptions);
}

/**
 * @brief routines for invalidating EPT (A Single Context)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallInvalidateSingleContext(UINT32                     CoreId,
                                    DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_INVEPT_SINGLE_CONTEXT, DirectVmcallOptions);
}

/**
 * @brief routines for unsetting exception bitmap on VMCS
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallUnsetExceptionBitmap(UINT32                     CoreId,
                                 DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_UNSET_EXCEPTION_BITMAP, DirectVmcallOptions);
}

/**
 * @brief routines for restoring a single EPT entry and invalidating EPT cache
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallUnhookSinglePage(UINT32                     CoreId,
                             DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_UNHOOK_SINGLE_PAGE, DirectVmcallOptions);
}

/**
 * @brief routines for disabling external interrupt exiting only to clear !interrupt commands
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallSetDisableExternalInterruptExitingOnlyOnClearingInterruptEvents(UINT32                     CoreId,
                                                                            DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS, DirectVmcallOptions);
}

/**
 * @brief routines for resetting MSR Bitmap Read
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallResetMsrBitmapRead(UINT32                     CoreId,
                               DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_RESET_MSR_BITMAP_READ, DirectVmcallOptions);
}

/**
 * @brief routines for resetting MSR Bitmap Write
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallResetMsrBitmapWrite(UINT32                     CoreId,
                                DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_RESET_MSR_BITMAP_WRITE, DirectVmcallOptions);
}

/**
 * @brief routines for resetting exception bitmap on VMCS
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallResetExceptionBitmapOnlyOnClearingExceptionEvents(UINT32                     CoreId,
                                                              DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS, DirectVmcallOptions);
}

/**
 * @brief routines for resetting I/O Bitmaps (A & B)
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallResetIoBitmap(UINT32                     CoreId,
                          DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_RESET_IO_BITMAP, DirectVmcallOptions);
}

/**
 * @brief routines for clearing rdtsc exiting bit ONLY in the case of disabling
 * the events for !tsc command
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallDisableRdtscExitingForClearingTscEvents(UINT32                     CoreId,
                                                    DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS, DirectVmcallOptions);
}

/**
 * @brief routines for disabling rdpmc exiting in primary cpu-based controls
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallDisableRdpmcExiting(UINT32                     CoreId,
                                DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_UNSET_RDPMC_EXITING, DirectVmcallOptions);
}

/**
 * @brief routines for disabling syscall hook using EFER SCE bit
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallDisableEferSyscallEvents(UINT32                     CoreId,
                                     DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_DISABLE_SYSCALL_HOOK_EFER, DirectVmcallOptions);
}

/**
 * @brief routines for clearing mov 2 hw dr exiting bit ONLY in the case of disabling
 * the events for !dr command
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallDisableMov2DrExitingForClearingDrEvents(UINT32                     CoreId,
                                                    DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS, DirectVmcallOptions);
}

/**
 * @brief routines for clearing mov 2 cr exiting bit ONLY in the case of disabling
 * the events for !crwrite command
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallDisableMov2CrExitingForClearingCrEvents(UINT32                     CoreId,
                                                    DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    return VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS, DirectVmcallOptions);
}
