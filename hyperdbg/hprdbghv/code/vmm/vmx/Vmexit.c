/**
 * @file Vmexit.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The functions for VM-Exit handler for different exit reasons 
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief VM-Exit handler for different exit reasons
 * 
 * @param GuestRegs Registers that are automatically saved by AsmVmexitHandler (HOST_RIP)
 * @return BOOLEAN Return True if VMXOFF executed (not in vmx anymore),
 *  or return false if we are still in vmx (so we should use vm resume)
 */
BOOLEAN
VmxVmexitHandler(_Inout_ PGUEST_REGS GuestRegs)
{
    VMEXIT_INTERRUPT_INFORMATION          InterruptExit         = {0};
    VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification       = {0};
    RFLAGS                                Flags                 = {0};
    UINT64                                GuestPhysicalAddr     = 0;
    UINT64                                GuestRsp              = 0;
    UINT64                                GuestRip              = 0;
    ULONG                                 ExitReason            = 0;
    ULONG                                 ExitQualification     = 0;
    ULONG                                 Rflags                = 0;
    ULONG                                 EcxReg                = 0;
    ULONG                                 ExitInstructionLength = 0;
    ULONG                                 CurrentProcessorIndex = 0;
    BOOLEAN                               Result                = FALSE;
    BOOLEAN                               ShouldEmulateRdtscp   = TRUE;
    VIRTUAL_MACHINE_STATE *               CurrentGuestState     = NULL;

    //
    // *********** SEND MESSAGE AFTER WE SET THE STATE ***********
    //
    CurrentProcessorIndex = KeGetCurrentProcessorNumber();
    CurrentGuestState     = &g_GuestState[CurrentProcessorIndex];

    //
    // Indicates we are in Vmx root mode in this logical core
    //
    CurrentGuestState->IsOnVmxRootMode = TRUE;

    //
    // read the exit reason and exit qualification
    //
    __vmx_vmread(VMCS_EXIT_REASON, &ExitReason);
    ExitReason &= 0xffff;

    //
    // Check if we're operating in transparent-mode or not
    // If yes then we start operating in transparent-mode
    //
    if (g_TransparentMode)
    {
        ShouldEmulateRdtscp = TransparentModeStart(GuestRegs, CurrentProcessorIndex, ExitReason);
    }

    //
    // Increase the RIP by default
    //
    CurrentGuestState->IncrementRip = TRUE;

    //
    // Save the current rip
    //
    __vmx_vmread(VMCS_GUEST_RIP, &GuestRip);
    CurrentGuestState->LastVmexitRip = GuestRip;

    //
    // Set the rsp in general purpose registers structure
    //
    __vmx_vmread(VMCS_GUEST_RSP, &GuestRsp);
    GuestRegs->rsp = GuestRsp;

    //
    // Read the exit qualification
    //

    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification);

    //
    // Debugging purpose
    //
    // LogInfo("VM_EXIT_REASON : 0x%x", ExitReason);
    // LogInfo("VMCS_EXIT_QUALIFICATION : 0x%llx", ExitQualification);
    //

    switch (ExitReason)
    {
    case VMX_EXIT_REASON_TRIPLE_FAULT:
    {
        LogError("Err, triple fault error occurred");

        break;
    }
        //
        // 25.1.2  Instructions That Cause VM Exits Unconditionally
        // The following instructions cause VM exits when they are executed in VMX non-root operation: CPUID, GETSEC,
        // INVD, and XSETBV. This is also true of instructions introduced with VMX, which include: INVEPT, INVVPID,
        // VMCALL, VMCLEAR, VMLAUNCH, VMPTRLD, VMPTRST, VMRESUME, VMXOFF, and VMXON.
        //

    case VMX_EXIT_REASON_EXECUTE_VMCLEAR:
    case VMX_EXIT_REASON_EXECUTE_VMPTRLD:
    case VMX_EXIT_REASON_EXECUTE_VMPTRST:
    case VMX_EXIT_REASON_EXECUTE_VMREAD:
    case VMX_EXIT_REASON_EXECUTE_VMRESUME:
    case VMX_EXIT_REASON_EXECUTE_VMWRITE:
    case VMX_EXIT_REASON_EXECUTE_VMXOFF:
    case VMX_EXIT_REASON_EXECUTE_VMXON:
    case VMX_EXIT_REASON_EXECUTE_VMLAUNCH:
    {
        //
        // cf=1 indicate vm instructions fail
        //
        //__vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);
        //__vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags | 0x1);

        //
        // Handle unconditional vm-exits (inject #ud)
        //
        EventInjectUndefinedOpcode(CurrentProcessorIndex);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_INVEPT:
    case VMX_EXIT_REASON_EXECUTE_INVVPID:
    case VMX_EXIT_REASON_EXECUTE_GETSEC:
    case VMX_EXIT_REASON_EXECUTE_INVD:
    {
        //
        // Handle unconditional vm-exits (inject #ud)
        //
        EventInjectUndefinedOpcode(CurrentProcessorIndex);

        break;
    }
    case VMX_EXIT_REASON_MOV_CR:
    {
        HvHandleControlRegisterAccess(GuestRegs, CurrentProcessorIndex);
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDMSR:
    {
        EcxReg = GuestRegs->rcx & 0xffffffff;

        //
        // Handle vm-exit and perform changes
        //
        MsrHandleRdmsrVmexit(GuestRegs);

        //
        // As the context to event trigger, we send the ecx
        // which is the MSR index
        //
        DebuggerTriggerEvents(RDMSR_INSTRUCTION_EXECUTION, GuestRegs, EcxReg);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_WRMSR:
    {
        EcxReg = GuestRegs->rcx & 0xffffffff;

        //
        // Handle vm-exit and perform changes
        //
        MsrHandleWrmsrVmexit(GuestRegs);

        //
        // As the context to event trigger, we send the ecx
        // which is the MSR index
        //
        DebuggerTriggerEvents(WRMSR_INSTRUCTION_EXECUTION, GuestRegs, EcxReg);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_CPUID:
    {
        HvHandleCpuid(GuestRegs);
        break;
    }

    case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION:
    {
        //
        // Read the I/O Qualification which indicates the I/O instruction
        //
        __vmx_vmread(VMCS_EXIT_QUALIFICATION, &IoQualification);

        //
        // Read Guest's RFLAGS
        //
        __vmx_vmread(VMCS_GUEST_RFLAGS, &Flags);

        //
        // Call the I/O Handler
        //
        IoHandleIoVmExits(GuestRegs, IoQualification, Flags);

        //
        // As the context to event trigger, port address
        //
        if (IoQualification.DirectionOfAccess == AccessIn)
        {
            DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION, GuestRegs, IoQualification.PortNumber);
        }
        else if (IoQualification.DirectionOfAccess == AccessOut)
        {
            DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION, GuestRegs, IoQualification.PortNumber);
        }

        break;
    }
    case VMX_EXIT_REASON_EPT_VIOLATION:
    {
        //
        // Reading guest physical address
        //
        __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

        if (EptHandleEptViolation(GuestRegs, ExitQualification, GuestPhysicalAddr) == FALSE)
        {
            LogError("Err, there were errors in handling EPT violation");
        }

        break;
    }
    case VMX_EXIT_REASON_EPT_MISCONFIGURATION:
    {
        __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

        EptHandleMisconfiguration(GuestPhysicalAddr);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_VMCALL:
    {
        //
        // Handle vm-exits of VMCALLs
        //
        VmxHandleVmcallVmExit(CurrentProcessorIndex, GuestRegs);

        break;
    }
    case VMX_EXIT_REASON_EXCEPTION_OR_NMI:
    {
        //
        // read the exit reason
        //
        __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &InterruptExit);

        //
        // Handle the emulation
        //
        IdtEmulationHandleExceptionAndNmi(CurrentProcessorIndex, InterruptExit, GuestRegs);

        break;
    }
    case VMX_EXIT_REASON_EXTERNAL_INTERRUPT:
    {
        //
        // read the exit reason (for interrupt)
        //
        __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &InterruptExit);

        //
        // Call External Interrupt Handler
        //
        IdtEmulationHandleExternalInterrupt(CurrentProcessorIndex, InterruptExit, GuestRegs);

        break;
    }
    case VMX_EXIT_REASON_INTERRUPT_WINDOW:
    {
        //
        // Call the interrupt-window exiting handler to re-inject the previous
        // interrupts or disable the interrupt-window exiting bit
        //
        IdtEmulationHandleInterruptWindowExiting(CurrentProcessorIndex);

        break;
    }
    case VMX_EXIT_REASON_NMI_WINDOW:
    {
        //
        // Call the NMI-window exiting handler
        //
        IdtEmulationHandleNmiWindowExiting(CurrentProcessorIndex, GuestRegs);

        break;
    }
    case VMX_EXIT_REASON_MONITOR_TRAP_FLAG:
    {
        //
        // General handler to monitor trap flags (MTF)
        //
        MtfHandleVmexit(CurrentProcessorIndex, GuestRegs);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_HLT:
    {
        //
        // We don't wanna halt
        //

        //
        //__halt();
        //
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDTSC:
    {
        //
        // Check whether we are allowed to change
        // the registers and emulate rdtsc or not
        //
        if (ShouldEmulateRdtscp)
        {
            //
            // handle rdtsc (emulate rdtsc)
            //
            CounterEmulateRdtsc(GuestRegs);

            //
            // As the context to event trigger, we send the false which means
            // it's an rdtsc (for rdtscp we set Context to true)
            // Note : Using !tsc command in transparent-mode is not allowed
            //
            DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, FALSE);
        }
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDTSCP:
    {
        //
        // Check whether we are allowed to change
        // the registers and emulate rdtscp or not
        //
        if (ShouldEmulateRdtscp)
        {
            //
            // handle rdtscp (emulate rdtscp)
            //
            CounterEmulateRdtscp(GuestRegs);
            //
            // As the context to event trigger, we send the false which means
            // it's an rdtsc (for rdtscp we set Context to true)
            // Note : Using !tsc command in transparent-mode is not allowed
            //
            DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, TRUE);
        }

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDPMC:
    {
        //
        // handle rdpmc (emulate rdpmc)
        //
        CounterEmulateRdpmc(GuestRegs);

        //
        // As the context to event trigger, we send the NULL
        //
        DebuggerTriggerEvents(PMC_INSTRUCTION_EXECUTION, GuestRegs, NULL);

        break;
    }
    case VMX_EXIT_REASON_MOV_DR:
    {
        //
        // Handle access to debug registers, if we should not ignore it, it is
        // because on detecting thread scheduling we ignore the hardware debug
        // registers modifications
        //
        if (!CurrentGuestState->DebuggingState.ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
        {
            HvHandleMovDebugRegister(CurrentProcessorIndex, GuestRegs);

            //
            // Trigger the event
            // As the context to event trigger, we send NULL
            //
            DebuggerTriggerEvents(DEBUG_REGISTERS_ACCESSED, GuestRegs, NULL);
        }

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_XSETBV:
    {
        //
        // Handle xsetbv (unconditional vm-exit)
        //
        EcxReg = GuestRegs->rcx & 0xffffffff;
        VmxHandleXsetbv(EcxReg, GuestRegs->rdx << 32 | GuestRegs->rax);

        break;
    }
    case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED:
    {
        //
        // Handle the VMX preemption timer vm-exit
        //
        VmxHandleVmxPreemptionTimerVmexit(CurrentProcessorIndex, GuestRegs);
        break;
    }
    default:
    {
        LogError("Err, unknown vmexit, reason : 0x%llx", ExitReason);
        break;
    }
    }

    //
    // Check whether we need to increment the guest's ip or not
    // Also, we should not increment rip if a vmxoff executed
    //
    if (!CurrentGuestState->VmxoffState.IsVmxoffExecuted && CurrentGuestState->IncrementRip)
    {
        HvResumeToNextInstruction();
    }

    //
    // Set indicator of Vmx non root mode to false
    //
    CurrentGuestState->IsOnVmxRootMode = FALSE;

    //
    // Check for vmxoff request
    //
    if (CurrentGuestState->VmxoffState.IsVmxoffExecuted)
        Result = TRUE;

    //
    // Restore the previous time
    //
    if (g_TransparentMode)
    {
        if (ExitReason != VMX_EXIT_REASON_EXECUTE_RDTSC && ExitReason != VMX_EXIT_REASON_EXECUTE_RDTSCP && ExitReason != VMX_EXIT_REASON_EXECUTE_CPUID)
        {
            //
            // We not wanna change the global timer while RDTSC and RDTSCP
            // was the reason of vm-exit
            //
            __writemsr(MSR_IA32_TIME_STAMP_COUNTER, CurrentGuestState->TransparencyState.PreviousTimeStampCounter);
        }
    }

    //
    // By default it's FALSE, if we want to exit vmx then it's TRUE
    //
    return Result;
}
