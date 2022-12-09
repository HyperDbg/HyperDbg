/**
 * @file IdtEmulation.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Handlers of Guest's IDT Emulator
 * @details
 * @version 0.1
 * @date 2020-06-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief re-inject interrupt or exception to the guest
 *
 * @param InterruptExit interrupt info from vm-exit
 *
 * @return BOOLEAN
 */
BOOLEAN
IdtEmulationReInjectInterruptOrException(_In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    ULONG ErrorCode = 0;

    //
    // Re-inject it
    //
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Read the error code
        //
        __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);

        //
        // Write the error code
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief inject #PFs to the guest
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt info from vm-exit
 * @param Address cr2 address
 * @param ErrorCode Page-fault error code
 *
 * @return BOOLEAN
 */
BOOLEAN
IdtEmulationHandlePageFaults(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                             _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                             _In_ UINT64                       Address,
                             _In_ ULONG                        ErrorCode)
{
    //
    // #PF is treated differently, we have to deal with cr2 too.
    //
    PAGE_FAULT_ERROR_CODE PageFaultCode = {0};

    __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &PageFaultCode);

    if (Address == NULL)
    {
        UINT64 PageFaultAddress = 0;

        __vmx_vmread(VMCS_EXIT_QUALIFICATION, &PageFaultAddress);

        //
        // Cr2 is used as the page-fault address
        //
        __writecr2(PageFaultAddress);
    }
    else
    {
        //
        // Cr2 is used as the page-fault address
        //
        __writecr2(Address);
    }

    //
    // Test
    //

    //
    // LogInfo("#PF Fault = %016llx, Page Fault Code = 0x%x", PageFaultAddress, PageFaultCode.Flags);
    //

    VmFuncSuppressRipIncrement(VCpu->CoreId);

    //
    // Re-inject the interrupt/exception
    //
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Write the error code
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief Handle Nmi and expection vm-exits
 *
 * @param VCpu The virtual processor's state
 * @param GuestRegs guest registers
 * @return VOID
 */
VOID
IdtEmulationHandleExceptionAndNmi(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                  _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    ULONG                       ErrorCode            = 0;
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState = &VCpu->DebuggingState;

    //
    // Exception or non-maskable interrupt (NMI). Either:
    //	1: Guest software caused an exception and the bit in the exception bitmap associated with exception's vector was set to 1
    //	2: An NMI was delivered to the logical processor and the "NMI exiting" VM-execution control was 1.
    //
    // VMCS_VMEXIT_INTERRUPTION_INFORMATION shows the exit infromation about event that occurred and causes this exit
    // Don't forget to read VMCS_VMEXIT_INTERRUPTION_ERROR_CODE in the case of re-injectiong event
    //

    switch (InterruptExit.Vector)
    {
    case EXCEPTION_VECTOR_BREAKPOINT:

        //
        // Handle software breakpoints
        //
        BreakpointHandleBpTraps(VCpu);

        break;

    case EXCEPTION_VECTOR_UNDEFINED_OPCODE:

        //
        // Handle the #UD, checking if this exception was intentional.
        //
        if (!SyscallHookHandleUD(VCpu))
        {
            //
            // If this #UD was found to be unintentional, inject a #UD interruption into the guest.
            //
            EventInjectUndefinedOpcode(VCpu);
        }

        break;

    case EXCEPTION_VECTOR_PAGE_FAULT:

        //
        // Read the error code
        //
        __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);

        //
        // Handle page-faults
        //
        if (g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger &&
            AttachingCheckPageFaultsWithUserDebugger(VCpu,
                                                     InterruptExit,
                                                     NULL,
                                                     ErrorCode))
        {
            //
            // The page-fault is handled through the user debugger, no need further action
            //
        }
        else
        {
            //
            // The #pf is not related to our debugger
            //
            IdtEmulationHandlePageFaults(VCpu, InterruptExit, NULL, ErrorCode);
        }

        break;

    case EXCEPTION_VECTOR_DEBUG_BREAKPOINT:

        //
        // Check whether it is because of thread change detection or not
        //
        if (CurrentDebuggerState->ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
        {
            //
            // This way of handling has a problem, if the user set to change
            // the thread and instead of using 'g', it pressed the 'p' to
            // set or a trap happens somewhere then will be ignored
            // it because we don't know the origin of this debug breakpoint
            // and it only happens on '.thread2' command, the correct way
            // to handle it is to find the exact hw debug register that caused
            // this vm-exit, but it's a really rare case, so we left it without
            // handling this case
            //
            ThreadHandleThreadChange(&VCpu->DebuggingState);
        }
        else if (g_UserDebuggerState == TRUE &&
                 (g_IsWaitingForUserModeModuleEntrypointToBeCalled || g_IsWaitingForReturnAndRunFromPageFault))
        {
            //
            // Handle for user-mode attaching mechanism
            //
            AttachingHandleEntrypointDebugBreak(VCpu);
        }
        else if (g_KernelDebuggerState == TRUE)
        {
            //
            // Handle debug events (breakpoint, traps, hardware debug register when kernel
            // debugger is attached.)
            //
            KdHandleDebugEventsWhenKernelDebuggerIsAttached(VCpu);
        }
        else if (UdCheckAndHandleBreakpointsAndDebugBreaks(VCpu,
                                                           DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
                                                           NULL))
        {
            //
            // if the above function returns true, no need for further action
            // it's handled in the user debugger
            //
        }
        else
        {
            //
            // It's not because of thread change detection, so re-inject it
            //
            IdtEmulationReInjectInterruptOrException(InterruptExit);
        }

        break;

    case EXCEPTION_VECTOR_NMI:

        if (CurrentDebuggerState->EnableExternalInterruptsOnContinue ||
            CurrentDebuggerState->EnableExternalInterruptsOnContinueMtf ||
            CurrentDebuggerState->InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
        {
            //
            // Ignore the nmi
            //
        }
        else
        {
            //
            // Re-inject the interrupt/exception because it doesn't relate to us
            //
            IdtEmulationReInjectInterruptOrException(InterruptExit);
        }

        break;

    default:

        //
        // Re-inject the interrupt/exception, nothing special to handle
        //
        IdtEmulationReInjectInterruptOrException(InterruptExit);

        break;
    }
}

/**
 * @brief if the guest is not interruptible, then we save the details of each
 * interrupt so we can re-inject them to the guest whenever the interrupt window
 * is open
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt info from vm-exit
 * @return BOOLEAN
 */
BOOLEAN
IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                                     _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    BOOLEAN FoundAPlaceForFutureInjection = FALSE;

    //
    // We can't inject interrupt because the guest's state is not interruptible
    // we have to queue it an re-inject it when the interrupt window is opened !
    //
    for (size_t i = 0; i < PENDING_INTERRUPTS_BUFFER_CAPACITY; i++)
    {
        //
        // Find an empty space
        //
        if (VCpu->PendingExternalInterrupts[i] == NULL)
        {
            //
            // Save it for future re-injection (interrupt-window exiting)
            //
            VCpu->PendingExternalInterrupts[i] = InterruptExit.AsUInt;
            FoundAPlaceForFutureInjection      = TRUE;
            break;
        }
    }

    return FoundAPlaceForFutureInjection;
}

/**
 * @brief Handle process or thread switches
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt info from vm-exit
 *
 * @return BOOLEAN
 */
BOOLEAN
IdtEmulationCheckProcessOrThreadChange(_In_ VIRTUAL_MACHINE_STATE *      VCpu,
                                       _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState = &VCpu->DebuggingState;

    //
    // Check whether intercepting this process or thread is active or not,
    // Windows fires a clk interrupt on core 0 and fires IPI on other cores
    // to change a thread
    //
    if ((CurrentDebuggerState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange ||
         CurrentDebuggerState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange) &&
        ((VCpu->CoreId == 0 && InterruptExit.Vector == CLOCK_INTERRUPT) ||
         (VCpu->CoreId != 0 && InterruptExit.Vector == IPI_INTERRUPT)))
    {
        //
        // We only handle interrupts that are related to the clock-timer interrupt
        //
        if (CurrentDebuggerState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange)
        {
            return ThreadHandleThreadChange(&VCpu->DebuggingState);
        }
        else
        {
            return ProcessHandleProcessChange(&VCpu->DebuggingState);
        }
    }

    //
    // Not handled here
    //
    return FALSE;
}

/**
 * @brief external-interrupt vm-exit handler
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt info from vm-exit
 *
 * @return VOID
 */
VOID
IdtEmulationHandleExternalInterrupt(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                    _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    BOOLEAN                     Interruptible         = TRUE;
    VMX_INTERRUPTIBILITY_STATE  InterruptibilityState = {0};
    RFLAGS                      GuestRflags           = {0};
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState  = &VCpu->DebuggingState;

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
    if (CurrentDebuggerState->EnableExternalInterruptsOnContinue ||
        CurrentDebuggerState->EnableExternalInterruptsOnContinueMtf)
    {
        //
        // Ignore the interrupt as it's suppressed supressed because of instrumentation step-in
        //

        //
        // During developing HyperDbg, we realized that if we just ignore the interrupts completely
        // while we are waiting on 'i' instrumentation step-in command, then the serial device becomes
        // unresposive, to solve this issue we hold the details of interrupts so we can re-inject
        // and process them when we decide to continue the debuggee (guest interrupt windows is open)
        // this way, the serial device works normally and won't become unresponsive
        //
        IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(VCpu, InterruptExit);

        //
        // avoid incrementing rip
        //
        VmFuncSuppressRipIncrement(VCpu->CoreId);
    }
    else if (InterruptExit.Valid && InterruptExit.InterruptionType == INTERRUPT_TYPE_EXTERNAL_INTERRUPT)
    {
        __vmx_vmread(VMCS_GUEST_RFLAGS, &GuestRflags);
        __vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE, &InterruptibilityState);

        //
        // External interrupts cannot be injected into the
        // guest if guest isn't interruptible (e.g.: guest
        // is blocked by "mov ss", or EFLAGS.IF == 0).
        //
        Interruptible = GuestRflags.InterruptEnableFlag && !InterruptibilityState.BlockingByMovSs;

        if (Interruptible)
        {
            //
            // Re-inject the interrupt/exception
            //
            IdtEmulationReInjectInterruptOrException(InterruptExit);
        }
        else
        {
            //
            // We can't inject interrupt because the guest's state is not interruptible
            // we have to queue it an re-inject it when the interrupt window is opened !
            //
            IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(VCpu, InterruptExit);

            //
            // Enable Interrupt-window exiting.
            //
            HvSetInterruptWindowExiting(TRUE);
        }

        //
        // avoid incrementing rip
        //
        VmFuncSuppressRipIncrement(VCpu->CoreId);
    }
    else
    {
        Interruptible = FALSE;

        LogError("Err, why we are here? it's a vm-exit due to the external"
                 "interrupt and its type is not external interrupt? weird!");
    }
}

/**
 * @brief Handle NMI-window exitings
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
IdtEmulationHandleNmiWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu)
{
    LogError("Why NMI-window exiting happens?");
}

/**
 * @brief Handle interrupt-window exitings
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
IdtEmulationHandleInterruptWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu)
{
    VMEXIT_INTERRUPT_INFORMATION InterruptExit = {0};
    ULONG                        ErrorCode     = 0;

    //
    // Find the pending interrupt to inject
    //

    for (size_t i = 0; i < PENDING_INTERRUPTS_BUFFER_CAPACITY; i++)
    {
        //
        // Find an empty space
        //
        if (VCpu->PendingExternalInterrupts[i] != NULL)
        {
            //
            // Save it for re-injection (interrupt-window exiting)
            //
            InterruptExit.AsUInt = VCpu->PendingExternalInterrupts[i];

            //
            // Free the entry
            //
            VCpu->PendingExternalInterrupts[i] = NULL;
            break;
        }
    }

    if (InterruptExit.AsUInt == 0)
    {
        //
        // Nothing left in pending state, let's disable the interrupt window exiting
        //
        HvSetInterruptWindowExiting(FALSE);
    }
    else
    {
        //
        // Re-inject the interrupt/exception
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, InterruptExit.AsUInt);

        //
        // re-write error code (if any)
        //
        if (InterruptExit.ErrorCodeValid)
        {
            //
            // Read the error code
            //
            __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);

            //
            // Write the error code
            //
            __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
        }
    }

    //
    // avoid incrementing rip
    //
    VmFuncSuppressRipIncrement(VCpu->CoreId);
}
