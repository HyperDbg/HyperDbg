#include <ntddk.h>
#include <wdf.h>
#include "Msr.h"
#include "Common.h"
#include "Vmx.h"

/* Power function in order to computer address for MSR bitmaps */
int MathPower(int Base, int Exp) {

	int result;

	result = 1;

	for (;;)
	{
		if (Exp & 1)
		{
			result *= Base;
		}
		Exp >>= 1;
		if (!Exp)
		{
			break;
		}
		Base *= Base;
	}
	return result;
}

// This function is deprecated as we want to supporrt more than 32 processors.
/* Broadcast a function to all logical cores */
BOOLEAN BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine)
{

	KIRQL OldIrql;

	KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

	OldIrql = KeRaiseIrqlToDpcLevel();

	Routine(ProcessorNumber);

	KeLowerIrql(OldIrql);

	KeRevertToUserAffinityThread();

	return TRUE;
}

// Note : Because of deadlock and synchronization problem, no longer use this instead use Vmcall with VMCALL_VMXOFF
/* Broadcast a 0x41414141 - 0x42424242 message to CPUID Handler of all logical cores in order to turn off VMX in VMX root-mode */
/*BOOLEAN BroadcastToProcessorsToTerminateVmx(ULONG ProcessorNumber)
{
	KIRQL OldIrql;

	KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

	OldIrql = KeRaiseIrqlToDpcLevel();

	INT32 cpu_info[4];
	__cpuidex(cpu_info, 0x41414141, 0x42424242);

	KeLowerIrql(OldIrql);

	KeRevertToUserAffinityThread();

	return TRUE;
}
*/

/* Set Bits for a special address (used on MSR Bitmaps) */
void SetBit(PVOID Addr, UINT64 bit, BOOLEAN Set) {

	UINT64 byte;
	UINT64 temp;
	UINT64 n;
	BYTE* Addr2;

	byte = bit / 8;
	temp = bit % 8;
	n = 7 - temp;

	Addr2 = Addr;

	if (Set)
	{
		Addr2[byte] |= (1 << n);
	}
	else
	{
		Addr2[byte] &= ~(1 << n);
	}
}

/* Get Bits of a special address (used on MSR Bitmaps) */
void GetBit(PVOID Addr, UINT64 bit) {

	UINT64 byte, k;
	BYTE* Addr2;

	byte = 0;
	k = 0;
	byte = bit / 8;
	k = 7 - bit % 8;

	Addr2 = Addr;

	return Addr2[byte] & (1 << k);
}

/* Converts Virtual Address to Physical Address */
UINT64 VirtualAddressToPhysicalAddress(PVOID VirtualAddress)
{
	return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/* Converts Physical Address to Virtual Address */
UINT64 PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress)
{
	PHYSICAL_ADDRESS PhysicalAddr;
	PhysicalAddr.QuadPart = PhysicalAddress;

	return MmGetVirtualForPhysical(PhysicalAddr);
}

