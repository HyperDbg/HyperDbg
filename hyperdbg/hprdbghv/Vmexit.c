/**
 * @file Exit.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The functions for VM-Exit handler for different exit reasons 
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include "Vmx.h"
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "GlobalVariables.h"
#include "Vmcall.h"
#include "Vpid.h"
#include "Invept.h"
#include "Vmcall.h"
#include "Hooks.h"
#include "Invept.h"
#include "HypervisorRoutines.h"
#include "Events.h"
#include "IoHandler.h"
#include "Counters.h"
#include "IdtEmulation.h"

/**
 * @brief VM-Exit handler for different exit reasons
 * 
 * @param GuestRegs Registers that are automatically saved by AsmVmexitHandler (HOST_RIP)
 * @return BOOLEAN Return True if VMXOFF executed (not in vmx anymore),
 *  or return false if we are still in vmx (so we should use vm resume)
 */

BOOLEAN
VmxVmexitHandler(PGUEST_REGS GuestRegs)
{
    VMEXIT_INTERRUPT_INFO InterruptExit         = {0};
    IO_EXIT_QUALIFICATION IoQualification       = {0};
    RFLAGS                Flags                 = {0};
    UINT64                GuestPhysicalAddr     = 0;
    UINT64                GuestRsp              = 0;
    ULONG                 ExitReason            = 0;
    ULONG                 ExitQualification     = 0;
    ULONG                 Rflags                = 0;
    ULONG                 EcxReg                = 0;
    ULONG                 ExitInstructionLength = 0;
    ULONG                 CurrentProcessorIndex = 0;

    //
    // *********** SEND MESSAGE AFTER WE SET THE STATE ***********
    //
    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // Indicates we are in Vmx root mode in this logical core
    //
    g_GuestState[CurrentProcessorIndex].IsOnVmxRootMode = TRUE;
    g_GuestState[CurrentProcessorIndex].IncrementRip    = TRUE;

    //
    // Set the rsp in general purpose registers structure
    //
    __vmx_vmread(GUEST_RSP, &GuestRsp);
    GuestRegs->rsp = GuestRsp;

    //
    // read the exit reason and exit qualification
    //

    __vmx_vmread(VM_EXIT_REASON, &ExitReason);
    ExitReason &= 0xffff;

    __vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);

    //
    // Debugging purpose
    //
    //LogInfo("VM_EXIT_REASON : 0x%x", ExitReason);
    //LogInfo("EXIT_QUALIFICATION : 0x%llx", ExitQualification);
    //

    switch (ExitReason)
    {
    case EXIT_REASON_TRIPLE_FAULT:
    {
        LogError("Triple fault error occured.");

        break;
    }
        //
        // 25.1.2  Instructions That Cause VM Exits Unconditionally
        // The following instructions cause VM exits when they are executed in VMX non-root operation: CPUID, GETSEC,
        // INVD, and XSETBV. This is also true of instructions introduced with VMX, which include: INVEPT, INVVPID,
        // VMCALL, VMCLEAR, VMLAUNCH, VMPTRLD, VMPTRST, VMRESUME, VMXOFF, and VMXON.
        //

    case EXIT_REASON_VMCLEAR:
    case EXIT_REASON_VMPTRLD:
    case EXIT_REASON_VMPTRST:
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMRESUME:
    case EXIT_REASON_VMWRITE:
    case EXIT_REASON_VMXOFF:
    case EXIT_REASON_VMXON:
    case EXIT_REASON_VMLAUNCH:
    {
        //
        // cf=1 indicate vm instructions fail
        //
        //__vmx_vmread(GUEST_RFLAGS, &Rflags);
        //__vmx_vmwrite(GUEST_RFLAGS, Rflags | 0x1);

        //
        // Handle unconditional vm-exits (inject #ud)
        //
        EventInjectUndefinedOpcode(CurrentProcessorIndex);

        break;
    }
    case EXIT_REASON_INVEPT:
    case EXIT_REASON_INVVPID:
    case EXIT_REASON_GETSEC:
    case EXIT_REASON_INVD:
    {
        //
        // Handle unconditional vm-exits (inject #ud)
        //
        EventInjectUndefinedOpcode(CurrentProcessorIndex);

        break;
    }
    case EXIT_REASON_CR_ACCESS:
    {
        HvHandleControlRegisterAccess(GuestRegs);
        break;
    }
    case EXIT_REASON_MSR_READ:
    {
        EcxReg = GuestRegs->rcx & 0xffffffff;
        HvHandleMsrRead(GuestRegs);

        //
        // As the context to event trigger, we send the ecx
        // which is the MSR index
        //
        DebuggerTriggerEvents(RDMSR_INSTRUCTION_EXECUTION, GuestRegs, EcxReg);

        break;
    }
    case EXIT_REASON_MSR_WRITE:
    {
        EcxReg = GuestRegs->rcx & 0xffffffff;
        HvHandleMsrWrite(GuestRegs);

        //
        // As the context to event trigger, we send the ecx
        // which is the MSR index
        //
        DebuggerTriggerEvents(WRMSR_INSTRUCTION_EXECUTION, GuestRegs, EcxReg);

        break;
    }
    case EXIT_REASON_CPUID:
    {
        HvHandleCpuid(GuestRegs);
        break;
    }

    case EXIT_REASON_IO_INSTRUCTION:
    {
        //
        // Read the I/O Qualification which indicates the I/O instruction
        //
        __vmx_vmread(EXIT_QUALIFICATION, &IoQualification);

        //
        // Read Guest's RFLAGS
        //
        __vmx_vmread(GUEST_RFLAGS, &Flags);

        //
        // Call the I/O Handler
        //
        IoHandleIoVmExits(GuestRegs, IoQualification, Flags);

        //
        // As the context to event trigger, port address
        //
        if (IoQualification.AccessType == AccessIn)
        {
            DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION, GuestRegs, IoQualification.PortNumber);
        }
        else if (IoQualification.AccessType == AccessOut)
        {
            DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION, GuestRegs, IoQualification.PortNumber);
        }

        break;
    }
    case EXIT_REASON_EPT_VIOLATION:
    {
        //
        // Reading guest physical address
        //
        __vmx_vmread(GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

        if (EptHandleEptViolation(GuestRegs, ExitQualification, GuestPhysicalAddr) == FALSE)
        {
            LogError("There were errors in handling Ept Violation");
        }

        break;
    }
    case EXIT_REASON_EPT_MISCONFIG:
    {
        __vmx_vmread(GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

        EptHandleMisconfiguration(GuestPhysicalAddr);

        break;
    }
    case EXIT_REASON_VMCALL:
    {
        //
        // Handle vm-exits of VMCALLs
        //
        VmxHandleVmcallVmExit(GuestRegs);

        break;
    }
    case EXIT_REASON_EXCEPTION_NMI:
    {
        //
        // read the exit reason
        //
        __vmx_vmread(VM_EXIT_INTR_INFO, &InterruptExit);

        //
        // Call the Exception Bitmap and NMI Handler
        //
        IdtEmulationHandleExceptionAndNmi(InterruptExit, CurrentProcessorIndex, GuestRegs);

        //
        // Trigger the event
        //
        // As the context to event trigger, we send the vector
        // or IDT Index
        //
        DebuggerTriggerEvents(EXCEPTION_OCCURRED, GuestRegs, InterruptExit.Vector);

        break;
    }
    case EXIT_REASON_EXTERNAL_INTERRUPT:
    {
        //
        // read the exit reason (for interrupt)
        //
        __vmx_vmread(VM_EXIT_INTR_INFO, &InterruptExit);

        //
        // Call External Interrupt Handler
        //
        IdtEmulationHandleExternalInterrupt(InterruptExit, CurrentProcessorIndex);

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

        break;
    }
    case EXIT_REASON_PENDING_VIRT_INTR:
    {
        //
        // Call the interrupt-window exiting handler to re-inject the previous
        // interrupts or disable the interrupt-window exiting bit
        //
        IdtEmulationHandleInterruptWindowExiting(CurrentProcessorIndex);

        break;
    }
    case EXIT_REASON_MONITOR_TRAP_FLAG:
    {
        //
        // Monitor Trap Flag
        //
        if (g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint)
        {
            //
            // Restore the previous state
            //
            EptHandleMonitorTrapFlag(g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint);

            //
            // Set it to NULL
            //
            g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint = NULL;
        }
        else
        {
            LogError("Why MTF occured ?!");
        }
        //
        // Redo the instruction
        //
        g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

        //
        // We don't need MTF anymore
        //
        HvSetMonitorTrapFlag(FALSE);

        break;
    }
    case EXIT_REASON_HLT:
    {
        //
        // We don't wanna halt
        //

        //
        //__halt();
        //
        break;
    }
    case EXIT_REASON_RDTSC:
    {
        //
        // handle rdtsc (emulate rdtsc)
        //
        CounterEmulateRdtsc(GuestRegs);

        //
        // As the context to event trigger, we send the false which means
        // it's an rdtsc (for rdtscp we set Context to true)
        //
        DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, FALSE);

        break;
    }
    case EXIT_REASON_RDTSCP:
    {
        //
        // handle rdtscp (emulate rdtscp)
        //
        CounterEmulateRdtscp(GuestRegs);

        //
        // As the context to event trigger, we send the false which means
        // it's an rdtsc (for rdtscp we set Context to true)
        //
        DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, TRUE);

        break;
    }
    case EXIT_REASON_RDPMC:
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
    case EXIT_REASON_DR_ACCESS:
    {
        //
        // Handle access to debug registers
        //
        HvHandleMovDebugRegister(CurrentProcessorIndex, GuestRegs);

        //
        // Trigger the event
        //
        // As the context to event trigger, we send NULL
        //
        DebuggerTriggerEvents(DEBUG_REGISTERS_ACCESSED, GuestRegs, NULL);

        break;
    }
    case EXIT_REASON_XSETBV:
    {
        //
        // Handle xsetbv (unconditional vm-exit)
        //
        EcxReg = GuestRegs->rcx & 0xffffffff;
        VmxHandleXsetbv(EcxReg, GuestRegs->rdx << 32 | GuestRegs->rax);

        break;
    }
    default:
    {
        LogError("Unkown Vmexit, reason : 0x%llx", ExitReason);
        break;
    }
    }

    //
    // Check whether we need to increment the guest's ip or not
    // Also, we should not increment rip if a vmxoff executed
    //
    if (!g_GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted && g_GuestState[CurrentProcessorIndex].IncrementRip)
    {
        HvResumeToNextInstruction();
    }

    //
    // Set indicator of Vmx non root mode to false
    //
    g_GuestState[CurrentProcessorIndex].IsOnVmxRootMode = FALSE;

    if (g_GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted)
        return TRUE;

    return FALSE;
}
