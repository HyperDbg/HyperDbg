/**
 * @file IdtEmulation.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Handlers of Guest's IDT Emulator 
 * @details
 * @version 0.1
 * @date 2020-06-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief re-inject interrupt or exception to the guest
 * 
 * @param InterruptExit interrupt info from vm-exit
 * 
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationReInjectInterruptOrException(VMEXIT_INTERRUPT_INFO InterruptExit)
{
    ULONG ErrorCode = 0;

    //
    // Re-inject it
    //
    __vmx_vmwrite(VM_ENTRY_INTR_INFO, InterruptExit.Flags);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Read the error code
        //
        __vmx_vmread(VM_EXIT_INTR_ERROR_CODE, &ErrorCode);

        //
        // Write the error code
        //
        __vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief inject #PFs to the guest
 * 
 * @param CurrentProcessorIndex processor index
 * @param InterruptExit interrupt info from vm-exit
 * 
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationHandlePageFaults(UINT32 CurrentProcessorIndex, VMEXIT_INTERRUPT_INFO InterruptExit)
{
    ULONG ErrorCode = 0;

    //
    // #PF is treated differently, we have to deal with cr2 too.
    //
    PAGE_FAULT_ERROR_CODE PageFaultCode = {0};

    __vmx_vmread(VM_EXIT_INTR_ERROR_CODE, &PageFaultCode);

    UINT64 PageFaultAddress = 0;

    __vmx_vmread(EXIT_QUALIFICATION, &PageFaultAddress);

    //
    // Test
    //

    //
    // LogInfo("#PF Fault = %016llx, Page Fault Code = 0x%x", PageFaultAddress, PageFaultCode.All);
    //

    //
    // Cr2 is used as the page-fault address
    //
    __writecr2(PageFaultAddress);

    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    //
    // Re-inject the interrupt/exception
    //
    __vmx_vmwrite(VM_ENTRY_INTR_INFO, InterruptExit.Flags);

    //
    // re-write error code (if any)
    //
    if (InterruptExit.ErrorCodeValid)
    {
        //
        // Read the error code
        //
        __vmx_vmread(VM_EXIT_INTR_ERROR_CODE, &ErrorCode);

        //
        // Write the error code
        //
        __vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }
}

/**
 * @brief Handle Nmi and expection vm-exits
 * 
 * @param CurrentProcessorIndex index of processor
 * @param InterruptExit vm-exit information for interrupt
 * @param GuestRegs guest registers
 * @return VOID 
 */
VOID
IdtEmulationHandleExceptionAndNmi(UINT32 CurrentProcessorIndex, VMEXIT_INTERRUPT_INFO InterruptExit, PGUEST_REGS GuestRegs)
{
    //
    // This type of vm-exit, can be either because of an !exception event,
    // or it might be because we triggered APIC or X2APIC to generate an
    // NMI, we want to halt the debuggee. We perform the checks here to
    // avoid triggering an event for NMIs when the debuggee requested it
    //
    if (InterruptExit.InterruptionType == INTERRUPT_TYPE_NMI &&
        InterruptExit.Vector == EXCEPTION_VECTOR_NMI)
    {
        //
        // Check if we're waiting for an NMI on this core and if the guest is NOT in
        // a instrument step-in ('i' command) routine
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.WaitingForNmi &&
            !g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
        {
            g_GuestState[CurrentProcessorIndex].DebuggingState.WaitingForNmi = FALSE;
            KdHandleNmi(CurrentProcessorIndex, GuestRegs);
            return;
        }
    }

    //
    // Also, avoid exception when we're running instrumentation step-in
    //
    if (g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
    {
        //
        // We ignore it because an MTF should handle it as it's an instrumentation step-in
        //
        return;
    }

    //
    // *** When we reached here it means that this is not a NMI cause by guest,
    // probably an event ***
    //

    //
    // Trigger the event
    //
    // As the context to event trigger, we send the vector
    // or IDT Index
    //
    DebuggerTriggerEvents(EXCEPTION_OCCURRED, GuestRegs, InterruptExit.Vector);

    //
    // Now, we check if the guest enabled MTF for instrumentation stepping
    // This is because based on Intel SDM :
    // If the "monitor trap flag" VM-execution control is 1 and VM entry is
    // injecting a vectored event, an MTF VM exit is pending on the instruction
    // boundary before the first instruction following the VM entry
    // and,
    // If VM entry is injecting a pending MTF VM exit, an MTF VM exit is pending on the
    // instruction boundary before the first instruction following the VM entry
    // This is the case even if the "monitor trap flag" VM-execution control is 0
    //
    // So, we'll ignore the injection of Exception in this case
    //
    if (g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
    {
        return;
    }

    //
    // Exception or non-maskable interrupt (NMI). Either:
    //	1: Guest software caused an exception and the bit in the exception bitmap associated with exception's vector was set to 1
    //	2: An NMI was delivered to the logical processor and the "NMI exiting" VM-execution control was 1.
    //
    // VM_EXIT_INTR_INFO shows the exit infromation about event that occurred and causes this exit
    // Don't forget to read VM_EXIT_INTR_ERROR_CODE in the case of re-injectiong event
    //

    switch (InterruptExit.Vector)
    {
    case EXCEPTION_VECTOR_BREAKPOINT:

        //
        // Handle software breakpoints
        //
        BreakpointHandleBpTraps(CurrentProcessorIndex, GuestRegs);

        break;

    case EXCEPTION_VECTOR_UNDEFINED_OPCODE:

        //
        // Handle the #UD, checking if this exception was intentional.
        //
        if (!SyscallHookHandleUD(GuestRegs, CurrentProcessorIndex))
        {
            //
            // If this #UD was found to be unintentional, inject a #UD interruption into the guest.
            //
            EventInjectUndefinedOpcode(CurrentProcessorIndex);
        }

        break;

    case EXCEPTION_VECTOR_PAGE_FAULT:

        //
        // Handle page-faults
        //
        IdtEmulationHandlePageFaults(CurrentProcessorIndex, InterruptExit);

        break;

    case EXCEPTION_VECTOR_DEBUG_BREAKPOINT:

        //
        // Check whether it is because of thread change detection or not
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
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
            ThreadHandleThreadChange(CurrentProcessorIndex, GuestRegs);
        }
        else if (g_KernelDebuggerState == TRUE)
        {
            //
            // Handle debug events (breakpoint, traps, hardware debug register when kernel
            // debugger is attached.)
            //
            KdHandleDebugEventsWhenKernelDebuggerIsAttached(CurrentProcessorIndex, GuestRegs);
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

        if (g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinue ||
            g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinueMtf ||
            g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
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
 * @param InterruptExit interrupt info from vm-exit
 * @param CurrentProcessorIndex processor index
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(VMEXIT_INTERRUPT_INFO InterruptExit, UINT32 CurrentProcessorIndex)
{
    BOOLEAN FoundAPlaceForFutureInjection;

    FoundAPlaceForFutureInjection = FALSE;

    //
    // We can't inject interrupt because the guest's state is not interruptible
    // we have to queue it an re-inject it when the interrupt window is opened !
    //
    for (size_t i = 0; i < PENDING_INTERRUPTS_BUFFER_CAPACITY; i++)
    {
        //
        // Find an empty space
        //
        if (g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[i] == NULL)
        {
            //
            // Save it for future re-injection (interrupt-window exiting)
            //
            g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[i] = InterruptExit.Flags;
            FoundAPlaceForFutureInjection                                    = TRUE;
            break;
        }
    }

    return FoundAPlaceForFutureInjection;
}

/**
 * @brief Handle process or thread switches
 * 
 * @param CurrentProcessorIndex processor index
 * @param InterruptExit interrupt info from vm-exit
 * @param GuestRegs guest context
 * 
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationCheckProcessOrThreadChange(UINT32 CurrentProcessorIndex, VMEXIT_INTERRUPT_INFO InterruptExit, PGUEST_REGS GuestRegs)
{
    //
    // Check whether intercepting this process or thread is active or not,
    // Windows fires a clk interrupt on core 0 and fires IPI on other cores
    // to change a thread
    //
    if ((g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange || g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange) &&
        ((CurrentProcessorIndex == 0 && InterruptExit.Vector == CLOCK_INTERRUPT) ||
         (CurrentProcessorIndex != 0 && InterruptExit.Vector == IPI_INTERRUPT)))
    {
        //
        // We only handle interrupts that are related to the clock-timer interrupt
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange)
        {
            return ThreadHandleThreadChange(CurrentProcessorIndex, GuestRegs);
        }
        else
        {
            return ProcessHandleProcessChange(CurrentProcessorIndex, GuestRegs);
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
 * @param CurrentProcessorIndex processor index
 * @param InterruptExit interrupt info from vm-exit
 * @param GuestRegs guest contexts
 * 
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationHandleExternalInterrupt(UINT32 CurrentProcessorIndex, VMEXIT_INTERRUPT_INFO InterruptExit, PGUEST_REGS GuestRegs)
{
    BOOLEAN                Interruptible         = TRUE;
    INTERRUPTIBILITY_STATE InterruptibilityState = {0};
    RFLAGS                 GuestRflags           = {0};
    ULONG                  ErrorCode             = 0;

    //
    // Check process or thread change detections
    // we cannot ignore injecting the interrupt to the guest if the target interrupt
    // and process or thread proved to cause a system halt. it halts the system as
    // we Windows expects to switch the thread while we're forcing it to not do it
    //
    IdtEmulationCheckProcessOrThreadChange(CurrentProcessorIndex, InterruptExit, GuestRegs);

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
    if ((g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinue ||
         g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinueMtf))
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
        IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(InterruptExit, CurrentProcessorIndex);

        //
        // avoid incrementing rip
        //
        g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;
    }
    else if (InterruptExit.Valid && InterruptExit.InterruptionType == INTERRUPT_TYPE_EXTERNAL_INTERRUPT)
    {
        __vmx_vmread(GUEST_RFLAGS, &GuestRflags);
        __vmx_vmread(GUEST_INTERRUPTIBILITY_INFO, &InterruptibilityState);

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
            IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(InterruptExit, CurrentProcessorIndex);

            //
            // Enable Interrupt-window exiting.
            //
            HvSetInterruptWindowExiting(TRUE);
        }

        //
        // avoid incrementing rip
        //
        g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;
    }
    else
    {
        Interruptible = FALSE;

        LogError("Err, why we are here ? it's a vm-exit due to the external"
                 "interrupt and its type is not external interrupt? weird!");
    }

    //
    // Trigger the event
    //
    // As the context to event trigger, we send the vector index
    //
    // Keep in mind that interrupt might be inseted in pending list
    // because the guest is not in a interruptible state and will
    // be re-injected when the guest is ready for interrupts
    //
    DebuggerTriggerEvents(EXTERNAL_INTERRUPT_OCCURRED, GuestRegs, InterruptExit.Vector);

    //
    // Signalize whether the interrupt was handled and re-injected or the
    // guest is not in a state of handling the interrupt and we have to
    // wait for a interrupt windows-exiting
    //
    return Interruptible;
}

/**
 * @brief Handle interrupt-window exitings
 * 
 * @param CurrentProcessorIndex processor index
 * @return VOID 
 */
VOID
IdtEmulationHandleInterruptWindowExiting(UINT32 CurrentProcessorIndex)
{
    VMEXIT_INTERRUPT_INFO InterruptExit = {0};
    ULONG                 ErrorCode     = 0;

    //
    // Find the pending interrupt to inject
    //

    for (size_t i = 0; i < PENDING_INTERRUPTS_BUFFER_CAPACITY; i++)
    {
        //
        // Find an empty space
        //
        if (g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[i] != NULL)
        {
            //
            // Save it for re-injection (interrupt-window exiting)
            //
            InterruptExit.Flags = g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[i];

            //
            // Free the entry
            //
            g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[i] = NULL;
            break;
        }
    }

    if (InterruptExit.Flags == 0)
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
        __vmx_vmwrite(VM_ENTRY_INTR_INFO, InterruptExit.Flags);

        //
        // re-write error code (if any)
        //
        if (InterruptExit.ErrorCodeValid)
        {
            //
            // Read the error code
            //
            __vmx_vmread(VM_EXIT_INTR_ERROR_CODE, &ErrorCode);

            //
            // Write the error code
            //
            __vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
        }
    }

    //
    // avoid incrementing rip
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;
}
