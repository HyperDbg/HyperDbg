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
 * @brief Perform query for IDT entries
 *
 * @param IdtQueryRequest
 * @param ReadFromVmxRoot
 *
 * @return VOID
 */
VOID
IdtEmulationQueryIdtEntriesRequest(PINTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS IdtQueryRequest,
                                   BOOLEAN                                     ReadFromVmxRoot)
{
    SIDT_ENTRY   IdtrReg;
    KIDT_ENTRY * IdtEntries;

    //
    // Read IDTR register
    //

    if (!ReadFromVmxRoot)
    {
        //
        // Since it's not in VMX Root, we can directly read the IDTR register
        //
        __sidt(&IdtrReg);

        //
        // Get the IDT base address
        //
        IdtEntries = (KIDT_ENTRY *)IdtrReg.IdtBase;
    }
    else
    {
        //
        // Since it's in VMX Root, we need to read the IDTR register from the VMCS
        //
        IdtEntries = (KIDT_ENTRY *)GetGuestIdtr();
    }

    //
    // Gather a list of IDT entries
    //
    for (UINT32 i = 0; i < MAX_NUMBER_OF_IDT_ENTRIES; i++)
    {
        IdtQueryRequest->IdtEntry[i] = (UINT64)((unsigned long long)IdtEntries[i].HighestPart << 32) |
                                       ((unsigned long long)IdtEntries[i].HighPart << 16) |
                                       (unsigned long long)IdtEntries[i].LowPart;
    }
}

/**
 * @brief Create an interrupt gate that points to the supplied interrupt handler
 *
 * @param VCpu The virtual processor's state
 * @param Entry
 *
 * @return VOID
 */
VOID
IdtEmulationCreateInterruptGate(PVOID Handler, SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 * Entry)
{
    // SEGMENT_SELECTOR HostCsSelector = {0, 0, 1};
    //
    // Entry->InterruptStackTable      = 0;
    // Entry->SegmentSelector          = HostCsSelector.AsUInt;
    // Entry->MustBeZero0              = 0;
    // Entry->Type                     = SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE;
    // Entry->MustBeZero1              = 0;
    // Entry->DescriptorPrivilegeLevel = 0;
    // Entry->Present                  = 1;
    // Entry->Reserved                 = 0;

    UINT64 Offset       = (UINT64)Handler;
    Entry->OffsetLow    = (Offset >> 0) & 0xFFFF;
    Entry->OffsetMiddle = (Offset >> 16) & 0xFFFF;
    Entry->OffsetHigh   = (Offset >> 32) & 0xFFFFFFFF;

#if USE_INTERRUPT_STACK_TABLE == TRUE

    //
    // Use first index (IST1)
    //
    Entry->InterruptStackTable = 1;

#else

    //
    // Make sure to unset IST since we didn't use a separate stack pointer
    // on TSS entry at GDT
    //
    Entry->InterruptStackTable = 0;

#endif // USE_INTERRUPT_STACK_TABLE == TRUE
}

/**
 * @brief Prepare Host IDT
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
IdtEmulationPrepareHostIdt(_Inout_ VIRTUAL_MACHINE_STATE * VCpu)
{
    SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 * VmxHostIdt = (SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *)VCpu->HostIdt;
    SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 * WindowsIdt = (SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *)AsmGetIdtBase();

    //
    // Zero the memory
    //
    RtlZeroMemory(VmxHostIdt, HOST_IDT_DESCRIPTOR_COUNT * sizeof(SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64));

    //
    // Copy OS interrupt (IDT) entries
    //
    RtlCopyBytes(VmxHostIdt,
                 WindowsIdt,
                 HOST_IDT_DESCRIPTOR_COUNT * sizeof(SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64));

    /*
    for (size_t i = 0; i < HOST_IDT_DESCRIPTOR_COUNT; i++)
    {
        SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 CurrentEntry = WindowsIdt[i];

        UINT64 Offset = 0;
        Offset |= ((UINT64)CurrentEntry.OffsetLow) << 0;
        Offset |= ((UINT64)CurrentEntry.OffsetMiddle) << 16;
        Offset |= ((UINT64)CurrentEntry.OffsetHigh) << 32;

        // LogInfo("IDT Entry [%d] at: %llx", i, Offset);

        IdtEmulationCreateInterruptGate((PVOID)Offset, &VmxHostIdt[i]);
    }
    */

    //
    // Function related to handling host IDT are a modified version of the following project:
    //      https://github.com/jonomango/hv/blob/main/hv
    //

    //
    // Add customize interrupt handlers
    //
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler0, &VmxHostIdt[0]);
    // IdtEmulationCreateInterruptGate((PVOID)InterruptHandler1, &VmxHostIdt[1]); // #DB
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler2, &VmxHostIdt[2]);
    // IdtEmulationCreateInterruptGate((PVOID)InterruptHandler3, &VmxHostIdt[3]); // #BP
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler4, &VmxHostIdt[4]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler5, &VmxHostIdt[5]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler6, &VmxHostIdt[6]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler7, &VmxHostIdt[7]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler8, &VmxHostIdt[8]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler10, &VmxHostIdt[10]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler11, &VmxHostIdt[11]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler12, &VmxHostIdt[12]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler13, &VmxHostIdt[13]);
    // IdtEmulationCreateInterruptGate((PVOID)InterruptHandler14, &VmxHostIdt[14]); // #PF
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler16, &VmxHostIdt[16]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler17, &VmxHostIdt[17]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler18, &VmxHostIdt[18]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler19, &VmxHostIdt[19]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler20, &VmxHostIdt[20]);
    IdtEmulationCreateInterruptGate((PVOID)InterruptHandler30, &VmxHostIdt[30]);
}

/**
 * @brief Handle Page-fault exception bitmap VM-exits
 *
 * @param VCpu The virtual processor's state
 * @param InterruptExit interrupt exit information
 *
 * @return VOID
 */
VOID
IdtEmulationhandleHostInterrupt(_Inout_ INTERRUPT_TRAP_FRAME * IntrTrapFrame)
{
    UINT64 PageFaultCr2;
    ULONG  CurrentCore;
    CurrentCore                  = KeGetCurrentProcessorNumberEx(NULL);
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[CurrentCore];

    //
    // Store the latest exception vector
    //
    VCpu->LastExceptionOccuredInHost = IntrTrapFrame->vector;

    switch (IntrTrapFrame->vector)
    {
    case EXCEPTION_VECTOR_NMI:

        //
        // host NMIs
        //
        // LogInfo("NMI received!");

        //
        // Check if NMI needs to be injected back to the guest or not
        // Trap frame is sent because as this function unreference this
        // parameter, NULL cannot be sent
        //
        if (!VmxBroadcastHandleNmiCallback((PVOID)IntrTrapFrame, FALSE))
        {
            //
            // Inject NMI when the NMI Window opened
            //
            HvSetNmiWindowExiting(TRUE);
        }

        break;

    case EXCEPTION_VECTOR_PAGE_FAULT:

        //
        // host page-fault
        //
        PageFaultCr2 = __readcr2();

        LogInfo("Page-fault received, rip: %llx, rsp: %llx, error: %llx, CR2: %llx",
                IntrTrapFrame->rip,
                IntrTrapFrame->rsp,
                IntrTrapFrame->error,
                PageFaultCr2);

        break;

    default:

        //
        // host exceptions
        //
        LogInfo("Host exception, rip: %llx, rsp: %llx, error: %llx, vector: %x",
                IntrTrapFrame->rip,
                IntrTrapFrame->rsp,
                IntrTrapFrame->error,
                IntrTrapFrame->vector);

        break;
    }
}

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

        //
        // Check if transparent mode is enabled and if so, then we need to
        // check whether the this trap flag is set because of intercepting
        // the result of a system-call or not
        //
        if (/*g_TransparentMode &&*/
            TransparentCheckAndHandleAfterSyscallTrapFlags(VCpu, HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId())))
        {
            //
            // Being here means that this #DB was caused by a trap flag of
            // the system-call in the transparent-mode, so no need to further handle
            // it (nothing to do)
            //
        }
        else if (!DebuggingCallbackHandleDebugBreakpointException(VCpu->CoreId))
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
    //
    // Inject the NMI into the guest
    //
    EventInjectNmi(VCpu);

    //
    // Disable NMI-window exiting since we have no more NMIs to inject
    //
    HvSetNmiWindowExiting(FALSE);
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
