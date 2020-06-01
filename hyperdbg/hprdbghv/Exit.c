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
    UINT64                GuestPhysicalAddr     = 0;
    UINT64                GuestRsp              = 0;
    ULONG                 ExitReason            = 0;
    ULONG                 ExitQualification     = 0;
    ULONG                 Rflags                = 0;
    ULONG                 EcxReg                = 0;
    ULONG                 ErrorCode             = 0;
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
        __vmx_vmread(GUEST_RFLAGS, &Rflags);

        //
        // cf=1 indicate vm instructions fail
        //
        __vmx_vmwrite(GUEST_RFLAGS, Rflags | 0x1);

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
        LogError("Exit reason for I/O instructions are not supported yet.");
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
        // Check if it's our routines that request the VMCALL our it relates to Hyper-V
        //
        if (GuestRegs->r10 == 0x48564653 && GuestRegs->r11 == 0x564d43414c4c && GuestRegs->r12 == 0x4e4f485950455256)
        {
            //
            // Then we have to manage it as it relates to us
            //
            GuestRegs->rax = VmxVmcallHandler(GuestRegs->rcx, GuestRegs->rdx, GuestRegs->r8, GuestRegs->r9);
        }
        else
        {
            HYPERCALL_INPUT_VALUE InputValue = {0};
            InputValue.Value                 = GuestRegs->rcx;

            switch (InputValue.Bitmap.CallCode)
            {
            case HvSwitchVirtualAddressSpace:
            case HvFlushVirtualAddressSpace:
            case HvFlushVirtualAddressList:
            case HvCallFlushVirtualAddressSpaceEx:
            case HvCallFlushVirtualAddressListEx:
            {
                InvvpidAllContexts();
                break;
            }
            case HvCallFlushGuestPhysicalAddressSpace:
            case HvCallFlushGuestPhysicalAddressList:
            {
                InveptSingleContext(g_EptState->EptPointer.Flags);
                break;
            }
            }
            //
            // Let the top-level hypervisor to manage it
            //
            GuestRegs->rax = AsmHypervVmcall(GuestRegs->rcx, GuestRegs->rdx, GuestRegs->r8, GuestRegs->r9);
        }
        break;
    }
    case EXIT_REASON_EXCEPTION_NMI:
    {
        //
        // Exception or non-maskable interrupt (NMI). Either:
        //	1: Guest software caused an exception and the bit in the exception bitmap associated with exception�s vector was set to 1
        //	2: An NMI was delivered to the logical processor and the �NMI exiting� VM-execution control was 1.
        //
        // VM_EXIT_INTR_INFO shows the exit infromation about event that occured and causes this exit
        // Don't forget to read VM_EXIT_INTR_ERROR_CODE in the case of re-injectiong event
        //

        //
        // read the exit reason
        //
        __vmx_vmread(VM_EXIT_INTR_INFO, &InterruptExit);

        if (InterruptExit.InterruptionType == INTERRUPT_TYPE_SOFTWARE_EXCEPTION && InterruptExit.Vector == EXCEPTION_VECTOR_BREAKPOINT)
        {
            ULONG64 GuestRip;
            //
            // Reading guest's RIP
            //
            __vmx_vmread(GUEST_RIP, &GuestRip);

            //
            // Send the user
            //
            LogInfo("Breakpoint Hit (Process Id : 0x%x) at : %llx ", PsGetCurrentProcessId(), GuestRip);

            g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

            //
            // re-inject #BP back to the guest
            //
            EventInjectBreakpoint();
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
                EventInjectUndefinedOpcode();
            }
        }
        else
        {
            if (InterruptExit.Vector == EXCEPTION_VECTOR_PAGE_FAULT)
            {
                //
                // #PF is treated differently, we have to deal with cr2 too.
                //
                PAGE_FAULT_ERROR_CODE PageFaultCode = {0};

                __vmx_vmread(VM_EXIT_INTR_ERROR_CODE, &PageFaultCode);

                UINT64 PageFaultAddress = 0;

                __vmx_vmread(EXIT_QUALIFICATION, &PageFaultAddress);

                // EventInjectPageFault(PageFaultCode.All);

                LogInfo("#PF Fault = %016llx, Page Fault Code = 0x%x", PageFaultAddress, PageFaultCode.All);

                //
                // Cr2 is used as the page-fault address
                //
                __writecr2(PageFaultAddress);

                g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;
            }
            LogInfo("Interrupt vector : 0x%x", InterruptExit.Vector);
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
            //
            //LogError("Not expected event occured");
            //
        }
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
        //__halt();
        //
        break;
    }
    case EXIT_REASON_RDTSC:
    {
        //
        // I realized that if you log anything here (LogInfo) then
        // the system-halts, currently don't have any idea of how
        // to solve it, in the future we solve it using tsc offsectiong
        // or tsc scalling
        //
        ULONG64 Tsc    = __rdtsc();
        GuestRegs->rax = 0x00000000ffffffff & Tsc;
        GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);
        break;
    }
    case EXIT_REASON_RDTSCP:
    {
        int     Aux    = 0;
        ULONG64 Tsc    = __rdtscp(&Aux);
        GuestRegs->rax = 0x00000000ffffffff & Tsc;
        GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);
        break;
    }
    default:
    {
        LogError("Unkown Vmexit, reason : 0x%llx", ExitReason);
        break;
    }
    }

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
