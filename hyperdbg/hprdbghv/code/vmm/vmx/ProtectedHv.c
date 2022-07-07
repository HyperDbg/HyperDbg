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
 * @param CurrentMask The mask that debugger wants to write
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for exceptions
 * 
 * @return VOID  
 */
UINT32
ProtectedHvChangeExceptionBitmapWithIntegrityCheck(UINT32 CurrentMask, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    UINT32 CurrentCoreId = 0;

    CurrentCoreId = KeGetCurrentProcessorNumber();

    //
    // Check if the integrity check is because of clearing
    // events or not, if it's for clearing events, the debugger
    // will automatically set
    //
    if (!(PassOver & PASSING_OVER_EXCEPTION_EVENTS))
    {
        //
        // we have to check for !exception events and apply the mask
        //
        CurrentMask |= DebuggerExceptionEventBitmapMask(CurrentCoreId);
    }

    //
    // Check if it's because of disabling !syscall or !sysret commands
    // or not, if it's because of clearing #UD in these events then we
    // can ignore the checking for this command, otherwise, we have to
    // check it
    //
    if (!(PassOver & PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK))
    {
        //
        // Check if the debugger has events relating to syscall or sysret,
        // if no, we can safely ignore #UDs, otherwise, #UDs should be
        // activated
        //
        if (DebuggerEventListCountByCore(&g_Events->SyscallHooksEferSyscallEventsHead, CurrentCoreId) != 0 ||
            DebuggerEventListCountByCore(&g_Events->SyscallHooksEferSysretEventsHead, CurrentCoreId) != 0)
        {
            //
            // #UDs should be activated
            //
            CurrentMask |= 1 << EXCEPTION_VECTOR_UNDEFINED_OPCODE;
        }
    }

    //
    // Check for kernel or user debugger presence
    //
    if (g_KernelDebuggerState || g_UserDebuggerState)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
        CurrentMask |= 1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT;
    }

    //
    // Check for intercepting #DB by threads tracer
    //
    if (g_GuestState[CurrentCoreId].DebuggingState.ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT;
    }

    //
    // Check for #PF by thread interception mechanism in user debugger
    //
    if (g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_PAGE_FAULT;
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
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
ProtectedHvSetExceptionBitmap(UINT32 IdtIndex)
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
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(ExceptionBitmap, PASSING_OVER_NONE);
}

/**
 * @brief Unset exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
ProtectedHvUnsetExceptionBitmap(UINT32 IdtIndex)
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
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(ExceptionBitmap, PASSING_OVER_NONE);
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing 
 * !exception commands
 * @details Should be called in vmx-root
 * 
 * @return VOID 
 */
VOID
ProtectedHvResetExceptionBitmapToClearEvents()
{
    UINT32 ExceptionBitmap = 0;

    //
    // Set the new value
    //
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(ExceptionBitmap, PASSING_OVER_EXCEPTION_EVENTS);
}

/**
 * @brief Reset exception bitmap in VMCS because of clearing 
 * !exception commands
 * @details Should be called in vmx-root
 * 
 * @return VOID 
 */
VOID
ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands()
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
    ProtectedHvChangeExceptionBitmapWithIntegrityCheck(ExceptionBitmap, PASSING_OVER_UD_EXCEPTIONS_FOR_SYSCALL_SYSRET_HOOK);
}

/**
 * @brief Set the External Interrupt Exiting
 * 
 * @param Set Set or unset the External Interrupt Exiting
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for interrupts

 * @return VOID 
 */
VOID
ProtectedHvApplySetExternalInterruptExiting(BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    ULONG  PinBasedControls = 0;
    ULONG  VmExitControls   = 0;
    UINT32 CurrentCoreId    = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check if the integrity check is because of clearing
        // events or not, if it's for clearing events, the debugger
        // will automatically set
        //
        if (!(PassOver & PASSING_OVER_INTERRUPT_EVENTS))
        {
            CurrentCoreId = KeGetCurrentProcessorNumber();

            //
            // we have to check for !interrupt events and decide whether to
            // ignore this event or not
            //
            if (DebuggerEventListCountByCore(&g_Events->ExternalInterruptOccurredEventsHead, CurrentCoreId) != 0)
            {
                //
                // We should ignore this unset, because !interrupt is enabled for this core
                //

                return;
            }
        }

        //
        // Check if it should remain active for thread or process changing or not
        //
        if (g_GuestState[CurrentCoreId].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange ||
            g_GuestState[CurrentCoreId].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange)
        {
            return;
        }
    }

    //
    // In order to enable External Interrupt Exiting we have to set
    // PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT in vmx
    // pin-based controls (PIN_BASED_VM_EXEC_CONTROL) and also
    // we should enable VM_EXIT_ACK_INTR_ON_EXIT on vmx vm-exit
    // controls (VMCS_CTRL_VMEXIT_CONTROLS), also this function might not
    // always be successful if the guest is not in the interruptible
    // state so it wait for and interrupt-window exiting to re-inject
    // the interrupt into the guest
    //

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmExitControls);

    if (Set)
    {
        PinBasedControls |= PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT;
        VmExitControls |= VM_EXIT_ACK_INTR_ON_EXIT;
    }
    else
    {
        PinBasedControls &= ~PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT;
        VmExitControls &= ~VM_EXIT_ACK_INTR_ON_EXIT;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
    __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmExitControls);
}

/**
 * @brief Set the External Interrupt Exiting
 * 
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID 
 */
VOID
ProtectedHvSetExternalInterruptExiting(BOOLEAN Set)
{
    ProtectedHvApplySetExternalInterruptExiting(Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !interrupt
 * 
 * @return VOID 
 */
VOID
ProtectedHvExternalInterruptExitingForDisablingInterruptCommands()
{
    ProtectedHvApplySetExternalInterruptExiting(FALSE, PASSING_OVER_INTERRUPT_EVENTS);
}

/**
 * @brief Set vm-exit for tsc instructions (rdtsc/rdtscp) 
 * @details Should be called in vmx-root
 * 
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for tsc

 * @return VOID 
 */
VOID
ProtectedHvSetTscVmexit(BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    ULONG  CpuBasedVmExecControls = 0;
    UINT32 CurrentCoreId          = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check if the integrity check is because of clearing
        // events or not, if it's for clearing events, the debugger
        // will automatically set
        //
        if (!(PassOver & PASSING_OVER_TSC_EVENTS))
        {
            CurrentCoreId = KeGetCurrentProcessorNumber();

            //
            // we have to check for !tsc events and decide whether to
            // ignore this event or not
            //
            if (DebuggerEventListCountByCore(&g_Events->TscInstructionExecutionEventsHead, CurrentCoreId) != 0)
            {
                //
                // We should ignore this unset, because !tsc is enabled for this core
                //

                return;
            }
        }

        //
        // Check if transparent mode is enabled
        //
        if (g_TransparentMode)
        {
            //
            // We should ignore it as we want this bit on transparent mode
            //
            return;
        }
    }

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_RDTSC_EXITING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_RDTSC_EXITING;
    }
    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set vm-exit for mov to debug registers 
 * @details Should be called in vmx-root
 * 
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for dr

 * @return VOID 
 */
VOID
ProtectedHvSetMovDebugRegsVmexit(BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    ULONG CpuBasedVmExecControls = 0;
    ULONG CurrentCoreId          = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check if the integrity check is because of clearing
        // events or not, if it's for clearing events, the debugger
        // will automatically set
        //
        if (!(PassOver & PASSING_OVER_MOV_TO_HW_DEBUG_REGS_EVENTS))
        {
            CurrentCoreId = KeGetCurrentProcessorNumber();

            //
            // we have to check for !dr events and decide whether to
            // ignore this event or not
            //
            if (DebuggerEventListCountByCore(&g_Events->DebugRegistersAccessedEventsHead, CurrentCoreId) != 0)
            {
                //
                // We should ignore this unset, because !dr is enabled for this core
                //

                return;
            }
        }

        //
        // Check if thread switching is enabled or not
        //
        if (g_GuestState[CurrentCoreId].DebuggingState.ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
        {
            //
            // We should ignore it as we want this to switch to new thread
            //
            return;
        }
    }

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_MOV_DR_EXITING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_MOV_DR_EXITING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
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
            __vmx_vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, MaskRegister);
            __vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
        }
        else
        {
            __vmx_vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
            __vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, 0);     
        }
    }
    else if (ControlRegister == VMX_EXIT_QUALIFICATION_REGISTER_CR4)
    {
        if (Set)
        {
            __vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, MaskRegister);
            __vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr0());
        }
        else
        {
            __vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
            __vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, 0);
        }
    }
}

/**
 * @brief Set vm-exit for mov to control registers
 * @details Should be called in vmx-root
 *
 * @param Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * @param Control Register
 * @param Mask Register

 * @return VOID
 */
VOID
ProtectedHvSetMovControlRegsVmexit(BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver, UINT64 ControlRegister, UINT64 MaskRegister)
{
    ULONG CpuBasedVmExecControls = 0;
    ULONG CurrentCoreId          = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check if the integrity check is because of clearing
        // events or not, if it's for clearing events, the debugger
        // will automatically set
        //
        if (!(PassOver & PASSING_OVER_MOV_TO_CONTROL_REGS_EVENTS))
        {
            CurrentCoreId = KeGetCurrentProcessorNumber();

            //
            // we have to check for !dr events and decide whether to
            // ignore this event or not
            //
            if (DebuggerEventListCountByCore(&g_Events->ControlRegisterModifiedEventsHead, CurrentCoreId) != 0)
            {
                //
                // We should ignore this unset, because !dr is enabled for this core
                //

                return;
            }
        }
    }
    ProtectedHvSetMovToCrVmexit(Set, ControlRegister, MaskRegister);
}

/**
 * @brief Set vm-exit for mov to cr3 register
 * @details Should be called in vmx-root
 * 
 * @param Set Set or unset the vm-exits
 * @param PassOver Adds some pass over to the checks
 * thus we won't check for dr

 * @return VOID 
 */
VOID
ProtectedHvSetMovToCr3Vmexit(BOOLEAN Set, PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    ULONG CurrentCoreId          = 0;
    ULONG CpuBasedVmExecControls = 0;

    //
    // The protected checks are only performed if the "Set" is "FALSE",
    // because if sb wants to set it to "TRUE" then we're no need to
    // worry about it as it remains enabled
    //
    if (Set == FALSE)
    {
        //
        // Check if process switching is enabled or not
        //
        if (g_GuestState[CurrentCoreId].DebuggingState.ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits)
        {
            //
            // We should ignore it as we want this to switch to new process
            //
            return;
        }

        //
        // Check if use debugger is in intercepting phase for threads or not
        //
        if (g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger)
        {
            //
            // The user debugger needs mov2cr3s
            //
            return;
        }
    }

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_CR3_LOAD_EXITING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_CR3_LOAD_EXITING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set the RDTSC/P Exiting
 * 
 * @param Set Set or unset the RDTSC/P Exiting
 * @return VOID 
 */
VOID
ProtectedHvSetRdtscExiting(BOOLEAN Set)
{
    ProtectedHvSetTscVmexit(Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !tsc
 * 
 * @return VOID 
 */
VOID
ProtectedHvDisableRdtscExitingForDisablingTscCommands()
{
    ProtectedHvSetTscVmexit(FALSE, PASSING_OVER_TSC_EVENTS);
}

/**
 * @brief Set MOV to HW Debug Regs Exiting
 * 
 * @param Set Set or unset the MOV to HW Debug Regs Exiting
 * @return VOID 
 */
VOID
ProtectedHvSetMovDebugRegsExiting(BOOLEAN Set)
{
    ProtectedHvSetMovDebugRegsVmexit(Set, PASSING_OVER_NONE);
}

/**
 * @brief Clear events of !dr
 * 
 * @return VOID 
 */
VOID
ProtectedHvDisableMovDebugRegsExitingForDisablingDrCommands()
{
    ProtectedHvSetMovDebugRegsVmexit(FALSE, PASSING_OVER_MOV_TO_HW_DEBUG_REGS_EVENTS);
}

/**
 * @brief Clear events of !crwrite
 *
 * @param Control Register
 * @param Mask Register
 * @return VOID
 */
VOID
ProtectedHvDisableMovControlRegsExitingForDisablingCrCommands(UINT64 ControlRegister, UINT64 MaskRegister)
{
    ProtectedHvSetMovControlRegsVmexit(FALSE, PASSING_OVER_MOV_TO_CONTROL_REGS_EVENTS, ControlRegister, MaskRegister);
}

/**
 * @brief Set MOV to CR3 Exiting
 * 
 * @param Set Set or unset the MOV to CR3 Exiting
 * @return VOID 
 */
VOID
ProtectedHvSetMov2Cr3Exiting(BOOLEAN Set)
{
    ProtectedHvSetMovToCr3Vmexit(Set, PASSING_OVER_NONE);
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