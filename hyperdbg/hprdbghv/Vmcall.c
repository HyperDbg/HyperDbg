#include "Vmcall.h"
#include "GlobalVariables.h"
#include "Common.h"
#include "Invept.h"

/* Main Vmcall Handler */
NTSTATUS VmxVmcallHandler(UINT64 VmcallNumber, UINT64 OptionalParam1, UINT64 OptionalParam2, UINT64 OptionalParam3)
{
	NTSTATUS VmcallStatus;
	BOOLEAN HookResult;
	BOOLEAN UnsetExec, UnsetWrite, UnsetRead;

	VmcallStatus = STATUS_UNSUCCESSFUL;

	// Only 32bit of Vmcall is valid, this way we can use the upper 32 bit of the Vmcall
	switch (VmcallNumber & 0xffffffff)
	{
	case VMCALL_TEST:
	{
		VmcallStatus = VmcallTest(OptionalParam1, OptionalParam2, OptionalParam3);
		break;
	}
	case VMCALL_VMXOFF:
	{
		VmxVmxoff();
		VmcallStatus = STATUS_SUCCESS;
		break;
	}
	// Mask is the upper 32 bits to this Vmcall
	case VMCALL_CHANGE_PAGE_ATTRIB:
	{
		// Upper 32 bits of the Vmcall contains the attribute mask
		UINT32 AttributeMask = (UINT32)((VmcallNumber & 0xFFFFFFFF00000000LL) >> 32);;

		UnsetExec = UnsetWrite = UnsetRead = FALSE;

		if (AttributeMask & PAGE_ATTRIB_READ)
		{
			UnsetRead = TRUE;
		}
		if (AttributeMask & PAGE_ATTRIB_WRITE)
		{
			UnsetWrite = TRUE;
		}
		if (AttributeMask & PAGE_ATTRIB_EXEC)
		{
			UnsetExec = TRUE;
		}
		HookResult = EptPerformPageHook(OptionalParam1 /* TargetAddress */, OptionalParam2 /* Hook Function*/,
			OptionalParam3 /* OrigFunction */, UnsetRead, UnsetWrite, UnsetExec);

		if (HookResult)
		{
			VmcallStatus = STATUS_SUCCESS;
		}
		else
		{
			VmcallStatus = STATUS_UNSUCCESSFUL;
		}
		break;
	}
	case VMCALL_INVEPT_SINGLE_CONTEXT:
	{
		InveptSingleContext(OptionalParam1);
		VmcallStatus = STATUS_SUCCESS;
		break;
	}
	case VMCALL_INVEPT_ALL_CONTEXTS:
	{
		InveptAllContexts();
		VmcallStatus = STATUS_SUCCESS;
		break;
	}	
	case VMCALL_UNHOOK_ALL_PAGES:
	{
		EptPageUnHookAllPages();
		VmcallStatus = STATUS_SUCCESS;
		break;
	}
	case VMCALL_UNHOOK_SINGLE_PAGE:
	{
		if (!EptPageUnHookSinglePage(OptionalParam1))
		{
			VmcallStatus = STATUS_UNSUCCESSFUL;
		}
		break;
	}
	default:
	{
		LogError("Unsupported VMCALL");
		VmcallStatus = STATUS_UNSUCCESSFUL;
		break;
	}

	}
	return VmcallStatus;
}

/* Test Vmcall (VMCALL_TEST) */
NTSTATUS VmcallTest(UINT64 Param1, UINT64 Param2, UINT64 Param3) {

	LogInfo("VmcallTest called with @Param1 = 0x%llx , @Param2 = 0x%llx , @Param3 = 0x%llx", Param1, Param2, Param3);
	return STATUS_SUCCESS;
}
