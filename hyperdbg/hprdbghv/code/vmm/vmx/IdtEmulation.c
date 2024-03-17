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
 * @brief Handle Page-fault exception bitmap VM-exits
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt exit information
 *
 * @return VOID
 */
VOID
IdtEmulationHandlePageFaults(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                             _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    UINT32               ErrorCode          = 0;
    PAGE_FAULT_EXCEPTION PageFaultErrorCode = {0};
    UINT64               PageFaultAddress   = 0;

    //
    // Read the error code and exiting address
    //
    VmxVmread32P(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &ErrorCode);
    PageFaultErrorCode.AsUInt = ErrorCode;

    //
    // Read the page-fault address
    //
    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &PageFaultAddress);

    // LogInfo("#PF Fault = %016llx, Page Fault Code = 0x%x | %s%s%s%s",
    //         PageFaultAddress,
    //         PageFaultErrorCode.AsUInt,
    //         PageFaultErrorCode.Present ? "p" : "",
    //         PageFaultErrorCode.Write ? "w" : "",
    //         PageFaultErrorCode.UserModeAccess ? "u" : "",
    //         PageFaultErrorCode.Execute ? "f" : "");

    // Handle page-faults
    // Check page-fault with user-debugger
    // The page-fault is handled through the user debugger, no need further action
    // NOTE: THE ADDRESS SHOULD BE NULL HERE
    //
    if (!DebuggingCallbackConditionalPageFaultException(VCpu->CoreId, PageFaultAddress, PageFaultErrorCode.AsUInt))
    {
        //
        // The #pf is not related to the debugger, re-inject it
        //
        EventInjectPageFaults(VCpu, InterruptExit, PageFaultAddress, PageFaultErrorCode);
    }
}

/**
 * @brief Handle NMI and exception vm-exits
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt exit information
 * @return VOID
 */
VOID
IdtEmulationHandleExceptionAndNmi(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                  _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit)
{
    //
    // Exception or non-maskable interrupt (NMI). Either:
    //	1: Guest software caused an exception and the bit in the exception bitmap associated with exception's vector was set to 1
    //	2: An NMI was delivered to the logical processor and the "NMI exiting" VM-execution control was 1.
    //
    // VMCS_VMEXIT_INTERRUPTION_INFORMATION shows the exit information about event that occurred and causes this exit
    // Don't forget to read VMCS_VMEXIT_INTERRUPTION_ERROR_CODE in the case of re-injectiong event
    //

    switch (InterruptExit.Vector)
    {
    case EXCEPTION_VECTOR_BREAKPOINT:

        //
        // Handle software breakpoints
        //
        {
            UINT64 GuestRip  = NULL64_ZERO;
            BYTE   TargetMem = NULL_ZERO;

            __vmx_vmread(VMCS_GUEST_RIP, &GuestRip);
            MemoryMapperReadMemorySafe(GuestRip, &TargetMem, sizeof(BYTE));
            if (!EptCheckAndHandleBreakpoint(VCpu) || TargetMem == 0xcc)
            {
                if (!DebuggingCallbackHandleBreakpointException(VCpu->CoreId))
                {
                    //
                    // Don't increment rip
                    //
                    HvSuppressRipIncrement(VCpu);

                    //
                    // Kernel debugger (debugger-mode) is not attached, re-inject the breakpoint
                    //
                    EventInjectBreakpoint();
                }
            }
        }

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
        // Handle page-faults (#PFs)
        //
        IdtEmulationHandlePageFaults(VCpu, InterruptExit);

        break;

    case EXCEPTION_VECTOR_DEBUG_BREAKPOINT:

        if (!DebuggingCallbackHandleDebugBreakpointException(VCpu->CoreId))
        {
            //
            // It's not because of thread change detection, so re-inject it
            //
            EventInjectInterruptOrException(InterruptExit);
        }

        break;

    case EXCEPTION_VECTOR_NMI:

        if (VCpu->EnableExternalInterruptsOnContinue ||
            VCpu->EnableExternalInterruptsOnContinueMtf ||
            VCpu->RegisterBreakOnMtf)
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
            EventInjectInterruptOrException(InterruptExit);
        }

        break;

    default:

        //
        // Re-inject the interrupt/exception, nothing special to handle
        //
        EventInjectInterruptOrException(InterruptExit);

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
        if (VCpu->PendingExternalInterrupts[i] == NULL_ZERO)
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
    BOOLEAN                    Interruptible         = TRUE;
    VMX_INTERRUPTIBILITY_STATE InterruptibilityState = {0};
    RFLAGS                     GuestRflags           = {0};

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
    if (VCpu->EnableExternalInterruptsOnContinue ||
        VCpu->EnableExternalInterruptsOnContinueMtf)
    {
        //
        // Ignore the interrupt as it's suppressed because of instrumentation step-in
        //

        //
        // During developing HyperDbg, we realized that if we just ignore the interrupts completely
        // while we are waiting on 'i' instrumentation step-in command, then the serial device becomes
        // unresponsive, to solve this issue we hold the details of interrupts so we can re-inject
        // and process them when we decide to continue the debuggee (guest interrupt windows is open)
        // this way, the serial device works normally and won't become unresponsive
        //
        IdtEmulationInjectInterruptWhenInterruptWindowIsOpen(VCpu, InterruptExit);

        //
        // avoid incrementing rip
        //
        HvSuppressRipIncrement(VCpu);
    }

    else if (InterruptExit.Valid && InterruptExit.InterruptionType == INTERRUPT_TYPE_EXTERNAL_INTERRUPT)
    {
        VmxVmread64P(VMCS_GUEST_RFLAGS, &GuestRflags.AsUInt);
        VmxVmread32P(VMCS_GUEST_INTERRUPTIBILITY_STATE, &InterruptibilityState.AsUInt);

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
            EventInjectInterruptOrException(InterruptExit);
        }
        else
        {
            //
            // We can't inject interrupt because the guest's state is not interruptible
            // we have to queue it an re-inject it when the interrupt window is opened!
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
        HvSuppressRipIncrement(VCpu);
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
    UNREFERENCED_PARAMETER(VCpu);

    LogError("Why NMI-window exiting happens?");
}

/**
 * @brief Injects a page-fault when interrupt window is open
 *
 * @param VCpu The virtual processor's state
 * @return BOOLEAN
 */
BOOLEAN
IdtEmulationInjectPageFaultWhenInterruptWindowsIsOpen(_Inout_ VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Check if all the injections are done or not
    //
    if (g_PageFaultInjectionAddressFrom > g_PageFaultInjectionAddressTo)
    {
        g_WaitingForInterruptWindowToInjectPageFault = FALSE;
        return FALSE;
    }

    //
    // Inject the page-fault (by cr2)
    //
    EventInjectPageFaultWithCr2(VCpu,
                                g_PageFaultInjectionAddressFrom,
                                g_PageFaultInjectionErrorCode);

    if (MemoryMapperCheckIfPdeIsLargePageOnTargetProcess((PVOID)g_PageFaultInjectionAddressFrom))
    {
        g_PageFaultInjectionAddressFrom = g_PageFaultInjectionAddressFrom + SIZE_2_MB;
    }
    else
    {
        g_PageFaultInjectionAddressFrom = g_PageFaultInjectionAddressFrom + PAGE_SIZE;
    }

    return TRUE;
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
    VMEXIT_INTERRUPT_INFORMATION InterruptExit   = {0};
    BOOLEAN                      InjectPageFault = FALSE;

    //
    // Check if page-fault needs to be injected or not
    //
    if (g_WaitingForInterruptWindowToInjectPageFault)
    {
        InjectPageFault = IdtEmulationInjectPageFaultWhenInterruptWindowsIsOpen(VCpu);
    }

    //
    // Check if another interrupt (page-fault) needed to injected or not
    //
    if (!InjectPageFault)
    {
        for (size_t i = 0; i < PENDING_INTERRUPTS_BUFFER_CAPACITY; i++)
        {
            //
            // Find an empty space
            //
            if (VCpu->PendingExternalInterrupts[i] != NULL_ZERO)
            {
                //
                // Save it for re-injection (interrupt-window exiting)
                //
                InterruptExit.AsUInt = VCpu->PendingExternalInterrupts[i];

                //
                // Free the entry
                //
                VCpu->PendingExternalInterrupts[i] = NULL_ZERO;
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
            // Inject the interrupt/exception
            //

            EventInjectInterruptOrException(InterruptExit);
        }
    }

    //
    // avoid incrementing rip
    //
    HvSuppressRipIncrement(VCpu);
}
