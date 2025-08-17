/**
 * @file Vmx.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @brief VMX VMREAD instruction (64-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread64(size_t Field,
            UINT64 FieldValue)
{
    return __vmx_vmread((size_t)Field, (size_t *)FieldValue);
}

/**
 * @brief VMX VMREAD instruction (32-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread32(size_t Field,
            UINT32 FieldValue)
{
    UINT64 TargetField = 0ull;

    TargetField = FieldValue;

    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
}

/**
 * @brief VMX VMREAD instruction (16-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread16(size_t Field,
            UINT16 FieldValue)
{
    UINT64 TargetField = 0ull;

    TargetField = FieldValue;

    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
}

/**
 * @brief VMX VMREAD instruction (64-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread64P(size_t   Field,
             UINT64 * FieldValue)
{
    return __vmx_vmread((size_t)Field, (size_t *)FieldValue);
}

/**
 * @brief VMX VMREAD instruction (32-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread32P(size_t   Field,
             UINT32 * FieldValue)
{
    UINT64 TargetField = 0ull;

    TargetField = (UINT64)FieldValue;

    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
}

/**
 * @brief VMX VMREAD instruction (16-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread16P(size_t   Field,
             UINT16 * FieldValue)
{
    UINT64 TargetField = 0ull;

    TargetField = (UINT64)FieldValue;

    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
}

/**
 * @brief VMX VMWRITE instruction (64-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite64(size_t Field,
             UINT64 FieldValue)
{
    return __vmx_vmwrite((size_t)Field, (size_t)FieldValue);
}

/**
 * @brief VMX VMWRITE instruction (32-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite32(size_t Field,
             UINT32 FieldValue)
{
    UINT64 TargetValue = NULL64_ZERO;
    TargetValue        = (UINT64)FieldValue;
    return __vmx_vmwrite((size_t)Field, (size_t)TargetValue);
}

/**
 * @brief VMX VMWRITE instruction (16-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite16(size_t Field,
             UINT16 FieldValue)
{
    UINT64 TargetValue = NULL64_ZERO;
    TargetValue        = (UINT64)FieldValue;
    return __vmx_vmwrite((size_t)Field, (size_t)TargetValue);
}

/**
 * @brief Check whether VMX Feature is supported or not
 *
 * @return BOOLEAN Returns true if vmx is supported or false if it's not supported
 */
BOOLEAN
VmxCheckVmxSupport()
{
    CPUID                         Data              = {0};
    IA32_FEATURE_CONTROL_REGISTER FeatureControlMsr = {0};

    //
    // Gets Processor Info and Feature Bits
    //
    __cpuid((int *)&Data, 1);

    //
    // Check For VMX Bit CPUID.ECX[5]
    //
    if (!_bittest((const LONG *)&Data.ecx, 5))
    {
        //
        // returns FALSE if vmx is not supported
        //
        return FALSE;
    }

    FeatureControlMsr.AsUInt = __readmsr(IA32_FEATURE_CONTROL);

    //
    // Commented because of https://stackoverflow.com/questions/34900224/
    // and https://github.com/HyperDbg/HyperDbg/issues/24
    // the problem is when writing to IA32_FEATURE_CONTROL MSR, the lock bit
    // of this MSR Is not set to 0 on most computers, if the user enabled VT-X
    // from the BIOS the VMXON will be already set so checking lock bit and
    // then writing to EnableVmxon again is not meaningful since its already
    // there
    //

    //
    // if (FeatureControlMsr.Fields.Lock == 0)
    // {
    //     FeatureControlMsr.Fields.Lock        = TRUE;
    //     FeatureControlMsr.Fields.EnableVmxon = TRUE;
    //     __writemsr(IA32_FEATURE_CONTROL, FeatureControlMsr.Flags);
    // }

    if (FeatureControlMsr.EnableVmxOutsideSmx == FALSE)
    {
        LogError("Err, you should enable vt-x from BIOS");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Check current execution mode (vmx-root and non-root)
 *
 * @return BOOLEAN Returns true if the execution is on vmx-root, otherwise false
 */
BOOLEAN
VmxGetCurrentExecutionMode()
{
    if (g_GuestState)
    {
        ULONG                   CurrentCore    = KeGetCurrentProcessorNumberEx(NULL);
        VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

        return CurrentVmState->IsOnVmxRootMode ? VmxExecutionModeRoot : VmxExecutionModeNonRoot;
    }
    else
    {
        //
        // The structure for guest state is not initialized, thus, we're in VMX non-root
        //
        return VmxExecutionModeNonRoot;
    }
}

/**
 * @brief Check if the VMX is launched or not
 *
 * @return BOOLEAN Returns true if it's launched, otherwise false
 */
BOOLEAN
VmxGetCurrentLaunchState()
{
    ULONG                   CurrentCore    = KeGetCurrentProcessorNumberEx(NULL);
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    return CurrentVmState->HasLaunched;
}

/**
 * @brief Initialize the VMX operation
 *
 * @return BOOLEAN Returns true if vmx initialized successfully
 */
BOOLEAN
VmxInitialize()
{
    ULONG ProcessorsCount;

    //
    // ****** Start Virtualizing Current System ******
    //

    //
    // Initiating EPTP and VMX
    //
    if (!VmxPerformVirtualizationOnAllCores())
    {
        //
        // there was error somewhere in initializing
        //
        return FALSE;
    }

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (size_t ProcessorID = 0; ProcessorID < ProcessorsCount; ProcessorID++)
    {
        //
        // *** Launching VM for Test (in the all logical processor) ***
        //

        VIRTUAL_MACHINE_STATE * GuestState = &g_GuestState[ProcessorID];

        //
        // Allocating VMM Stack
        //
        if (!VmxAllocateVmmStack(GuestState))
        {
            //
            // Some error in allocating Vmm Stack
            //
            return FALSE;
        }

        //
        // Allocating MSR Bit
        //
        if (!VmxAllocateMsrBitmap(GuestState))
        {
            //
            // Some error in allocating Msr Bitmaps
            //
            return FALSE;
        }

        //
        // Allocating I/O Bit
        //
        if (!VmxAllocateIoBitmaps(GuestState))
        {
            //
            // Some error in allocating I/O Bitmaps
            //
            return FALSE;
        }

#if USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

        //
        // Allocating Host IDT
        //
        if (!VmxAllocateHostIdt(GuestState))
        {
            //
            // Some error in allocating Host IDT
            //
            return FALSE;
        }
#endif // USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

#if USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

        //
        // Allocating Host GDT
        //
        if (!VmxAllocateHostGdt(GuestState))
        {
            //
            // Some error in allocating Host GDT
            //
            return FALSE;
        }

        //
        // Allocating Host TSS
        //
        if (!VmxAllocateHostTss(GuestState))
        {
            //
            // Some error in allocating Host TSS
            //
            return FALSE;
        }

#endif // USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

#if USE_INTERRUPT_STACK_TABLE == TRUE

        //
        // Allocating Host Interrupt Stack
        //
        if (!VmxAllocateHostInterruptStack(GuestState))
        {
            //
            // Some error in allocating Interrupt Stack
            //
            return FALSE;
        }

#endif // USE_INTERRUPT_STACK_TABLE == TRUE
    }

    //
    // Create a bitmap of the MSRs that cause #GP
    //
    g_MsrBitmapInvalidMsrs = VmxAllocateInvalidMsrBimap();

    if (g_MsrBitmapInvalidMsrs == NULL)
    {
        return FALSE;
    }

    //
    // As we want to support more than 32 processor (64 logical-core)
    // we let windows execute our routine for us
    //
    KeGenericCallDpc(DpcRoutineInitializeGuest, 0x0);

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
 * @brief Initialize essential VMX Operation tasks
 *
 * @return BOOLEAN Returns true if vmx is successfully initialized
 */
BOOLEAN
VmxPerformVirtualizationOnAllCores()
{
    PAGED_CODE();

    if (!VmxCheckVmxSupport())
    {
        LogError("Err, VMX is not supported in this machine");
        return FALSE;
    }

    //
    // Allocate	global variable to hold Ept State
    //
    g_EptState = PlatformMemAllocateZeroedNonPagedPool(sizeof(EPT_STATE));

    if (!g_EptState)
    {
        LogError("Err, insufficient memory");
        return FALSE;
    }

    //
    // Initialize the list of hooked pages detail
    //
    InitializeListHead(&g_EptState->HookedPagesList);

    //
    // Check whether EPT is supported or not
    //
    if (!EptCheckFeatures())
    {
        LogError("Err, your processor doesn't support all EPT features");
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
            LogError("Err, could not build MTRR memory map");
            return FALSE;
        }

        LogDebugInfo("MTRR memory map built successfully");
    }

    //
    // Initialize Pool Manager
    //
    if (!PoolManagerInitialize())
    {
        LogError("Err, could not initialize pool manager");
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
    // Broadcast to run vmx-specific task to virtualize cores
    //
    BroadcastVmxVirtualizationAllCores();

    //
    // Everything is ok, let's return true
    //
    return TRUE;
}

/**
 * @brief Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region)
 *
 * @return BOOLEAN
 */
BOOLEAN
VmxPerformVirtualizationOnSpecificCore()
{
    ULONG                   CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    VIRTUAL_MACHINE_STATE * VCpu        = &g_GuestState[CurrentCore];

    LogDebugInfo("Allocating vmx regions for logical core %d", CurrentCore);

    //
    // Enabling VMX Operation
    //
    AsmEnableVmxOperation();

    //
    // Fix Cr4 and Cr0 bits during VMX operation
    //
    VmxFixCr4AndCr0Bits();

    LogDebugInfo("VMX-Operation enabled successfully");

    if (!VmxAllocateVmxonRegion(VCpu))
    {
        LogError("Err, allocating memory for vmxon region was not successful");
        return FALSE;
    }
    if (!VmxAllocateVmcsRegion(VCpu))
    {
        LogError("Err, allocating memory for vmcs region was not successful");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Fix values for cr0 and cr4 bits
 * @details The Cr4 And Cr0 Bits During VMX Operation Preventing Them From Any Change
 * (https://revers.engineering/day-2-entering-vmx-operation/)
 *
 * @return VOID
 */
VOID
VmxFixCr4AndCr0Bits()
{
    CR_FIXED CrFixed = {0};
    CR4      Cr4     = {0};
    CR0      Cr0     = {0};

    //
    // Fix Cr0
    //
    CrFixed.Flags = __readmsr(IA32_VMX_CR0_FIXED0);
    Cr0.AsUInt    = __readcr0();
    Cr0.AsUInt |= CrFixed.Fields.Low;
    CrFixed.Flags = __readmsr(IA32_VMX_CR0_FIXED1);
    Cr0.AsUInt &= CrFixed.Fields.Low;
    __writecr0(Cr0.AsUInt);

    //
    // Fix Cr4
    //
    CrFixed.Flags = __readmsr(IA32_VMX_CR4_FIXED0);
    Cr4.AsUInt    = __readcr4();
    Cr4.AsUInt |= CrFixed.Fields.Low;
    CrFixed.Flags = __readmsr(IA32_VMX_CR4_FIXED1);
    Cr4.AsUInt &= CrFixed.Fields.Low;
    __writecr4(Cr4.AsUInt);
}

/**
 * @brief It can deterministically check whether the caller is on vmx-root mode
 * or not
 *
 * @return BOOLEAN Returns true if current operation mode is vmx-root and false
 * if current operation mode is vmx non-root
 */
BOOLEAN
VmxCheckIsOnVmxRoot()
{
    UINT64 VmcsLink = 0;

    __try
    {
        if (!__vmx_vmread(VMCS_GUEST_VMCS_LINK_POINTER, &VmcsLink))
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
 * @param GuestStack Guest stack for the this core (VMCS_GUEST_RSP)
 * @return BOOLEAN This function won't return true as when Vmlaunch is executed the
   rest of the function never executes but returning FALSE is an indication of error
 */
BOOLEAN
VmxVirtualizeCurrentSystem(PVOID GuestStack)
{
    UINT64                  ErrorCode   = 0;
    ULONG                   CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    VIRTUAL_MACHINE_STATE * VCpu        = &g_GuestState[CurrentCore];

    LogDebugInfo("Virtualizing current system (logical core : 0x%x)", CurrentCore);

#if USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

    //
    // Prepare Host IDT
    //
    IdtEmulationPrepareHostIdt(VCpu);
#endif // USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

#if USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

    //
    // Prepare Host GDT and TSS
    //
    SegmentPrepareHostGdt((SEGMENT_DESCRIPTOR_32 *)AsmGetGdtBase(),
                          AsmGetGdtLimit(),
                          AsmGetTr(),
                          VCpu->HostInterruptStack,
                          (SEGMENT_DESCRIPTOR_32 *)VCpu->HostGdt,
                          (TASK_STATE_SEGMENT_64 *)VCpu->HostTss);

#endif // USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

    //
    // Clear the VMCS State
    //
    if (!VmxClearVmcsState(VCpu))
    {
        LogError("Err, failed to clear vmcs");
        return FALSE;
    }

    //
    // Load VMCS (Set the Current VMCS)
    //
    if (!VmxLoadVmcs(VCpu))
    {
        LogError("Err, failed to load vmcs");
        return FALSE;
    }

    LogDebugInfo("Setting up VMCS for current logical core");

    VmxSetupVmcs(VCpu, GuestStack);

    LogDebugInfo("Executing VMLAUNCH on logical core %d", CurrentCore);

    //
    // Setting the state to indicate current core is currently virtualized
    //

    VCpu->HasLaunched = TRUE;

    __vmx_vmlaunch();

    //
    // ******** if Vmlaunch succeed will never be here ! ********
    //

    //
    // If failed, then indicate that current core is not currently virtualized
    //
    VCpu->HasLaunched = FALSE;

    //
    // Read error code firstly
    //
    __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);

    LogError("Err, unable to execute VMLAUNCH, status : 0x%llx", ErrorCode);

    //
    // Then Execute Vmxoff
    //
    __vmx_off();
    LogError("Err, VMXOFF Executed Successfully but it was because of an error");

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
    NTSTATUS                Status      = STATUS_SUCCESS;
    ULONG                   CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    VIRTUAL_MACHINE_STATE * VCpu        = &g_GuestState[CurrentCore];

    //
    // Execute Vmcall to to turn off vmx from Vmx root mode
    //
    Status = AsmVmxVmcall(VMCALL_VMXOFF, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);

    if (Status == STATUS_SUCCESS)
    {
        LogDebugInfo("VMX terminated on logical core %d\n", CurrentCore);

        //
        // Free the destination memory
        //
        MmFreeContiguousMemory((PVOID)VCpu->VmxonRegionVirtualAddress);
        MmFreeContiguousMemory((PVOID)VCpu->VmcsRegionVirtualAddress);
        PlatformMemFreePool((PVOID)VCpu->VmmStack);
        PlatformMemFreePool((PVOID)VCpu->MsrBitmapVirtualAddress);
        PlatformMemFreePool((PVOID)VCpu->IoBitmapVirtualAddressA);
        PlatformMemFreePool((PVOID)VCpu->IoBitmapVirtualAddressB);
#if USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE
        PlatformMemFreePool((PVOID)VCpu->HostIdt);
#endif // USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

#if USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE
        PlatformMemFreePool((PVOID)VCpu->HostGdt);
        PlatformMemFreePool((PVOID)VCpu->HostTss);
#endif // USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

#if USE_INTERRUPT_STACK_TABLE == TRUE
        PlatformMemFreePool((PVOID)VCpu->HostInterruptStack);
#endif // USE_INTERRUPT_STACK_TABLE == FALSE

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

    LogDebugInfo("VMPTRST result : %llx", VmcsPhysicalAddr);
}

/**
 * @brief Clearing Vmcs status using vmclear instruction
 *
 * @param VCpu
 * @return BOOLEAN If vmclear execution was successful it returns true
 * otherwise and if there was error with vmclear then it returns false
 */
_Use_decl_annotations_
BOOLEAN
VmxClearVmcsState(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT8 VmclearStatus;

    //
    // Clear the state of the VMCS to inactive
    //
    VmclearStatus = __vmx_vmclear(&VCpu->VmcsRegionPhysicalAddress);

    LogDebugInfo("VMCS VMCLEAR status : 0x%x", VmclearStatus);

    if (VmclearStatus)
    {
        //
        // Otherwise terminate the VMX
        //
        LogDebugInfo("VMCS failed to clear, status : 0x%x", VmclearStatus);
        __vmx_off();
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Implementation of VMPTRLD instruction
 *
 * @param VCpu
 * @return BOOLEAN If vmptrld was unsuccessful then it returns false otherwise
 * it returns false
 */
_Use_decl_annotations_
BOOLEAN
VmxLoadVmcs(VIRTUAL_MACHINE_STATE * VCpu)
{
    int VmptrldStatus;

    VmptrldStatus = __vmx_vmptrld(&VCpu->VmcsRegionPhysicalAddress);
    if (VmptrldStatus)
    {
        LogDebugInfo("VMCS failed to load, status : 0x%x", VmptrldStatus);
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Create and Configure a Vmcs Layout
 *
 * @param VCpu
 * @param GuestStack
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
VmxSetupVmcs(VIRTUAL_MACHINE_STATE * VCpu, PVOID GuestStack)
{
    UINT32                  CpuBasedVmExecControls;
    UINT32                  SecondaryProcBasedVmExecControls;
    PVOID                   HostRsp;
    UINT64                  GdtBase         = 0;
    IA32_VMX_BASIC_REGISTER VmxBasicMsr     = {0};
    VMX_SEGMENT_SELECTOR    SegmentSelector = {0};

    //
    // Reading IA32_VMX_BASIC_MSR
    //
    VmxBasicMsr.AsUInt = __readmsr(IA32_VMX_BASIC);

    VmxVmwrite64(VMCS_HOST_ES_SELECTOR, AsmGetEs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_CS_SELECTOR, AsmGetCs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_SS_SELECTOR, AsmGetSs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_DS_SELECTOR, AsmGetDs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_FS_SELECTOR, AsmGetFs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_GS_SELECTOR, AsmGetGs() & 0xF8);
    VmxVmwrite64(VMCS_HOST_TR_SELECTOR, AsmGetTr() & 0xF8);

    //
    // Setting the link pointer to the required value for 4KB VMCS
    //
    VmxVmwrite64(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);

    VmxVmwrite64(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL) & 0xFFFFFFFF);
    VmxVmwrite64(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(IA32_DEBUGCTL) >> 32);

    //
    // ******* Time-stamp counter offset *******
    //
    VmxVmwrite64(VMCS_CTRL_TSC_OFFSET, 0);

    VmxVmwrite64(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK, 0);
    VmxVmwrite64(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH, 0);

    VmxVmwrite64(VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0);
    VmxVmwrite64(VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0);

    VmxVmwrite64(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0);
    VmxVmwrite64(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, 0);

    GdtBase = AsmGetGdtBase();

    HvFillGuestSelectorData((PVOID)GdtBase, ES, AsmGetEs());
    HvFillGuestSelectorData((PVOID)GdtBase, CS, AsmGetCs());
    HvFillGuestSelectorData((PVOID)GdtBase, SS, AsmGetSs());
    HvFillGuestSelectorData((PVOID)GdtBase, DS, AsmGetDs());
    HvFillGuestSelectorData((PVOID)GdtBase, FS, AsmGetFs());
    HvFillGuestSelectorData((PVOID)GdtBase, GS, AsmGetGs());
    HvFillGuestSelectorData((PVOID)GdtBase, LDTR, AsmGetLdtr());
    HvFillGuestSelectorData((PVOID)GdtBase, TR, AsmGetTr());

    VmxVmwrite64(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
    VmxVmwrite64(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));

    CpuBasedVmExecControls = HvAdjustControls(
        IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_FLAG |
            IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG |
            IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG,
        VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS);

    VmxVmwrite64(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuBasedVmExecControls);

    LogDebugInfo("CPU Based VM Exec Controls (Based on %s) : 0x%x",
                 VmxBasicMsr.VmxControls ? "IA32_VMX_TRUE_PROCBASED_CTLS" : "IA32_VMX_PROCBASED_CTLS",
                 CpuBasedVmExecControls);

    SecondaryProcBasedVmExecControls = HvAdjustControls(
        IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG |
            IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_FLAG |
            IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_FLAG |
            IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG |
            IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID_FLAG |
            IA32_VMX_PROCBASED_CTLS2_ENABLE_USER_WAIT_PAUSE_FLAG,
        IA32_VMX_PROCBASED_CTLS2);

    VmxVmwrite64(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, SecondaryProcBasedVmExecControls);

    LogDebugInfo("Secondary Proc Based VM Exec Controls (IA32_VMX_PROCBASED_CTLS2) : 0x%x", SecondaryProcBasedVmExecControls);

    VmxVmwrite64(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
                 HvAdjustControls(
                     0,
                     VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS));

    VmxVmwrite64(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
                 HvAdjustControls(
                     IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG |
                         IA32_VMX_EXIT_CTLS_LOAD_IA32_CET_STATE_FLAG,
                     VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS));

    VmxVmwrite64(VMCS_CTRL_VMENTRY_CONTROLS,
                 HvAdjustControls(
                     IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG |
                         IA32_VMX_ENTRY_CTLS_LOAD_CET_STATE_FLAG,
                     VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS));

    VmxVmwrite64(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
    VmxVmwrite64(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);

    VmxVmwrite64(VMCS_CTRL_CR0_READ_SHADOW, 0);
    VmxVmwrite64(VMCS_CTRL_CR4_READ_SHADOW, 0);

    VmxVmwrite64(VMCS_GUEST_CR0, __readcr0());
    VmxVmwrite64(VMCS_GUEST_CR3, __readcr3());
    VmxVmwrite64(VMCS_GUEST_CR4, __readcr4());

    VmxVmwrite64(VMCS_GUEST_DR7, 0x400);

    VmxVmwrite64(VMCS_HOST_CR0, __readcr0());
    VmxVmwrite64(VMCS_HOST_CR4, __readcr4());

    //
    // Because we may be executing in an arbitrary user-mode, process as part
    // of the DPC interrupt we execute in We have to save Cr3, for VMCS_HOST_CR3
    //

    VmxVmwrite64(VMCS_HOST_CR3, LayoutGetSystemDirectoryTableBase());

    VmxVmwrite64(VMCS_GUEST_GDTR_BASE, AsmGetGdtBase());
    VmxVmwrite64(VMCS_GUEST_IDTR_BASE, AsmGetIdtBase());

    VmxVmwrite64(VMCS_GUEST_GDTR_LIMIT, AsmGetGdtLimit());
    VmxVmwrite64(VMCS_GUEST_IDTR_LIMIT, AsmGetIdtLimit());

    VmxVmwrite64(VMCS_GUEST_RFLAGS, AsmGetRflags());

    VmxVmwrite64(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
    VmxVmwrite64(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
    VmxVmwrite64(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));

#if USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

    SegmentGetDescriptor((PUCHAR)VCpu->HostGdt, AsmGetTr(), &SegmentSelector);

    VmxVmwrite64(VMCS_HOST_TR_BASE, SegmentSelector.Base);
    VmxVmwrite64(VMCS_HOST_GDTR_BASE, VCpu->HostGdt);

#else

    SegmentGetDescriptor((PUCHAR)AsmGetGdtBase(), AsmGetTr(), &SegmentSelector);

    VmxVmwrite64(VMCS_HOST_TR_BASE, SegmentSelector.Base);
    VmxVmwrite64(VMCS_HOST_GDTR_BASE, AsmGetGdtBase());

#endif // USE_DEFAULT_OS_GDT_AS_HOST_GDT == FALSE

    VmxVmwrite64(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
    VmxVmwrite64(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));

#if USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

    VmxVmwrite64(VMCS_HOST_IDTR_BASE, VCpu->HostIdt);

#else

    VmxVmwrite64(VMCS_HOST_IDTR_BASE, AsmGetIdtBase());

#endif // USE_DEFAULT_OS_IDT_AS_HOST_IDT == FALSE

    VmxVmwrite64(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
    VmxVmwrite64(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
    VmxVmwrite64(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));

    //
    // Set MSR Bitmaps
    //
    VmxVmwrite64(VMCS_CTRL_MSR_BITMAP_ADDRESS, VCpu->MsrBitmapPhysicalAddress);

    //
    // Set I/O Bitmaps
    //
    VmxVmwrite64(VMCS_CTRL_IO_BITMAP_A_ADDRESS, VCpu->IoBitmapPhysicalAddressA);
    VmxVmwrite64(VMCS_CTRL_IO_BITMAP_B_ADDRESS, VCpu->IoBitmapPhysicalAddressB);

    //
    // Set up EPT
    //
    VmxVmwrite64(VMCS_CTRL_EPT_POINTER, VCpu->EptPointer.AsUInt);

    //
    // Set up VPID

    //
    // For all processors, we will use a VPID = 1. This allows the processor to separate caching
    //  of EPT structures away from the regular OS page translation tables in the TLB.
    //
    VmxVmwrite64(VIRTUAL_PROCESSOR_ID, VPID_TAG);

    //
    // setup guest rsp
    //
    VmxVmwrite64(VMCS_GUEST_RSP, (UINT64)GuestStack);

    //
    // setup guest rip
    //
    VmxVmwrite64(VMCS_GUEST_RIP, (UINT64)AsmVmxRestoreState);

    //
    // Stack should be aligned to 16 because we wanna save XMM and FPU registers and those instructions
    // needs alignment to 16
    //
    HostRsp = (PVOID)((UINT64)VCpu->VmmStack + VMM_STACK_SIZE - 1);
    HostRsp = ((PVOID)((ULONG_PTR)(HostRsp) & ~(16 - 1)));
    VmxVmwrite64(VMCS_HOST_RSP, (UINT64)HostRsp);
    VmxVmwrite64(VMCS_HOST_RIP, (UINT64)AsmVmexitHandler);

    return TRUE;
}

/**
 * @brief Resume VM using VMRESUME instruction
 *
 * @return VOID
 */
VOID
VmxVmresume()
{
    UINT64 ErrorCode = 0;

    __vmx_vmresume();

    //
    // if VMRESUME succeed will never be here !
    //

    __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
    __vmx_off();

    //
    // It's such a bad error because we don't where to go !
    // prefer to break
    //

    LogError("Err, in executing VMRESUME, status : 0x%llx, last VM-exit reason: 0x%x",
             ErrorCode,
             g_GuestState[KeGetCurrentProcessorNumberEx(NULL)].ExitReason);
}

/**
 * @brief VMFUNC instruction
 * @details Should be executed in VMX NON-root
 *
 * @param EptpIndex
 * @param Function
 *
 * @return UINT64
 */
UINT64
VmxVmfunc(UINT32 EptpIndex, UINT32 Function)
{
    //
    // *** To be executed in VMX non-root ***
    //

    //
    // Description from : https://users.cs.utah.edu/~aburtsev/lls-sem/index.php?n=Main.VMFUNCNotes
    //
    // VMFUNC is a new Intel primitive that allows to change an EPT page table underneath a VT-x VM without exiting into the hypervisor
    // Effectively, it's a page table switch in hardware and thus it allows one to build a fast context switch
    //

    //
    // Each VT-x virtual machine is configured with a Virtual Machine Control Structure (VMCS)
    //  This is a page of memory in which the VMM writes configuration data for things like how interrupts are handled,
    //  initial control register values during guest entry, and a whole bunch of other things
    //
    // One of those other things is a pointer to a page of candidate EPT pointers. These are pointers to different EPT
    //  page table hierarchies, each one giving a possibly different physical -> machine mapping. The VMM sets up this page
    //  of EPT pointers and has to also turn on a couple other settings in the VMCS to fully enable EPT switching via VMFUNC
    //
    // In non-root operation (inside a VM) code running in any privilege level can switch EPT hierarchies through the following
    //  steps:
    //
    // Storing 0 in %rax (EPT switching is VMFUNC 0)
    // Storing the index into the candidate EPT table in %rcx
    // Invoking the VMFUNC instruction
    // The processor will switch EPTs. Invoking VMFUNC will not cause a VM Exit
    //
    // All of this is detailed in the Intel SDM Volume 3, 25.5.5.3 "EPT Switching"
    //
    // It is worth noting that this will not change/save values in control registers (e.g. %cr3), general purpose registers,
    //  and so on. It's up to the code and VMM to set things up so it all works gracefully
    //

    //
    // Are TLBs flushed ? No, unless VPIDs are not being used(which I, Charlie, would say is rare)
    // See Intel SDM Volume 3, 25.5.5.3 "EPT Switching" and 28.3.3.1 "Operations that Invalidate Cached Mappings"
    //

    return AsmVmfunc(EptpIndex, Function);
}

/**
 * @brief Prepare and execute Vmxoff instruction
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
VmxVmxoff(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 GuestRSP              = 0; // Save a pointer to guest rsp for times that we want to return to previous guest stateS
    UINT64 GuestRIP              = 0; // Save a pointer to guest rip for times that we want to return to previous guest state
    UINT64 GuestCr3              = 0;
    UINT64 ExitInstructionLength = 0;

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

    __vmx_vmread(VMCS_GUEST_CR3, &GuestCr3);
    __writecr3(GuestCr3);

    //
    // Read guest rsp and rip
    //
    __vmx_vmread(VMCS_GUEST_RIP, &GuestRIP);
    __vmx_vmread(VMCS_GUEST_RSP, &GuestRSP);

    //
    // Read instruction length
    //
    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstructionLength);
    GuestRIP += ExitInstructionLength;

    //
    // Set the previous register states
    //
    VCpu->VmxoffState.GuestRip = GuestRIP;
    VCpu->VmxoffState.GuestRsp = GuestRSP;

    //
    // Notify the Vmexit handler that VMX already turned off
    //
    VCpu->VmxoffState.IsVmxoffExecuted = TRUE;

    //
    // Restore the previous FS, GS , GDTR and IDTR register as patchguard might find the modified
    //
    HvRestoreRegisters();

    //
    // Restore XMM registers
    // We restore XMM registers here because we are going to execute vmxoff instruction
    // which enables interrupts and we don't want to lose the XMM registers
    // since immediately after vmxoff, an interrupt might occur and context switch
    // might change the XMM registers
    //
    AsmVmxoffRestoreXmmRegs((unsigned long long)VCpu->XmmRegs);

    //
    // Before using vmxoff, you first need to use vmclear on any VMCSes that you want to be able to use again.
    // See sections 24.1 and 24.11 of the SDM.
    //
    VmxClearVmcsState(VCpu);

    //
    // Execute Vmxoff
    //
    __vmx_off();

    //
    // *** Note: After executing VMXOFF, XMM registers should not be used anymore
    // Since we already restored them ***
    //

    //
    // Indicate the current core is not currently virtualized
    //
    VCpu->HasLaunched = FALSE;

    //
    // Now that VMX is OFF, we have to unset vmx-enable bit on cr4
    //
    __writecr4(__readcr4() & (~X86_CR4_VMXE));
}

/**
 * @brief Get the RIP of guest (VMCS_GUEST_RIP) in the case of return from VMXOFF
 *
 * @return UINT64 Returns the stack pointer, to change in the case of Vmxoff
 */
UINT64
VmxReturnStackPointerForVmxoff()
{
    return g_GuestState[KeGetCurrentProcessorNumberEx(NULL)].VmxoffState.GuestRsp;
}

/**
 * @brief Get the RIP of guest (VMCS_GUEST_RIP) in the case of return from VMXOFF
 *
 * @return UINT64 Returns the instruction pointer, to change in the case of Vmxoff
 */
UINT64
VmxReturnInstructionPointerForVmxoff()
{
    return g_GuestState[KeGetCurrentProcessorNumberEx(NULL)].VmxoffState.GuestRip;
}

/**
 * @brief Terminate Vmx on all logical cores
 *
 * @return VOID
 */
VOID
VmxPerformTermination()
{
    ULONG ProcessorsCount;

    LogDebugInfo("Terminating VMX...\n");

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // ******* Terminating Vmx *******
    //

    //
    // Unhide (disable and de-allocate) transparent-mode
    //
    if (g_CheckForFootprints)
    {
        TransparentUnhideDebuggerWrapper(NULL);
    }

    //
    // Remove All the hooks if any
    //
    EptHookUnHookAll();

    //
    // Restore the state of execution trap hooks
    //
    ExecTrapUninitialize();

    //
    // Broadcast to terminate Vmx
    //
    KeGenericCallDpc(DpcRoutineTerminateGuest, 0x0);

    //
    // ****** De-allocatee global variables ******
    //

    //
    // Free the buffer related to MSRs that cause #GP
    //
    PlatformMemFreePool(g_MsrBitmapInvalidMsrs);
    g_MsrBitmapInvalidMsrs = NULL;

    //
    // Free Identity Page Table
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        if (g_GuestState[i].EptPageTable != NULL)
        {
            MmFreeContiguousMemory(g_GuestState[i].EptPageTable);
        }

        g_GuestState[i].EptPageTable = NULL;
    }

    //
    // Free EptState
    //
    PlatformMemFreePool(g_EptState);
    g_EptState = NULL;

    //
    // Free the Pool manager
    //
    PoolManagerUninitialize();

    //
    // Uninitialize memory mapper
    //
    MemoryMapperUninitialize();

    //
    // Free g_GuestState
    //
    GlobalGuestStateFreeMemory();

    LogDebugInfo("VMX operation turned off successfully");
}

/**
 * @brief implementation of vmx-root mode compatible strlen
 * @param S
 *
 * @return UINT32 If 0x0 indicates an error, otherwise length of the
 * string
 */
UINT32
VmxCompatibleStrlen(const CHAR * S)
{
    CHAR     Temp  = NULL_ZERO;
    UINT32   Count = 0;
    UINT64   AlignedAddress;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // First check
    //
    if (!CheckAccessValidityAndSafety(AlignedAddress, sizeof(CHAR)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0;
    }

    while (TRUE)
    {
        /*
        Temp = *S;
        */
        MemoryMapperReadMemorySafe((UINT64)S, &Temp, sizeof(CHAR));

        if (Temp != '\0')
        {
            Count++;
            S++;
        }
        else
        {
            //
            // Move back to original cr3
            //
            __writecr3(OriginalCr3.Flags);
            return Count;
        }

        if (!((UINT64)S & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)S, sizeof(CHAR)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0;
            }
        }
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
}

/**
 * @brief implementation of vmx-root mode compatible wcslen
 * @param S
 *
 * @return UINT32 If 0x0 indicates an error, otherwise length of the
 * string
 */
UINT32
VmxCompatibleWcslen(const wchar_t * S)
{
    wchar_t  Temp  = NULL_ZERO;
    UINT32   Count = 0;
    UINT64   AlignedAddress;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // First check
    //
    if (!CheckAccessValidityAndSafety(AlignedAddress, sizeof(wchar_t)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0;
    }

    while (TRUE)
    {
        /*
        Temp = *S;
        */
        MemoryMapperReadMemorySafe((UINT64)S, &Temp, sizeof(wchar_t));

        if (Temp != '\0\0')
        {
            Count++;
            S++;
        }
        else
        {
            //
            // Move back to original cr3
            //
            __writecr3(OriginalCr3.Flags);
            return Count;
        }

        if (!((UINT64)S & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)S, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0;
            }
        }
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
}

/**
 * @brief VMX-root compatible micro sleep
 * @param Us Delay in micro seconds
 *
 * @return VOID
 */
VOID
VmxCompatibleMicroSleep(UINT64 Us)
{
    LARGE_INTEGER Start, End, Frequency;
    KeQueryPerformanceCounter(&Frequency);

    LONGLONG Ticks = (Frequency.QuadPart / 1000000) * Us;

    Start = KeQueryPerformanceCounter(NULL);

    while (TRUE)
    {
        End = KeQueryPerformanceCounter(NULL);
        if (End.QuadPart - Start.QuadPart > Ticks)
            break;
    }
}

/**
 * @brief implementation of vmx-root mode compatible strcmp and strncmp
 * @param Address1
 * @param Address2
 * @param Num
 * param IsStrncmp
 *
 * @return INT32 0x2 indicates error, otherwise the same result as strcmp in string.h
 */
INT32
VmxCompatibleStrcmp(const CHAR * Address1,
                    const CHAR * Address2,
                    SIZE_T       Num,
                    BOOLEAN      IsStrncmp)
{
    CHAR     C1 = NULL_ZERO, C2 = NULL_ZERO;
    INT32    Result = 0;
    UINT32   Count  = 0;
    UINT64   AlignedAddress1, AlignedAddress2;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress1 = (UINT64)PAGE_ALIGN((UINT64)Address1);
    AlignedAddress2 = (UINT64)PAGE_ALIGN((UINT64)Address2);

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // First check
    //
    if (!CheckAccessValidityAndSafety(AlignedAddress1, sizeof(CHAR)) || !CheckAccessValidityAndSafety(AlignedAddress2, sizeof(CHAR)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0x2;
    }

    do
    {
        //
        // Check to see if we have byte number constraints
        //
        if (IsStrncmp)
        {
            if (Count == Num)
            {
                //
                // Maximum number of bytes reached
                //
                break;
            }
            else
            {
                //
                // Maximum number of bytes not reached
                //
                Count++;
            }
        }

        /*
        C1 = *Address1;
        */
        MemoryMapperReadMemorySafe((UINT64)Address1, &C1, sizeof(CHAR));

        /*
        C2 = *Address2;
        */
        MemoryMapperReadMemorySafe((UINT64)Address2, &C2, sizeof(CHAR));

        Address1++;
        Address2++;

        if (!((UINT64)AlignedAddress1 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress1, sizeof(CHAR)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }

        if (!((UINT64)AlignedAddress2 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress2, sizeof(CHAR)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }
        Result = C1 - C2;
    } while (!Result && C2);

    if (Result < 0)
    {
        Result = -1;
    }
    else if (Result > 0)
    {
        Result = 1;
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
    return Result;
}

/**
 * @brief implementation of vmx-root mode compatible wcscmp and wcsncmp
 * @param Address1
 * @param Address2
 * @param Num
 * @param IsWcsncmp
 *
 * @return INT32 0x2 indicates error, otherwise the same result as wcscmp in string.h
 */
INT32
VmxCompatibleWcscmp(const wchar_t * Address1,
                    const wchar_t * Address2,
                    SIZE_T          Num,
                    BOOLEAN         IsWcsncmp)
{
    wchar_t  C1 = NULL_ZERO, C2 = NULL_ZERO;
    INT32    Result = 0;
    UINT32   Count  = 0;
    UINT64   AlignedAddress1, AlignedAddress2;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress1 = (UINT64)PAGE_ALIGN((UINT64)Address1);
    AlignedAddress2 = (UINT64)PAGE_ALIGN((UINT64)Address2);

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // First check
    //
    if (!CheckAccessValidityAndSafety(AlignedAddress1, sizeof(wchar_t)) || !CheckAccessValidityAndSafety(AlignedAddress2, sizeof(wchar_t)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0x2;
    }

    do
    {
        //
        // Check to see if we have byte number constraints
        //
        if (IsWcsncmp)
        {
            if (Count == Num)
            {
                //
                // Maximum number of bytes reached
                //
                break;
            }
            else
            {
                //
                // Maximum number of bytes not reached
                //
                Count++;
            }
        }

        /*
        C1 = *Address1;
        */
        MemoryMapperReadMemorySafe((UINT64)Address1, &C1, sizeof(wchar_t));

        /*
        C2 = *Address2;
        */
        MemoryMapperReadMemorySafe((UINT64)Address2, &C2, sizeof(wchar_t));

        Address1++;
        Address2++;

        if (!((UINT64)AlignedAddress1 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress1, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }

        if (!((UINT64)AlignedAddress2 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress2, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }

        Result = C1 - C2;
    } while (!Result && C2);

    if (Result < 0)
    {
        Result = -1;
    }
    else if (Result > 0)
    {
        Result = 1;
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
    return Result;
}

/**
 * @brief implementation of vmx-root mode compatible memcmp
 * @param Address1
 * @param Address2
 * @param Count
 *
 * @return INT32 0x2 indicates error, otherwise the same result as memcmp in string.h
 */
INT32
VmxCompatibleMemcmp(const CHAR * Address1, const CHAR * Address2, size_t Count)
{
    CHAR     C1 = NULL_ZERO, C2 = NULL_ZERO;
    INT32    Result = 0;
    UINT64   AlignedAddress1, AlignedAddress2;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress1 = (UINT64)PAGE_ALIGN((UINT64)Address1);
    AlignedAddress2 = (UINT64)PAGE_ALIGN((UINT64)Address2);

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // First check
    //
    if (!CheckAccessValidityAndSafety(AlignedAddress1, sizeof(wchar_t)) || !CheckAccessValidityAndSafety(AlignedAddress2, sizeof(wchar_t)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0x2;
    }

    while (Count-- > 0 && !Result)
    {
        /*
        C1 = *Address1;
        */
        MemoryMapperReadMemorySafe((UINT64)Address1, &C1, sizeof(CHAR));

        /*
        C2 = *Address2;
        */
        MemoryMapperReadMemorySafe((UINT64)Address2, &C2, sizeof(CHAR));

        Address1++;
        Address2++;

        if (!((UINT64)AlignedAddress1 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress1, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }

        if (!((UINT64)AlignedAddress2 & (PAGE_SIZE - 1)))
        {
            if (!CheckAccessValidityAndSafety((UINT64)AlignedAddress2, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0x2;
            }
        }

        Result = C1 - C2;
    }

    if (Result < 0)
    {
        Result = -1;
    }
    else if (Result > 0)
    {
        Result = 1;
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
    return Result;
}
