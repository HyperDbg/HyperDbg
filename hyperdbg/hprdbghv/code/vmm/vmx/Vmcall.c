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

_Use_decl_annotations_
NTSTATUS
VmxHypervVmcallHandler(PGUEST_REGS GuestRegs)
{
    UINT64                GuestRsp   = NULL;
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

        EptInveptSingleContext(g_EptState->EptPointer.AsUInt);
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
    AsmHypervVmcall(GuestRegs);

    //
    // Restore the guest's RSP
    //
    GuestRegs->rsp = GuestRsp;

    return STATUS_SUCCESS;
}

/**
 * @brief Handle vm-exits of VMCALLs
 * 
 * @param GuestRegs Guest Registers
 * @return NTSTATUS 
 */
_Use_decl_annotations_
NTSTATUS
VmxHandleVmcallVmExit(UINT32      CoreIndex,
                      PGUEST_REGS GuestRegs)
{
    UINT64  GuestRsp         = NULL;
    BOOLEAN IsHyperdbgVmcall = FALSE;

    //
    // Trigger the event
    //
    // As the context to event trigger, we send NULL
    // Registers are the best source to know the purpose
    //
    if (g_TriggerEventForVmcalls)
    {
        DebuggerTriggerEvents(VMCALL_INSTRUCTION_EXECUTION, GuestRegs, NULL);
    }

    IsHyperdbgVmcall = (GuestRegs->r10 == 0x48564653 &&
                        GuestRegs->r11 == 0x564d43414c4c &&
                        GuestRegs->r12 == 0x4e4f485950455256);
    //
    // Check if it's our routines that request the VMCALL, or it relates to the Hyper-V
    //
    if (IsHyperdbgVmcall)
    {
        GuestRegs->rax = VmxVmcallHandler(CoreIndex,
                                          GuestRegs->rcx,
                                          GuestRegs->rdx,
                                          GuestRegs->r8,
                                          GuestRegs->r9,
                                          GuestRegs);
    }
    else
    {
        return VmxHypervVmcallHandler(GuestRegs);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Main Vmcall Handler
 * 
 * @param VmcallNumber Request Number
 * @param OptionalParam1 
 * @param OptionalParam2 
 * @param OptionalParam3 
 * @return NTSTATUS 
 */
_Use_decl_annotations_
NTSTATUS
VmxVmcallHandler(UINT32      CurrentCoreIndex,
                 UINT64      VmcallNumber,
                 UINT64      OptionalParam1,
                 UINT64      OptionalParam2,
                 UINT64      OptionalParam3,
                 PGUEST_REGS GuestRegs)
{
    NTSTATUS VmcallStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN  HookResult   = FALSE;
    BOOLEAN  UnsetExec    = FALSE;
    BOOLEAN  UnsetWrite   = FALSE;
    BOOLEAN  UnsetRead    = FALSE;

    //
    // Only 32bit of Vmcall is valid, this way we can use the upper 32 bit of the Vmcall
    //
    switch (VmcallNumber & 0xffffffff)
    {
    case VMCALL_TEST:
    {
        VmcallStatus = VmcallTest(OptionalParam1, OptionalParam2, OptionalParam3);
        break;
    }
    case VMCALL_VMXOFF:
    {
        VmxVmxoff();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_PAGE_ATTRIB:
    {
        //
        // Mask is the upper 32 bits to this Vmcall
        // Upper 32 bits of the Vmcall contains the attribute mask
        //
        UINT32 AttributeMask = (UINT32)((VmcallNumber & 0xFFFFFFFF00000000LL) >> 32);

        UnsetRead  = (AttributeMask & PAGE_ATTRIB_READ) ? TRUE : FALSE;
        UnsetWrite = (AttributeMask & PAGE_ATTRIB_WRITE) ? TRUE : FALSE;
        UnsetExec  = (AttributeMask & PAGE_ATTRIB_EXEC) ? TRUE : FALSE;

        CR3_TYPE ProcCr3 = {.Flags = OptionalParam3};

        HookResult = EptHookPerformPageHook2(OptionalParam1 /* TargetAddress */,
                                             OptionalParam2 /* Hook Function*/,
                                             ProcCr3 /* Process cr3 */,
                                             UnsetRead,
                                             UnsetWrite,
                                             UnsetExec);

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
        EptHookRestoreAllHooksToOrginalEntry();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNHOOK_SINGLE_PAGE:
    {
        if (EptHookRestoreSingleHookToOrginalEntry(OptionalParam1))
            VmcallStatus = STATUS_SUCCESS;
        else
            VmcallStatus = STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_ENABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_READ:
    {
        MsrHandlePerformMsrBitmapReadChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_WRITE:
    {
        MsrHandlePerformMsrBitmapWriteChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_RDTSC_EXITING:
    {
        HvSetRdtscExiting(TRUE);
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
        HvSetExceptionBitmap(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING:
    {
        HvSetMovDebugRegsExiting(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING:
    {
        HvSetExternalInterruptExiting(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_IO_BITMAP:
    {
        IoHandlePerformIoBitmapChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_HIDDEN_CC_BREAKPOINT:
    {
        CR3_TYPE ProcCr3 = {.Flags = OptionalParam2};

        HookResult = EptHookPerformPageHook(OptionalParam1, /* TargetAddress */
                                            ProcCr3);       /* process cr3 */

        VmcallStatus = (HookResult == TRUE) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS:
    {
        ProtectedHvExternalInterruptExitingForDisablingInterruptCommands();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_RDTSC_EXITING:
    {
        HvSetRdtscExiting(FALSE);
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
        HvSetMovDebugRegsExiting(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_MSR_BITMAP_READ:
    {
        MsrHandlePerformMsrBitmapReadReset();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_MSR_BITMAP_WRITE:
    {
        MsrHandlePerformMsrBitmapWriteReset();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS:
    {
        ProtectedHvResetExceptionBitmapToClearEvents();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_IO_BITMAP:
    {
        IoHandlePerformIoBitmapReset();
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
        HvSetMovToCr3Vmexit(TRUE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_CR3_EXITING:
    {
        HvSetMovToCr3Vmexit(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_UNSET_EXCEPTION_BITMAP:
    {
        HvUnsetExceptionBitmap(OptionalParam1);
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
    case VMCALL_VM_EXIT_HALT_SYSTEM:
    {
        KdHandleBreakpointAndDebugBreakpoints(CurrentCoreIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_REQUEST_FROM_DEBUGGER,
                                              NULL);
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
    case VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED:
    {
        KdSendCommandFinishedSignal(CurrentCoreIndex, GuestRegs);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SEND_MESSAGES_TO_DEBUGGER:
    {
        //
        // Kernel debugger is active, we should send the bytes over serial
        //
        KdLoggingResponsePacketToDebugger(
            OptionalParam1,
            OptionalParam2,
            OPERATION_LOG_INFO_MESSAGE);

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER:
    {
        //
        // Cast the buffer received to perform sending buffer and possibly
        // halt the the debuggee
        //
        PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest =
            OptionalParam1;

        KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                   DebuggeeBufferRequest->RequestedAction,
                                   (UINT64)DebuggeeBufferRequest + (SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER),
                                   DebuggeeBufferRequest->LengthOfBuffer);

        //
        // Check if we expect a buffer and command from the debugger or the
        // request is just finished
        //
        if (DebuggeeBufferRequest->PauseDebuggeeWhenSent)
        {
            KdHandleBreakpointAndDebugBreakpoints(CurrentCoreIndex,
                                                  GuestRegs,
                                                  DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM,
                                                  NULL);
        }

        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT:
    {
        DEBUGGER_TRIGGERED_EVENT_DETAILS TriggeredEventDetail = {0};

        TriggeredEventDetail.Context = OptionalParam1;
        TriggeredEventDetail.Tag     = OptionalParam2;

        KdHandleBreakpointAndDebugBreakpoints(CurrentCoreIndex,
                                              OptionalParam3, // We won't send current vmcall registers
                                                              // instead we send the registers provided
                                                              // from the third parameter
                                                              //
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,
                                              &TriggeredEventDetail);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS:
    {
        ProtectedHvDisableRdtscExitingForDisablingTscCommands();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS:
    {
        ProtectedHvDisableMovDebugRegsExitingForDisablingDrCommands();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS:
    {
        ProtectedHvDisableMovControlRegsExitingForDisablingCrCommands(OptionalParam1, OptionalParam2);
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
    LogSendBuffer(OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED,
                  "$",
                  1,
                  TRUE);

    return STATUS_SUCCESS;
}
