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
    UINT32                  ExitReason          = 0;
    BOOLEAN                 Result              = FALSE;
    BOOLEAN                 ShouldEmulateRdtscp = TRUE;
    VIRTUAL_MACHINE_STATE * VCpu                = NULL;

    //
    // *********** SEND MESSAGE AFTER WE SET THE STATE ***********
    //
    VCpu = &g_GuestState[KeGetCurrentProcessorNumberEx(NULL)];

    //
    // Set the registers
    //
    VCpu->Regs = GuestRegs;

    //
    // Indicates we are in Vmx root mode in this logical core
    //
    VCpu->IsOnVmxRootMode = TRUE;

    //
    // read the exit reason and exit qualification
    //
    VmxVmread32P(VMCS_EXIT_REASON, &ExitReason);
    ExitReason &= 0xffff;

    //
    // Check if we're operating in transparent-mode or not
    // If yes then we start operating in transparent-mode
    //
    if (g_TransparentMode)
    {
        ShouldEmulateRdtscp = TransparentModeStart(VCpu, ExitReason);
    }

    //
    // Increase the RIP by default
    //
    VCpu->IncrementRip = TRUE;

    //
    // Save the current rip
    //
    __vmx_vmread(VMCS_GUEST_RIP, &VCpu->LastVmexitRip);

    //
    // Set the rsp in general purpose registers structure
    //
    __vmx_vmread(VMCS_GUEST_RSP, &VCpu->Regs->rsp);

    //
    // Read the exit qualification
    //
    VmxVmread32P(VMCS_EXIT_QUALIFICATION, &VCpu->ExitQualification);

    //
    // Debugging purpose
    //
    // LogInfo("VM_EXIT_REASON : 0x%x", ExitReason);
    // LogInfo("VMCS_EXIT_QUALIFICATION : 0x%llx", VCpu->ExitQualification);
    //
    switch (ExitReason)
    {
    case VMX_EXIT_REASON_TRIPLE_FAULT:
    {
        VmxHandleTripleFaults(VCpu);

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
        // UINT64 Rflags = 0;
        // __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);
        // VmxVmwrite64(VMCS_GUEST_RFLAGS, Rflags | 0x1);

        //
        // Handle unconditional vm-exits (inject #ud)
        //
        EventInjectUndefinedOpcode(VCpu);

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
        EventInjectUndefinedOpcode(VCpu);

        break;
    }
    case VMX_EXIT_REASON_MOV_CR:
    {
        //
        // Handle vm-exit, events, dispatches and perform changes from CR access
        //
        DispatchEventMovToFromControlRegisters(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDMSR:
    {
        //
        // Handle vm-exit, events, dispatches and perform changes
        //
        DispatchEventRdmsr(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_WRMSR:
    {
        //
        // Handle vm-exit, events, dispatches and perform changes
        //
        DispatchEventWrmsr(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_CPUID:
    {
        DispatchEventCpuid(VCpu);

        break;
    }

    case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION:
    {
        //
        // Dispatch and trigger the I/O instruction events
        //
        DispatchEventIO(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EPT_VIOLATION:
    {
        if (EptHandleEptViolation(VCpu) == FALSE)
        {
            LogError("Err, there were errors in handling EPT violation");
        }

        break;
    }
    case VMX_EXIT_REASON_EPT_MISCONFIGURATION:
    {
        EptHandleMisconfiguration();

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_VMCALL:
    {
        //
        // Handle vm-exits of VMCALLs
        //
        DispatchEventVmcall(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXCEPTION_OR_NMI:
    {
        //
        // Handle the EXCEPTION injection/emulation
        //
        DispatchEventException(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXTERNAL_INTERRUPT:
    {
        //
        // Call the external-interrupt handler
        //
        DispatchEventExternalInterrupts(VCpu);

        break;
    }
    case VMX_EXIT_REASON_INTERRUPT_WINDOW:
    {
        //
        // Call the interrupt-window exiting handler to re-inject the previous
        // interrupts or disable the interrupt-window exiting bit
        //
        IdtEmulationHandleInterruptWindowExiting(VCpu);

        break;
    }
    case VMX_EXIT_REASON_NMI_WINDOW:
    {
        //
        // Call the NMI-window exiting handler
        //
        IdtEmulationHandleNmiWindowExiting(VCpu);

        break;
    }
    case VMX_EXIT_REASON_MONITOR_TRAP_FLAG:
    {
        //
        // General handler to monitor trap flags (MTF)
        //
        MtfHandleVmexit(VCpu);

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
    case VMX_EXIT_REASON_EXECUTE_RDTSCP:

    {
        //
        // Check whether we are allowed to change the registers
        // and emulate rdtsc or not
        // Note : Using !tsc command in transparent-mode is not allowed
        //
        if (ShouldEmulateRdtscp)
        {
            DispatchEventTsc(VCpu, ExitReason == VMX_EXIT_REASON_EXECUTE_RDTSCP ? TRUE : FALSE);
        }

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDPMC:
    {
        //
        // Handle RDPMC's events, triggers and dispatches (emulate RDPMC)
        //
        DispatchEventRdpmc(VCpu);

        break;
    }
    case VMX_EXIT_REASON_MOV_DR:
    {
        //
        // Trigger, dispatch and handle the event
        //
        DispatchEventMov2DebugRegs(VCpu);

        break;
    }
    case VMX_EXIT_REASON_EXECUTE_XSETBV:
    {
        //
        // Handle xsetbv (unconditional vm-exit)
        //
        VmxHandleXsetbv(VCpu);

        break;
    }
    case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED:
    {
        //
        // Handle the VMX preemption timer vm-exit
        //
        VmxHandleVmxPreemptionTimerVmexit(VCpu);

        break;
    }
    case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL:
    {
        //
        // Handle page-modification log
        //
        DirtyLoggingHandleVmexits(VCpu);

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
    if (!VCpu->VmxoffState.IsVmxoffExecuted && VCpu->IncrementRip)
    {
        HvResumeToNextInstruction();
    }

    //
    // Check for vmxoff request
    //
    if (VCpu->VmxoffState.IsVmxoffExecuted)
    {
        Result = TRUE;
    }

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
            __writemsr(MSR_IA32_TIME_STAMP_COUNTER, VCpu->TransparencyState.PreviousTimeStampCounter);
        }
    }

    //
    // Set indicator of Vmx non root mode to false
    //
    VCpu->IsOnVmxRootMode = FALSE;

    //
    // By default it's FALSE, if we want to exit vmx then it's TRUE
    //
    return Result;
}
