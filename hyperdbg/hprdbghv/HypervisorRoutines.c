/**
 * @file HypervisorRoutines.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief This file describes the routines in Hypervisor
 * @details vmx related routines
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "Msr.h"
#include "Vmx.h"
#include "Common.h"
#include "GlobalVariables.h"
#include "HypervisorRoutines.h"
#include "Invept.h"
#include "InlineAsm.h"
#include "Vpid.h"
#include "Vmcall.h"
#include "Dpc.h"

/**
 * @brief Initialize Vmx operation
 * 
 * @return BOOLEAN Returns true if vmx initialized successfully
 */
BOOLEAN
HvVmxInitialize()
{
    int                LogicalProcessorsCount;
    IA32_VMX_BASIC_MSR VmxBasicMsr = {0};

    //
    // ****** Start Virtualizing Current System ******
    //

    //
    // Initiating EPTP and VMX
    //
    if (!VmxInitializer())
    {
        //
        // there was error somewhere in initializing
        //
        return FALSE;
    }

    LogicalProcessorsCount = KeQueryActiveProcessorCount(0);

    for (size_t ProcessorID = 0; ProcessorID < LogicalProcessorsCount; ProcessorID++)
    {
        //
        // *** Launching VM for Test (in the all logical processor) ***
        //

        //
        //Allocating VMM Stack
        //
        if (!VmxAllocateVmmStack(ProcessorID))
        {
            //
            // Some error in allocating Vmm Stack
            //
            return FALSE;
        }

        //
        // Allocating MSR Bit
        //
        if (!VmxAllocateMsrBitmap(ProcessorID))
        {
            //
            // Some error in allocating Msr Bitmaps
            //
            return FALSE;
        }
    }

    //
    // As we want to support more than 32 processor (64 logical-core) we let windows execute our routine for us
    //
    KeGenericCallDpc(HvDpcBroadcastInitializeGuest, 0x0);

    //
    // Check if everything is ok then return true otherwise false
    //
    if (AsmVmxVmcall(VMCALL_TEST, 0x22, 0x333, 0x4444) == STATUS_SUCCESS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Check whether VMX Feature is supported or not
 * 
 * @return BOOLEAN Returns true if vmx is supported or false if it's not supported
 */
BOOLEAN
HvIsVmxSupported()
{
    CPUID                    Data              = {0};
    IA32_FEATURE_CONTROL_MSR FeatureControlMsr = {0};

    //
    // VMX bit
    //
    __cpuid((int *)&Data, 1);
    if ((Data.ecx & (1 << 5)) == 0)
        return FALSE;

    FeatureControlMsr.All = __readmsr(MSR_IA32_FEATURE_CONTROL);

    //
    // BIOS lock check
    //
    if (FeatureControlMsr.Fields.Lock == 0)
    {
        FeatureControlMsr.Fields.Lock        = TRUE;
        FeatureControlMsr.Fields.EnableVmxon = TRUE;
        __writemsr(MSR_IA32_FEATURE_CONTROL, FeatureControlMsr.All);
    }
    else if (FeatureControlMsr.Fields.EnableVmxon == FALSE)
    {
        LogError("Intel VMX feature is locked in BIOS");
        return FALSE;
    }

    return TRUE;
}

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

    MsrValue.Content = __readmsr(Msr);
    Ctl &= MsrValue.High; /* bit == 0 in high word ==> must be zero */
    Ctl |= MsrValue.Low;  /* bit == 1 in low word  ==> must be one  */
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
HvSetGuestSelector(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector)
{
    SEGMENT_SELECTOR SegmentSelector = {0};
    ULONG            AccessRights;

    HvGetSegmentDescriptor(&SegmentSelector, Selector, GdtBase);
    AccessRights = ((PUCHAR)&SegmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&SegmentSelector.ATTRIBUTES)[1] << 12);

    if (!Selector)
        AccessRights |= 0x10000;

    __vmx_vmwrite(GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    __vmx_vmwrite(GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.LIMIT);
    __vmx_vmwrite(GUEST_ES_AR_BYTES + SegmentRegister * 2, AccessRights);
    __vmx_vmwrite(GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.BASE);

    return TRUE;
}

/**
 * @brief Get Segment Descriptor
 * 
 * @param SegmentSelector 
 * @param Selector 
 * @param GdtBase 
 * @return BOOLEAN 
 */
BOOLEAN
HvGetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase)
{
    PSEGMENT_DESCRIPTOR SegDesc;

    if (!SegmentSelector)
        return FALSE;

    if (Selector & 0x4)
    {
        return FALSE;
    }

    SegDesc = (PSEGMENT_DESCRIPTOR)((PUCHAR)GdtBase + (Selector & ~0x7));

    SegmentSelector->SEL               = Selector;
    SegmentSelector->BASE              = SegDesc->BASE0 | SegDesc->BASE1 << 16 | SegDesc->BASE2 << 24;
    SegmentSelector->LIMIT             = SegDesc->LIMIT0 | (SegDesc->LIMIT1ATTR1 & 0xf) << 16;
    SegmentSelector->ATTRIBUTES.UCHARs = SegDesc->ATTR0 | (SegDesc->LIMIT1ATTR1 & 0xf0) << 4;

    if (!(SegDesc->ATTR0 & 0x10))
    {
        //
        // LA_ACCESSED
        //
        ULONG64 tmp;

        //
        // this is a TSS or callgate etc, save the base high part
        //
        tmp                   = (*(PULONG64)((PUCHAR)SegDesc + 8));
        SegmentSelector->BASE = (SegmentSelector->BASE & 0xffffffff) | (tmp << 32);
    }

    if (SegmentSelector->ATTRIBUTES.Fields.G)
    {
        //
        // 4096-bit granularity is enabled for this segment, scale the limit
        //
        SegmentSelector->LIMIT = (SegmentSelector->LIMIT << 12) + 0xfff;
    }

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
    INT32  cpu_info[4];
    ULONG  Mode    = 0;
    UINT64 Context = 0;

    //
    // Set the context (save eax for the debugger)
    //
    Context = RegistersState->rax;

    //
    // Otherwise, issue the CPUID to the logical processor based on the indexes
    // on the VP's GPRs.
    //
    __cpuidex(cpu_info, (INT32)RegistersState->rax, (INT32)RegistersState->rcx);

    //
    // Check if this was CPUID 1h, which is the features request
    //
    if (RegistersState->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        //
        // Set the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        cpu_info[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (RegistersState->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS)
    {
        //
        // Return a maximum supported hypervisor CPUID leaf range and a vendor
        // ID signature as required by the spec
        //

        cpu_info[0] = HYPERV_CPUID_INTERFACE;
        cpu_info[1] = 'epyH'; /* "[Hyperdbg] [H]yper[v]isor = HyperdbgHv" */
        cpu_info[2] = 'gbdr';
        cpu_info[3] = '  vH';
    }
    else if (RegistersState->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // Return non Hv#1 value. This indicate that our hypervisor does NOT
        // conform to the Microsoft hypervisor interface.
        //

        cpu_info[0] = '0#vH'; // Hv#0
        cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
    }

    //
    // Copy the values from the logical processor registers into the VP GPRs
    //
    RegistersState->rax = cpu_info[0];
    RegistersState->rbx = cpu_info[1];
    RegistersState->rcx = cpu_info[2];
    RegistersState->rdx = cpu_info[3];

    //
    // As the context to event trigger, we send the eax before the cpuid
    // so that the debugger can both read the eax as it's now changed by
    // the cpuid instruction and also can modify the results
    //
    DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION, RegistersState, Context);
}

/**
 * @brief Handles Guest Access to control registers
 * 
 * @param GuestState Guest's gp registers
 * @return VOID 
 */
VOID
HvHandleControlRegisterAccess(PGUEST_REGS GuestState)
{
    ULONG                 ExitQualification = 0;
    PMOV_CR_QUALIFICATION CrExitQualification;
    PULONG64              RegPtr;
    INT64                 GuestRsp = 0;
    UINT64                NewCr3;

    __vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);

    CrExitQualification = (PMOV_CR_QUALIFICATION)&ExitQualification;

    RegPtr = (PULONG64)&GuestState->rax + CrExitQualification->Fields.Register;

    //
    // Because its RSP and as we didn't save RSP correctly (because of pushes)
    // so we have make it points to the GUEST_RSP
    //
    if (CrExitQualification->Fields.Register == 4)
    {
        __vmx_vmread(GUEST_RSP, &GuestRsp);
        *RegPtr = GuestRsp;
    }

    switch (CrExitQualification->Fields.AccessType)
    {
    case TYPE_MOV_TO_CR:
    {
        switch (CrExitQualification->Fields.ControlRegister)
        {
        case 0:
            __vmx_vmwrite(GUEST_CR0, *RegPtr);
            __vmx_vmwrite(CR0_READ_SHADOW, *RegPtr);
            break;
        case 3:
            NewCr3 = (*RegPtr & ~(1ULL << 63));
            LogInfo("New process cr3 : 0x%llx , Proc id = : 0x%x", NewCr3, PsGetCurrentProcessId());
            __vmx_vmwrite(GUEST_CR3, (*RegPtr & ~(1ULL << 63)));
            InvvpidSingleContext(VPID_TAG);
            break;
        case 4:
            __vmx_vmwrite(GUEST_CR4, *RegPtr);
            __vmx_vmwrite(CR4_READ_SHADOW, *RegPtr);

            break;
        default:
            LogWarning("Unsupported register %d in handling control registers access", CrExitQualification->Fields.ControlRegister);
            break;
        }
    }
    break;

    case TYPE_MOV_FROM_CR:
    {
        switch (CrExitQualification->Fields.ControlRegister)
        {
        case 0:
            __vmx_vmread(GUEST_CR0, RegPtr);
            break;
        case 3:
            __vmx_vmread(GUEST_CR3, RegPtr);
            break;
        case 4:
            __vmx_vmread(GUEST_CR4, RegPtr);
            break;
        default:
            LogWarning("Unsupported register %d in handling control registers access", CrExitQualification->Fields.ControlRegister);
            break;
        }
    }
    break;

    default:
        LogWarning("Unsupported operation %d in handling control registers access", CrExitQualification->Fields.AccessType);
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
HvFillGuestSelectorData(PVOID GdtBase, ULONG SegmentRegister, USHORT Selector)
{
    SEGMENT_SELECTOR SegmentSelector = {0};
    ULONG            AccessRights;

    HvGetSegmentDescriptor(&SegmentSelector, Selector, GdtBase);
    AccessRights = ((PUCHAR)&SegmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&SegmentSelector.ATTRIBUTES)[1] << 12);

    if (!Selector)
        AccessRights |= 0x10000;

    __vmx_vmwrite(GUEST_ES_SELECTOR + SegmentRegister * 2, Selector);
    __vmx_vmwrite(GUEST_ES_LIMIT + SegmentRegister * 2, SegmentSelector.LIMIT);
    __vmx_vmwrite(GUEST_ES_AR_BYTES + SegmentRegister * 2, AccessRights);
    __vmx_vmwrite(GUEST_ES_BASE + SegmentRegister * 2, SegmentSelector.BASE);
}

/**
 * @brief Handles in the cases when RDMSR causes a vm-exit
 * 
 * @param GuestRegs Guest's gp registers
 * @return VOID 
 */
VOID
HvHandleMsrRead(PGUEST_REGS GuestRegs)
{
    MSR msr = {0};

    //
    // RDMSR. The RDMSR instruction causes a VM exit if any of the following are true:
    //
    // The "use MSR bitmaps" VM-execution control is 0.
    // The value of ECX is not in the ranges 00000000H - 00001FFFH and C0000000H - C0001FFFH
    // The value of ECX is in the range 00000000H - 00001FFFH and bit n in read bitmap for low MSRs is 1,
    //   where n is the value of ECX.
    // The value of ECX is in the range C0000000H - C0001FFFH and bit n in read bitmap for high MSRs is 1,
    //   where n is the value of ECX & 00001FFFH.
    //

    //
    // Execute WRMSR or RDMSR on behalf of the guest. Important that this
    // can cause bug check when the guest tries to access unimplemented MSR
    // even within the SEH block* because the below WRMSR or RDMSR raises
    // #GP and are not protected by the SEH block (or cannot be protected
    // either as this code run outside the thread stack region Windows
    // requires to proceed SEH). Hypervisors typically handle this by noop-ing
    // WRMSR and returning zero for RDMSR with non-architecturally defined
    // MSRs. Alternatively, one can probe which MSRs should cause #GP prior
    // to installation of a hypervisor and the hypervisor can emulate the
    // results.
    //

    //
    // Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    //
    if ((GuestRegs->rcx <= 0x00001FFF) || ((0xC0000000 <= GuestRegs->rcx) && (GuestRegs->rcx <= 0xC0001FFF)) || (GuestRegs->rcx >= RESERVED_MSR_RANGE_LOW && (GuestRegs->rcx <= RESERVED_MSR_RANGE_HI)))
    {
        msr.Content = __readmsr(GuestRegs->rcx);
    }

    //
    // Check if it's EFER MSR then we show a false SCE state so the
    // patchguard won't cause BSOD
    //
    if (GuestRegs->rcx == MSR_EFER)
    {
        DbgBreakPoint();

        EFER_MSR MsrEFER;
        MsrEFER.Flags         = msr.Content;
        MsrEFER.SyscallEnable = TRUE;
        msr.Content           = MsrEFER.Flags;
    }

    GuestRegs->rax = msr.Low;
    GuestRegs->rdx = msr.High;
}

/**
 * @brief Handles in the cases when RDMSR causes a vm-exit
 * 
 * @param GuestRegs Guest's gp registers
 * @return VOID 
 */
VOID
HvHandleMsrWrite(PGUEST_REGS GuestRegs)
{
    MSR msr = {0};

    //
    // Execute WRMSR or RDMSR on behalf of the guest. Important that this
    // can cause bug check when the guest tries to access unimplemented MSR
    // even within the SEH block* because the below WRMSR or RDMSR raises
    // #GP and are not protected by the SEH block (or cannot be protected
    // either as this code run outside the thread stack region Windows
    // requires to proceed SEH). Hypervisors typically handle this by noop-ing
    // WRMSR and returning zero for RDMSR with non-architecturally defined
    // MSRs. Alternatively, one can probe which MSRs should cause #GP prior
    // to installation of a hypervisor and the hypervisor can emulate the
    // results.
    //

    //
    // Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    //
    if ((GuestRegs->rcx <= 0x00001FFF) || ((0xC0000000 <= GuestRegs->rcx) && (GuestRegs->rcx <= 0xC0001FFF)) || (GuestRegs->rcx >= RESERVED_MSR_RANGE_LOW && (GuestRegs->rcx <= RESERVED_MSR_RANGE_HI)))
    {
        msr.Low  = (ULONG)GuestRegs->rax;
        msr.High = (ULONG)GuestRegs->rdx;
        __writemsr(GuestRegs->rcx, msr.Content);
    }
}

/**
 * @brief Set bits in Msr Bitmap
 * 
 * @param Msr MSR Address
 * @param ProcessorID Processor to set MSR Bitmap for it
 * @param ReadDetection set read bit 
 * @param WriteDetection set write bit
 * @return BOOLEAN Returns true if the MSR Bitmap is succcessfully applied or false if not applied
 */
BOOLEAN
HvSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection)
{
    if (!ReadDetection && !WriteDetection)
    {
        //
        // Invalid Command
        //
        return FALSE;
    }

    if (Msr <= 0x00001FFF)
    {
        if (ReadDetection)
        {
            SetBit(Msr, g_GuestState[ProcessorID].MsrBitmapVirtualAddress);
        }
        if (WriteDetection)
        {
            SetBit(Msr, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 2048);
        }
    }
    else if ((0xC0000000 <= Msr) && (Msr <= 0xC0001FFF))
    {
        if (ReadDetection)
        {
            SetBit(Msr - 0xC0000000, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 1024);
        }
        if (WriteDetection)
        {
            SetBit(Msr - 0xC0000000, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 3072);
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief UnSet bits in Msr Bitmap
 * 
 * @param Msr MSR Address
 * @param ProcessorID Processor to set MSR Bitmap for it
 * @param ReadDetection Unset read bit 
 * @param WriteDetection Unset write bit
 * @return BOOLEAN Returns true if the MSR Bitmap is succcessfully applied or false if not applied
 */
BOOLEAN
HvUnSetMsrBitmap(ULONG64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection)
{
    if (!ReadDetection && !WriteDetection)
    {
        //
        // Invalid Command
        //
        return FALSE;
    }

    if (Msr <= 0x00001FFF)
    {
        if (ReadDetection)
        {
            ClearBit(Msr, g_GuestState[ProcessorID].MsrBitmapVirtualAddress);
        }
        if (WriteDetection)
        {
            ClearBit(Msr, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 2048);
        }
    }
    else if ((0xC0000000 <= Msr) && (Msr <= 0xC0001FFF))
    {
        if (ReadDetection)
        {
            ClearBit(Msr - 0xC0000000, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 1024);
        }
        if (WriteDetection)
        {
            ClearBit(Msr - 0xC0000000, g_GuestState[ProcessorID].MsrBitmapVirtualAddress + 3072);
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Add the current instruction length to guest rip to resume to next instruction
 * 
 * @return VOID 
 */
VOID
HvResumeToNextInstruction()
{
    ULONG64 ResumeRIP             = NULL;
    ULONG64 CurrentRIP            = NULL;
    ULONG   ExitInstructionLength = 0;

    __vmx_vmread(GUEST_RIP, &CurrentRIP);
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);

    ResumeRIP = CurrentRIP + ExitInstructionLength;

    __vmx_vmwrite(GUEST_RIP, ResumeRIP);
}

/**
 * @brief Notify all core to invalidate their EPT
 * 
 * @return VOID 
 */
VOID
HvNotifyAllToInvalidateEpt()
{
    //
    // Let's notify them all
    //
    KeIpiGenericCall(HvInvalidateEptByVmcall, g_EptState->EptPointer.Flags);
}

/**
 * @brief Invalidate EPT using Vmcall (should be called from Vmx non root mode)
 * 
 * @param Context Single context or all contexts
 * @return VOID 
 */
VOID
HvInvalidateEptByVmcall(UINT64 Context)
{
    if (Context == NULL)
    {
        //
        // We have to invalidate all contexts
        //
        AsmVmxVmcall(VMCALL_INVEPT_ALL_CONTEXTS, NULL, NULL, NULL);
    }
    else
    {
        //
        // We have to invalidate all contexts
        //
        AsmVmxVmcall(VMCALL_INVEPT_SINGLE_CONTEXT, Context, NULL, NULL);
    }
}

/**
 * @brief Get the RIP of guest (GUEST_RIP) in the case of return from VMXOFF
 * 
 * @return UINT64 Returns the stack pointer, to change in the case of Vmxoff
 */
UINT64
HvReturnStackPointerForVmxoff()
{
    return g_GuestState[KeGetCurrentProcessorNumber()].VmxoffState.GuestRsp;
}

/**
 * @brief Get the RIP of guest (GUEST_RIP) in the case of return from VMXOFF
 * 
 * @return UINT64 Returns the instruction pointer, to change in the case of Vmxoff
 */
UINT64
HvReturnInstructionPointerForVmxoff()
{
    return g_GuestState[KeGetCurrentProcessorNumber()].VmxoffState.GuestRip;
}

//
// The broadcast function which initialize the guest
//
VOID
HvDpcBroadcastInitializeGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Save the vmx state and prepare vmcs setup and finally execute vmlaunch instruction
    //
    AsmVmxSaveState();
    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Terminate Vmx on all logical cores
 * 
 * @return VOID 
 */
VOID
HvTerminateVmx()
{
    LogInfo("Terminating VMX...\n");

    //
    // ******* Terminating Vmx *******
    //

    //
    // Remve All the hooks if any
    //
    HvPerformPageUnHookAllPages();

    //
    // Broadcast to terminate Vmx
    //
    KeGenericCallDpc(HvDpcBroadcastTerminateGuest, 0x0);

    //
    // ****** De-allocatee global variables ******
    //

    //
    // Free Identity Page Table
    //
    MmFreeContiguousMemory(g_EptState->EptPageTable);

    //
    // Free EptState
    //
    ExFreePoolWithTag(g_EptState, POOLTAG);

    //
    // Free the Pool manager
    //
    PoolManagerUninitialize();

    LogInfo("VMX Operation turned off successfully :)\n");
}

/**
 * @brief The broadcast function which terminate the guest
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
HvDpcBroadcastTerminateGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Terminate Vmx using vmcall
    //
    if (!VmxTerminate())
    {
        LogError("There were an error terminating Vmx");
    }

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
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
    // Read the previous flag
    //
    __vmx_vmread(CPU_BASED_VM_EXEC_CONTROL, &CpuBasedVmExecControls);

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
    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);
}

/**
 * @brief Set the vm-exit on cr3 for finding a process
 * 
 * @param Set Set or Unset the controls relationg to cr3 load exit
 * @return VOID 
 */
VOID
HvSetExitOnCr3Change(BOOLEAN Set)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flag
    //
    __vmx_vmread(CPU_BASED_VM_EXEC_CONTROL, &CpuBasedVmExecControls);

    if (Set)
    {
        CpuBasedVmExecControls |= CPU_BASED_CR3_LOAD_EXITING;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_CR3_LOAD_EXITING;
    }

    //
    // Set the new value
    //
    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);
}

/**
 * @brief Reset GDTR/IDTR and other old when you do vmxoff as the patchguard will detect them left modified
 * 
 * @return VOID 
 */
VOID
HvRestoreRegisters()
{
    ULONG64 FsBase;
    ULONG64 GsBase;
    ULONG64 GdtrBase;
    ULONG64 GdtrLimit;
    ULONG64 IdtrBase;
    ULONG64 IdtrLimit;

    //
    // Restore FS Base
    //
    __vmx_vmread(GUEST_FS_BASE, &FsBase);
    __writemsr(MSR_FS_BASE, FsBase);

    //
    // Restore Gs Base
    //
    __vmx_vmread(GUEST_GS_BASE, &GsBase);
    __writemsr(MSR_GS_BASE, GsBase);

    //
    // Restore GDTR
    //
    __vmx_vmread(GUEST_GDTR_BASE, &GdtrBase);
    __vmx_vmread(GUEST_GDTR_LIMIT, &GdtrLimit);

    AsmReloadGdtr(GdtrBase, GdtrLimit);

    //
    // Restore IDTR
    //
    __vmx_vmread(GUEST_IDTR_BASE, &IdtrBase);
    __vmx_vmread(GUEST_IDTR_LIMIT, &IdtrLimit);

    AsmReloadIdtr(IdtrBase, IdtrLimit);
}

/**
 * @brief The broadcast function which removes all the hooks and invalidate TLB
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
HvDpcBroadcastRemoveHookAndInvalidateAllEntries(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Execute the VMCALL to remove the hook and invalidate
    //
    AsmVmxVmcall(VMCALL_UNHOOK_ALL_PAGES, NULL, NULL, NULL);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief The broadcast function which removes the single hook and invalidate TLB
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
HvDpcBroadcastRemoveHookAndInvalidateSingleEntry(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Execute the VMCALL to remove the hook and invalidate
    //
    AsmVmxVmcall(VMCALL_UNHOOK_SINGLE_PAGE, DeferredContext, NULL, NULL);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from vmx non-root
 * 
 * @param VirtualAddress Virtual address to unhook
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
HvPerformPageUnHookSinglePage(UINT64 VirtualAddress)
{
    PLIST_ENTRY TempList = 0;
    SIZE_T      PhysicalAddress;

    PhysicalAddress = PAGE_ALIGN(VirtualAddressToPhysicalAddress(VirtualAddress));

    //
    // Should be called from vmx non-root
    //
    if (g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return FALSE;
    }

    TempList = &g_EptState->HookedPagesList;
    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList                            = TempList->Flink;
        PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

        if (HookedEntry->PhysicalBaseAddress == PhysicalAddress)
        {
            //
            // Remove it in all the cores
            //
            KeGenericCallDpc(HvDpcBroadcastRemoveHookAndInvalidateSingleEntry, HookedEntry->PhysicalBaseAddress);

            //
            // remove the entry from the list
            //
            RemoveEntryList(HookedEntry->PageHookList.Flink);

            return TRUE;
        }
    }
    //
    // Nothing found , probably the list is not found
    //
    return FALSE;
}

/**
 * @brief Remove all hooks from the hooked pages list and invalidate TLB
 * @detailsShould be called from Vmx Non-root
 * 
 * @return VOID 
 */
VOID
HvPerformPageUnHookAllPages()
{
    //
    // Should be called from vmx non-root
    //
    if (g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return;
    }

    //
    // Remove it in all the cores
    //
    KeGenericCallDpc(HvDpcBroadcastRemoveHookAndInvalidateAllEntries, 0x0);

    //
    // No need to remove the list as it will automatically remove by the pool uninitializer
    //
}

/**
 * @brief if Disable is true means to exit on all msrs
 * 
 * @param Disable If true, removes all msr bitmaps and vm-exit happens
 * @return VOID 
 */
VOID
HvDisableOrEnableMsrBitmaps(BOOLEAN Disable)
{
    ULONG CpuBasedVmExecControls = 0;

    //
    // Read the previous flag
    //
    __vmx_vmread(CPU_BASED_VM_EXEC_CONTROL, &CpuBasedVmExecControls);

    if (!Disable)
    {
        CpuBasedVmExecControls |= CPU_BASED_ACTIVATE_MSR_BITMAP;
    }
    else
    {
        CpuBasedVmExecControls &= ~CPU_BASED_ACTIVATE_MSR_BITMAP;
    }
    //
    // Set the new value
    //
    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);
}
