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
#include "pch.h"

/**
 * @brief Handle Nmi and expection vm-exits
 * 
 * @param InterruptExit vm-exit information for interrupt
 * @param CurrentProcessorIndex index of processor
 * @param GuestRegs guest registers
 * @return VOID 
 */
VOID
IdtEmulationHandleExceptionAndNmi(VMEXIT_INTERRUPT_INFO InterruptExit, UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    ULONG                            ErrorCode = 0;
    ULONG64                          GuestRip;
    PLIST_ENTRY                      TempList      = 0;
    BOOLEAN                          IsHandled     = FALSE;
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag = {0};

    //
    // Exception or non-maskable interrupt (NMI). Either:
    //	1: Guest software caused an exception and the bit in the exception bitmap associated with exception's vector was set to 1
    //	2: An NMI was delivered to the logical processor and the "NMI exiting" VM-execution control was 1.
    //
    // VM_EXIT_INTR_INFO shows the exit infromation about event that occured and causes this exit
    // Don't forget to read VM_EXIT_INTR_ERROR_CODE in the case of re-injectiong event
    //

    if (InterruptExit.InterruptionType == INTERRUPT_TYPE_SOFTWARE_EXCEPTION && InterruptExit.Vector == EXCEPTION_VECTOR_BREAKPOINT)
    {
        //
        // Reading guest's RIP
        //
        __vmx_vmread(GUEST_RIP, &GuestRip);

        //
        // Check whether the breakpoint was due to a !epthook command or not
        //
        TempList = &g_EptState->HookedPagesList;

        while (&g_EptState->HookedPagesList != TempList->Flink)
        {
            TempList                            = TempList->Flink;
            PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

            if (HookedEntry->IsExecutionHook)
            {
                for (size_t i = 0; i < HookedEntry->CountOfBreakpoints; i++)
                {
                    if (HookedEntry->BreakpointAddresses[i] == GuestRip)
                    {
                        //
                        // We found an address that matches the details, let's trigger the event
                        //
                        // As the context to event trigger, we send the rip
                        // of where triggered this event
                        //
                        DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_CC, GuestRegs, GuestRip);

                        //
                        // Restore to its orginal entry for one instruction
                        //
                        EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);

                        //
                        // Next we have to save the current hooked entry to restore on the next instruction's vm-exit
                        //
                        g_GuestState[KeGetCurrentProcessorNumber()].MtfEptHookRestorePoint = HookedEntry;

                        //
                        // We have to set Monitor trap flag and give it the HookedEntry to work with
                        //
                        HvSetMonitorTrapFlag(TRUE);

                        //
                        // Indicate that we handled the ept violation
                        //
                        IsHandled = TRUE;

                        //
                        // Get out of the loop
                        //
                        break;
                    }
                }
            }
        }

        //
        // notify the user about #BP
        //
        //LogInfo("Breakpoint Hit (Process Id : 0x%x) at : %llx ", PsGetCurrentProcessId(), GuestRip);
        //
        g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

        //
        // re-inject #BP back to the guest if not handled by the hidden breakpoint
        //
        if (!IsHandled)
        {
            if (g_KernelDebuggerState)
            {
                //
                // Kernel debugger is attached, let's halt everything
                //
                ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;
                KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                                      GuestRegs,
                                                      DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                      &ContextAndTag);

                g_GuestState[CurrentProcessorIndex].IncrementRip = TRUE;
            }
            else
            {
                EventInjectBreakpoint();
            }
        }
    }
    else if (InterruptExit.InterruptionType == INTERRUPT_TYPE_HARDWARE_EXCEPTION && InterruptExit.Vector == EXCEPTION_VECTOR_UNDEFINED_OPCODE)
    {
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
    }
    else if (InterruptExit.Vector == EXCEPTION_VECTOR_PAGE_FAULT)
    {
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
    else if (g_KernelDebuggerState == TRUE &&
             InterruptExit.Vector == EXCEPTION_VECTOR_DEBUG_BREAKPOINT)
    {
        //
        // It's a breakpoint and should be handled by the kernel debugger
        //
        ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;
        KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT,
                                              &ContextAndTag);
    }
    else if (InterruptExit.Vector == EXCEPTION_VECTOR_DEBUG_BREAKPOINT)
    {
        //
        // Check whether it is because of thread change detection or not
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggerUserModeSteppingDetails.DebugRegisterInterceptionState)
        {
            SteppingsHandleThreadChanges(GuestRegs, CurrentProcessorIndex);
            // HvSetMonitorTrapFlag(TRUE);
            //g_GuestState[CurrentProcessorIndex].MtfTest = TRUE;
        }
        else
        {
            //
            // It's not because of thread change detection, so re-inject it
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
    }
    else if (InterruptExit.InterruptionType == INTERRUPT_TYPE_NMI &&
             InterruptExit.Vector == EXCEPTION_VECTOR_NMI)
    {
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.WaitingForNmi)
        {
            g_GuestState[CurrentProcessorIndex].DebuggingState.WaitingForNmi = FALSE;
            KdHandleNmi(CurrentProcessorIndex, GuestRegs);
        }
        else if (g_GuestState[CurrentProcessorIndex].DebuggingState.EnableInterruptFlagOnContinue)
        {
            //
            // Ignore the nmi
            //
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
    }
    else
    {
        //
        // Test
        //
        //LogInfo("Interrupt vector : 0x%x", InterruptExit.Vector);
        //

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
}

/**
 * @brief external-interrupt vm-exit handler
 * 
 * @param InterruptExit interrupt info from vm-exit
 * @param CurrentProcessorIndex processor index
 * @return BOOLEAN 
 */
BOOLEAN
IdtEmulationHandleExternalInterrupt(VMEXIT_INTERRUPT_INFO InterruptExit, UINT32 CurrentProcessorIndex)
{
    BOOLEAN                Interruptible         = TRUE;
    INTERRUPTIBILITY_STATE InterruptibilityState = {0};
    RFLAGS                 GuestRflags           = {0};
    ULONG                  ErrorCode             = 0;

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

    if (InterruptExit.Valid && InterruptExit.InterruptionType == INTERRUPT_TYPE_EXTERNAL_INTERRUPT)
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
        else
        {
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
                    break;
                }
            }

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
        LogError("Why we are here ? It's a vm-exit due to the external"
                 "interrupt and its type is not external interrupt? weird!");

        return FALSE;
    }

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
