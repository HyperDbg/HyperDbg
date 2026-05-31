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
 * @return UINT32 Returns the Cpu Based and Secondary Processor Based Controls
 *  and other controls based on hardware support
 */
UINT32
HvAdjustControls(UINT32 Ctl, UINT32 Msr)
{
    MSR MsrValue = {0};

    MsrValue.Flags = CpuReadMsr(Msr);
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
HvSetGuestSelector(PVOID GdtBase, UINT32 SegmentRegister, UINT16 Selector)
{
    VMX_SEGMENT_SELECTOR SegmentSelector = {0};
    SegmentGetDescriptor(GdtBase, Selector, &SegmentSelector);

    if (Selector == 0x0)
    {
        SegmentSelector.Attributes.Unusable = TRUE;
    }

    VmxVmwrite64(VMCS_GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    VmxVmwrite64(VMCS_GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.Limit);
    VmxVmwrite64(VMCS_GUEST_ES_ACCESS_RIGHTS + SegmentRegister * 2, SegmentSelector.Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.Base);

    return TRUE;
}

/**
 * @brief Handle Cpuid Vmexits
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
HvHandleCpuid(VIRTUAL_MACHINE_STATE * VCpu)
{
    INT32       CpuInfo[4];
    PGUEST_REGS Regs = VCpu->Regs;

    //
    // Otherwise, issue the CPUID to the logical processor based on the indexes
    // on the VP's GPRs.
    //
    CpuCpuIdEx(CpuInfo, (INT32)Regs->rax, (INT32)Regs->rcx);

    //
    // check whether we are in transparent mode or not
    // if we are in transparent mode then ignore the
    // cpuid modifications e.g. hyperviosr name or bit
    //
    if (!g_CheckForFootprints)
    {
        //
        // Check if this was CPUID 1h, which is the features request
        //
        if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
        {
            //
            // Set the Hypervisor Present-bit in RCX, which Intel and AMD have both
            // reserved for this indication
            //
            CpuInfo[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
        }
        else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS)
        {
            //
            // Return a maximum supported hypervisor CPUID leaf range and a vendor
            // ID signature as required by the spec
            //

            CpuInfo[0] = HYPERV_CPUID_INTERFACE;
            CpuInfo[1] = 'epyH'; // [HyperDbg]
            CpuInfo[2] = 'gbDr';
            CpuInfo[3] = 0;
        }
        else if (Regs->rax == HYPERV_CPUID_INTERFACE)
        {
            //
            // Return non Hv#1 value. This indicate that our hypervisor does NOT
            // conform to the Microsoft hypervisor interface.
            //

            CpuInfo[0] = '0#vH'; // Hv#0
            CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0;
        }
    }
    else
    {
        TransparentCheckAndModifyCpuid(Regs, CpuInfo);
    }

    //
    // Copy the values from the logical processor registers into the VP GPRs
    //
    Regs->rax = CpuInfo[0];
    Regs->rbx = CpuInfo[1];
    Regs->rcx = CpuInfo[2];
    Regs->rdx = CpuInfo[3];
}

/**
 * @brief Handles Guest Access to control registers
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
HvHandleControlRegisterAccess(VIRTUAL_MACHINE_STATE *         VCpu,
                              VMX_EXIT_QUALIFICATION_MOV_CR * CrExitQualification)
{
    UINT64 * RegPtr;
    UINT64   NewCr3;
    CR3_TYPE NewCr3Reg;

    RegPtr = (UINT64 *)&VCpu->Regs->rax + CrExitQualification->GeneralPurposeRegister;

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
        VmxVmread64P(VMCS_GUEST_RSP, &GuestRsp);
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

            VmxVmwrite64(VMCS_GUEST_CR0, *RegPtr);
            VmxVmwrite64(VMCS_CTRL_CR0_READ_SHADOW, *RegPtr);

            break;

        case VMX_EXIT_QUALIFICATION_REGISTER_CR3:

            NewCr3          = (*RegPtr & ~(1ULL << 63));
            NewCr3Reg.Flags = NewCr3;

            //
            // Apply the new cr3
            //
            VmxVmwrite64(VMCS_GUEST_CR3, NewCr3Reg.Flags);

            //
            // Invalidate as we used VPID tags so the vm-exit won't
            // normally (automatically) flush the TLB, we have to do
            // it manually
            //
            VpidInvvpidSingleContext(VPID_TAG);

            //
            // Call kernel debugger handler for mov to cr3 in kernel debugger
            //
            InterceptionCallbackTriggerCr3ProcessChange(VCpu->CoreId);

            //
            // Call handler of the reversing machine
            //
            if (g_ExecTrapInitialized)
            {
                ExecTrapHandleCr3Vmexit(VCpu);
            }

            break;

        case VMX_EXIT_QUALIFICATION_REGISTER_CR4:

            VmxVmwrite64(VMCS_GUEST_CR4, *RegPtr);
            VmxVmwrite64(VMCS_CTRL_CR4_READ_SHADOW, *RegPtr);

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

            VmxVmread64P(VMCS_GUEST_CR0, RegPtr);

            break;

        case VMX_EXIT_QUALIFICATION_REGISTER_CR3:

            VmxVmread64P(VMCS_GUEST_CR3, RegPtr);

            break;

        case VMX_EXIT_QUALIFICATION_REGISTER_CR4:

            VmxVmread64P(VMCS_GUEST_CR4, RegPtr);

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
HvFillGuestSelectorData(PVOID GdtBase, UINT32 SegmentRegister, UINT16 Selector)
{
    VMX_SEGMENT_SELECTOR SegmentSelector = {0};

    SegmentGetDescriptor(GdtBase, Selector, &SegmentSelector);

    if (Selector == 0x0)
    {
        SegmentSelector.Attributes.Unusable = TRUE;
    }

    SegmentSelector.Attributes.Reserved1 = 0;
    SegmentSelector.Attributes.Reserved2 = 0;

    VmxVmwrite64(VMCS_GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    VmxVmwrite64(VMCS_GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.Limit);
    VmxVmwrite64(VMCS_GUEST_ES_ACCESS_RIGHTS + SegmentRegister * 2, SegmentSelector.Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.Base);
}

/**
 * @brief Add the current instruction length to guest rip to resume to next instruction
 *
 * @return VOID
 */
VOID
HvResumeToNextInstruction()
{
    UINT64 ResumeRIP             = NULL64_ZERO;
    UINT64 CurrentRIP            = NULL64_ZERO;
    SIZE_T ExitInstructionLength = 0;

    VmxVmread64P(VMCS_GUEST_RIP, &CurrentRIP);
    VmxVmread64P(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstructionLength);

    ResumeRIP = CurrentRIP + ExitInstructionLength;

    VmxVmwrite64(VMCS_GUEST_RIP, ResumeRIP);
}

/**
 * @brief Suppress the incrementation of RIP
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
inline VOID
HvSuppressRipIncrement(VIRTUAL_MACHINE_STATE * VCpu)
{
    VCpu->IncrementRip = FALSE;
}

/**
 * @brief Perform the incrementation of RIP
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
inline VOID
HvPerformRipIncrement(VIRTUAL_MACHINE_STATE * VCpu)
{
    VCpu->IncrementRip = TRUE;
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
    UINT32 CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_MONITOR_TRAP_FLAG_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set the rflag's trap flag
 *
 * @param Set Set or unset the TF
 *
 * @return VOID
 */
VOID
HvSetRflagTrapFlag(BOOLEAN Set)
{
    RFLAGS Rflags = {0};

    //
    // Unset the trap-flag, as we set it before we have to mask it now
    //
    Rflags.AsUInt = HvGetRflags();

    Rflags.TrapFlag = Set;

    HvSetRflags(Rflags.AsUInt);
}

/**
 * @brief Set LOAD DEBUG CONTROLS on Vm-entry controls
 *
 * @param VCpu
 * @param Set Set or unset
 * @return VOID
 */
VOID
HvSetLoadDebugControls(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetLoadDebugControls(VCpu, Set);
}

/**
 * @brief Set LOAD GUEST IA32_LBR_CTL on Vm-entry controls
 *
 * @param VCpu
 * @param Set Set or unset
 * @return VOID
 */
VOID
HvSetLoadGuestIa32LbrCtl(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    UNREFERENCED_PARAMETER(VCpu);

    UINT32 VmentryControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_VMENTRY_CONTROLS, &VmentryControls);

    if (Set)
    {
        VmentryControls |= IA32_VMX_ENTRY_CTLS_LOAD_IA32_LBR_CTL_FLAG;
    }
    else
    {
        VmentryControls &= ~IA32_VMX_ENTRY_CTLS_LOAD_IA32_LBR_CTL_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite32(VMCS_CTRL_VMENTRY_CONTROLS, VmentryControls);
}

/**
 * @brief Set SAVE DEBUG CONTROLS on Vm-exit controls
 *
 * @param VCpu
 * @param Set Set or unset
 * @return VOID
 */
VOID
HvSetSaveDebugControls(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetSaveDebugControls(VCpu, Set);
}

/**
 * @brief Set SAVE GUEST IA32_LBR_CTL on Vm-exit controls
 *
 * @param VCpu
 * @param Set Set or unset
 * @return VOID
 */
VOID
HvSetClearGuestIa32LbrCtl(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    UNREFERENCED_PARAMETER(VCpu);

    UINT32 VmexitControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmexitControls);

    if (Set)
    {
        VmexitControls |= IA32_VMX_EXIT_CTLS_CLEAR_IA32_LBR_CTL_FLAG;
    }
    else
    {
        VmexitControls &= ~IA32_VMX_EXIT_CTLS_CLEAR_IA32_LBR_CTL_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite32(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmexitControls);
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
    UINT16 DsSelector;
    UINT16 EsSelector;
    UINT16 SsSelector;
    UINT16 FsSelector;

    //
    // Restore FS Base
    //
    VmxVmread64P(VMCS_GUEST_FS_BASE, &FsBase);
    CpuWriteMsr(IA32_FS_BASE, FsBase);

    //
    // Restore Gs Base
    //
    VmxVmread64P(VMCS_GUEST_GS_BASE, &GsBase);
    CpuWriteMsr(IA32_GS_BASE, GsBase);

    //
    // Restore GDTR
    //
    VmxVmread64P(VMCS_GUEST_GDTR_BASE, &GdtrBase);
    VmxVmread64P(VMCS_GUEST_GDTR_LIMIT, &GdtrLimit);

    AsmReloadGdtr((PVOID)GdtrBase, (ULONG)GdtrLimit);

    //
    // Restore Segment Selector
    //
    VmxVmread16P(VMCS_GUEST_DS_SELECTOR, &DsSelector);
    AsmSetDs(DsSelector);
    VmxVmread16P(VMCS_GUEST_ES_SELECTOR, &EsSelector);
    AsmSetEs(EsSelector);
    VmxVmread16P(VMCS_GUEST_SS_SELECTOR, &SsSelector);
    AsmSetSs(SsSelector);
    VmxVmread16P(VMCS_GUEST_FS_SELECTOR, &FsSelector);
    AsmSetFs(FsSelector);

    //
    // Restore IDTR
    //
    VmxVmread64P(VMCS_GUEST_IDTR_BASE, &IdtrBase);
    VmxVmread64P(VMCS_GUEST_IDTR_LIMIT, &IdtrLimit);

    AsmReloadIdtr((PVOID)IdtrBase, (ULONG)IdtrLimit);
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
    UINT32 CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
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
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the vm-exits
 *
 * @return VOID
 */
VOID
HvSetMovToCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetMov2Cr3Exiting(VCpu, Set);
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
    VmxVmwrite64(VMCS_CTRL_EXCEPTION_BITMAP, BitmapMask);
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
    VmxVmread32P(VMCS_CTRL_EXCEPTION_BITMAP, &ExceptionBitmap);

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
    UINT32 CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    //
    // interrupt-window exiting
    //
    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_INTERRUPT_WINDOW_EXITING_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Set Page Modification Logging Enable bit
 *
 * @param Set Set or unset the PML
 * @return VOID
 */
VOID
HvSetPmlEnableFlag(BOOLEAN Set)
{
    UINT32 AdjSecCtrl;
    UINT32 SecondaryProcBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &SecondaryProcBasedVmExecControls);

    //
    // PML enable flag
    //
    if (Set)
    {
        SecondaryProcBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_FLAG;
    }
    else
    {
        SecondaryProcBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_FLAG;
    }

    AdjSecCtrl = HvAdjustControls(SecondaryProcBasedVmExecControls, IA32_VMX_PROCBASED_CTLS2);

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, AdjSecCtrl);
}

/**
 * @brief Set Mode-based Execution Control (MBEC) Enable bit
 *
 * @param Set Set or unset the MBEC
 * @return VOID
 */
VOID
HvSetModeBasedExecutionEnableFlag(BOOLEAN Set)
{
    UINT32 AdjSecCtrl;
    UINT32 SecondaryProcBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &SecondaryProcBasedVmExecControls);

    //
    // PML enable flag
    //
    if (Set)
    {
        SecondaryProcBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT_FLAG;
    }
    else
    {
        SecondaryProcBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS2_MODE_BASED_EXECUTE_CONTROL_FOR_EPT_FLAG;
    }

    AdjSecCtrl = HvAdjustControls(SecondaryProcBasedVmExecControls, IA32_VMX_PROCBASED_CTLS2);

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, AdjSecCtrl);
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
    UINT32 CpuBasedVmExecControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    //
    // interrupt-window exiting
    //
    if (Set)
    {
        CpuBasedVmExecControls |= IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING_FLAG;
    }
    else
    {
        CpuBasedVmExecControls &= ~IA32_VMX_PROCBASED_CTLS_NMI_WINDOW_EXITING_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);
}

/**
 * @brief Handle Mov to Debug Registers Exitings
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
HvHandleMovDebugRegister(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMX_EXIT_QUALIFICATION_MOV_DR ExitQualification;
    CR4                           Cr4;
    DR7                           Dr7;
    VMX_SEGMENT_SELECTOR          Cs;
    UINT64 *                      GpRegs = (UINT64 *)VCpu->Regs;

    //
    // The implementation is derived from Hvpp
    //
    VmxVmread64P(VMCS_EXIT_QUALIFICATION, &ExitQualification.AsUInt);

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
        HvSuppressRipIncrement(VCpu);
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
    VmxVmread64P(VMCS_GUEST_CR4, &Cr4.AsUInt);

    if (ExitQualification.DebugRegister == 4 || ExitQualification.DebugRegister == 5)
    {
        if (Cr4.DebuggingExtensions)
        {
            //
            // re-inject #UD
            //
            EventInjectUndefinedOpcode(VCpu);
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
    VmxVmread64P(VMCS_GUEST_DR7, &Dr7.AsUInt);

    if (Dr7.GeneralDetect)
    {
        DR6 Dr6 = {
            .AsUInt                      = CpuReadDr(6),
            .BreakpointCondition         = 0,
            .DebugRegisterAccessDetected = TRUE};

        CpuWriteDr(6, Dr6.AsUInt);

        Dr7.GeneralDetect = FALSE;

        VmxVmwrite64(VMCS_GUEST_DR7, Dr7.AsUInt);

        EventInjectDebugBreakpoint();

        //
        // Redo the instruction
        //
        HvSuppressRipIncrement(VCpu);

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
        HvSuppressRipIncrement(VCpu);
        return;
    }

    switch (ExitQualification.DirectionOfAccess)
    {
    case VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR:
        switch (ExitQualification.DebugRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_DR0:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR0, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR1:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR1, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR2:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR2, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR3:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR3, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR6:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR6, GpRegister);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR7:
            CpuWriteDr(VMX_EXIT_QUALIFICATION_REGISTER_DR7, GpRegister);
            break;
        default:
            break;
        }
        break;

    case VMX_EXIT_QUALIFICATION_DIRECTION_MOV_FROM_DR:
        switch (ExitQualification.DebugRegister)
        {
        case VMX_EXIT_QUALIFICATION_REGISTER_DR0:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR0);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR1:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR1);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR2:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR2);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR3:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR3);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR6:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR6);
            break;
        case VMX_EXIT_QUALIFICATION_REGISTER_DR7:
            GpRegister = CpuReadDr(VMX_EXIT_QUALIFICATION_REGISTER_DR7);
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
    UINT32 PinBasedControls = 0;
    UINT32 VmExitControls   = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);
    VmxVmread32P(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmExitControls);

    if (Set)
    {
        PinBasedControls |= IA32_VMX_PINBASED_CTLS_NMI_EXITING_FLAG;
        VmExitControls |= IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG;
    }
    else
    {
        PinBasedControls &= ~IA32_VMX_PINBASED_CTLS_NMI_EXITING_FLAG;
        VmExitControls &= ~IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
    VmxVmwrite64(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, VmExitControls);
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
    UINT32 PinBasedControls = 0;

    //
    // Read the previous flags
    //
    VmxVmread32P(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &PinBasedControls);

    if (Set)
    {
        PinBasedControls |= IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER_FLAG;
    }
    else
    {
        PinBasedControls &= ~IA32_VMX_PINBASED_CTLS_ACTIVATE_VMX_PREEMPTION_TIMER_FLAG;
    }

    //
    // Set the new value
    //
    VmxVmwrite64(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, PinBasedControls);
}

/**
 * @brief Set exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
HvSetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvSetExceptionBitmap(VCpu, IdtIndex);
}

/**
 * @brief Unset exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param VCpu The virtual processor's state
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
HvUnsetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvUnsetExceptionBitmap(VCpu, IdtIndex);
}

/**
 * @brief Set the External Interrupt Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID
 */
VOID
HvSetExternalInterruptExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    //
    // This is a wrapper to perform extra checks
    //
    ProtectedHvSetExternalInterruptExiting(VCpu, Set);
}

/**
 * @brief Checks to enable and reinject previous interrupts
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the External Interrupt Exiting
 *
 * @return VOID
 */
VOID
HvEnableAndCheckForPreviousExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Check if we should enable interrupts in this core or not,
    // we have another same check in SWITCHING CORES too
    //
    if (VCpu->EnableExternalInterruptsOnContinueMtf)
    {
        //
        // Enable normal interrupts
        //
        HvSetExternalInterruptExiting(VCpu, FALSE);

        //
        // Check if there is at least an interrupt that needs to be delivered
        //
        if (VCpu->PendingExternalInterrupts[0] != NULL_ZERO)
        {
            //
            // Enable Interrupt-window exiting.
            //
            HvSetInterruptWindowExiting(TRUE);
        }

        VCpu->EnableExternalInterruptsOnContinueMtf = FALSE;
    }
}

/**
 * @brief Set the RDTSC/P Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the RDTSC/P Exiting
 * @return VOID
 */
VOID
HvSetRdtscExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetRdtscExiting(VCpu, Set);
}

/**
 * @brief Set or unset the Mov to Debug Registers Exiting
 *
 * @param VCpu The virtual processor's state
 * @param Set Set or unset the Mov to Debug Registers Exiting
 * @return VOID
 */
VOID
HvSetMovDebugRegsExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set)
{
    ProtectedHvSetMovDebugRegsExiting(VCpu, Set);
}

/**
 * @brief Read CS selector
 *
 * @return UINT16
 */
UINT16
HvGetCsSelector()
{
    //
    // Only 16 bit is needed however, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64 CsSel = NULL64_ZERO;

    VmxVmread64P(VMCS_GUEST_CS_SELECTOR, &CsSel);

    return CsSel & 0xffff;
}

/**
 * @brief Read guest's RFLAGS
 *
 * @return UINT64
 */
UINT64
HvGetRflags()
{
    UINT64 Rflags = NULL64_ZERO;

    VmxVmread64P(VMCS_GUEST_RFLAGS, &Rflags);

    return Rflags;
}

/**
 * @brief Set guest's RFLAGS
 * @param Rflags
 *
 * @return VOID
 */
VOID
HvSetRflags(UINT64 Rflags)
{
    VmxVmwrite64(VMCS_GUEST_RFLAGS, Rflags);
}

/**
 * @brief Read guest's RIP
 *
 * @return UINT64
 */
UINT64
HvGetRip()
{
    UINT64 Rip = NULL64_ZERO;

    VmxVmread64P(VMCS_GUEST_RIP, &Rip);

    return Rip;
}

/**
 * @brief Set guest's RIP
 * @param Rip
 *
 * @return VOID
 */
VOID
HvSetRip(UINT64 Rip)
{
    VmxVmwrite64(VMCS_GUEST_RIP, Rip);
}

/**
 * @brief Read guest's interruptibility state
 *
 * @return UINT64
 */
UINT64
HvGetInterruptibilityState()
{
    UINT64 InterruptibilityState = NULL64_ZERO;

    VmxVmread64P(VMCS_GUEST_INTERRUPTIBILITY_STATE, &InterruptibilityState);

    return InterruptibilityState;
}

/**
 * @brief Clear STI and MOV SS bits
 *
 * @return UINT64
 */
UINT64
HvClearSteppingBits(UINT64 Interruptibility)
{
    UINT64 InterruptibilityState = Interruptibility;

    InterruptibilityState &= ~(GUEST_INTR_STATE_STI | GUEST_INTR_STATE_MOV_SS);

    return InterruptibilityState;
}

/**
 * @brief Set guest's interruptibility state
 * @param InterruptibilityState
 *
 * @return VOID
 */
VOID
HvSetInterruptibilityState(UINT64 InterruptibilityState)
{
    VmxVmwrite64(VMCS_GUEST_INTERRUPTIBILITY_STATE, InterruptibilityState);
}

/**
 * @brief Inject pending external interrupts
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvInjectPendingExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Check if there is at least an interrupt that needs to be delivered
    //
    if (VCpu->PendingExternalInterrupts[0] != NULL_ZERO)
    {
        //
        // Enable Interrupt-window exiting.
        //
        HvSetInterruptWindowExiting(TRUE);
    }
}

/**
 * @brief Check and enable external interrupts
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvCheckAndEnableExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Check if we should enable interrupts in this core or not
    //
    if (VCpu->EnableExternalInterruptsOnContinue)
    {
        //
        // Enable normal interrupts
        //
        HvSetExternalInterruptExiting(VCpu, FALSE);

        //
        // Check if there is at least an interrupt that needs to be delivered
        //
        HvInjectPendingExternalInterrupts(VCpu);

        VCpu->EnableExternalInterruptsOnContinue = FALSE;
    }
}

/**
 * @brief Disable external-interrupts and interrupt window
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvDisableExternalInterruptsAndInterruptWindow(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change guest interrupt-state
    //
    HvSetExternalInterruptExiting(VCpu, TRUE);

    //
    // Do not vm-exit on interrupt windows
    //
    HvSetInterruptWindowExiting(FALSE);

    VCpu->EnableExternalInterruptsOnContinue = TRUE;
}

/**
 * @brief Initializes the hypervisor
 * @param VmmCallbacks
 *
 * @return BOOLEAN
 */
BOOLEAN
HvInitVmm(VMM_CALLBACKS * VmmCallbacks)
{
    ULONG   ProcessorsCount;
    BOOLEAN Result = FALSE;

    //
    // Save the callbacks
    //
    RtlCopyMemory(&g_Callbacks, VmmCallbacks, sizeof(VMM_CALLBACKS));

    //
    // Check and define compatibility checks and processor constraints
    //
    CompatibilityCheckPerformChecks();

    //
    // we allocate virtual machine here because
    // we want to use its state (vmx-root or vmx non-root) in logs
    //
    Result = GlobalGuestStateAllocateZeroedMemory();

    if (!Result)
    {
        return FALSE;
    }

    //
    // We have a zeroed guest state
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Set the core's id and initialize memory mapper
    //
    for (UINT32 i = 0; i < ProcessorsCount; i++)
    {
        g_GuestState[i].CoreId = i;
    }

    //
    // Initialize memory mapper
    //
    MemoryMapperInitialize();

    //
    // Make sure that transparent-mode is disabled
    //
    if (g_CheckForFootprints)
    {
        TransparentUnhideDebuggerWrapper(NULL);
    }

    //
    // Not waiting for the interrupt-window to inject page-faults
    //
    g_WaitingForInterruptWindowToInjectPageFault = FALSE;

    //
    // Initializes VMX
    //
    return VmxInitialize();
}

/**
 * @brief Enables MTF and adjust external interrupt state
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvEnableMtfAndChangeExternalInterruptState(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // We have to set Monitor trap flag and give it the HookedEntry to work with
    //
    HvSetMonitorTrapFlag(TRUE);

    //
    // The following codes are added because we realized if the execution takes long then
    // the execution might be switched to another routines, thus, MTF might conclude on
    // another routine and we might (and will) trigger the same instruction soon
    //

    //
    // Change guest interrupt-state
    //
    HvSetExternalInterruptExiting(VCpu, TRUE);

    //
    // Do not vm-exit on interrupt windows
    //
    HvSetInterruptWindowExiting(FALSE);

    //
    // Indicate that we should enable external interrupts and configure external interrupt
    // window exiting somewhere at MTF
    //
    VCpu->EnableExternalInterruptsOnContinueMtf = TRUE;
}

/**
 * @brief Adjust external interrupt state
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
HvPreventExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change guest interrupt-state
    //
    HvSetExternalInterruptExiting(VCpu, TRUE);

    //
    // Do not vm-exit on interrupt windows
    //
    HvSetInterruptWindowExiting(FALSE);

    //
    // Indicate that we should enable external interrupts and configure external interrupt
    // window exiting somewhere at MTF
    //
    VCpu->EnableExternalInterruptsOnContinueMtf = TRUE;
}

/**
 * @brief Get the guest state of pending debug exceptions
 *
 * @return UINT64
 */
UINT64
HvGetPendingDebugExceptions()
{
    UINT64 Value;
    VmxVmread64P(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, &Value);

    return Value;
}

/**
 * @brief Set the guest state of pending debug exceptions
 * @param Value The new state
 *
 * @return VOID
 */
VOID
HvSetPendingDebugExceptions(UINT64 Value)
{
    VmxVmwrite64(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, Value);
}

/**
 * @brief Get the guest state of IA32_DEBUGCTL
 *
 * @return UINT64
 */
UINT64
HvGetDebugctl()
{
    UINT32 LowPart;
    UINT32 HighPart;

    VmxVmread32P(VMCS_GUEST_DEBUGCTL, &LowPart);
    VmxVmread32P(VMCS_GUEST_DEBUGCTL_HIGH, &HighPart);

    return (UINT64)HighPart << 32 | LowPart;
}

/**
 * @brief Get the guest state of IA32_LBR_CTL
 *
 * @return UINT64
 */
UINT64
HvGetGuestIa32LbrCtl()
{
    UINT64 GuestIa32LbrCtl;

    VmxVmread64P(VMCS_GUEST_LBR_CTL, &GuestIa32LbrCtl);

    return GuestIa32LbrCtl;
}

/**
 * @brief Get and store the guest state of IA32_DEBUGCTL
 * @details mainly used from the VMCALL handler
 *
 * @param StoreDebugctl
 *
 * @return VOID
 */
VOID
HvGetAndStoreDebugctl(UINT64 * StoreDebugctl)
{
    UINT64 DebugctlValue;

    //
    // Read DEBUGCTL from VMCS
    //
    DebugctlValue = HvGetDebugctl();

    //
    // Store the DEBUGCTL
    //
    *StoreDebugctl = DebugctlValue;
}

/**
 * @brief Get and store the guest state of IA32_LBR_CTL
 * @details mainly used from the VMCALL handler
 *
 * @param StoreGuestIa32Lbr
 *
 * @return VOID
 */
VOID
HvGetAndStoreGuestIa32LbrCtl(UINT64 * StoreGuestIa32Lbr)
{
    UINT64 GuestIa32LbrCtl;

    //
    // Read IA32_LBR_CTL from VMCS
    //
    GuestIa32LbrCtl = HvGetGuestIa32LbrCtl();

    //
    // Store the IA32_LBR_CTL
    //
    *StoreGuestIa32Lbr = GuestIa32LbrCtl;
}

/**
 * @brief Set the guest state of IA32_DEBUGCTL
 * @param Value The new state
 *
 * @return VOID
 */
VOID
HvSetDebugctl(UINT64 Value)
{
    VmxVmwrite32(VMCS_GUEST_DEBUGCTL, Value & 0xFFFFFFFF);
    VmxVmwrite32(VMCS_GUEST_DEBUGCTL_HIGH, Value >> 32);
}

/**
 * @brief Set the guest state of IA32_LBR_CTL
 * @param Value The new state
 *
 * @return VOID
 */
VOID
HvSetGuestIa32LbrCtl(UINT64 Value)
{
    VmxVmwrite64(VMCS_GUEST_LBR_CTL, Value);
}

/**
 * @brief Set LBR selector
 * @details If VMM is active, this should be done in vmx-root, otherwise, it doesn't work
 * @param FilterOptions The value to write on MSR_LEGACY_LBR_SELECT
 *
 * @return VOID
 */
VOID
HvSetLbrSelect(UINT64 FilterOptions)
{
    CpuWriteMsr(MSR_LEGACY_LBR_SELECT, FilterOptions);
}

/**
 * @brief Check if CPU support save and load debug controls on exit and load entries
 *
 * @return BOOLEAN
 */
BOOLEAN
HvCheckCpuSupportForSaveAndLoadDebugControls()
{
    IA32_VMX_BASIC_REGISTER VmxBasicMsr = {0};

    //
    // Reading IA32_VMX_BASIC_MSR
    //
    VmxBasicMsr.AsUInt = CpuReadMsr(IA32_VMX_BASIC);

    //
    // Read 1-settings of save debug controls (exit controls)
    //
    UINT32 ExitCtls = HvAdjustControls(
        IA32_VMX_EXIT_CTLS_SAVE_DEBUG_CONTROLS_FLAG,
        VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS);

    //
    // Read 1-settings of load debug controls (entry controls)
    //
    UINT32 EntryCtls = HvAdjustControls(
        IA32_VMX_ENTRY_CTLS_LOAD_DEBUG_CONTROLS_FLAG,
        VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS);

    //
    // Check if entry and exit controls are supported on this system
    //
    if (ExitCtls != NULL_ZERO && EntryCtls != NULL_ZERO)
    {
        //
        // Supported
        //
        return TRUE;
    }
    else
    {
        //
        // Not supported
        //
        return FALSE;
    }
}

/**
 * @brief Check if CPU support load and clear guest IA32_LBR_CTL controls on entry and exit
 *
 * @return BOOLEAN
 */
BOOLEAN
HvCheckCpuSupportForLoadAndClearGuestIa32LbrCtlControls()
{
    IA32_VMX_BASIC_REGISTER VmxBasicMsr = {0};

    //
    // Reading IA32_VMX_BASIC_MSR
    //
    VmxBasicMsr.AsUInt = CpuReadMsr(IA32_VMX_BASIC);

    //
    // Read 1-settings of save debug controls (exit controls)
    //
    UINT32 ExitCtls = HvAdjustControls(
        IA32_VMX_EXIT_CTLS_CLEAR_IA32_LBR_CTL_FLAG,
        VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS);

    //
    // Read 1-settings of load debug controls (entry controls)
    //
    UINT32 EntryCtls = HvAdjustControls(
        IA32_VMX_ENTRY_CTLS_LOAD_IA32_LBR_CTL_FLAG,
        VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS);

    //
    // Check if entry and exit controls are supported on this system
    //
    if (ExitCtls != NULL_ZERO && EntryCtls != NULL_ZERO)
    {
        //
        // Supported
        //
        return TRUE;
    }
    else
    {
        //
        // Not supported
        //
        return FALSE;
    }
}

/**
 * @brief Set the guest state of DR7
 * @param Value The new value for DR7
 *
 * @return VOID
 */
VOID
HvSetDebugReg7(UINT64 Value)
{
    VmxVmwrite64(VMCS_GUEST_DR7, Value);
}

/**
 * @brief Handle the case when the trap flag is set, and
 * we need to inject the single-step exception right
 * after vm-entry
 *
 * @return VOID
 */
VOID
HvHandleTrapFlag()
{
    IA32_DEBUGCTL_REGISTER       Debugctl    = {.AsUInt = HvGetDebugctl()};
    RFLAGS                       GuestRFlags = {.AsUInt = GetGuestRFlags()};
    VMX_PENDING_DEBUG_EXCEPTIONS PendingDebugExceptions;
    VMX_INTERRUPTIBILITY_STATE   InterruptibilityState;

    //
    // The btf flag means that the trap flag generates the single-step exception
    // only for branch instructions
    //
    if (GuestRFlags.TrapFlag && !Debugctl.Btf)
    {
        PendingDebugExceptions.AsUInt = HvGetPendingDebugExceptions();
        PendingDebugExceptions.Bs     = 1;
        HvSetPendingDebugExceptions(PendingDebugExceptions.AsUInt);

        InterruptibilityState.AsUInt = (UINT32)HvGetInterruptibilityState();

        //
        // We also must clear this flag in case of instruction emulation to achieve
        // correctness of the single-step exception
        //
        if (InterruptibilityState.BlockingByMovSs)
        {
            InterruptibilityState.BlockingByMovSs = 0;
            HvSetInterruptibilityState(InterruptibilityState.AsUInt);
        }
    }
}
