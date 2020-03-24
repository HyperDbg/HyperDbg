#include "Vmx.h"
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "GlobalVariables.h"
#include "Vmcall.h"
#include "HypervisorRoutines.h"
#include "Events.h"


/* Main Vmexit events handler */
BOOLEAN VmxVmexitHandler(PGUEST_REGS GuestRegs)
{
	int CurrentProcessorIndex;
	VMEXIT_INTERRUPT_INFO InterruptExit;
	UINT64 GuestPhysicalAddr;
	ULONG ExitReason;
	ULONG ExitQualification;
	ULONG Rflags;
	ULONG EcxReg;
	ULONG ExitInstructionLength;

	/*********** SEND MESSAGE AFTER WE SET THE STATE ***********/

	CurrentProcessorIndex = KeGetCurrentProcessorNumber();

	// Indicates we are in Vmx root mode in this logical core
	GuestState[CurrentProcessorIndex].IsOnVmxRootMode = TRUE;

	GuestState[CurrentProcessorIndex].IncrementRip = TRUE;

	ExitReason = 0;
	__vmx_vmread(VM_EXIT_REASON, &ExitReason);
	ExitReason &= 0xffff;

	ExitQualification = 0;
	__vmx_vmread(EXIT_QUALIFICATION, &ExitQualification);


	// Debugging purpose
	//LogInfo("VM_EXIT_REASON : 0x%x", ExitReason);
	//LogInfo("EXIT_QUALIFICATION : 0x%llx", ExitQualification);

	switch (ExitReason)
	{
	case EXIT_REASON_TRIPLE_FAULT:
	{
		LogError("Triple fault error occured.");

		break;
	}

	// 25.1.2  Instructions That Cause VM Exits Unconditionally
	// The following instructions cause VM exits when they are executed in VMX non-root operation: CPUID, GETSEC,
	// INVD, and XSETBV. This is also true of instructions introduced with VMX, which include: INVEPT, INVVPID, 
	// VMCALL, VMCLEAR, VMLAUNCH, VMPTRLD, VMPTRST, VMRESUME, VMXOFF, and VMXON.

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

		Rflags = 0;
		__vmx_vmread(GUEST_RFLAGS, &Rflags);
		__vmx_vmwrite(GUEST_RFLAGS, Rflags | 0x1); // cf=1 indicate vm instructions fail

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

		break;
	}
	case EXIT_REASON_MSR_WRITE:
	{
		EcxReg = GuestRegs->rcx & 0xffffffff;
		HvHandleMsrWrite(GuestRegs);

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

		// Reading guest physical address
		GuestPhysicalAddr = 0;
		__vmx_vmread(GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

		if (!EptHandleEptViolation(ExitQualification, GuestPhysicalAddr))
		{
			LogError("There were errors in handling Ept Violation");
		}

		break;
	}
	case EXIT_REASON_EPT_MISCONFIG:
	{

		GuestPhysicalAddr = 0;
		__vmx_vmread(GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

		EptHandleMisconfiguration(GuestPhysicalAddr);

		break;
	}
	case EXIT_REASON_VMCALL:
	{
		// Check if it's our routines that request the VMCALL our it relates to Hyper-V
		if (GuestRegs->r10 == 0x48564653 && GuestRegs->r11 == 0x564d43414c4c && GuestRegs->r12 == 0x4e4f485950455256)
		{
			// Then we have to manage it as it relates to us
			GuestRegs->rax = VmxVmcallHandler(GuestRegs->rcx, GuestRegs->rdx, GuestRegs->r8, GuestRegs->r9);
		}
		else
		{
			// Otherwise let the top-level hypervisor to manage it
			GuestRegs->rax = AsmHypervVmcall(GuestRegs->rcx, GuestRegs->rdx, GuestRegs->r8);
		}
		break;
	}
	case EXIT_REASON_EXCEPTION_NMI:
	{
		/*

		Exception or non-maskable interrupt (NMI). Either:
			1: Guest software caused an exception and the bit in the exception bitmap associated with exception’s vector was set to 1
			2: An NMI was delivered to the logical processor and the “NMI exiting” VM-execution control was 1.

		VM_EXIT_INTR_INFO shows the exit infromation about event that occured and causes this exit
		Don't forget to read VM_EXIT_INTR_ERROR_CODE in the case of re-injectiong event

		*/

		// read the exit reason
		__vmx_vmread(VM_EXIT_INTR_INFO, &InterruptExit);

		if (InterruptExit.InterruptionType == INTERRUPT_TYPE_SOFTWARE_EXCEPTION && InterruptExit.Vector == EXCEPTION_VECTOR_BREAKPOINT)
		{

			ULONG64 GuestRip;
			// Reading guest's RIP 
			__vmx_vmread(GUEST_RIP, &GuestRip);

			// Send the user
			LogInfo("Breakpoint Hit (Process Id : 0x%x) at : %llx ", PsGetCurrentProcessId(), GuestRip);

			GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

			// re-inject #BP back to the guest
			EventInjectBreakpoint();

		}
		else
		{
			LogError("Not expected event occured");
		}
		break;
	}
	case EXIT_REASON_MONITOR_TRAP_FLAG:
	{
		/* Monitor Trap Flag */
		if (GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint)
		{
			// Restore the previous state
			EptHandleMonitorTrapFlag(GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint);
			// Set it to NULL
			GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint = NULL;
		}
		else
		{
			LogError("Why MTF occured ?!");
		}

		// Redo the instruction 
		GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

		// We don't need MTF anymore
		HvSetMonitorTrapFlag(FALSE);

		break;
	}
	case EXIT_REASON_HLT:
	{
		//__halt();
		break;
	}
	default:
	{
		LogError("Unkown Vmexit, reason : 0x%llx", ExitReason);
		break;
	}
	}

	if (!GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted && GuestState[CurrentProcessorIndex].IncrementRip)
	{
		HvResumeToNextInstruction();
	}

	// Set indicator of Vmx non root mode to false
	GuestState[CurrentProcessorIndex].IsOnVmxRootMode = FALSE;

	if (GuestState[CurrentProcessorIndex].VmxoffState.IsVmxoffExecuted)
	{
		return TRUE;
	}

	return FALSE;
}
