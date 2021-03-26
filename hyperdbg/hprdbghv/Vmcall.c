/**
 * @file Vmcall.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @param GuestRegs Guest Registers
 * @return NTSTATUS 
 */
NTSTATUS
VmxHandleVmcallVmExit(PGUEST_REGS GuestRegs, UINT32 CoreIndex)
{
    //
    // Triggeer the event
    //
    // As the context to event trigger, we send NULL
    // Registers are the best source to know the purpose
    //
    if (g_TriggerEventForVmcalls)
    {
        DebuggerTriggerEvents(VMCALL_INSTRUCTION_EXECUTION, GuestRegs, NULL);
    }

    //
    // Check if it's our routines that request the VMCALL our it relates to Hyper-V
    //
    if (GuestRegs->r10 == 0x48564653 && GuestRegs->r11 == 0x564d43414c4c && GuestRegs->r12 == 0x4e4f485950455256)
    {
        //
        // Then we have to manage it as it relates to us
        //
        GuestRegs->rax = VmxVmcallHandler(GuestRegs->rcx,
                                          GuestRegs->rdx,
                                          GuestRegs->r8,
                                          GuestRegs->r9,
                                          GuestRegs,
                                          CoreIndex);
    }
    else
    {
        HYPERCALL_INPUT_VALUE InputValue = {0};
        InputValue.Value                 = GuestRegs->rcx;

        switch (InputValue.Bitmap.CallCode)
        {
        case HvSwitchVirtualAddressSpace:
        case HvFlushVirtualAddressSpace:
        case HvFlushVirtualAddressList:
        case HvCallFlushVirtualAddressSpaceEx:
        case HvCallFlushVirtualAddressListEx:
        {
            InvvpidAllContexts();
            break;
        }
        case HvCallFlushGuestPhysicalAddressSpace:
        case HvCallFlushGuestPhysicalAddressList:
        {
            InveptSingleContext(g_EptState->EptPointer.Flags);
            break;
        }
        }
        //
        // Let the top-level hypervisor to manage it
        //
        GuestRegs->rax = AsmHypervVmcall(GuestRegs->rcx, GuestRegs->rdx, GuestRegs->r8, GuestRegs->r9);
    }
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
NTSTATUS
VmxVmcallHandler(UINT64      VmcallNumber,
                 UINT64      OptionalParam1,
                 UINT64      OptionalParam2,
                 UINT64      OptionalParam3,
                 PGUEST_REGS GuestRegs,
                 UINT32      CurrentCoreIndex)
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

        CR3_TYPE ProcCr3 = {0};
        ProcCr3.Flags    = OptionalParam3;

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
        InveptSingleContext(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_INVEPT_ALL_CONTEXTS:
    {
        InveptAllContexts();
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
        if (!EptHookRestoreSingleHookToOrginalEntry(OptionalParam1))
        {
            VmcallStatus = STATUS_UNSUCCESSFUL;
        }
        break;
    }
    case VMCALL_ENABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(TRUE);
        break;
    }
    case VMCALL_DISABLE_SYSCALL_HOOK_EFER:
    {
        SyscallHookConfigureEFER(FALSE);
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_READ:
    {
        HvPerformMsrBitmapReadChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_CHANGE_MSR_BITMAP_WRITE:
    {
        HvPerformMsrBitmapWriteChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_RDTSC_EXITING:
    {
        HvSetTscVmexit(TRUE);
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
        HvPerformIoBitmapChange(OptionalParam1);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_SET_HIDDEN_CC_BREAKPOINT:
    {
        CR3_TYPE ProcCr3 = {0};
        ProcCr3.Flags    = OptionalParam2;

        HookResult = EptHookPerformPageHook(OptionalParam1, /* TargetAddress */
                                            ProcCr3);       /* process cr3 */

        VmcallStatus = (HookResult == TRUE) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

        break;
    }
    case VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING:
    {
        HvSetExternalInterruptExiting(FALSE);
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_ENABLE_BREAKPOINT_ON_EXCEPTION_BITMAP:
    {
        //
        // Enable vm-exits on breakpoints (exception bitmap)
        //
        HvSetExceptionBitmap(EXCEPTION_VECTOR_BREAKPOINT);

        VmcallStatus = STATUS_SUCCESS;

        break;
    }
    case VMCALL_DISABLE_BREAKPOINT_ON_EXCEPTION_BITMAP:
    {
        //
        // Disable vm-exits on breakpoints (exception bitmap)
        //
        HvUnsetExceptionBitmap(EXCEPTION_VECTOR_BREAKPOINT);

        VmcallStatus = STATUS_SUCCESS;

        break;
    }
    case VMCALL_UNSET_RDTSC_EXITING:
    {
        HvSetTscVmexit(FALSE);
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
        HvPerformMsrBitmapReadReset();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_MSR_BITMAP_WRITE:
    {
        HvPerformMsrBitmapWriteReset();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_EXCEPTION_BITMAP:
    {
        HvResetExceptionBitmap();
        VmcallStatus = STATUS_SUCCESS;
        break;
    }
    case VMCALL_RESET_IO_BITMAP:
    {
        HvPerformIoBitmapReset();
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
    case VMCALL_VM_EXIT_HALT_SYSTEM_AND_CHANGE_CR3:
    {
        CR3_TYPE ProcCr3 = {0};
        ProcCr3.Flags    = OptionalParam1;

        KdChangeCr3AndTriggerBreakpointHandler(CurrentCoreIndex,
                                               GuestRegs,
                                               DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED,
                                               ProcCr3);
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
    default:
    {
        LogError("Unsupported VMCALL");
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
VmcallTest(UINT64 Param1, UINT64 Param2, UINT64 Param3)
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
                  1);

    return STATUS_SUCCESS;
}
