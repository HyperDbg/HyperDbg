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
#    define IMPORT_EXPORT_VMM __declspec(dllexport)
#else
#    define IMPORT_EXPORT_VMM __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//                 VM Functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM NTSTATUS
VmFuncVmxVmcall(unsigned long long VmcallNumber,
                unsigned long long OptionalParam1,
                unsigned long long OptionalParam2,
                long long          OptionalParam3);

IMPORT_EXPORT_VMM VOID
VmFuncPerformRipIncrement(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncSuppressRipIncrement(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncChangeMtfUnsettingState(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncChangeIgnoreOneMtfState(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetMonitorTrapFlag(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncRegisterMtfBreak(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncUnRegisterMtfBreak(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncSetLoadDebugControls(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetSaveDebugControls(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetPmcVmexit(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

IMPORT_EXPORT_VMM VOID
VmFuncSetMovToCr3Vmexit(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncWriteExceptionBitmap(UINT32 BitmapMask);

IMPORT_EXPORT_VMM VOID
VmFuncSetInterruptWindowExiting(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetNmiWindowExiting(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetNmiExiting(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

IMPORT_EXPORT_VMM VOID
VmFuncUnsetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

IMPORT_EXPORT_VMM VOID
VmFuncSetExternalInterruptExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetRdtscExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetMovDebugRegsExiting(UINT32 CoreId, BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncInjectPendingExternalInterrupts(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncSetRflags(UINT64 Rflags);

IMPORT_EXPORT_VMM VOID
VmFuncSetRip(UINT64 Rip);

IMPORT_EXPORT_VMM VOID
VmFuncSetTriggerEventForVmcalls(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetTriggerEventForCpuids(BOOLEAN Set);

IMPORT_EXPORT_VMM VOID
VmFuncSetInterruptibilityState(UINT64 InterruptibilityState);

IMPORT_EXPORT_VMM VOID
VmFuncCheckAndEnableExternalInterrupts(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncDisableExternalInterruptsAndInterruptWindow(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncEventInjectPageFaultWithCr2(UINT32 CoreId, UINT64 Address);

IMPORT_EXPORT_VMM VOID
VmFuncVmxBroadcastInitialize();

IMPORT_EXPORT_VMM VOID
VmFuncVmxBroadcastUninitialize();

IMPORT_EXPORT_VMM VOID
VmFuncEventInjectBreakpoint();

IMPORT_EXPORT_VMM VOID
VmFuncEptHookAllocateExtraHookingPages(UINT32 Count);

IMPORT_EXPORT_VMM VOID
VmFuncUninitVmm();

IMPORT_EXPORT_VMM UINT16
VmFuncGetCsSelector();

IMPORT_EXPORT_VMM UINT32
VmFuncReadExceptionBitmap();

IMPORT_EXPORT_VMM UINT64
VmFuncGetLastVmexitRip(UINT32 CoreId);

IMPORT_EXPORT_VMM UINT64
VmFuncGetRflags();

IMPORT_EXPORT_VMM UINT64
VmFuncGetRip();

IMPORT_EXPORT_VMM UINT64
VmFuncGetInterruptibilityState();

IMPORT_EXPORT_VMM UINT32
VmFuncClearSteppingBits(UINT32 Interruptibility);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncInitVmm(VMM_CALLBACKS * VmmCallbacks);

IMPORT_EXPORT_VMM UINT32
VmFuncVmxCompatibleStrlen(const CHAR * s);

IMPORT_EXPORT_VMM UINT32
VmFuncVmxCompatibleWcslen(const wchar_t * s);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncVmxGetCurrentExecutionMode();

//////////////////////////////////////////////////
//            Configuration Functions 	   		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM VOID
ConfigureEnableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureDisableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureEnableEferSyscallEventsOnAllProcessors(DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType);

IMPORT_EXPORT_VMM VOID
ConfigureDisableEferSyscallEventsOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureSetExternalInterruptExitingOnSingleCore(UINT32 TargetCoreId);

IMPORT_EXPORT_VMM VOID
ConfigureEnableRdtscExitingOnSingleCore(UINT32 TargetCoreId);

IMPORT_EXPORT_VMM VOID
ConfigureEnableRdpmcExitingOnSingleCore(UINT32 TargetCoreId);

IMPORT_EXPORT_VMM VOID
ConfigureEnableMovToDebugRegistersExitingOnSingleCore(UINT32 TargetCoreId);

IMPORT_EXPORT_VMM VOID
ConfigureSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT32 BitMask);

IMPORT_EXPORT_VMM VOID
ConfigureEnableMovToControlRegisterExitingOnSingleCore(UINT32 TargetCoreId, DEBUGGER_BROADCASTING_OPTIONS * BroadcastingOption);

IMPORT_EXPORT_VMM VOID
ConfigureChangeMsrBitmapWriteOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask);

IMPORT_EXPORT_VMM VOID
ConfigureChangeMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask);

IMPORT_EXPORT_VMM VOID
ConfigureChangeIoBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 Port);

IMPORT_EXPORT_VMM VOID
ConfigureEnableEferSyscallHookOnSingleCore(UINT32 TargetCoreId, DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHook(PVOID TargetAddress, UINT32 ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHook2(PVOID TargetAddress, PVOID HookFunction, UINT32 ProcessId, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookUnHookSingleAddress(UINT64 VirtualAddress, UINT64 PhysAddress, UINT32 ProcessId);

//////////////////////////////////////////////////
//                General Functions 	   		//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Exported Interfaces For Virtual Addresses
//

IMPORT_EXPORT_VMM UINT64
VirtualAddressToPhysicalAddress(_In_ PVOID VirtualAddress);

IMPORT_EXPORT_VMM UINT64
VirtualAddressToPhysicalAddressByProcessId(_In_ PVOID  VirtualAddress,
                                           _In_ UINT32 ProcessId);

IMPORT_EXPORT_VMM UINT64
VirtualAddressToPhysicalAddressByProcessCr3(_In_ PVOID    VirtualAddress,
                                            _In_ CR3_TYPE TargetCr3);

IMPORT_EXPORT_VMM UINT64
VirtualAddressToPhysicalAddressOnTargetProcess(_In_ PVOID VirtualAddress);

// ----------------------------------------------------------------------------
// Exported Interfaces For Physical Addresses
//
IMPORT_EXPORT_VMM UINT64
PhysicalAddressToVirtualAddress(_In_ UINT64 PhysicalAddress);

IMPORT_EXPORT_VMM UINT64
PhysicalAddressToVirtualAddressByProcessId(_In_ PVOID PhysicalAddress, _In_ UINT32 ProcessId);

IMPORT_EXPORT_VMM UINT64
PhysicalAddressToVirtualAddressByCr3(_In_ PVOID PhysicalAddress, _In_ CR3_TYPE TargetCr3);

IMPORT_EXPORT_VMM UINT64
PhysicalAddressToVirtualAddressOnTargetProcess(_In_ PVOID PhysicalAddress);

// ----------------------------------------------------------------------------
// Exported Interfaces For Layout Switching Functions
//
IMPORT_EXPORT_VMM CR3_TYPE
SwitchOnAnotherProcessMemoryLayout(_In_ UINT32 ProcessId);

IMPORT_EXPORT_VMM CR3_TYPE
SwitchOnMemoryLayoutOfTargetProcess();

IMPORT_EXPORT_VMM CR3_TYPE
SwitchOnAnotherProcessMemoryLayoutByCr3(_In_ CR3_TYPE TargetCr3);

// ----------------------------------------------------------------------------
// Exported Interfaces For Check Validity of Addresses
//
IMPORT_EXPORT_VMM BOOLEAN
CheckIfAddressIsValidUsingTsx(CHAR * Address);

IMPORT_EXPORT_VMM BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

// ----------------------------------------------------------------------------
// Exported Interfaces For Miscellaneous Functions
//
IMPORT_EXPORT_VMM CR3_TYPE
GetRunningCr3OnTargetProcess();

IMPORT_EXPORT_VMM VOID
RestoreToPreviousProcess(_In_ CR3_TYPE PreviousProcess);

//////////////////////////////////////////////////
//         Memory Management Functions 	   		//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// PTE-related Functions
//

IMPORT_EXPORT_VMM PVOID
MemoryMapperGetPteVa(_In_ PVOID        Va,
                     _In_ PAGING_LEVEL Level);

IMPORT_EXPORT_VMM PVOID
MemoryMapperGetPteVaByCr3(_In_ PVOID        Va,
                          _In_ PAGING_LEVEL Level,
                          _In_ CR3_TYPE     TargetCr3);

IMPORT_EXPORT_VMM PVOID
MemoryMapperGetPteVaWithoutSwitchingByCr3(_In_ PVOID        Va,
                                          _In_ PAGING_LEVEL Level,
                                          _In_ CR3_TYPE     TargetCr3);

// ----------------------------------------------------------------------------
// Reading Memory Functions
//
IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperReadMemorySafe(_In_ UINT64   VaAddressToRead,
                           _Inout_ PVOID BufferToSaveMemory,
                           _In_ SIZE_T   SizeToRead);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(_In_ UINT64    PaAddressToRead,
                                            _Inout_ UINT64 BufferToSaveMemory,
                                            _In_ SIZE_T    SizeToRead);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(_In_ UINT64   VaAddressToRead,
                                          _Inout_ PVOID BufferToSaveMemory,
                                          _In_ SIZE_T   SizeToRead);

// ----------------------------------------------------------------------------
// Writing Memory Functions
//
IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperWriteMemorySafe(_Inout_ UINT64 Destination,
                            _In_ PVOID     Source,
                            _In_ SIZE_T    SizeToWrite,
                            _In_ CR3_TYPE  TargetProcessCr3);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(_Inout_ UINT64 Destination,
                                           _In_ PVOID     Source,
                                           _In_ SIZE_T    Size);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(_Inout_ UINT64 DestinationPa,
                                             _In_ UINT64    Source,
                                             _In_ SIZE_T    SizeToWrite);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperWriteMemoryUnsafe(_Inout_ UINT64 Destination,
                              _In_ PVOID     Source,
                              _In_ SIZE_T    SizeToWrite,
                              _In_ UINT32    TargetProcessId);

// ----------------------------------------------------------------------------
// Reserving Memory Functions
//
IMPORT_EXPORT_VMM UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(_In_ UINT32  ProcessId,
                                                  _In_ BOOLEAN Allocate);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperFreeMemoryInTargetProcess(_In_ UINT32   ProcessId,
                                      _Inout_ PVOID BaseAddress);

// ----------------------------------------------------------------------------
// Miscellaneous Memory Functions
//
IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(_In_ PVOID        Va,
                                                  _In_ BOOLEAN      Set,
                                                  _In_ PAGING_LEVEL Level,
                                                  _In_ CR3_TYPE     TargetCr3);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetOnTargetProcess(_In_ PVOID Va);

//////////////////////////////////////////////////
//				Memory Manager		    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM BOOLEAN
MemoryManagerReadProcessMemoryNormal(HANDLE PID, PVOID Address, DEBUGGER_READ_MEMORY_TYPE MemType, PVOID UserBuffer, SIZE_T Size, PSIZE_T ReturnSize);

//////////////////////////////////////////////////
//                 Pool Manager     	   		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM BOOLEAN
PoolManagerCheckAndPerformAllocationAndDeallocation();

IMPORT_EXPORT_VMM BOOLEAN
PoolManagerRequestAllocation(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);

IMPORT_EXPORT_VMM UINT64
PoolManagerRequestPool(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);

IMPORT_EXPORT_VMM BOOLEAN
PoolManagerFreePool(UINT64 AddressToFree);

//////////////////////////////////////////////////
//          VMX Registers Modification  		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM VOID
SetGuestCsSel(PVMX_SEGMENT_SELECTOR Cs);

IMPORT_EXPORT_VMM VOID
SetGuestCs(PVMX_SEGMENT_SELECTOR Cs);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestCs();

IMPORT_EXPORT_VMM VOID
SetGuestSsSel(PVMX_SEGMENT_SELECTOR Ss);

IMPORT_EXPORT_VMM VOID
SetGuestSs(PVMX_SEGMENT_SELECTOR Ss);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestSs();

IMPORT_EXPORT_VMM VOID
SetGuestDsSel(PVMX_SEGMENT_SELECTOR Ds);

IMPORT_EXPORT_VMM VOID
SetGuestDs(PVMX_SEGMENT_SELECTOR Ds);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestDs();

IMPORT_EXPORT_VMM VOID
SetGuestFsSel(PVMX_SEGMENT_SELECTOR Fs);

IMPORT_EXPORT_VMM VOID
SetGuestFs(PVMX_SEGMENT_SELECTOR Fs);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestFs();

IMPORT_EXPORT_VMM VOID
SetGuestGsSel(PVMX_SEGMENT_SELECTOR Gs);

IMPORT_EXPORT_VMM VOID
SetGuestGs(PVMX_SEGMENT_SELECTOR Gs);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestGs();

IMPORT_EXPORT_VMM VOID
SetGuestEsSel(PVMX_SEGMENT_SELECTOR Es);

IMPORT_EXPORT_VMM VOID
SetGuestEs(PVMX_SEGMENT_SELECTOR Es);

IMPORT_EXPORT_VMM VMX_SEGMENT_SELECTOR
GetGuestEs();

IMPORT_EXPORT_VMM VOID
SetGuestIdtr(UINT64 Idtr);

IMPORT_EXPORT_VMM UINT64
GetGuestIdtr();

IMPORT_EXPORT_VMM VOID
SetGuestLdtr(UINT64 Ldtr);

IMPORT_EXPORT_VMM UINT64
GetGuestLdtr();

IMPORT_EXPORT_VMM VOID
SetGuestGdtr(UINT64 Gdtr);

IMPORT_EXPORT_VMM UINT64
GetGuestGdtr();

IMPORT_EXPORT_VMM VOID
SetGuestTr(UINT64 Tr);

IMPORT_EXPORT_VMM UINT64
GetGuestTr();

IMPORT_EXPORT_VMM VOID
SetGuestRFlags(UINT64 RFlags);

IMPORT_EXPORT_VMM UINT64
GetGuestRFlags();

IMPORT_EXPORT_VMM VOID
SetGuestRIP(UINT64 RIP);

IMPORT_EXPORT_VMM VOID
SetGuestRSP(UINT64 RSP);

IMPORT_EXPORT_VMM UINT64
GetGuestRIP();

IMPORT_EXPORT_VMM UINT64
GetGuestCr0();

IMPORT_EXPORT_VMM UINT64
GetGuestCr2();

IMPORT_EXPORT_VMM UINT64
GetGuestCr3();

IMPORT_EXPORT_VMM UINT64
GetGuestCr4();

IMPORT_EXPORT_VMM UINT64
GetGuestCr8();

IMPORT_EXPORT_VMM VOID
SetGuestCr0(UINT64 Cr0);

IMPORT_EXPORT_VMM VOID
SetGuestCr2(UINT64 Cr2);

IMPORT_EXPORT_VMM VOID
SetGuestCr3(UINT64 Cr3);

IMPORT_EXPORT_VMM VOID
SetGuestCr4(UINT64 Cr4);

IMPORT_EXPORT_VMM VOID
SetGuestCr8(UINT64 Cr8);

IMPORT_EXPORT_VMM UINT64
GetGuestDr0();

IMPORT_EXPORT_VMM UINT64
GetGuestDr1();

IMPORT_EXPORT_VMM UINT64
GetGuestDr2();

IMPORT_EXPORT_VMM UINT64
GetGuestDr3();

IMPORT_EXPORT_VMM UINT64
GetGuestDr6();

IMPORT_EXPORT_VMM UINT64
GetGuestDr7();

IMPORT_EXPORT_VMM VOID
SetGuestDr0(UINT64 value);

IMPORT_EXPORT_VMM VOID
SetGuestDr1(UINT64 value);

IMPORT_EXPORT_VMM VOID
SetGuestDr2(UINT64 value);

IMPORT_EXPORT_VMM VOID
SetGuestDr3(UINT64 value);

IMPORT_EXPORT_VMM VOID
SetGuestDr6(UINT64 value);

IMPORT_EXPORT_VMM VOID
SetGuestDr7(UINT64 value);

IMPORT_EXPORT_VMM BOOLEAN
SetDebugRegisters(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress);

//////////////////////////////////////////////////
//              Transparent Mode        		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM NTSTATUS
TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements);

IMPORT_EXPORT_VMM NTSTATUS
TransparentUnhideDebugger();

//////////////////////////////////////////////////
//     Non-internal Broadcasting Functions    	//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM VOID
BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastEnableDbAndBpExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableDbAndBpExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastEnableRdtscExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableRdtscExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);

IMPORT_EXPORT_VMM VOID
BroadcastResetChangeAllMsrBitmapReadAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);

IMPORT_EXPORT_VMM VOID
BroadcastResetAllMsrBitmapWriteAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableRdtscExitingForClearingEventsAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

IMPORT_EXPORT_VMM VOID
BroadcastDisableMov2DebugRegsExitingForClearingEventsAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastEnableRdpmcExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableRdpmcExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastSetExceptionBitmapAllCores(UINT64 ExceptionIndex);

IMPORT_EXPORT_VMM VOID
BroadcastUnsetExceptionBitmapAllCores(UINT64 ExceptionIndex);

IMPORT_EXPORT_VMM VOID
BroadcastResetExceptionBitmapAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastEnableMovControlRegisterExitingAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

IMPORT_EXPORT_VMM VOID
BroadcastDisableMovToControlRegistersExitingAllCores(PDEBUGGER_BROADCASTING_OPTIONS BroadcastingOption);

IMPORT_EXPORT_VMM VOID
BroadcastEnableMovDebugRegistersExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastDisableMovDebugRegistersExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastSetExternalInterruptExitingAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastIoBitmapChangeAllCores(UINT64 Port);

IMPORT_EXPORT_VMM VOID
BroadcastIoBitmapResetAllCores();

IMPORT_EXPORT_VMM VOID
BroadcastEnableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
BroadcastDisableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
BroadcastEnableEferSyscallEventsOnAllProcessors();

IMPORT_EXPORT_VMM VOID
BroadcastDisableEferSyscallEventsOnAllProcessors();
