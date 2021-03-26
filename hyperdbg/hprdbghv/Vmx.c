/**
 * @file Vmx.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief VMX Instructions and VMX Related Functions
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Initialize VMX Operation
 * 
 * @return BOOLEAN Returns true if vmx is successfully initialized
 */
BOOLEAN
VmxInitializer()
{
    int       ProcessorCount;
    KAFFINITY AffinityMask;

    if (!HvIsVmxSupported())
    {
        LogError("VMX is not supported in this machine !");
        return FALSE;
    }

    PAGED_CODE();

    //
    // Allocate	global variable to hold Ept State
    //
    g_EptState = ExAllocatePoolWithTag(NonPagedPool, sizeof(EPT_STATE), POOLTAG);
    if (!g_EptState)
    {
        LogError("Insufficient memory");
        return FALSE;
    }

    //
    // Zero memory
    //
    RtlZeroMemory(g_EptState, sizeof(EPT_STATE));

    //
    // Initialize the list of hooked pages detail
    //
    InitializeListHead(&g_EptState->HookedPagesList);

    //
    // Check whether EPT is supported or not
    //
    if (!EptCheckFeatures())
    {
        LogError("Your processor doesn't support all EPT features");
        return FALSE;
    }
    else
    {
        //
        // Our processor supports EPT, now let's build MTRR
        //
        LogDebugInfo("Your processor supports all EPT features");

        //
        // Build MTRR Map
        //
        if (!EptBuildMtrrMap())
        {
            LogError("Could not build Mtrr memory map");
            return FALSE;
        }

        LogDebugInfo("Mtrr memory map built successfully");
    }

    //
    // Initialize Pool Manager
    //
    if (!PoolManagerInitialize())
    {
        LogError("Could not initialize pool manager");
        return FALSE;
    }

    if (!EptLogicalProcessorInitialize())
    {
        //
        // There were some errors in EptLogicalProcessorInitialize
        //
        return FALSE;
    }

    //
    // Allocate and run Vmxon and Vmptrld on all logical cores
    //
    KeGenericCallDpc(VmxDpcBroadcastAllocateVmxonRegions, 0x0);

    //
    // Everything is ok, let's return true
    //
    return TRUE;
}

/**
 * @brief It can deteministcly check whether the caller is on vmx-root mode
 * or not
 * 
 * @return BOOLEAN Returns true if current operation mode is vmx-root and false
 * if current operation mode is vmx non-root
 */
BOOLEAN
VmxCheckIsOnVmxRoot()
{
    ULONG64 VmcsLink = 0;

    __try
    {
        if (!__vmx_vmread(VMCS_LINK_POINTER, &VmcsLink))
        {
            if (VmcsLink != 0)
            {
                return TRUE;
            }
        }
    }
    __except (1)
    {
    }

    return FALSE;
}

/**
 * @brief Initialize VMX Operation
 * 
 * @param GuestStack Guest stack for the this core (GUEST_RSP)
 * @return BOOLEAN This function won't return true as when Vmlaunch is executed the
   rest of the function never executes but returning FALSE is an indication of error
 */
BOOLEAN
VmxVirtualizeCurrentSystem(PVOID GuestStack)
{
    ULONG64 ErrorCode   = 0;
    INT     ProcessorID = 0;

    ProcessorID = KeGetCurrentProcessorNumber();

    LogDebugInfo("Virtualizing Current System (Logical Core : 0x%x)", ProcessorID);

    //
    // Clear the VMCS State
    //
    if (!VmxClearVmcsState(&g_GuestState[ProcessorID]))
    {
        LogError("Failed to clear vmcs");
        return FALSE;
    }

    //
    // Load VMCS (Set the Current VMCS)
    //
    if (!VmxLoadVmcs(&g_GuestState[ProcessorID]))
    {
        LogError("Failed to load vmcs");
        return FALSE;
    }

    LogDebugInfo("Setting up VMCS for current logical core");

    VmxSetupVmcs(&g_GuestState[ProcessorID], GuestStack);

    LogDebugInfo("Executing VMLAUNCH on logical core %d", ProcessorID);

    //
    // Setting the state to indicate current core is currently virtualized
    //

    g_GuestState[ProcessorID].HasLaunched = TRUE;

    __vmx_vmlaunch();

    //
    // ******** if Vmlaunch succeed will never be here ! ********
    //

    //
    // If failed, then indiacte that current core is not currently virtualized
    //
    g_GuestState[ProcessorID].HasLaunched = FALSE;

    //
    // Execute Vmxoff
    //
    __vmx_off();

    __vmx_vmread(VM_INSTRUCTION_ERROR, &ErrorCode);
    LogError("VMLAUNCH Error : 0x%llx", ErrorCode);

    LogError("VMXOFF Executed Successfully but it was because of an error.");

    return FALSE;
}

/**
 * @brief Broadcast to terminate VMX on all logical cores
 * 
 * @return BOOLEAN Returns true if vmxoff successfully executed in vmcall or otherwise
 * returns false
 */
BOOLEAN
VmxTerminate()
{
    NTSTATUS Status           = STATUS_SUCCESS;
    INT      CurrentCoreIndex = 0;

    //
    // Get the current core index
    //
    CurrentCoreIndex = KeGetCurrentProcessorNumber();

    //
    // Execute Vmcall to to turn off vmx from Vmx root mode
    //
    Status = AsmVmxVmcall(VMCALL_VMXOFF, NULL, NULL, NULL);
    if (Status == STATUS_SUCCESS)
    {
        LogDebugInfo("VMX Terminated on logical core %d\n", CurrentCoreIndex);

        //
        // Free the destination memory
        //
        MmFreeContiguousMemory(g_GuestState[CurrentCoreIndex].VmxonRegionVirtualAddress);
        MmFreeContiguousMemory(g_GuestState[CurrentCoreIndex].VmcsRegionVirtualAddress);
        ExFreePoolWithTag(g_GuestState[CurrentCoreIndex].VmmStack, POOLTAG);
        ExFreePoolWithTag(g_GuestState[CurrentCoreIndex].MsrBitmapVirtualAddress, POOLTAG);
        ExFreePoolWithTag(g_GuestState[CurrentCoreIndex].IoBitmapVirtualAddressA, POOLTAG);
        ExFreePoolWithTag(g_GuestState[CurrentCoreIndex].IoBitmapVirtualAddressB, POOLTAG);

        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Implementation of VMPTRST instruction
 * 
 * @return VOID 
 */
VOID
VmxVmptrst()
{
    PHYSICAL_ADDRESS VmcsPhysicalAddr;
    VmcsPhysicalAddr.QuadPart = 0;
    __vmx_vmptrst((unsigned __int64 *)&VmcsPhysicalAddr);

    LogDebugInfo("Vmptrst result : %llx", VmcsPhysicalAddr);
}

/*  */
/**
 * @brief Clearing Vmcs status using vmclear instruction
 * 
 * @param CurrentGuestState 
 * @return BOOLEAN If vmclear execution was successful it returns true
 * otherwise and if there was error with vmclear then it returns false
 */
BOOLEAN
VmxClearVmcsState(VIRTUAL_MACHINE_STATE * CurrentGuestState)
{
    int VmclearStatus;

    //
    // Clear the state of the VMCS to inactive
    //
    VmclearStatus = __vmx_vmclear(&CurrentGuestState->VmcsRegionPhysicalAddress);

    LogDebugInfo("Vmcs Vmclear Status : %d", VmclearStatus);

    if (VmclearStatus)
    {
        //
        // Otherwise terminate the VMX
        //
        LogDebugInfo("VMCS failed to clear ( status : %d )", VmclearStatus);
        __vmx_off();
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Implementation of VMPTRLD instruction
 * 
 * @param CurrentGuestState 
 * @return BOOLEAN If vmptrld was unsuccessful then it returns false otherwise
 * it returns false
 */
BOOLEAN
VmxLoadVmcs(VIRTUAL_MACHINE_STATE * CurrentGuestState)
{
    int VmptrldStatus;

    VmptrldStatus = __vmx_vmptrld(&CurrentGuestState->VmcsRegionPhysicalAddress);
    if (VmptrldStatus)
    {
        LogDebugInfo("VMCS failed to load ( status : %d )", VmptrldStatus);
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Create and Configure a Vmcs Layout
 * 
 * @param CurrentGuestState 
 * @param GuestStack 
 * @return BOOLEAN 
 */
BOOLEAN
VmxSetupVmcs(VIRTUAL_MACHINE_STATE * CurrentGuestState, PVOID GuestStack)
{
    ULONG              CpuBasedVmExecControls;
    ULONG              SecondaryProcBasedVmExecControls;
    PVOID              HostRsp;
    ULONG64            GdtBase         = 0;
    SEGMENT_SELECTOR   SegmentSelector = {0};
    IA32_VMX_BASIC_MSR VmxBasicMsr     = {0};

    //
    // Reading IA32_VMX_BASIC_MSR
    //
    VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);

    __vmx_vmwrite(HOST_ES_SELECTOR, AsmGetEs() & 0xF8);
    __vmx_vmwrite(HOST_CS_SELECTOR, AsmGetCs() & 0xF8);
    __vmx_vmwrite(HOST_SS_SELECTOR, AsmGetSs() & 0xF8);
    __vmx_vmwrite(HOST_DS_SELECTOR, AsmGetDs() & 0xF8);
    __vmx_vmwrite(HOST_FS_SELECTOR, AsmGetFs() & 0xF8);
    __vmx_vmwrite(HOST_GS_SELECTOR, AsmGetGs() & 0xF8);
    __vmx_vmwrite(HOST_TR_SELECTOR, AsmGetTr() & 0xF8);

    //
    // Setting the link pointer to the required value for 4KB VMCS
    //
    __vmx_vmwrite(VMCS_LINK_POINTER, ~0ULL);

    __vmx_vmwrite(GUEST_IA32_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xFFFFFFFF);
    __vmx_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32);

    //
    // ******* Time-stamp counter offset *******
    //
    __vmx_vmwrite(TSC_OFFSET, 0);

    __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
    __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

    __vmx_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
    __vmx_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);

    __vmx_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
    __vmx_vmwrite(VM_ENTRY_INTR_INFO, 0);

    GdtBase = AsmGetGdtBase();

    HvFillGuestSelectorData((PVOID)GdtBase, ES, AsmGetEs());
    HvFillGuestSelectorData((PVOID)GdtBase, CS, AsmGetCs());
    HvFillGuestSelectorData((PVOID)GdtBase, SS, AsmGetSs());
    HvFillGuestSelectorData((PVOID)GdtBase, DS, AsmGetDs());
    HvFillGuestSelectorData((PVOID)GdtBase, FS, AsmGetFs());
    HvFillGuestSelectorData((PVOID)GdtBase, GS, AsmGetGs());
    HvFillGuestSelectorData((PVOID)GdtBase, LDTR, AsmGetLdtr());
    HvFillGuestSelectorData((PVOID)GdtBase, TR, AsmGetTr());

    __vmx_vmwrite(GUEST_FS_BASE, __readmsr(MSR_FS_BASE));
    __vmx_vmwrite(GUEST_GS_BASE, __readmsr(MSR_GS_BASE));

    CpuBasedVmExecControls = HvAdjustControls(CPU_BASED_ACTIVATE_IO_BITMAP | CPU_BASED_ACTIVATE_MSR_BITMAP | CPU_BASED_ACTIVATE_SECONDARY_CONTROLS,
                                              VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_PROCBASED_CTLS : MSR_IA32_VMX_PROCBASED_CTLS);

    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);

    LogDebugInfo("Cpu Based VM Exec Controls (Based on %s) : 0x%x",
                 VmxBasicMsr.Fields.VmxCapabilityHint ? "MSR_IA32_VMX_TRUE_PROCBASED_CTLS" : "MSR_IA32_VMX_PROCBASED_CTLS",
                 CpuBasedVmExecControls);

    SecondaryProcBasedVmExecControls = HvAdjustControls(CPU_BASED_CTL2_RDTSCP |
                                                            CPU_BASED_CTL2_ENABLE_EPT | CPU_BASED_CTL2_ENABLE_INVPCID |
                                                            CPU_BASED_CTL2_ENABLE_XSAVE_XRSTORS | CPU_BASED_CTL2_ENABLE_VPID,
                                                        MSR_IA32_VMX_PROCBASED_CTLS2);

    __vmx_vmwrite(SECONDARY_VM_EXEC_CONTROL, SecondaryProcBasedVmExecControls);

    LogDebugInfo("Secondary Proc Based VM Exec Controls (MSR_IA32_VMX_PROCBASED_CTLS2) : 0x%x", SecondaryProcBasedVmExecControls);

    __vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL, HvAdjustControls(0, VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_PINBASED_CTLS : MSR_IA32_VMX_PINBASED_CTLS));

    __vmx_vmwrite(VM_EXIT_CONTROLS, HvAdjustControls(VM_EXIT_HOST_ADDR_SPACE_SIZE, VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS));

    __vmx_vmwrite(VM_ENTRY_CONTROLS, HvAdjustControls(VM_ENTRY_IA32E_MODE, VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS));

    __vmx_vmwrite(CR0_GUEST_HOST_MASK, 0);
    __vmx_vmwrite(CR4_GUEST_HOST_MASK, 0);

    __vmx_vmwrite(CR0_READ_SHADOW, 0);
    __vmx_vmwrite(CR4_READ_SHADOW, 0);

    __vmx_vmwrite(GUEST_CR0, __readcr0());
    __vmx_vmwrite(GUEST_CR3, __readcr3());
    __vmx_vmwrite(GUEST_CR4, __readcr4());

    __vmx_vmwrite(GUEST_DR7, 0x400);

    __vmx_vmwrite(HOST_CR0, __readcr0());
    __vmx_vmwrite(HOST_CR4, __readcr4());

    //
    // Because we may be executing in an arbitrary user-mode, process as part
    // of the DPC interrupt we execute in We have to save Cr3, for HOST_CR3
    //

    __vmx_vmwrite(HOST_CR3, FindSystemDirectoryTableBase());

    __vmx_vmwrite(GUEST_GDTR_BASE, AsmGetGdtBase());
    __vmx_vmwrite(GUEST_IDTR_BASE, AsmGetIdtBase());

    __vmx_vmwrite(GUEST_GDTR_LIMIT, AsmGetGdtLimit());
    __vmx_vmwrite(GUEST_IDTR_LIMIT, AsmGetIdtLimit());

    __vmx_vmwrite(GUEST_RFLAGS, AsmGetRflags());

    __vmx_vmwrite(GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
    __vmx_vmwrite(GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
    __vmx_vmwrite(GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));

    GetSegmentDescriptor(&SegmentSelector, AsmGetTr(), (PUCHAR)AsmGetGdtBase());
    __vmx_vmwrite(HOST_TR_BASE, SegmentSelector.BASE);

    __vmx_vmwrite(HOST_FS_BASE, __readmsr(MSR_FS_BASE));
    __vmx_vmwrite(HOST_GS_BASE, __readmsr(MSR_GS_BASE));

    __vmx_vmwrite(HOST_GDTR_BASE, AsmGetGdtBase());
    __vmx_vmwrite(HOST_IDTR_BASE, AsmGetIdtBase());

    __vmx_vmwrite(HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
    __vmx_vmwrite(HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
    __vmx_vmwrite(HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));

    //
    // Set MSR Bitmaps
    //
    __vmx_vmwrite(MSR_BITMAP, CurrentGuestState->MsrBitmapPhysicalAddress);

    //
    // Set I/O Bitmaps
    //
    __vmx_vmwrite(IO_BITMAP_A, CurrentGuestState->IoBitmapPhysicalAddressA);
    __vmx_vmwrite(IO_BITMAP_B, CurrentGuestState->IoBitmapPhysicalAddressB);

    //
    // Set up EPT
    //
    __vmx_vmwrite(EPT_POINTER, g_EptState->EptPointer.Flags);

    //
    // Set up VPID

    //
    // For all processors, we will use a VPID = 1. This allows the processor to separate caching
    //  of EPT structures away from the regular OS page translation tables in the TLB.
    //
    __vmx_vmwrite(VIRTUAL_PROCESSOR_ID, VPID_TAG);

    //
    //setup guest rsp
    //
    __vmx_vmwrite(GUEST_RSP, (ULONG64)GuestStack);

    //
    //setup guest rip
    //
    __vmx_vmwrite(GUEST_RIP, (ULONG64)AsmVmxRestoreState);

    //
    // Stack should be aligned to 16 because we wanna save XMM and FPU registers and those instructions
    // needs aligment to 16
    //
    HostRsp = (ULONG64)CurrentGuestState->VmmStack + VMM_STACK_SIZE - 1;
    HostRsp = ((PVOID)((ULONG_PTR)(HostRsp) & ~(16 - 1)));
    __vmx_vmwrite(HOST_RSP, HostRsp);
    __vmx_vmwrite(HOST_RIP, (ULONG64)AsmVmexitHandler);

    return TRUE;
}

/**
 * @brief Resume vm using VMRESUME instruction
 * 
 * @return VOID 
 */
VOID
VmxVmresume()
{
    ULONG64 ErrorCode = 0;

    __vmx_vmresume();

    //
    // if VMRESUME succeed will never be here !
    //

    __vmx_vmread(VM_INSTRUCTION_ERROR, &ErrorCode);
    __vmx_off();

    //
    // It's such a bad error because we don't where to go !
    // prefer to break
    //
    LogError("Error in executing Vmresume , status : 0x%llx", ErrorCode);
}

/**
 * @brief Prepare and execute Vmxoff instruction
 * 
 * @return VOID 
 */
VOID
VmxVmxoff()
{
    INT    CurrentProcessorIndex = 0;
    UINT64 GuestRSP              = 0; // Save a pointer to guest rsp for times that we want to return to previous guest stateS
    UINT64 GuestRIP              = 0; // Save a pointer to guest rip for times that we want to return to previous guest state
    UINT64 GuestCr3              = 0;
    UINT64 ExitInstructionLength = 0;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // According to SimpleVisor :
    //  	Our callback routine may have interrupted an arbitrary user process,
    //  	and therefore not a thread running with a system-wide page directory.
    //  	Therefore if we return back to the original caller after turning off
    //  	VMX, it will keep our current "host" CR3 value which we set on entry
    //  	to the PML4 of the SYSTEM process. We want to return back with the
    //  	correct value of the "guest" CR3, so that the currently executing
    //  	process continues to run with its expected address space mappings.
    //

    __vmx_vmread(GUEST_CR3, &GuestCr3);
    __writecr3(GuestCr3);

    //
    // Read guest rsp and rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRIP);
    __vmx_vmread(GUEST_RSP, &GuestRSP);

    //
    // Read instruction length
    //
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);
    GuestRIP += ExitInstructionLength;

    //
    // Set the previous registe states
    //
    g_GuestState[CurrentProcessorIndex].VmxoffState.GuestRip = GuestRIP;
    g_GuestState[CurrentProcessorIndex].VmxoffState.GuestRsp = GuestRSP;

    //
    // Notify the Vmexit handler that VMX already turned off
    //
    g_GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted = TRUE;

    //
    // Restore the previous FS, GS , GDTR and IDTR register as patchguard might find the modified
    //
    HvRestoreRegisters();

    //
    // Before using vmxoff, you first need to use vmclear on any VMCSes that you want to be able to use again.
    // See sections 24.1 and 24.11 of the SDM.
    //
    VmxClearVmcsState(&g_GuestState[CurrentProcessorIndex]);

    //
    // Execute Vmxoff
    //
    __vmx_off();

    //
    // Inidcate the current core is not currently virtualized
    //
    g_GuestState[CurrentProcessorIndex].HasLaunched = FALSE;

    //
    // Now that VMX is OFF, we have to unset vmx-enable bit on cr4
    //
    __writecr4(__readcr4() & (~X86_CR4_VMXE));
}
