/**
 * @file HyperDbgVmmImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hypervisor
 * @version 0.1
 * @date 2022-12-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_VMM
#    define IMPORT_EXPORT_VMFUNC __declspec(dllexport)
#else
#    define IMPORT_EXPORT_VMFUNC __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//                 VM Functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMFUNC VOID
VmFuncPerformRipIncrement(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSuppressRipIncrement(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncChangeMtfUnsettingState(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncChangeIgnoreOneMtfState(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetMonitorTrapFlag(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncRegisterMtfBreak(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncUnRegisterMtfBreak(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetLoadDebugControls(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetSaveDebugControls(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetPmcVmexit(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetMovToCr3Vmexit(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncWriteExceptionBitmap(UINT32 BitmapMask);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetInterruptWindowExiting(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetNmiWindowExiting(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetNmiExiting(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

IMPORT_EXPORT_VMFUNC VOID
VmFuncUnsetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetExternalInterruptExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetRdtscExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetMovDebugRegsExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncInjectPendingExternalInterrupts(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetRflags(UINT64 Rflags);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetRip(UINT64 Rip);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetTriggerEventForVmcalls(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetTriggerEventForCpuids(BOOLEAN Set);

IMPORT_EXPORT_VMFUNC VOID
VmFuncSetInterruptibilityState(UINT64 InterruptibilityState);

IMPORT_EXPORT_VMFUNC VOID
VmFuncCheckAndEnableExternalInterrupts(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC VOID
VmFuncDisableExternalInterruptsAndInterruptWindow(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC UINT16
VmFuncGetCsSelector();

IMPORT_EXPORT_VMFUNC UINT32
VmFuncReadExceptionBitmap();

IMPORT_EXPORT_VMFUNC UINT64
VmFuncGetLastVmexitRip(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC UINT64
VmFuncGetRflags();

IMPORT_EXPORT_VMFUNC UINT64
VmFuncGetRip();

IMPORT_EXPORT_VMFUNC UINT64
VmFuncGetInterruptibilityState();

IMPORT_EXPORT_VMFUNC BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC BOOLEAN
VmFuncInitVmm(VMM_CALLBACKS * VmmCallbacks);

IMPORT_EXPORT_VMFUNC VOID
VmFuncUninitializeMemory();

IMPORT_EXPORT_VMFUNC BOOLEAN
VmFuncVmxGetCurrentExecutionMode();

//////////////////////////////////////////////////
//                General Functions 	   		//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Exported Interfaces For Virtual Addresses
//

IMPORT_EXPORT_VMFUNC UINT64
VirtualAddressToPhysicalAddress(_In_ PVOID VirtualAddress);

IMPORT_EXPORT_VMFUNC UINT64
VirtualAddressToPhysicalAddressByProcessId(_In_ PVOID  VirtualAddress,
                                           _In_ UINT32 ProcessId);

IMPORT_EXPORT_VMFUNC UINT64
VirtualAddressToPhysicalAddressByProcessCr3(_In_ PVOID    VirtualAddress,
                                            _In_ CR3_TYPE TargetCr3);

IMPORT_EXPORT_VMFUNC UINT64
VirtualAddressToPhysicalAddressOnTargetProcess(_In_ PVOID VirtualAddress);

// ----------------------------------------------------------------------------
// Exported Interfaces For Physical Addresses
//
IMPORT_EXPORT_VMFUNC UINT64
PhysicalAddressToVirtualAddress(_In_ UINT64 PhysicalAddress);

IMPORT_EXPORT_VMFUNC UINT64
PhysicalAddressToVirtualAddressByProcessId(_In_ PVOID PhysicalAddress, _In_ UINT32 ProcessId);

IMPORT_EXPORT_VMFUNC UINT64
PhysicalAddressToVirtualAddressByCr3(_In_ PVOID PhysicalAddress, _In_ CR3_TYPE TargetCr3);

IMPORT_EXPORT_VMFUNC UINT64
PhysicalAddressToVirtualAddressOnTargetProcess(_In_ PVOID PhysicalAddress);

// ----------------------------------------------------------------------------
// Exported Interfaces For Layout Switching Functions
//
IMPORT_EXPORT_VMFUNC CR3_TYPE
SwitchOnAnotherProcessMemoryLayout(_In_ UINT32 ProcessId);

IMPORT_EXPORT_VMFUNC CR3_TYPE
SwitchOnMemoryLayoutOfTargetProcess();

IMPORT_EXPORT_VMFUNC CR3_TYPE
SwitchOnAnotherProcessMemoryLayoutByCr3(_In_ CR3_TYPE TargetCr3);

// ----------------------------------------------------------------------------
// Exported Interfaces For Check Validity of Addresses
//
IMPORT_EXPORT_VMFUNC BOOLEAN
CheckIfAddressIsValidUsingTsx(CHAR * Address);

IMPORT_EXPORT_VMFUNC BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

// ----------------------------------------------------------------------------
// Exported Interfaces For Miscellaneous Functions
//
IMPORT_EXPORT_VMFUNC CR3_TYPE
GetRunningCr3OnTargetProcess();

IMPORT_EXPORT_VMFUNC VOID
RestoreToPreviousProcess(_In_ CR3_TYPE PreviousProcess);
