/**
 * @file ProtectedHv.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief File for protected hypervisor resources
 * @details Protected Hypervisor Routines are those resource that
 * are used in different parts of the debugger or hypervisor,
 * these resources need extra checks to avoid integrity problems
 *
 * @version 0.1
 * @date 2021-10-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Add extra mask to this resource and write it
 * @details As exception bitmap is a protected resource, this
 * routine makes sure that modifying exception bitmap won't
 * break the debugger's integrity
 *
 * @param VCpu The virtual processor's state
 * @param CurrentMask The mask that debugger wants to write
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for exceptions
 *
 * @return VOID
 */
VOID
ProtectedHvChangeExceptionBitmapWithIntegrityCheck(VIRTUAL_MACHINE_STATE * VCpu, UINT32 CurrentMask, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    //
    // Ask the top-level module to reshape the mask
    //
    if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                   PROTECTED_HV_RESOURCES_EXCEPTION_BITMAP,
                                                   &CurrentMask,
                                                   PassOver))
    {
        return;
    }

    //
    // Check for syscall callback (masking #DBs and #BPs)
    //
    if (g_SyscallCallbackStatus)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT;
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
    }

    //
    // Check for possible EPT Hooks (Hidden Breakpoints)
    //
    if (EptHookGetCountOfEpthooks(FALSE) != 0)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
    }

    //
    // Write the final value
    //
    HvWriteExceptionBitmap(CurrentMask);
}

/**
 * @brief Set exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
ProtectedHvSetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    if (IdtIndex == DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES)
    {
        ExceptionBitmap = 0xffffffff;
    }
    else
    {
        ExceptionBitmap |= 1 << IdtIndex;
    }

    //
    // Set the new value
    //
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(VCpu, ExceptionBitmap, PASSING_OVER_NONE);
}

/**
 * @brief Unset exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
ProtectedHvUnsetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    if (IdtIndex == DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES)
    {
        ExceptionBitmap = 0x0;
    }
    else
    {
        ExceptionBitmap &= ~(1 << IdtIndex);
    }

    //
    // Set the new value
    //
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(VCpu, ExceptionBitmap, PASSING_OVER_NONE);
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing
 * !exception commands
 * @details Should be called in vmx-root
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ProtectedHvResetExceptionBitmapToClearEvents(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Set the new value
    //
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(VCpu, ExceptionBitmap, PASSING_OVER_EXCEPTION_EVENTS);
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing
 * !exception commands
 * @details Should be called in vmx-root
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    ExceptionBitmap = HvReadExceptionBitmap();

    //
    // Unset exception bitmap for #UD
    //
    ExceptionBitmap &= ~(1 << EXCEPTION_VECTOR_UNDEFINED_OPCODE);

    //
    // Set the new value
    //
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(VCpu, ExceptionBitmap, PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK);
}

/**
 * @brief Set the External Interrupt Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the External Interrupt Exiting
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for interrupts

 * @return VOID
 */
VOID
ProtectedHvApplySetExternalInterruptExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 PinBasedControls = 0;
    UINT32 VmExitControls   = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Ask the top-level driver whether to terminate this operation or not
        //
        if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                       PROTECTED_HV_RESOURCES_EXTERNAL_INTERRUPT_EXITING,
                                                       NULL,
                                                       PassOver))
        {
            return;
        }
    }

    //
    // In order to enable External Interrupt Exiting we have to set
    // IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_FLAG in vmx
    // pin-based controls (PIN_BASED_VM_EXEC_CONTROL) and also
    // we should enable IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG
    // on vmx vm-exit controls (VMCS_CTRL_VMEXIT_CONTROLS), also this function
    // might not always be successful if the guest is not in the interruptible
    // state so it wait for and interrupt-window exiting to re-inject
    // the interrupt into the guest
    //

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);
    VmxVmread32P(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmExitControls);

    if (Set)
    {
        PinBasedControls |= IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_FLAG;
        VmExitControls |= IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG;
    }
    else
    {
        PinBasedControls &= ~IA32_VMX_PINBASED_CTLS_EXTERNAL_INTERRUPT_EXITING_FLAG;
        VmExitControls &= ~IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
    VmxVmwrite64(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmExitControls);
}

/**
 * @brief Set the External Interrupt Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID
 */
VOID
ProtectedHvSetExternalInterruptExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvApplySetExternalInterruptExiting(VCpu, Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !interrupt
 *
 * @return VOID
 */
VOID
ProtectedHvExternalInterruptExitingForDisablingInterruptCommands(VIRTUAL_MACHINE_STATE * VCpu)
{
    ProtectedHvApplySetExternalInterruptExiting(VCpu, FALSE, PASSING_OVER_INTERRUPT_EVENTS);
}

/**
 * @brief Set vm-exit for tsc instructions (rdtsc/rdtscp)
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for tsc

 * @return VOID
 */
VOID
ProtectedHvSetTscVmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 CpuBasedVmExecControls = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check the top-level driver's state
        //
        if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                       PROTECTED_HV_RESOURCES_RDTSC_RDTSCP_EXITING,
                                                       NULL,
                                                       PassOver))
        {
            return;
        }
    }

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_FLAG;
    }
    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set vm-exit for mov to debug registers
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for dr

 * @return VOID
 */
VOID
ProtectedHvSetMovDebugRegsVmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 CpuBasedVmExecControls = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check the top-level driver's state
        //
        if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                       PROTECTED_HV_RESOURCES_MOV_TO_DEBUG_REGISTER_EXITING,
                                                       NULL,
                                                       PassOver))
        {
            return;
        }
    }

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set vm-exit for mov to cr0 / cr4 register
 * @details Should be called in vmx-root
 *
 * @param Set or unset the vm-exits
 * @param ControlRegister
 * @param MaskRegister

 * @return VOID
 */
VOID
ProtectedHvSetMovToCrVmexit(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister)
{
    if (ControlRegister == VMX_EXIT_QUALIFICATION_REGISTER_CR0)
    {
        if (Set)
        {
            VmxVmwrite64(VMCS_CTRL_CR0_GUEST_HOST_MASK, MaskRegister);
            VmxVmwrite64(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
        }
        else
        {
            VmxVmwrite64(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
            VmxVmwrite64(VMCS_CTRL_CR0_READ_SHADOW, 0);
        }
    }
    else if (ControlRegister == VMX_EXIT_QUALIFICATION_REGISTER_CR4)
    {
        if (Set)
        {
            VmxVmwrite64(VMCS_CTRL_CR4_GUEST_HOST_MASK, MaskRegister);
            VmxVmwrite64(VMCS_CTRL_CR4_READ_SHADOW, __readcr0());
        }
        else
        {
            VmxVmwrite64(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
            VmxVmwrite64(VMCS_CTRL_CR4_READ_SHADOW, 0);
        }
    }
}

/**
 * @brief Set vm-exit for mov to control registers
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * @param Control Register
 * @param Mask Register

 * @return VOID
 */
VOID
ProtectedHvSetMovControlRegsVmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver, UINT64 ControlRegister, UINT64 MaskRegister)
{
    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check the state of top-level driver
        //
        if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                       PROTECTED_HV_RESOURCES_MOV_CONTROL_REGISTER_EXITING,
                                                       NULL,
                                                       PassOver))
        {
            return;
        }
    }

    ProtectedHvSetMovToCrVmexit(Set, ControlRegister, MaskRegister);
}

/**
 * @brief Set vm-exit for mov to cr3 register
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for dr

 * @return VOID
 */
VOID
ProtectedHvSetMovToCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 CpuBasedVmExecControls = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check the top-level driver's state
        //
        if (VmmCallbackQueryTerminateProtectedResource(VCpu->CoreId,
                                                       PROTECTED_HV_RESOURCES_MOV_TO_CR3_EXITING,
                                                       NULL,
                                                       PassOver))
        {
            return;
        }

        //
        // Check if the trap execution is enabled or not, and whether the
        // uninitialization phase started or not
        //
        if (g_ExecTrapInitialized && !g_ExecTrapUnInitializationStarted)
        {
            //
            // The VMM needs mov2cr3s
            //
            return;
        }
    }

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set the RDTSC/P Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the RDTSC/P Exiting
 * @return VOID
 */
VOID
ProtectedHvSetRdtscExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetTscVmexit(VCpu, Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !tsc
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ProtectedHvDisableRdtscExitingForDisablingTscCommands(VIRTUAL_MACHINE_STATE * VCpu)
{
    ProtectedHvSetTscVmexit(VCpu, FALSE, PASSING_OVER_TSC_EVENTS);
}

/**
 * @brief Set MOV to HW Debug Regs Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the MOV to HW Debug Regs Exiting
 * @return VOID
 */
VOID
ProtectedHvSetMovDebugRegsExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetMovDebugRegsVmexit(VCpu, Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !dr
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ProtectedHvDisableMovDebugRegsExitingForDisablingDrCommands(VIRTUAL_MACHINE_STATE * VCpu)
{
    ProtectedHvSetMovDebugRegsVmexit(VCpu, FALSE, PASSING_OVER_MOV_TO_HW_DEBUG_REGS_EVENTS);
}

/**
 * @brief Clear events of !crwrite
 *
 * @param VCpu The virtual processor's state
 * @param Control Register
 * @param Mask Register
 * @return VOID
 */
VOID
ProtectedHvDisableMovControlRegsExitingForDisablingCrCommands(VIRTUAL_MACHINE_STATE * VCpu, UINT64 ControlRegister, UINT64 MaskRegister)
{
    ProtectedHvSetMovControlRegsVmexit(VCpu, FALSE, PASSING_OVER_MOV_TO_CONTROL_REGS_EVENTS, ControlRegister, MaskRegister);
}

/**
 * @brief Set MOV to CR3 Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the MOV to CR3 Exiting
 * @return VOID
 */
VOID
ProtectedHvSetMov2Cr3Exiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetMovToCr3Vmexit(VCpu, Set, PASSING_OVER_NONE);
}

/**
 * @brief Set MOV to CR0/4 Exiting
 *
 * @param Set or unset the MOV to CR0/4 Exiting
 * @param Control Register
 * @param Mask Register
 * @return VOID
 */
VOID
ProtectedHvSetMov2CrExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister)
{
    ProtectedHvSetMovToCrVmexit(Set, ControlRegister, MaskRegister);
}
