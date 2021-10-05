/**
 * @file ProtectedHvRoutines.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
#include "..\hprdbghv\pch.h"

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
    // Check for kernel debugger (kHyperDbg) presence
    //
    if (g_KernelDebuggerState)
    {
        CurrentMask |= 1 << EXCEPTION_VECTOR_BREAKPOINT;
        CurrentMask |= 1 << EXCEPTION_VECTOR_DEBUG_BREAKPOINT;
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
    }

    //
    // In order to enable External Interrupt Exiting we have to set
    // PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT in vmx
    // pin-based controls (PIN_BASED_VM_EXEC_CONTROL) and also
    // we should enable VM_EXIT_ACK_INTR_ON_EXIT on vmx vm-exit
    // controls (VM_EXIT_CONTROLS), also this function might not
    // always be successful if the guest is not in the interruptible
    // state so it wait for and interrupt-window exiting to re-inject
    // the interrupt into the guest
    //

    //
    // Read the previous flags
    //
    __vmx_vmread(PIN_BASED_VM_EXEC_CONTROL, &PinBasedControls);
    __vmx_vmread(VM_EXIT_CONTROLS, &VmExitControls);

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
    __vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL, PinBasedControls);
    __vmx_vmwrite(VM_EXIT_CONTROLS, VmExitControls);
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
    __vmx_vmread(CPU_BASED_VM_EXEC_CONTROL, &CpuBasedVmExecControls);

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
    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);
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
