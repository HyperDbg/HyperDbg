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

IMPORT_EXPORT_VMFUNC NTSTATUS
VmFuncVmxVmcall(unsigned long long VmcallNumber,
                unsigned long long OptionalParam1,
                unsigned long long OptionalParam2,
                long long          OptionalParam3);

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

IMPORT_EXPORT_VMFUNC VOID
VmFuncEventInjectPageFaultWithCr2(UINT32 CoreId, UINT64 Address);

IMPORT_EXPORT_VMFUNC VOID
VmFuncUninitializeMemory();

IMPORT_EXPORT_VMFUNC VOID
VmFuncVmxBroadcastInitialize();

IMPORT_EXPORT_VMFUNC VOID
VmFuncVmxBroadcastUninitialize();

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

IMPORT_EXPORT_VMFUNC UINT32
VmFuncVmxCompatibleStrlen(const CHAR * s);

IMPORT_EXPORT_VMFUNC UINT32
VmFuncVmxCompatibleWcslen(const wchar_t * s);

IMPORT_EXPORT_VMFUNC BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId);

IMPORT_EXPORT_VMFUNC BOOLEAN
VmFuncInitVmm(VMM_CALLBACKS * VmmCallbacks);

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

//////////////////////////////////////////////////
//         Memory Management Functions 	   		//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// PTE-related Functions
//

IMPORT_EXPORT_VMFUNC PVOID
MemoryMapperGetPteVa(_In_ PVOID        Va,
                     _In_ PAGING_LEVEL Level);

IMPORT_EXPORT_VMFUNC PVOID
MemoryMapperGetPteVaByCr3(_In_ PVOID        Va,
                          _In_ PAGING_LEVEL Level,
                          _In_ CR3_TYPE     TargetCr3);

IMPORT_EXPORT_VMFUNC PVOID
MemoryMapperGetPteVaWithoutSwitchingByCr3(_In_ PVOID        Va,
                                          _In_ PAGING_LEVEL Level,
                                          _In_ CR3_TYPE     TargetCr3);

// ----------------------------------------------------------------------------
// Reading Memory Functions
//
IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperReadMemorySafe(_In_ UINT64   VaAddressToRead,
                           _Inout_ PVOID BufferToSaveMemory,
                           _In_ SIZE_T   SizeToRead);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(_In_ UINT64    PaAddressToRead,
                                            _Inout_ UINT64 BufferToSaveMemory,
                                            _In_ SIZE_T    SizeToRead);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(_In_ UINT64   VaAddressToRead,
                                          _Inout_ PVOID BufferToSaveMemory,
                                          _In_ SIZE_T   SizeToRead);

// ----------------------------------------------------------------------------
// Writing Memory Functions
//
IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperWriteMemorySafe(_Inout_ UINT64 Destination,
                            _In_ PVOID     Source,
                            _In_ SIZE_T    SizeToWrite,
                            _In_ CR3_TYPE  TargetProcessCr3);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(_Inout_ UINT64 Destination,
                                           _In_ PVOID     Source,
                                           _In_ SIZE_T    Size);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(_Inout_ UINT64 DestinationPa,
                                             _In_ UINT64    Source,
                                             _In_ SIZE_T    SizeToWrite);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperWriteMemoryUnsafe(_Inout_ UINT64 Destination,
                              _In_ PVOID     Source,
                              _In_ SIZE_T    SizeToWrite,
                              _In_ UINT32    TargetProcessId);

// ----------------------------------------------------------------------------
// Reserving Memory Functions
//
IMPORT_EXPORT_VMFUNC UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(_In_ UINT32  ProcessId,
                                                  _In_ BOOLEAN Allocate);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperFreeMemoryInTargetProcess(_In_ UINT32   ProcessId,
                                      _Inout_ PVOID BaseAddress);

// ----------------------------------------------------------------------------
// Miscellaneous Memory Functions
//
IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(_In_ PVOID        Va,
                                                  _In_ BOOLEAN      Set,
                                                  _In_ PAGING_LEVEL Level,
                                                  _In_ CR3_TYPE     TargetCr3);

IMPORT_EXPORT_VMFUNC BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetOnTargetProcess(_In_ PVOID Va);

//////////////////////////////////////////////////
//                 Pool Manager     	   		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMFUNC BOOLEAN
PoolManagerCheckAndPerformAllocationAndDeallocation();

IMPORT_EXPORT_VMFUNC BOOLEAN
PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);

IMPORT_EXPORT_VMFUNC UINT64
PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);

IMPORT_EXPORT_VMFUNC BOOLEAN
PoolManagerFreePool(UINT64 AddressToFree);

//////////////////////////////////////////////////
//          VMX Registers Modification  		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMFUNC VOID
SetGuestCsSel(PVMX_SEGMENT_SELECTOR Cs);

IMPORT_EXPORT_VMFUNC VOID
SetGuestCs(PVMX_SEGMENT_SELECTOR Cs);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestCs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestSsSel(PVMX_SEGMENT_SELECTOR Ss);

IMPORT_EXPORT_VMFUNC VOID
SetGuestSs(PVMX_SEGMENT_SELECTOR Ss);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestSs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestDsSel(PVMX_SEGMENT_SELECTOR Ds);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDs(PVMX_SEGMENT_SELECTOR Ds);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestDs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestFsSel(PVMX_SEGMENT_SELECTOR Fs);

IMPORT_EXPORT_VMFUNC VOID
SetGuestFs(PVMX_SEGMENT_SELECTOR Fs);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestFs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestGsSel(PVMX_SEGMENT_SELECTOR Gs);

IMPORT_EXPORT_VMFUNC VOID
SetGuestGs(PVMX_SEGMENT_SELECTOR Gs);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestGs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestEsSel(PVMX_SEGMENT_SELECTOR Es);

IMPORT_EXPORT_VMFUNC VOID
SetGuestEs(PVMX_SEGMENT_SELECTOR Es);

IMPORT_EXPORT_VMFUNC VMX_SEGMENT_SELECTOR
GetGuestEs();

IMPORT_EXPORT_VMFUNC VOID
SetGuestIdtr(UINT64 Idtr);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestIdtr();

IMPORT_EXPORT_VMFUNC VOID
SetGuestLdtr(UINT64 Ldtr);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestLdtr();

IMPORT_EXPORT_VMFUNC VOID
SetGuestGdtr(UINT64 Gdtr);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestGdtr();

IMPORT_EXPORT_VMFUNC VOID
SetGuestTr(UINT64 Tr);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestTr();

IMPORT_EXPORT_VMFUNC VOID
SetGuestRFlags(UINT64 RFlags);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestRFlags();

IMPORT_EXPORT_VMFUNC VOID
SetGuestRIP(UINT64 RIP);

IMPORT_EXPORT_VMFUNC VOID
SetGuestRSP(UINT64 RSP);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestRIP();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestCr0();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestCr2();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestCr3();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestCr4();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestCr8();

IMPORT_EXPORT_VMFUNC VOID
SetGuestCr0(UINT64 Cr0);

IMPORT_EXPORT_VMFUNC VOID
SetGuestCr2(UINT64 Cr2);

IMPORT_EXPORT_VMFUNC VOID
SetGuestCr3(UINT64 Cr3);

IMPORT_EXPORT_VMFUNC VOID
SetGuestCr4(UINT64 Cr4);

IMPORT_EXPORT_VMFUNC VOID
SetGuestCr8(UINT64 Cr8);

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr0();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr1();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr2();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr3();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr6();

IMPORT_EXPORT_VMFUNC UINT64
GetGuestDr7();

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr0(UINT64 value);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr1(UINT64 value);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr2(UINT64 value);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr3(UINT64 value);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr6(UINT64 value);

IMPORT_EXPORT_VMFUNC VOID
SetGuestDr7(UINT64 value);

IMPORT_EXPORT_VMFUNC BOOLEAN
SetDebugRegisters(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress);

//////////////////////////////////////////////////
//              Transparent Mode        		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMFUNC NTSTATUS
TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements);

IMPORT_EXPORT_VMFUNC NTSTATUS
TransparentUnhideDebugger();

//////////////////////////////////////////////////
//     Non-internal Broadcasting Functions    	//
//////////////////////////////////////////////////

VOID
BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores();

VOID
BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores();

VOID
BroadcastEnableDbAndBpExitingAllCores();

VOID
BroadcastDisableDbAndBpExitingAllCores();

VOID
BroadcastEnableRdtscExitingAllCores();

VOID
BroadcastDisableRdtscExitingAllCores();

VOID
BroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);

VOID
BroadcastResetChangeAllMsrBitmapReadAllCores();

VOID
BroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);

VOID
BroadcastResetAllMsrBitmapWriteAllCores();

VOID
BroadcastDisableRdtscExitingForClearingEventsAllCores();

VOID
BroadcastDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

VOID
BroadcastDisableMov2DebugRegsExitingForClearingEventsAllCores();

VOID
BroadcastEnableRdpmcExitingAllCores();

VOID
BroadcastDisableRdpmcExitingAllCores();

VOID
BroadcastSetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
BroadcastUnsetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
BroadcastResetExceptionBitmapAllCores();

VOID
BroadcastEnableMovControlRegisterExitingAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

VOID
BroadcastDisableMovToControlRegistersExitingAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

VOID
BroadcastEnableMovDebugRegistersExitingAllCores();

VOID
BroadcastDisableMovDebugRegistersExitingAllCores();

VOID
BroadcastSetExternalInterruptExitingAllCores();

VOID
BroadcastUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();

VOID
BroadcastIoBitmapChangeAllCores(UINT64 Port);

VOID
BroadcastIoBitmapResetAllCores();
