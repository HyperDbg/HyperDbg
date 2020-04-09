#include <ntddk.h>
#include <Windef.h>
#include "Common.h"
#include "Msr.h"
#include "Hooks.h"
#include "Invept.h"
#include "Events.h"
#include "HypervisorRoutines.h"
#include "GlobalVariables.h"
#include "Vmx.h"
#include "Logging.h"


UINT64 SysretAddress;

#define IS_SYSRET_INSTRUCTION(Code) \
    (*((PUINT8)(Code) + 0) == 0x48 && \
     *((PUINT8)(Code) + 1) == 0x0F && \
     *((PUINT8)(Code) + 2) == 0x07)
#define IS_SYSCALL_INSTRUCTION(Code) \
    (*((PUINT8)(Code) + 0) == 0x0F && \
     *((PUINT8)(Code) + 1) == 0x05)







VOID SyscallHookDisableSCE() {

	EFER_MSR MsrValue;

	// Set the GUEST EFER to use this value as the EFER
	__vmx_vmread(GUEST_EFER, &MsrValue);
	MsrValue.SyscallEnable = FALSE;

	// Set the GUEST EFER to use this value as the EFER
	__vmx_vmwrite(GUEST_EFER, MsrValue.Flags);

}

VOID SyscallHookEnableSCE() {

	EFER_MSR MsrValue;

	// Set the GUEST EFER to use this value as the EFER
	__vmx_vmread(GUEST_EFER, &MsrValue);
	MsrValue.SyscallEnable = TRUE;

	// Set the GUEST EFER to use this value as the EFER
	__vmx_vmwrite(GUEST_EFER, MsrValue.Flags);

}

VOID SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook)
{
	EFER_MSR MsrValue;
	IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };
	UINT32 VmEntryControls = 0;
	UINT32 VmExitControls = 0;

	// Reading IA32_VMX_BASIC_MSR 
	VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);

	// Read previous VM-Entry and VM-Exit controls
	__vmx_vmread(VM_ENTRY_CONTROLS, &VmEntryControls);
	__vmx_vmread(VM_EXIT_CONTROLS, &VmExitControls);

	MsrValue.Flags = __readmsr(MSR_EFER);

	if (EnableEFERSyscallHook)
	{
		MsrValue.SyscallEnable = FALSE;

		// Set VM-Entry controls to load EFER
		__vmx_vmwrite(VM_ENTRY_CONTROLS, HvAdjustControls(VmEntryControls | VM_ENTRY_LOAD_IA32_EFER,
			VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS));

		// Set VM-Exit controls to save EFER
		__vmx_vmwrite(VM_EXIT_CONTROLS, HvAdjustControls(VmExitControls | VM_EXIT_SAVE_IA32_EFER,
			VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS));

		// Set the GUEST EFER to use this value as the EFER
		__vmx_vmwrite(GUEST_EFER, MsrValue.Flags);
	}
	else
	{
		// Btw, No need to this 
		MsrValue.SyscallEnable = TRUE;

		// Set VM-Entry controls to load EFER
		__vmx_vmwrite(VM_ENTRY_CONTROLS, HvAdjustControls(VmEntryControls & ~VM_ENTRY_LOAD_IA32_EFER,
			VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS));

		// Set VM-Exit controls to save EFER
		__vmx_vmwrite(VM_EXIT_CONTROLS, HvAdjustControls(VmExitControls & ~VM_EXIT_SAVE_IA32_EFER,
			VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS));

		// Set the GUEST EFER to use this value as the EFER
		__vmx_vmwrite(GUEST_EFER, MsrValue.Flags);
	}


}

VOID SetGuestCs(PSEGMENT_SELECTOR Cs)
{
	__vmx_vmwrite(GUEST_CS_BASE, Cs->BASE);
	__vmx_vmwrite(GUEST_CS_LIMIT, Cs->LIMIT);
	__vmx_vmwrite(GUEST_CS_AR_BYTES, Cs->ATTRIBUTES.UCHARs);
	__vmx_vmwrite(GUEST_CS_SELECTOR, Cs->SEL);
}

VOID SetGuestSs(PSEGMENT_SELECTOR Cs)
{
	__vmx_vmwrite(GUEST_SS_BASE, Cs->BASE);
	__vmx_vmwrite(GUEST_SS_LIMIT, Cs->LIMIT);
	__vmx_vmwrite(GUEST_SS_AR_BYTES, Cs->ATTRIBUTES.UCHARs);
	__vmx_vmwrite(GUEST_SS_SELECTOR, Cs->SEL);
}


/* SYSCALL instruction emulation routine */
BOOLEAN SyscallHookEmulateSYSCALL(PGUEST_REGS Regs)
{
	SEGMENT_SELECTOR Cs, Ss;
	UINT32 InstructionLength;
	UINT64 MsrValue;
	ULONG64 GuestRip;
	ULONG64 GuestRflags;

	// Reading guest's RIP 
	__vmx_vmread(GUEST_RIP, &GuestRip);

	// Reading instruction length 
	__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &InstructionLength);

	// Reading guest's Rflags
	__vmx_vmread(GUEST_RFLAGS, &GuestRflags);

	// Save the address of the instruction following SYSCALL into RCX and then
	// load RIP from MSR_LSTAR.
	MsrValue = __readmsr(MSR_LSTAR);
	Regs->rcx = GuestRip + InstructionLength;
	GuestRip = MsrValue;
	__vmx_vmwrite(GUEST_RIP, GuestRip);

	// Save RFLAGS into R11 and then mask RFLAGS using MSR_FMASK.
	MsrValue = __readmsr(MSR_FMASK);
	Regs->r11 = GuestRflags;
	GuestRflags &= ~(MsrValue | X86_FLAGS_RF);
	__vmx_vmwrite(GUEST_RFLAGS, GuestRflags);

	// Load the CS and SS selectors with values derived from bits 47:32 of MSR_STAR.
	MsrValue = __readmsr(MSR_STAR);
	Cs.SEL = (UINT16)((MsrValue >> 32) & ~3);          // STAR[47:32] & ~RPL3
	Cs.BASE = 0;                                            // flat segment
	Cs.LIMIT = (UINT32)~0;                                  // 4GB limit
	Cs.ATTRIBUTES.UCHARs = 0xA09B;                              // L+DB+P+S+DPL0+Code
	SetGuestCs(&Cs);


	Ss.SEL = (UINT16)(((MsrValue >> 32) & ~3) + 8);    // STAR[47:32] + 8
	Ss.BASE = 0;                                            // flat segment
	Ss.LIMIT = (UINT32)~0;                                  // 4GB limit
	Ss.ATTRIBUTES.UCHARs = 0xC093;                              // G+DB+P+S+DPL0+Data
	SetGuestSs(&Ss);
	return TRUE;
}


/* SYSRET instruction emulation routine */
BOOLEAN SyscallHookEmulateSYSRET(PGUEST_REGS Regs)
{
	SEGMENT_SELECTOR Cs, Ss;
	UINT64 MsrValue;

	ULONG64 GuestRip;
	ULONG64 GuestRflags;

	// Load RIP from RCX.
	GuestRip = Regs->rcx;
	__vmx_vmwrite(GUEST_RIP, GuestRip);

	// Load RFLAGS from R11. Clear RF, VM, reserved bits.
	GuestRflags = (Regs->r11 & ~(X86_FLAGS_RF | X86_FLAGS_VM | X86_FLAGS_RESERVED_BITS)) | X86_FLAGS_FIXED;
	__vmx_vmwrite(GUEST_RFLAGS, GuestRflags);

	// SYSRET loads the CS and SS selectors with values derived from bits 63:48 of MSR_STAR.
	MsrValue = __readmsr(MSR_STAR);
	Cs.SEL = (UINT16)(((MsrValue >> 48) + 16) | 3);    // (STAR[63:48]+16) | 3 (* RPL forced to 3 *)
	Cs.BASE = 0;                                            // Flat segment
	Cs.LIMIT = (UINT32)~0;                                  // 4GB limit
	Cs.ATTRIBUTES.UCHARs = 0xA0FB;                              // L+DB+P+S+DPL3+Code
	SetGuestCs(&Cs);


	Ss.SEL = (UINT16)(((MsrValue >> 48) + 8) | 3);     // (STAR[63:48]+8) | 3 (* RPL forced to 3 *)
	Ss.BASE = 0;                                            // Flat segment
	Ss.LIMIT = (UINT32)~0;                                  // 4GB limit
	Ss.ATTRIBUTES.UCHARs = 0xC0F3;                                  // G+DB+P+S+DPL3+Data
	SetGuestSs(&Ss);
	return TRUE;
}

BOOLEAN SyscallHookHandleUD(PGUEST_REGS Regs, UINT32 CoreIndex)
{
	UINT64 GuestCr3;
	UINT64 OriginalCr3;
	UINT64 Rip;
	BOOLEAN Result;

	// Reading guest's RIP 
	__vmx_vmread(GUEST_RIP, &Rip);

	if (SysretAddress == NULL)
	{
		/* Find the address of sysret */

		// Due to KVA Shadowing, we need to switch to a different directory table base 
		// if the PCID indicates this is a user mode directory table base.

		__vmx_vmread(GUEST_CR3, &GuestCr3);

		OriginalCr3 = __readcr3();
		NT_KPROCESS* CurrentProcess = (NT_KPROCESS*)(PsGetCurrentProcess());
		__writecr3(CurrentProcess->DirectoryTableBase);

		if (IS_SYSRET_INSTRUCTION(Rip))
		{
			__writecr3(OriginalCr3);

			// Save the address of Sysret, it won't change
			SysretAddress = Rip;
		}
		__writecr3(OriginalCr3);
	}

	if (Rip == SysretAddress)
	{
		// It's a sysret instruction, let's emulate it
		goto EmulateSYSRET;
	}
	else if (Rip & 0xff00000000000000)
	{
		// It's a #UD in kernel, not relate to us
		// this way the caller injects a #UD
		return FALSE;
	}
	else
	{
		// It's sth in usermode, might be a syscall
		goto EmulateSYSCALL;
	}

	// Emulate SYSRET instruction.
EmulateSYSRET:
	LogInfo("SYSRET instruction => 0x%llX", Rip);
	Result = SyscallHookEmulateSYSRET(Regs);
	GuestState[KeGetCurrentProcessorIndex()].IncrementRip = FALSE;
	return Result;
	// Emulate SYSCALL instruction.
EmulateSYSCALL:
	// Result = SyscallHookEmulateSYSCALL(Regs);
	SyscallHookEnableSCE();
	HvSetMonitorTrapFlag(TRUE);
	GuestState[CoreIndex].IncrementRip = FALSE;
	GuestState[CoreIndex].DebuggingState.UndefinedInstructionAddress = Rip;
	return TRUE;
}
