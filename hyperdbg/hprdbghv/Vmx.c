#include "Msr.h"
#include "Vmx.h"
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "GlobalVariables.h"
#include "Vmcall.h"
#include "HypervisorRoutines.h"
#include "Invept.h"
#include "Vpid.h"
#include "Dpc.h"
#include "Events.h"

/* Initialize VMX Operation */
BOOLEAN VmxInitializer()
{
	int ProcessorCount;
	KAFFINITY AffinityMask;

	if (!HvIsVmxSupported())
	{
		LogError("VMX is not supported in this machine !");
		return FALSE;
	}

	PAGED_CODE();

	// Allocate	global variable to hold Ept State
	EptState = ExAllocatePoolWithTag(NonPagedPool, sizeof(EPT_STATE), POOLTAG);

	if (!EptState)
	{
		LogError("Insufficient memory");
		return FALSE;
	}

	// Zero memory
	RtlZeroMemory(EptState, sizeof(EPT_STATE));

	// Initialize the list of hooked pages detail
	InitializeListHead(&EptState->HookedPagesList);

	// Check whether EPT is supported or not
	if (!EptCheckFeatures())
	{
		LogError("Your processor doesn't support all EPT features");
		return FALSE;
	}
	else
	{
		// Our processor supports EPT, now let's build MTRR
		LogInfo("Your processor supports all EPT features");

		// Build MTRR Map
		if (!EptBuildMtrrMap())
		{
			LogError("Could not build Mtrr memory map");
			return FALSE;
		}
		LogInfo("Mtrr memory map built successfully");
	}

	// Initialize Pool Manager
	if (!PoolManagerInitialize())
	{
		LogError("Could not initialize pool manager");
		return FALSE;
	}

	if (!EptLogicalProcessorInitialize())
	{
		// There were some errors in EptLogicalProcessorInitialize
		return FALSE;
	}

	// Allocate and run Vmxon and Vmptrld on all logical cores
	KeGenericCallDpc(VmxDpcBroadcastAllocateVmxonRegions, 0x0);

	// Everything is ok, let's return true
	return TRUE;
}

/* Virtualizing an already running system, this function won't return TRUE as when Vmlaunch is executed the
   rest of the function never executes but returning FALSE is an indication of error.
*/
BOOLEAN VmxVirtualizeCurrentSystem(PVOID GuestStack)
{
	ULONG64 ErrorCode;
	INT ProcessorID;

	ProcessorID = KeGetCurrentProcessorNumber();

	Log("======================== Virtualizing Current System (Logical Core : 0x%x) ========================", ProcessorID);

	// Clear the VMCS State
	if (!VmxClearVmcsState(&GuestState[ProcessorID])) {
		LogError("Failed to clear vmcs");
		return FALSE;
	}

	// Load VMCS (Set the Current VMCS)
	if (!VmxLoadVmcs(&GuestState[ProcessorID]))
	{
		LogError("Failed to load vmcs");
		return FALSE;
	}

	LogInfo("Setting up VMCS for current logical core");
	VmxSetupVmcs(&GuestState[ProcessorID], GuestStack);

	LogInfo("Executing VMLAUNCH on logical core %d", ProcessorID);

	// Setting the state to indicate current core is currently virtualized
	GuestState[ProcessorID].HasLaunched = TRUE;

	__vmx_vmlaunch();

	/* if Vmlaunch succeed will never be here ! */

	// If failed, then indiacte that current core is not currently virtualized
	GuestState[ProcessorID].HasLaunched = FALSE;

	// Execute Vmxoff
	__vmx_off();

	ErrorCode = 0;
	__vmx_vmread(VM_INSTRUCTION_ERROR, &ErrorCode);
	LogError("VMLAUNCH Error : 0x%llx", ErrorCode);

	LogError("VMXOFF Executed Successfully but it was because of an error.");

	return FALSE;
}


/* Broadcast to terminate VMX on all logical cores */
BOOLEAN VmxTerminate()
{

	int CurrentCoreIndex;
	NTSTATUS Status;

	// Get the current core index
	CurrentCoreIndex = KeGetCurrentProcessorNumber();

	// Execute Vmcall to to turn off vmx from Vmx root mode
	Status = AsmVmxVmcall(VMCALL_VMXOFF, NULL, NULL, NULL);

	if (Status == STATUS_SUCCESS)
	{
		DbgPrint("VMX Terminated on logical core %d\n", CurrentCoreIndex);

		// Free the destination memory
		MmFreeContiguousMemory(GuestState[CurrentCoreIndex].VmxonRegionVirtualAddress);
		MmFreeContiguousMemory(GuestState[CurrentCoreIndex].VmcsRegionVirtualAddress);
		ExFreePoolWithTag(GuestState[CurrentCoreIndex].VmmStack, POOLTAG);
		ExFreePoolWithTag(GuestState[CurrentCoreIndex].MsrBitmapVirtualAddress, POOLTAG);
		
		return TRUE;
	}

	return FALSE;
}

/* Implementation of Vmptrst instruction */
VOID VmxVmptrst()
{
	PHYSICAL_ADDRESS VmcsPhysicalAddr;
	VmcsPhysicalAddr.QuadPart = 0;
	__vmx_vmptrst((unsigned __int64*)&VmcsPhysicalAddr);

	LogInfo("Vmptrst result : %llx", VmcsPhysicalAddr);

}
/* Clearing Vmcs status using Vmclear instruction */
BOOLEAN VmxClearVmcsState(VIRTUAL_MACHINE_STATE* CurrentGuestState)
{
	int VmclearStatus;

	// Clear the state of the VMCS to inactive
	VmclearStatus = __vmx_vmclear(&CurrentGuestState->VmcsRegionPhysicalAddress);

	LogInfo("Vmcs Vmclear Status : %d", VmclearStatus);

	if (VmclearStatus)
	{
		// Otherwise terminate the VMX
		LogWarning("VMCS failed to clear ( status : %d )", VmclearStatus);
		__vmx_off();
		return FALSE;
	}
	return TRUE;
}

/* Implementation of Vmptrld instruction */
BOOLEAN VmxLoadVmcs(VIRTUAL_MACHINE_STATE* CurrentGuestState) {

	int VmptrldStatus;

	VmptrldStatus = __vmx_vmptrld(&CurrentGuestState->VmcsRegionPhysicalAddress);
	if (VmptrldStatus)
	{
		LogWarning("VMCS failed to load ( status : %d )", VmptrldStatus);
		return FALSE;
	}
	return TRUE;
}

/* Create and Configure a Vmcs Layout */
BOOLEAN VmxSetupVmcs(VIRTUAL_MACHINE_STATE* CurrentGuestState, PVOID GuestStack)
{

	ULONG CpuBasedVmExecControls;
	ULONG SecondaryProcBasedVmExecControls;
	PVOID HostRsp;
	ULONG64 GdtBase = 0;
	SEGMENT_SELECTOR SegmentSelector = { 0 };
	IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };

	// Reading IA32_VMX_BASIC_MSR 
	VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);

	__vmx_vmwrite(HOST_ES_SELECTOR, AsmGetEs() & 0xF8);
	__vmx_vmwrite(HOST_CS_SELECTOR, AsmGetCs() & 0xF8);
	__vmx_vmwrite(HOST_SS_SELECTOR, AsmGetSs() & 0xF8);
	__vmx_vmwrite(HOST_DS_SELECTOR, AsmGetDs() & 0xF8);
	__vmx_vmwrite(HOST_FS_SELECTOR, AsmGetFs() & 0xF8);
	__vmx_vmwrite(HOST_GS_SELECTOR, AsmGetGs() & 0xF8);
	__vmx_vmwrite(HOST_TR_SELECTOR, AsmGetTr() & 0xF8);

	// Setting the link pointer to the required value for 4KB VMCS.
	__vmx_vmwrite(VMCS_LINK_POINTER, ~0ULL);

	__vmx_vmwrite(GUEST_IA32_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32);

	/* Time-stamp counter offset */
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

	CpuBasedVmExecControls = HvAdjustControls(CPU_BASED_ACTIVATE_MSR_BITMAP | CPU_BASED_ACTIVATE_SECONDARY_CONTROLS,
		VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_PROCBASED_CTLS : MSR_IA32_VMX_PROCBASED_CTLS);

	__vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedVmExecControls);

	LogInfo("Cpu Based VM Exec Controls (Based on %s) : 0x%x",
		VmxBasicMsr.Fields.VmxCapabilityHint ? "MSR_IA32_VMX_TRUE_PROCBASED_CTLS" : "MSR_IA32_VMX_PROCBASED_CTLS", CpuBasedVmExecControls);

	SecondaryProcBasedVmExecControls = HvAdjustControls(CPU_BASED_CTL2_RDTSCP |
		CPU_BASED_CTL2_ENABLE_EPT | CPU_BASED_CTL2_ENABLE_INVPCID |
		CPU_BASED_CTL2_ENABLE_XSAVE_XRSTORS | CPU_BASED_CTL2_ENABLE_VPID, MSR_IA32_VMX_PROCBASED_CTLS2);

	__vmx_vmwrite(SECONDARY_VM_EXEC_CONTROL, SecondaryProcBasedVmExecControls);
	LogInfo("Secondary Proc Based VM Exec Controls (MSR_IA32_VMX_PROCBASED_CTLS2) : 0x%x", SecondaryProcBasedVmExecControls);

	__vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL, HvAdjustControls(0,
		VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_PINBASED_CTLS : MSR_IA32_VMX_PINBASED_CTLS));

	__vmx_vmwrite(VM_EXIT_CONTROLS, HvAdjustControls(VM_EXIT_IA32E_MODE,
		VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS));

	__vmx_vmwrite(VM_ENTRY_CONTROLS, HvAdjustControls(VM_ENTRY_IA32E_MODE,
		VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS));



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

	/*
	Because we may be executing in an arbitrary user-mode, process as part
	of the DPC interrupt we execute in We have to save Cr3, for HOST_CR3
	*/

	__vmx_vmwrite(HOST_CR3, InitiateCr3);

	__vmx_vmwrite(GUEST_GDTR_BASE, AsmGetGdtBase());
	__vmx_vmwrite(GUEST_IDTR_BASE, AsmGetIdtBase());
	__vmx_vmwrite(GUEST_GDTR_LIMIT, AsmGetGdtLimit());
	__vmx_vmwrite(GUEST_IDTR_LIMIT, AsmGetIdtLimit());

	__vmx_vmwrite(GUEST_RFLAGS, AsmGetRflags());

	__vmx_vmwrite(GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
	__vmx_vmwrite(GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));

	HvGetSegmentDescriptor(&SegmentSelector, AsmGetTr(), (PUCHAR)AsmGetGdtBase());
	__vmx_vmwrite(HOST_TR_BASE, SegmentSelector.BASE);

	__vmx_vmwrite(HOST_FS_BASE, __readmsr(MSR_FS_BASE));
	__vmx_vmwrite(HOST_GS_BASE, __readmsr(MSR_GS_BASE));

	__vmx_vmwrite(HOST_GDTR_BASE, AsmGetGdtBase());
	__vmx_vmwrite(HOST_IDTR_BASE, AsmGetIdtBase());

	__vmx_vmwrite(HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
	__vmx_vmwrite(HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));

	// Set MSR Bitmaps
	__vmx_vmwrite(MSR_BITMAP, CurrentGuestState->MsrBitmapPhysicalAddress);

	// Set exception bitmap to hook division by zero (bit 1 of EXCEPTION_BITMAP)
	// __vmx_vmwrite(EXCEPTION_BITMAP, 0x8); // breakpoint 3nd bit

	// Set up EPT 
	__vmx_vmwrite(EPT_POINTER, EptState->EptPointer.Flags);

	// Set up VPID
	/* For all processors, we will use a VPID = 1. This allows the processor to separate caching
	   of EPT structures away from the regular OS page translation tables in the TLB.	*/
	__vmx_vmwrite(VIRTUAL_PROCESSOR_ID, VPID_TAG);

	//setup guest rsp
	__vmx_vmwrite(GUEST_RSP, (ULONG64)GuestStack);

	//setup guest rip
	__vmx_vmwrite(GUEST_RIP, (ULONG64)AsmVmxRestoreState);

	// Stack should be aligned to 16 because we wanna save XMM and FPU registers and those instructions
	// needs aligment to 16
	HostRsp = (ULONG64)CurrentGuestState->VmmStack + VMM_STACK_SIZE - 1;
	HostRsp = ((PVOID)((ULONG_PTR)(HostRsp) & ~(16 - 1)));
	__vmx_vmwrite(HOST_RSP, HostRsp);
	__vmx_vmwrite(HOST_RIP, (ULONG64)AsmVmexitHandler);


	return TRUE;
}


/* Resume vm using Vmresume instruction */
VOID VmxVmresume()
{
	ULONG64 ErrorCode;

	__vmx_vmresume();

	// if VMRESUME succeed will never be here !

	ErrorCode = 0;
	__vmx_vmread(VM_INSTRUCTION_ERROR, &ErrorCode);
	__vmx_off();


	// It's such a bad error because we don't where to go !
	// prefer to break
	LogError("Error in executing Vmresume , status : 0x%llx", ErrorCode);

}

/* Prepare and execute Vmxoff instruction */
VOID VmxVmxoff()
{

	int CurrentProcessorIndex;
	UINT64 GuestRSP; 	// Save a pointer to guest rsp for times that we want to return to previous guest stateS
	UINT64 GuestRIP; 	// Save a pointer to guest rip for times that we want to return to previous guest state
	UINT64 GuestCr3;
	UINT64 ExitInstructionLength;

	// Initialize the variables
	ExitInstructionLength = 0;
	GuestRIP = 0;
	GuestRSP = 0;

	CurrentProcessorIndex = KeGetCurrentProcessorNumber();

	/*
	According to SimpleVisor :
		Our callback routine may have interrupted an arbitrary user process,
		and therefore not a thread running with a system-wide page directory.
		Therefore if we return back to the original caller after turning off
		VMX, it will keep our current "host" CR3 value which we set on entry
		to the PML4 of the SYSTEM process. We want to return back with the
		correct value of the "guest" CR3, so that the currently executing
		process continues to run with its expected address space mappings.
	*/

	__vmx_vmread(GUEST_CR3, &GuestCr3);
	__writecr3(GuestCr3);

	// Read guest rsp and rip
	__vmx_vmread(GUEST_RIP, &GuestRIP);
	__vmx_vmread(GUEST_RSP, &GuestRSP);

	// Read instruction length
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);
	GuestRIP += ExitInstructionLength;

	// Set the previous registe states
	GuestState[CurrentProcessorIndex].VmxoffState.GuestRip = GuestRIP;
	GuestState[CurrentProcessorIndex].VmxoffState.GuestRsp = GuestRSP;

	// Notify the Vmexit handler that VMX already turned off
	GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted = TRUE;

	// Restore the previous FS, GS , GDTR and IDTR register as patchguard might find the modified
	HvRestoreRegisters();

	// Execute Vmxoff
	__vmx_off();

	// Inidcate the current core is not currently virtualized
	GuestState[CurrentProcessorIndex].HasLaunched = FALSE;

	// Now that VMX is OFF, we have to unset vmx-enable bit on cr4
	__writecr4(__readcr4() & (~X86_CR4_VMXE));

}