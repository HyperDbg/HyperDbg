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
#include "IdtEmulation.h"

typedef enum MOV_TO_DEBUG_REG
{
    AccessToDebugRegister   = 0,
    AccessFromDebugRegister = 1,
};

typedef union _MOV_TO_DEBUG_REG_QUALIFICATION
{
    UINT64 Flags;

    struct
    {
        UINT64 DrNumber : 3;
        UINT64 Reserved1 : 1;
        UINT64 AccessType : 1;
        UINT64 Reserved2 : 3;
        UINT64 GpRegister : 4;
    };
} MOV_TO_DEBUG_REG_QUALIFICATION, *PMOV_TO_DEBUG_REG_QUALIFICATION;

typedef struct _CONTROL_REGISTER_4
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 VirtualModeExtensions : 1;
            UINT64 ProtectedModeVirtualInterrupts : 1;
            UINT64 TimestampDisable : 1;
            UINT64 DebuggingExtensions : 1;
            UINT64 PageSizeExtensions : 1;
            UINT64 PhysicalAddressExtension : 1;
            UINT64 MachineCheckEnable : 1;
            UINT64 PageGlobalEnable : 1;
            UINT64 PerformanceMonitoringCounterEnable : 1;
            UINT64 OsFxsaveFxrstorSupport : 1;
            UINT64 OsXmmExceptionSupport : 1;
            UINT64 UsermodeInstructionPrevention : 1;
            UINT64 Reserved1 : 1;
            UINT64 VmxEnable : 1;
            UINT64 SmxEnable : 1;
            UINT64 Reserved2 : 1;
            UINT64 FsGsBaseEnable : 1;
            UINT64 PcidEnable : 1;
            UINT64 OsXsave : 1;
            UINT64 Reserved3 : 1;
            UINT64 SmepEnable : 1;
            UINT64 SmapEnable : 1;
            UINT64 ProtectionKeyEnable : 1;
        };
    };
} CONTROL_REGISTER_4, *PCONTROL_REGISTER_4;

typedef union _DEBUG_REGISTER_7
{
    UINT64 Flags;

    struct
    {
        UINT64 LocalBreakpoint0 : 1;
        UINT64 GlobalBreakpoint0 : 1;
        UINT64 LocalBreakpoint1 : 1;
        UINT64 GlobalBreakpoint1 : 1;
        UINT64 LocalBreakpoint2 : 1;
        UINT64 GlobalBreakpoint2 : 1;
        UINT64 LocalBreakpoint3 : 1;
        UINT64 GlobalBreakpoint3 : 1;
        UINT64 LocalExactBreakpoint : 1;
        UINT64 GlobalExactBreakpoint : 1;
        UINT64 Reserved1 : 1; // always 1
        UINT64 RestrictedTransactionalMemory : 1;
        UINT64 Reserved2 : 1; // always 0
        UINT64 GeneralDetect : 1;
        UINT64 Reserved3 : 2; // always 0
        UINT64 ReadWrite0 : 2;
        UINT64 Length0 : 2;
        UINT64 ReadWrite1 : 2;
        UINT64 Length1 : 2;
        UINT64 ReadWrite2 : 2;
        UINT64 Length2 : 2;
        UINT64 ReadWrite3 : 2;
        UINT64 Length3 : 2;
    };
} DEBUG_REGISTER_7, *PDEBUG_REGISTER_7;

typedef union DEBUG_REGISTER_6
{
    UINT64 Flags;

    struct
    {
        UINT64 BreakpointCondition : 4;
        UINT64 Reserved1 : 8; // always 1
        UINT64 Reserved2 : 1; // always 0
        UINT64 DebugRegisterAccessDetected : 1;
        UINT64 SingleInstruction : 1;
        UINT64 TaskSwitch : 1;
        UINT64 RestrictedTransactionalMemory : 1;
        UINT64 Reserved3 : 15; // always 1
    };
} DEBUG_REGISTER_6, *PDEBUG_REGISTER_6;

/**
 * @brief Get the Guest Cs Selector
 * 
 * @return VOID 
 */

SEGMENT_SELECTOR
GetGuestCs()
{
    SEGMENT_SELECTOR Cs;

    __vmx_vmread(GUEST_CS_BASE, &Cs.BASE);
    __vmx_vmread(GUEST_CS_LIMIT, &Cs.LIMIT);
    __vmx_vmread(GUEST_CS_AR_BYTES, &Cs.ATTRIBUTES.UCHARs);
    __vmx_vmread(GUEST_CS_SELECTOR, &Cs.SEL);

    return Cs;
}

VOID
HvHandleMovDebugRegister(UINT32 ProcessorIndex, PGUEST_REGS Regs)
{
    MOV_TO_DEBUG_REG_QUALIFICATION ExitQualification;
    CONTROL_REGISTER_4             Cr4;
    DEBUG_REGISTER_7               Dr7;
    SEGMENT_SELECTOR               Cs;
    UINT64 *                       GpRegs = Regs;
    //
    // The implementation is derived from Hvpp
    //
    __vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);

    UINT64 GpRegister = GpRegs[ExitQualification.GpRegister];

    //
    // The MOV DR instruction causes a VM exit if the "MOV-DR exiting"
    // VM-execution control is 1.  Such VM exits represent an exception
    // to the principles identified in Section 25.1.1 (Relative Priority
    // of Faults and VM Exits) in that they take priority over the
    // following: general-protection exceptions based on privilege level;
    // and invalid-opcode exceptions that occur because CR4.DE = 1 and the
    // instruction specified access to DR4 or DR5.
    // (ref: Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)])
    //
    // TL;DR:
    //   CPU usually prioritizes exceptions.  For example RDMSR executed
    //   at CPL = 3 won't cause VM-exit - it causes #GP instead.  MOV DR
    //   is exception to this rule, as it ALWAYS cause VM-exit.
    //
    //   Normally, CPU allows you to write to DR registers only at CPL=0,
    //   otherwise it causes #GP.  Therefore we'll simulate the exact same
    //   behavior here.
    //

    Cs = GetGuestCs();

    if (Cs.ATTRIBUTES.Fields.DPL != 0)
    {
        EventInjectGeneralProtection();

        //
        // Redo the instruction
        //
        g_GuestState[ProcessorIndex].IncrementRip = FALSE;
        return;
    }

    //
    // Debug registers DR4 and DR5 are reserved when debug extensions
    // are enabled (when the DE flag in control register CR4 is set)
    // and attempts to reference the DR4 and DR5 registers cause
    // invalid-opcode exceptions (#UD).
    // When debug extensions are not enabled (when the DE flag is clear),
    // these registers are aliased to debug registers DR6 and DR7.
    // (ref: Vol3B[17.2.2(Debug Registers DR4 and DR5)])
    //

    //
    // Read guest cr4
    //
    __vmx_vmread(GUEST_CR4, &Cr4);

    if (ExitQualification.DrNumber == 4 || ExitQualification.DrNumber == 5)
    {
        if (Cr4.DebuggingExtensions)
        {
            EventInjectUndefinedOpcode();

            //
            // Redo the instruction
            //
            g_GuestState[ProcessorIndex].IncrementRip = FALSE;
            return;
        }
        else
        {
            ExitQualification.DrNumber += 2;
        }
    }

    //
    // Enables (when set) debug-register protection, which causes a
    // debug exception to be generated prior to any MOV instruction
    // that accesses a debug register.  When such a condition is
    // detected, the BD flag in debug status register DR6 is set prior
    // to generating the exception.  This condition is provided to
    // support in-circuit emulators.
    // When the emulator needs to access the debug registers, emulator
    // software can set the GD flag to prevent interference from the
    // program currently executing on the processor.
    // The processor clears the GD flag upon entering to the debug
    // exception handler, to allow the handler access to the debug
    // registers.
    // (ref: Vol3B[17.2.4(Debug Control Register (DR7)])
    //

    //
    // Read the DR7
    //
    __vmx_vmread(GUEST_DR7, &Dr7);

    if (Dr7.GeneralDetect)
    {
        DEBUG_REGISTER_6 Dr6;
        Dr6.Flags                       = __readdr(6);
        Dr6.BreakpointCondition         = 0;
        Dr6.DebugRegisterAccessDetected = TRUE;
        __writedr(6, Dr6.Flags);

        Dr7.GeneralDetect = FALSE;

        __vmx_vmwrite(GUEST_DR7, Dr7.Flags);

        EventInjectDebugBreakpoint();

        //
        // Redo the instruction
        //
        g_GuestState[ProcessorIndex].IncrementRip = FALSE;
        return;
    }

    //
    // In 64-bit mode, the upper 32 bits of DR6 and DR7 are reserved
    // and must be written with zeros.  Writing 1 to any of the upper
    // 32 bits results in a #GP(0) exception.
    // (ref: Vol3B[17.2.6(Debug Registers and Intel® 64 Processors)])
    //
    if (ExitQualification.AccessType == AccessToDebugRegister &&
        (ExitQualification.DrNumber == 6 || ExitQualification.DrNumber == 7) &&
        (GpRegister >> 32) != 0)
    {
        EventInjectGeneralProtection();

        //
        // Redo the instruction
        //
        g_GuestState[ProcessorIndex].IncrementRip = FALSE;
        return;
    }

    switch (ExitQualification.AccessType)
    {
    case AccessToDebugRegister:
        switch (ExitQualification.DrNumber)
        {
        case 0:
            __writedr(0, GpRegister);
            break;
        case 1:
            __writedr(1, GpRegister);
            break;
        case 2:
            __writedr(2, GpRegister);
            break;
        case 3:
            __writedr(3, GpRegister);
            break;
        case 6:
            __writedr(6, GpRegister);
            break;
        case 7:
            __writedr(7, GpRegister);
            break;
        default:
            break;
        }
        break;

    case AccessFromDebugRegister:
        switch (ExitQualification.DrNumber)
        {
        case 0:
            GpRegister = __readdr(0);
            break;
        case 1:
            GpRegister = __readdr(1);
            break;
        case 2:
            GpRegister = __readdr(2);
            break;
        case 3:
            GpRegister = __readdr(3);
            break;
        case 6:
            GpRegister = __readdr(6);
            break;
        case 7:
            GpRegister = __readdr(7);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}

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
        // or tsc scalling (The reason is because of that fucking patchguard :( )
        //
        ULONG64 Tsc    = __rdtsc();
        GuestRegs->rax = 0x00000000ffffffff & Tsc;
        GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);

        //
        // As the context to event trigger, we send the false which means
        // it's an rdtsc (for rdtscp we set Context to true)
        //
        DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, FALSE);

        break;
    }
    case EXIT_REASON_RDTSCP:
    {
        int     Aux    = 0;
        ULONG64 Tsc    = __rdtscp(&Aux);
        GuestRegs->rax = 0x00000000ffffffff & Tsc;
        GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);

        //
        // As the context to event trigger, we send the false which means
        // it's an rdtsc (for rdtscp we set Context to true)
        //
        DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION, GuestRegs, TRUE);

        break;
    }
    case EXIT_REASON_RDPMC:
    {
        EcxReg         = GuestRegs->rcx & 0xffffffff;
        ULONG64 Pmc    = __readpmc(EcxReg);
        GuestRegs->rax = 0x00000000ffffffff & Pmc;
        GuestRegs->rdx = 0x00000000ffffffff & (Pmc >> 32);

        //
        // As the context to event trigger, we send the NULL
        //
        DebuggerTriggerEvents(PMC_INSTRUCTION_EXECUTION, GuestRegs, NULL);

        break;
    }
    case EXIT_REASON_DR_ACCESS:
    {
        LogInfo("Debug register accessed");
        //
        // Handle access to debug registers
        //
        HvHandleMovDebugRegister(CurrentProcessorIndex, GuestRegs);

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
