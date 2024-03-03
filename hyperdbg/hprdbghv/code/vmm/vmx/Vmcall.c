/**
 * @file Vmcall.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The main VMCALL and Hypercall handler
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handle vm-exits of VMCALLs
 *
 * @param VCpu The virtual processor's state
 * @param GuestRegs
 *
 * @return NTSTATUS
 */
_Use_decl_annotations_
NTSTATUS
VmxHypervVmcallHandler(VIRTUAL_MACHINE_STATE * VCpu, PGUEST_REGS GuestRegs)
{
    UINT64                GuestRsp   = NULL64_ZERO;
    HYPERCALL_INPUT_VALUE InputValue = {.Flags = GuestRegs->rcx};

    switch (InputValue.Fields.CallCode)
    {
    case HvSwitchVirtualAddressSpace:
    case HvFlushVirtualAddressSpace:
    case HvFlushVirtualAddressList:
    case HvCallFlushVirtualAddressSpaceEx:
    case HvCallFlushVirtualAddressListEx:

        VpidInvvpidAllContext();
        break;

    case HvCallFlushGuestPhysicalAddressSpace:
    case HvCallFlushGuestPhysicalAddressList:

        EptInveptSingleContext(VCpu->EptPointer.AsUInt);
        break;
    }

    //
    // Save the guest rsp as it will be modified during the Hyper-V's
    // VMCALL process
    //
    GuestRsp = GuestRegs->rsp;

    //
    // Let the top-level hypervisor to manage it
    //
    AsmHypervVmcall((UINT64)GuestRegs);

    //
    // Restore the guest's RSP
    //
    GuestRegs->rsp = GuestRsp;

    return STATUS_SUCCESS;
}

/**
 * @brief Handle vm-exits of VMCALLs
 *
 * @param VCpu The virtual processor's state
 * @return NTSTATUS
 */
_Use_decl_annotations_
NTSTATUS
VmxHandleVmcallVmExit(VIRTUAL_MACHINE_STATE * VCpu)
{
    BOOLEAN      IsHyperDbgVmcall = FALSE;
    GUEST_REGS * GuestRegs        = VCpu->Regs;

    IsHyperDbgVmcall = (GuestRegs->r10 == 0x48564653 && GuestRegs->r11 == 0x564d43414c4c && GuestRegs->r12 == 0x4e4f485950455256);
    //
    // Check if it's our routines that request the VMCALL, or it relates to the Hyper-V
    //
    if (IsHyperDbgVmcall)
    {
        GuestRegs->rax = VmxVmcallHandler(VCpu,
                                          GuestRegs->rcx,
                                          GuestRegs->rdx,
                                          GuestRegs->r8,
                                          GuestRegs->r9);
    }
    else
    {
        return VmxHypervVmcallHandler(VCpu, GuestRegs);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Direct Vmcall Handler
 *
 * @param VCpu The virtual processor's state
 * @param VmcallNumber The number of the VMCALL
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
VmxVmcallDirectVmcallHandler(VIRTUAL_MACHINE_STATE *    VCpu,
                             UINT64                     VmcallNumber,
                             DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    return VmxVmcallHandler(VCpu,
                            VmcallNumber,
                            DirectVmcallOptions->OptionalParam1,
                            DirectVmcallOptions->OptionalParam2,
                            DirectVmcallOptions->OptionalParam3);
}

/**
 * @brief Main Vmcall Handler
 *
 * @param VCpu The virtual processor's state
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @return NTSTATUS
 */
_Use_decl_annotations_
NTSTATUS
VmxVmcallHandler(VIRTUAL_MACHINE_STATE * VCpu,
                 UINT64                  VmcallNumber,
                 UINT64                  OptionalParam1,
                 UINT64                  OptionalParam2,
                 UINT64                  OptionalParam3)
{
    NTSTATUS VmcallStatus = STATUS_UNSUCCESSFUL;

    //
    // Check for top-level driver's VMCALLs
    //
    if (VmcallNumber > TOP_LEVEL_DRIVERS_VMCALL_STARTING_NUMBER && VmcallNumber <= TOP_LEVEL_DRIVERS_VMCALL_ENDING_NUMBER)
    {
        if (VmmCallbackVmcallHandler(VCpu->CoreId, VmcallNumber, OptionalParam1, OptionalParam2, OptionalParam3))
        {
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Only 32bit of Vmcall is valid, this way we can use the upper 32 bit of the Vmcall
    //
    switch (VmcallNumber)
    {
    case VMCALL_TEST:
    {
        VmcallStatus = VmcallTest(OptionalParam1, OptionalParam2, OptionalParam3);
        break;
    }
    case VMCALL_VMXOFF:
    {
        VmxVmxoff(VCpu);
        VmcallStatus = STATUS_SUCCESS;

        break;
    }
    case VMCALL_CHANGE_PAGE_ATTRIB:
    {
        BOOLEAN  HookResult = FALSE;
        CR3_TYPE ProcCr3    = {.Flags = OptionalParam3};

        HookResult = EptHookPerformPageHookMonitorAndInlineHook(VCpu,
                                                                (PVOID)OptionalParam1 /* hook details */,
                                                                ProcCr3 /* Process cr3 */,
                                                                (UINT32)OptionalParam2 /* PageHookMask */);

        VmcallStatus = (HookResult == TRUE) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_INVEPT_SINGLE_CONTEXT:
    {
        EptInveptSingleContext(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_INVEPT_ALL_CONTEXTS:
    {
        EptInveptAllContexts();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNHOOK_ALL_PAGES:
    {
        EptHookRestoreAllHooksToOriginalEntry(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNHOOK_SINGLE_PAGE:
    {
        if (EptHookRestoreSingleHookToOriginalEntry(VCpu, OptionalParam1, OptionalParam2))
            VmcallStatus = STATUS_SUCCESS;
        else
            VmcallStatus = STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_ENABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(VCpu, TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(VCpu, FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_READ:
    {
        MsrHandlePerformMsrBitmapReadChange(VCpu, (UINT32)OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_WRITE:
    {
        MsrHandlePerformMsrBitmapWriteChange(VCpu, (UINT32)OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_RDTSC_EXITING:
    {
        HvSetRdtscExiting(VCpu, TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_RDPMC_EXITING:
    {
        HvSetPmcVmexit(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_EXCEPTION_BITMAP:
    {
        HvSetExceptionBitmap(VCpu, (UINT32)OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING:
    {
        HvSetMovDebugRegsExiting(VCpu, TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING:
    {
        HvSetExternalInterruptExiting(VCpu, TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_IO_BITMAP:
    {
        IoHandlePerformIoBitmapChange(VCpu, (UINT32)OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_HIDDEN_CC_BREAKPOINT:
    {
        BOOLEAN  HookResult = FALSE;
        CR3_TYPE ProcCr3    = {.Flags = OptionalParam2};

        HookResult = EptHookPerformPageHook(VCpu,
                                            (PVOID)OptionalParam1, /* TargetAddress */
                                            ProcCr3);              /* process cr3 */

        VmcallStatus = (HookResult == TRUE) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS:
    {
        ProtectedHvExternalInterruptExitingForDisablingInterruptCommands(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_RDTSC_EXITING:
    {
        HvSetRdtscExiting(VCpu, FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;

        break;
    }
    case VMCALL_UNSET_RDPMC_EXITING:
    {
        HvSetPmcVmexit(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_DEBUG_REGS_EXITING:
    {
        HvSetMovDebugRegsExiting(VCpu, FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_MSR_BITMAP_READ:
    {
        MsrHandlePerformMsrBitmapReadReset(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_MSR_BITMAP_WRITE:
    {
        MsrHandlePerformMsrBitmapWriteReset(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS:
    {
        ProtectedHvResetExceptionBitmapToClearEvents(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_IO_BITMAP:
    {
        IoHandlePerformIoBitmapReset(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING:
    {
        HvSetMovControlRegsExiting(TRUE, OptionalParam1, OptionalParam2);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_CONTROL_REGS_EXITING:
    {
        HvSetMovControlRegsExiting(FALSE, OptionalParam1, OptionalParam2);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_MOV_TO_CR3_EXITING:
    {
        HvSetMovToCr3Vmexit(VCpu, TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_CR3_EXITING:
    {
        HvSetMovToCr3Vmexit(VCpu, FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_EXCEPTION_BITMAP:
    {
        HvUnsetExceptionBitmap(VCpu, (UINT32)OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_VM_ENTRY_LOAD_DEBUG_CONTROLS:
    {
        HvSetLoadDebugControls(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_VM_ENTRY_LOAD_DEBUG_CONTROLS:
    {
        HvSetLoadDebugControls(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS:
    {
        HvSetSaveDebugControls(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_VM_EXIT_SAVE_DEBUG_CONTROLS:
    {
        HvSetSaveDebugControls(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_VM_EXIT_ON_NMIS:
    {
        HvSetNmiExiting(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_VM_EXIT_ON_NMIS:
    {
        HvSetNmiExiting(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS:
    {
        ProtectedHvDisableRdtscExitingForDisablingTscCommands(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS:
    {
        ProtectedHvDisableMovDebugRegsExitingForDisablingDrCommands(VCpu);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS:
    {
        ProtectedHvDisableMovControlRegsExitingForDisablingCrCommands(VCpu, OptionalParam1, OptionalParam2);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_DIRTY_LOGGING_MECHANISM:
    {
        if (DirtyLoggingEnable(VCpu))
        {
            VmcallStatus = STATUS_SUCCESS;
        }
        else
        {
            VmcallStatus = STATUS_UNSUCCESSFUL;
        }

        break;
    }
    case VMCALL_DISABLE_DIRTY_LOGGING_MECHANISM:
    {
        DirtyLoggingDisable(VCpu);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_TO_MBEC_SUPPORTED_EPTP:
    {
        ExecTrapChangeToUserDisabledMbecEptp(VCpu);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESTORE_TO_NORMAL_EPTP:
    {
        ExecTrapRestoreToNormalEptp(VCpu);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_OR_ENABLE_MBEC:
    {
        ModeBasedExecHookEnableOrDisable(VCpu, (UINT32)OptionalParam1);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    default:
    {
        LogError("Err, unsupported VMCALL");
        VmcallStatus = STATUS_UNSUCCESSFUL;
        break;
    }
    }

    return VmcallStatus;
}

/**
 * @brief Test Vmcall (VMCALL_TEST)
 *
 * @param Param1
 * @param Param2
 * @param Param3
 * @return NTSTATUS
 */
NTSTATUS
VmcallTest(_In_ UINT64 Param1,
           _In_ UINT64 Param2,
           _In_ UINT64 Param3)
{
    LogDebugInfo("VmcallTest called with @Param1 = 0x%llx , @Param2 = 0x%llx , @Param3 = 0x%llx",
                 Param1,
                 Param2,
                 Param3);

    //
    // Send one byte buffer to show that Hypervisor
    // is successfully loaded
    //
    LogCallbackSendBuffer(OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED,
                          "$",
                          sizeof(CHAR),
                          TRUE);

    return STATUS_SUCCESS;
}
