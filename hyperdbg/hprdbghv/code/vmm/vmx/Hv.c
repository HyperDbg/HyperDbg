/**
 * @file Hv.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief This file describes the routines in Hypervisor
 * @details vmx related routines
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Adjust controls for VMCS based on processor capability
 * 
 * @param Ctl 
 * @param Msr 
 * @return ULONG Returns the Cpu Based and Secondary Processor Based Controls
 *  and other controls based on hardware support
 */
ULONG
HvAdjustControls(ULONG Ctl, ULONG Msr)
{
    MSR MsrValue = {0};

    MsrValue.Flags = __readmsr(Msr);
    Ctl &= MsrValue.Fields.High; /* bit == 0 in high word ==> must be zero */
    Ctl |= MsrValue.Fields.Low;  /* bit == 1 in low word  ==> must be one  */
    return Ctl;
}

/**
 * @brief Set guest's selector registers
 * 
 * @param GdtBase 
 * @param SegmentRegister 
 * @param Selector 
 * @return BOOLEAN 
 */
BOOLEAN
HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, UINT16 Selector)
{
    VMX_SEGMENT_SELECTOR SegmentSelector = {0};
    GetSegmentDescriptor(GdtBase, Selector, &SegmentSelector);

    if (Selector == 0x0)
    {
        SegmentSelector.Attributes.Unusable = TRUE;
    }

    __vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    __vmx_vmwrite(VMCS_GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.Limit);
    __vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS + SegmentRegister * 2, SegmentSelector.Attributes.AsUInt);
    __vmx_vmwrite(VMCS_GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.Base);

    return TRUE;
}

/**
 * @brief Handle Cpuid Vmexits
 * 
 * @param RegistersState Guest's gp registers
 * @return VOID 
 */
VOID
HvHandleCpuid(PGUEST_REGS RegistersState)
{
    INT32  CpuInfo[4];
    ULONG  Mode    = 0;
    UINT64 Context = 0;

    //
    // Check if attaching is for command dispatching in user debugger
    // or a regular CPUID
    //
    if (g_UserDebuggerState && UdCheckForCommand())
    {
        //
        // It's a thread command for user debugger, no need to run the
        // actual CPUID instruction and change the registers
        //
        return;
    }

    //
    // Set the context (save eax for the debugger)
    //
    Context = RegistersState->rax;

    //
    // Otherwise, issue the CPUID to the logical processor based on the indexes
    // on the VP's GPRs.
    //
    __cpuidex(CpuInfo, (INT32)RegistersState->rax, (INT32)RegistersState->rcx);

    //
    // check whether we are in transparent mode or not
    // if we are in transparent mode then ignore the
    // cpuid modifications e.g. hyperviosr name or bit
    //
    if (!g_TransparentMode)
    {
        //
        // Check if this was CPUID 1h, which is the features request
        //
        if (RegistersState->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
        {
            //
            // Set the Hypervisor Present-bit in RCX, which Intel and AMD have both
            // reserved for this indication
            //
            CpuInfo[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
        }
        /*
        else if (RegistersState->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS)
        {
            //
            // Return a maximum supported hypervisor CPUID leaf range and a vendor
            // ID signature as required by the spec
            //

            CpuInfo[0] = HYPERV_CPUID_INTERFACE;
            CpuInfo[1] = 'epyH'; // [HyperDbg]
            CpuInfo[2] = 'gbDr';
            CpuInfo[3] = NULL;
        }
        else if (RegistersState->rax == HYPERV_CPUID_INTERFACE)
        {
            //
            // Return non Hv#1 value. This indicate that our hypervisor does NOT
            // conform to the Microsoft hypervisor interface.
            //

            CpuInfo[0] = '0#vH'; // Hv#0
            CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0;
        }
        */
    }

    //
    // Copy the values from the logical processor registers into the VP GPRs
    //
    RegistersState->rax = CpuInfo[0];
    RegistersState->rbx = CpuInfo[1];
    RegistersState->rcx = CpuInfo[2];
    RegistersState->rdx = CpuInfo[3];

    //
    // As the context to event trigger, we send the eax before the cpuid
    // so that the debugger can both read the eax as it's now changed by
    // the cpuid instruction and also can modify the results
    //
    if (g_TriggerEventForCpuids)
    {
        DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION, RegistersState, Context);
    }
}

/**
 * @brief Handles Guest Access to control registers
 * 
 * @param GuestState Guest's gp registers
 * @param ProcessorIndex Index of processor
 * @return VOID 
 */
VOID
HvHandleControlRegisterAccess(PGUEST_REGS GuestState, UINT32 ProcessorIndex)
{
    ULONG                           ExitQualification = 0;
    VMX_EXIT_QUALIFICATION_MOV_CR * CrExitQualification;
    UINT64 *                        RegPtr;
    UINT64                          NewCr3;
    CR3_TYPE                        NewCr3Reg;
    VIRTUAL_MACHINE_STATE *         CurrentVmState = &g_GuestState[ProcessorIndex];

    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification);

    CrExitQualification = (VMX_EXIT_QUALIFICATION_MOV_CR *)&ExitQualification;

    RegPtr = (UINT64 *)&GuestState->rax + CrExitQualification->GeneralPurposeRegister;

    //
    // Because its RSP and as we didn't save RSP correctly (because of pushes)
    // so we have make it points to the GUEST_RSP
    //

    //
    // We handled it in vm-exit handler, commented
    //

    /*    
    if (CrExitQualification->Fields.Register == 4)
    {
        __vmx_vmread(VMCS_GUEST_RSP, &GuestRsp);
        *RegPtr = GuestRsp;
    }
    */

    switch (CrExitQualification->AccessType)
    {
    case VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR:
    {
        switch (CrExitQualification->ControlRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
            __vmx_vmwrite(VMCS_GUEST_CR0, *RegPtr);
            __vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, *RegPtr);
            DebuggerTriggerEvents(CONTROL_REGISTER_MODIFIED, GuestState, VMX_EXIT_QUALIFICATION_REGISTER_CR0);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_CR3:

            NewCr3          = (*RegPtr & ~(1ULL << 63));
            NewCr3Reg.Flags = NewCr3;

            //
            // Apply the new cr3
            //
            __vmx_vmwrite(VMCS_GUEST_CR3, NewCr3Reg.Flags);

            //
            // Invalidate as we used VPID tags so the vm-exit won't
            // normally (automatically) flush the TLB, we have to do
            // it manually
            //
            VpidInvvpidSingleContext(VPID_TAG);

            //
            // Call kernel debugger handler for mov to cr3 in kernel debugger
            //
            if (CurrentVmState->DebuggingState.ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits)
            {
                ProcessHandleProcessChange(ProcessorIndex, GuestState);
            }

            //
            // Call user debugger handler of thread intercepting mechanism
            //
            if (g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger)
            {
                AttachingHandleCr3VmexitsForThreadInterception(ProcessorIndex, NewCr3Reg);
            }

            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
            __vmx_vmwrite(VMCS_GUEST_CR4, *RegPtr);
            __vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, *RegPtr);
            DebuggerTriggerEvents(CONTROL_REGISTER_MODIFIED, GuestState, VMX_EXIT_QUALIFICATION_REGISTER_CR4);
            break;
        default:
            LogWarning("Unsupported register 0x%x in handling control registers access",
                       CrExitQualification->ControlRegister);
            break;
        }
    }
    break;

    case VMX_EXIT_QUALIFICATION_ACCESS_MOV_FROM_CR:
    {
        switch (CrExitQualification->ControlRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
            __vmx_vmread(VMCS_GUEST_CR0, RegPtr);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_CR3:
            __vmx_vmread(VMCS_GUEST_CR3, RegPtr);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
            __vmx_vmread(VMCS_GUEST_CR4, RegPtr);
            break;
        default:
            LogWarning("Unsupported register 0x%x in handling control registers access",
                       CrExitQualification->ControlRegister);
            break;
        }
    }
    break;

    default:
        LogWarning("Unsupported operation 0x%x in handling control registers access",
                   CrExitQualification->AccessType);
        break;
    }
}

/**
 * @brief Fill the guest's selector data
 * 
 * @param GdtBase 
 * @param SegmentRegister 
 * @param Selector 
 * @return VOID 
 */
VOID
HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, UINT16 Selector)
{
    VMX_SEGMENT_SELECTOR SegmentSelector = {0};

    GetSegmentDescriptor(GdtBase, Selector, &SegmentSelector);

    if (Selector == 0x0)
    {
        SegmentSelector.Attributes.Unusable = TRUE;
    }

    SegmentSelector.Attributes.Reserved1 = 0;
    SegmentSelector.Attributes.Reserved2 = 0;

    __vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    __vmx_vmwrite(VMCS_GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.Limit);
    __vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS + SegmentRegister * 2, SegmentSelector.Attributes.AsUInt);
    __vmx_vmwrite(VMCS_GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.Base);
}

/**
 * @brief Add the current instruction length to guest rip to resume to next instruction
 * 
 * @return VOID 
 */
VOID
HvResumeToNextInstruction()
{
    UINT64 ResumeRIP             = NULL;
    UINT64 CurrentRIP            = NULL;
    size_t ExitInstructionLength = 0;

    __vmx_vmread(VMCS_GUEST_RIP, &CurrentRIP);
    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstructionLength);

    ResumeRIP = CurrentRIP + ExitInstructionLength;

    __vmx_vmwrite(VMCS_GUEST_RIP, ResumeRIP);
}

/**
 * @brief Set the monitor trap flag
 * 
 * @param Set Set or unset the MTFs
 * @return VOID 
 */
VOID
HvSetMonitorTrapFlag(BOOLEAN Set)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_MONITOR_TRAP_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_MONITOR_TRAP_FLAG;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set LOAD DEBUG CONTROLS on Vm-entry controls
 * 
 * @param Set Set or unset 
 * @return VOID 
 */
VOID
HvSetLoadDebugControls(BOOLEAN Set)
{
    ULONG VmentryControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_VMENTRY_CONTROLS, &VmentryControls);

    if (Set)
    {
        VmentryControls |= VM_ENTRY_LOAD_DEBUG_CONTROLS;
    }
    else
    {
        VmentryControls &= ~VM_ENTRY_LOAD_DEBUG_CONTROLS;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, VmentryControls);
}

/**
 * @brief Set SAVE DEBUG CONTROLS on Vm-exit controls
 * 
 * @param Set Set or unset 
 * @return VOID 
 */
VOID
HvSetSaveDebugControls(BOOLEAN Set)
{
    ULONG VmexitControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmexitControls);

    if (Set)
    {
        VmexitControls |= VM_EXIT_SAVE_DEBUG_CONTROLS;
    }
    else
    {
        VmexitControls &= ~VM_EXIT_SAVE_DEBUG_CONTROLS;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmexitControls);
}

/**
 * @brief Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified
 * 
 * @return VOID 
 */
VOID
HvRestoreRegisters()
{
    UINT64 FsBase;
    UINT64 GsBase;
    UINT64 GdtrBase;
    UINT64 GdtrLimit;
    UINT64 IdtrBase;
    UINT64 IdtrLimit;

    //
    // Restore FS Base
    //
    __vmx_vmread(VMCS_GUEST_FS_BASE, &FsBase);
    __writemsr(IA32_FS_BASE, FsBase);

    //
    // Restore Gs Base
    //
    __vmx_vmread(VMCS_GUEST_GS_BASE, &GsBase);
    __writemsr(IA32_GS_BASE, GsBase);

    //
    // Restore GDTR
    //
    __vmx_vmread(VMCS_GUEST_GDTR_BASE, &GdtrBase);
    __vmx_vmread(VMCS_GUEST_GDTR_LIMIT, &GdtrLimit);

    AsmReloadGdtr(GdtrBase, GdtrLimit);

    //
    // Restore IDTR
    //
    __vmx_vmread(VMCS_GUEST_IDTR_BASE, &IdtrBase);
    __vmx_vmread(VMCS_GUEST_IDTR_LIMIT, &IdtrLimit);

    AsmReloadIdtr(IdtrBase, IdtrLimit);
}

/**
 * @brief Set vm-exit for rdpmc instructions 
 * @details Should be called in vmx-root
 * 
 * @param Set Set or unset the vm-exits
 * @return VOID 
 */
VOID
HvSetPmcVmexit(BOOLEAN Set)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_RDPMC_EXITING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_RDPMC_EXITING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set vm-exit for mov-to-cr0/4
 * @details Should be called in vmx-root
 *
 * @param Set or unset the vm-exits
 * @param Control Register
 * @param Mask Register
 * @return VOID
 */
VOID
HvSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister)
{
    ProtectedHvSetMov2CrExiting(Set, ControlRegister, MaskRegister);
}

/**
 * @brief Set vm-exit for mov-to-cr3 
 * @details Should be called in vmx-root
 * 
 * @param Set Set or unset the vm-exits
 * @return VOID 
 */
VOID
HvSetMovToCr3Vmexit(BOOLEAN Set)
{
    ProtectedHvSetMov2Cr3Exiting(Set);
}

/**
 * @brief Write on exception bitmap in VMCS 
 * DO NOT CALL IT DIRECTLY, instead use HvSetExceptionBitmap
 * @details Should be called in vmx-root
 * 
 * @param BitmapMask The content to write on exception bitmap 
 * @return VOID 
 */
VOID
HvWriteExceptionBitmap(UINT32 BitmapMask)
{
    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, BitmapMask);
}

/**
 * @brief Read exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @return UINT32 
 */
UINT32
HvReadExceptionBitmap()
{
    UINT32 ExceptionBitmap = 0;

    //
    // Read the current bitmap
    //
    __vmx_vmread(VMCS_CTRL_EXCEPTION_BITMAP, &ExceptionBitmap);

    return ExceptionBitmap;
}

/**
 * @brief Set Interrupt-window exiting
 * 
 * @param Set Set or unset the Interrupt-window exiting
 * @return VOID 
 */
VOID
HvSetInterruptWindowExiting(BOOLEAN Set)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    //
    // interrupt-window exiting
    //
    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_VIRTUAL_INTR_PENDING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_VIRTUAL_INTR_PENDING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set NMI-window exiting
 * 
 * @param Set Set or unset the NMI-window exiting
 * @return VOID 
 */
VOID
HvSetNmiWindowExiting(BOOLEAN Set)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    //
    // interrupt-window exiting
    //
    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_VIRTUAL_NMI_PENDING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_VIRTUAL_NMI_PENDING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Handle Mov to Debug Registers Exitings
 * 
 * @param ProcessorIndex Index of processor
 * @param Regs Registers of guest
 * @return VOID 
 */
VOID
HvHandleMovDebugRegister(UINT32 ProcessorIndex, PGUEST_REGS Regs)
{
    VMX_EXIT_QUALIFICATION_MOV_DR ExitQualification;
    CR4                           Cr4;
    DR7                           Dr7;
    VMX_SEGMENT_SELECTOR          Cs;
    UINT64 *                      GpRegs         = Regs;
    VIRTUAL_MACHINE_STATE *       CurrentVmState = &g_GuestState[ProcessorIndex];
    //
    // The implementation is derived from Hvpp
    //
    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification);

    UINT64 GpRegister = GpRegs[ExitQualification.GeneralPurposeRegister];

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

    if (Cs.Attributes.DescriptorPrivilegeLevel != 0)
    {
        EventInjectGeneralProtection();

        //
        // Redo the instruction
        //
        CurrentVmState->IncrementRip = FALSE;
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
    __vmx_vmread(VMCS_GUEST_CR4, &Cr4);

    if (ExitQualification.DebugRegister == 4 || ExitQualification.DebugRegister == 5)
    {
        if (Cr4.DebuggingExtensions)
        {
            //
            // re-inject #UD
            //
            EventInjectUndefinedOpcode(ProcessorIndex);
            return;
        }
        else
        {
            ExitQualification.DebugRegister += 2;
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
    __vmx_vmread(VMCS_GUEST_DR7, &Dr7);

    if (Dr7.GeneralDetect)
    {
        DR6 Dr6 = {
            .AsUInt                      = __readdr(6),
            .BreakpointCondition         = 0,
            .DebugRegisterAccessDetected = TRUE};

        __writedr(6, Dr6.AsUInt);

        Dr7.GeneralDetect = FALSE;

        __vmx_vmwrite(VMCS_GUEST_DR7, Dr7.AsUInt);

        EventInjectDebugBreakpoint();

        //
        // Redo the instruction
        //
        CurrentVmState->IncrementRip = FALSE;
        return;
    }

    //
    // In 64-bit mode, the upper 32 bits of DR6 and DR7 are reserved
    // and must be written with zeros.  Writing 1 to any of the upper
    // 32 bits results in a #GP(0) exception.
    // (ref: Vol3B[17.2.6(Debug Registers and Intel 64 Processors)])
    //
    if (ExitQualification.DirectionOfAccess == VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR &&
        (ExitQualification.DebugRegister == VMX_EXIT_QUALIFICATION_REGISTER_DR6 ||
         ExitQualification.DebugRegister == VMX_EXIT_QUALIFICATION_REGISTER_DR7) &&
        (GpRegister >> 32) != 0)
    {
        EventInjectGeneralProtection();

        //
        // Redo the instruction
        //
        CurrentVmState->IncrementRip = FALSE;
        return;
    }

    switch (ExitQualification.DirectionOfAccess)
    {
    case VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR:
        switch (ExitQualification.DebugRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_DR0:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR0, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR1:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR1, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR2:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR2, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR3:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR3, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR6:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR6, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR7:
            __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR7, GpRegister);
            break;
        default:
            break;
        }
        break;

    case VMX_EXIT_QUALIFICATION_DIRECTION_MOV_FROM_DR:
        switch (ExitQualification.DebugRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_DR0:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR0);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR1:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR1);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR2:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR2);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR3:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR3);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR6:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR6);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR7:
            GpRegister = __readdr(VMX_EXIT_QUALIFICATION_REGISTER_DR7);
            break;
        default:
            break;
        }

    default:
        break;
    }
}

/**
 * @brief Set the NMI Exiting
 * 
 * @param Set Set or unset the NMI Exiting
 * @return VOID 
 */
VOID
HvSetNmiExiting(BOOLEAN Set)
{
    ULONG PinBasedControls = 0;
    ULONG VmExitControls   = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmExitControls);

    if (Set)
    {
        PinBasedControls |= PIN_BASED_VM_EXECUTION_CONTROLS_NMI_EXITING;
        VmExitControls |= VM_EXIT_ACK_INTR_ON_EXIT;
    }
    else
    {
        PinBasedControls &= ~PIN_BASED_VM_EXECUTION_CONTROLS_NMI_EXITING;
        VmExitControls &= ~VM_EXIT_ACK_INTR_ON_EXIT;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
    __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmExitControls);
}

/**
 * @brief Set the VMX preemption timer
 * 
 * @param Set Set or unset the VMX preemption timer
 * @return VOID 
 */
VOID
HvSetVmxPreemptionTimerExiting(BOOLEAN Set)
{
    ULONG PinBasedControls = 0;

    //
    // Read the previous flags
    //
    __vmx_vmread(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);

    if (Set)
    {
        PinBasedControls |= PIN_BASED_VM_EXECUTION_CONTROLS_ACTIVE_VMX_TIMER;
    }
    else
    {
        PinBasedControls &= ~PIN_BASED_VM_EXECUTION_CONTROLS_ACTIVE_VMX_TIMER;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
}

/**
 * @brief Set exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
HvSetExceptionBitmap(UINT32 IdtIndex)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvSetExceptionBitmap(IdtIndex);
}

/**
 * @brief Unset exception bitmap in VMCS 
 * @details Should be called in vmx-root
 * 
 * @param IdtIndex Interrupt Descriptor Table index of exception 
 * @return VOID 
 */
VOID
HvUnsetExceptionBitmap(UINT32 IdtIndex)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvUnsetExceptionBitmap(IdtIndex);
}

/**
 * @brief Set the External Interrupt Exiting
 * 
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID 
 */
VOID
HvSetExternalInterruptExiting(BOOLEAN Set)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvSetExternalInterruptExiting(Set);
}

/**
 * @brief Set the RDTSC/P Exiting
 * 
 * @param Set Set or unset the RDTSC/P Exiting
 * @return VOID 
 */
VOID
HvSetRdtscExiting(BOOLEAN Set)
{
    ProtectedHvSetRdtscExiting(Set);
}

/**
 * @brief Set or unset the Mov to Debug Registers Exiting
 * 
 * @param Set Set or unset the Mov to Debug Registers Exiting
 * @return VOID 
 */
VOID
HvSetMovDebugRegsExiting(BOOLEAN Set)
{
    ProtectedHvSetMovDebugRegsExiting(Set);
}
