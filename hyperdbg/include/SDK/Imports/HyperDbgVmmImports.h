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
                unsigned long long OptionalParam3);

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
VmFuncSetRflagTrapFlag(BOOLEAN Set);

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
VmFuncEventInjectPageFaultWithCr2(UINT32 CoreId, UINT64 Address, UINT32 PageFaultCode);

IMPORT_EXPORT_VMM VOID
VmFuncEventInjectPageFaultRangeAddress(UINT32 CoreId,
                                       UINT64 AddressFrom,
                                       UINT64 AddressTo,
                                       UINT32 PageFaultCode);

IMPORT_EXPORT_VMM VOID
VmFuncEventInjectInterruption(UINT32  InterruptionType,
                              UINT32  Vector,
                              BOOLEAN DeliverErrorCode,
                              UINT32  ErrorCode);

IMPORT_EXPORT_VMM VOID
VmFuncVmxBroadcastInitialize();

IMPORT_EXPORT_VMM VOID
VmFuncVmxBroadcastUninitialize();

IMPORT_EXPORT_VMM VOID
VmFuncEventInjectBreakpoint();

IMPORT_EXPORT_VMM VOID
VmFuncInvalidateEptSingleContext(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncInvalidateEptAllContexts();

IMPORT_EXPORT_VMM VOID
VmFuncUninitVmm();

IMPORT_EXPORT_VMM VOID
VmFuncEnableMtfAndChangeExternalInterruptState(UINT32 CoreId);

IMPORT_EXPORT_VMM VOID
VmFuncEnableAndCheckForPreviousExternalInterrupts(UINT32 CoreId);

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

IMPORT_EXPORT_VMM UINT64
VmFuncClearSteppingBits(UINT64 Interruptibility);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncInitVmm(VMM_CALLBACKS * VmmCallbacks);

IMPORT_EXPORT_VMM UINT32
VmFuncVmxCompatibleStrlen(const CHAR * s);

IMPORT_EXPORT_VMM UINT32
VmFuncVmxCompatibleWcslen(const wchar_t * s);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncNmiBroadcastRequest(UINT32 CoreId);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncNmiBroadcastInvalidateEptSingleContext(UINT32 CoreId);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncNmiBroadcastInvalidateEptAllContexts(UINT32 CoreId);

IMPORT_EXPORT_VMM BOOLEAN
VmFuncVmxGetCurrentExecutionMode();

IMPORT_EXPORT_VMM BOOLEAN
VmFuncQueryModeExecTrap();

IMPORT_EXPORT_VMM INT32
VmFuncVmxCompatibleStrcmp(const CHAR * Address1, const CHAR * Address2);

IMPORT_EXPORT_VMM INT32
VmFuncVmxCompatibleStrncmp(const CHAR * Address1, const CHAR * Address2, SIZE_T Num);

IMPORT_EXPORT_VMM INT32
VmFuncVmxCompatibleWcscmp(const wchar_t * Address1, const wchar_t * Address2);

IMPORT_EXPORT_VMM INT32
VmFuncVmxCompatibleWcsncmp(const wchar_t * Address1, const wchar_t * Address2, SIZE_T Num);

IMPORT_EXPORT_VMM INT32
VmFuncVmxCompatibleMemcmp(const CHAR * Address1, const CHAR * Address2, size_t Count);

//////////////////////////////////////////////////
//            Configuration Functions 	   		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM VOID
ConfigureEnableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureDisableMovToCr3ExitingOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureEnableEferSyscallEventsOnAllProcessors();

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
ConfigureEnableMovToControlRegisterExitingOnSingleCore(UINT32 TargetCoreId, DEBUGGER_EVENT_OPTIONS * BroadcastingOption);

IMPORT_EXPORT_VMM VOID
ConfigureChangeMsrBitmapWriteOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask);

IMPORT_EXPORT_VMM VOID
ConfigureChangeMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 MsrMask);

IMPORT_EXPORT_VMM VOID
ConfigureChangeIoBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 Port);

IMPORT_EXPORT_VMM VOID
ConfigureEnableEferSyscallHookOnSingleCore(UINT32 TargetCoreId);

IMPORT_EXPORT_VMM VOID
ConfigureSetEferSyscallOrSysretHookType(DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType);

IMPORT_EXPORT_VMM VOID
ConfigureDirtyLoggingInitializeOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureDirtyLoggingUninitializeOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureModeBasedExecHookUninitializeOnAllProcessors();

IMPORT_EXPORT_VMM VOID
ConfigureUninitializeExecTrapOnAllProcessors();

IMPORT_EXPORT_VMM BOOLEAN
ConfigureInitializeExecTrapOnAllProcessors();

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHook(PVOID TargetAddress, UINT32 ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookFromVmxRoot(PVOID TargetAddress);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHook2(UINT32 CoreId,
                  PVOID  TargetAddress,
                  PVOID  HookFunction,
                  UINT32 ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHook2FromVmxRoot(UINT32 CoreId,
                             PVOID  TargetAddress,
                             PVOID  HookFunction);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookMonitor(UINT32                                         CoreId,
                        EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * HookingDetails,
                        UINT32                                         ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookMonitorFromVmxRoot(UINT32                                         CoreId,
                                   EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * MemoryAddressDetails);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookModifyInstructionFetchState(UINT32  CoreId,
                                            PVOID   PhysicalAddress,
                                            BOOLEAN IsUnset);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookModifyPageReadState(UINT32  CoreId,
                                    PVOID   PhysicalAddress,
                                    BOOLEAN IsUnset);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookModifyPageWriteState(UINT32  CoreId,
                                     PVOID   PhysicalAddress,
                                     BOOLEAN IsUnset);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookUnHookSingleAddress(UINT64 VirtualAddress,
                                    UINT64 PhysAddress,
                                    UINT32 ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureEptHookUnHookSingleAddressFromVmxRoot(UINT64                              VirtualAddress,
                                               UINT64                              PhysAddress,
                                               EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails);

IMPORT_EXPORT_VMM VOID
ConfigureEptHookAllocateExtraHookingPagesForMemoryMonitorsAndExecEptHooks(UINT32 Count);

IMPORT_EXPORT_VMM VOID
ConfigureEptHookReservePreallocatedPoolsForEptHooks(UINT32 Count);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureExecTrapAddProcessToWatchingList(UINT32 ProcessId);

IMPORT_EXPORT_VMM BOOLEAN
ConfigureExecTrapRemoveProcessFromWatchingList(UINT32 ProcessId);

//////////////////////////////////////////////////
//           Direct VMCALL Functions 	   		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallTest(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallPerformVmcall(UINT32 CoreId, UINT64 VmcallNumber, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallChangeMsrBitmapRead(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallChangeMsrBitmapWrite(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallChangeIoBitmap(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableRdpmcExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableRdtscpExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableMov2DebugRegsExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallSetExceptionBitmap(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableExternalInterruptExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableMovToCrExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallEnableEferSyscall(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallSetHiddenBreakpointHook(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallInvalidateEptAllContexts(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallInvalidateSingleContext(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallUnsetExceptionBitmap(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallUnhookSinglePage(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallSetDisableExternalInterruptExitingOnlyOnClearingInterruptEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallResetMsrBitmapRead(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallResetMsrBitmapWrite(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallResetExceptionBitmapOnlyOnClearingExceptionEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallResetIoBitmap(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallDisableRdtscExitingForClearingTscEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallDisableRdpmcExiting(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallDisableEferSyscallEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallDisableMov2DrExitingForClearingDrEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

IMPORT_EXPORT_VMM NTSTATUS
DirectVmcallDisableMov2CrExitingForClearingCrEvents(UINT32 CoreId, DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions);

//////////////////////////////////////////////////
//                 Disassembler 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM BOOLEAN
DisassemblerShowInstructionsInVmxNonRootMode(PVOID Address, UINT32 Length, BOOLEAN Is32Bit);

IMPORT_EXPORT_VMM BOOLEAN
DisassemblerShowOneInstructionInVmxNonRootMode(PVOID Address, UINT64 ActualRip, BOOLEAN Is32Bit);

IMPORT_EXPORT_VMM UINT32
DisassemblerShowOneInstructionInVmxRootMode(PVOID Address, BOOLEAN Is32Bit);

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
SwitchToProcessMemoryLayout(_In_ UINT32 ProcessId);

IMPORT_EXPORT_VMM CR3_TYPE
SwitchToCurrentProcessMemoryLayout();

IMPORT_EXPORT_VMM CR3_TYPE
SwitchToProcessMemoryLayoutByCr3(_In_ CR3_TYPE TargetCr3);

IMPORT_EXPORT_VMM VOID
SwitchToPreviousProcess(_In_ CR3_TYPE PreviousProcess);

// ----------------------------------------------------------------------------
// Exported Interfaces For Check Validity of Addresses
//
IMPORT_EXPORT_VMM BOOLEAN
CheckAddressValidityUsingTsx(CHAR * Address);

IMPORT_EXPORT_VMM BOOLEAN
CheckAccessValidityAndSafety(UINT64 TargetAddress, UINT32 Size);

IMPORT_EXPORT_VMM BOOLEAN
CheckAddressPhysical(UINT64 PAddr);

IMPORT_EXPORT_VMM UINT32
CheckAddressMaximumInstructionLength(PVOID Address);

// ----------------------------------------------------------------------------
// Exported Interfaces For Layout Functions
//
IMPORT_EXPORT_VMM CR3_TYPE
LayoutGetCurrentProcessCr3();

IMPORT_EXPORT_VMM CR3_TYPE
LayoutGetExactGuestProcessCr3();

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

IMPORT_EXPORT_VMM PVOID
MemoryMapperGetPteVaOnTargetProcess(_In_ PVOID        Va,
                                    _In_ PAGING_LEVEL Level);

IMPORT_EXPORT_VMM PVOID
MemoryMapperSetExecuteDisableToPteOnTargetProcess(_In_ PVOID   Va,
                                                  _In_ BOOLEAN Set);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperCheckPteIsPresentOnTargetProcess(PVOID        Va,
                                             PAGING_LEVEL Level);

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
// Disassembler Functions
//
IMPORT_EXPORT_VMM UINT32
DisassemblerLengthDisassembleEngine(PVOID Address, BOOLEAN Is32Bit);

IMPORT_EXPORT_VMM UINT32
DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess(PVOID Address, BOOLEAN Is32Bit);

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
MemoryMapperReserveUsermodeAddressOnTargetProcess(_In_ UINT32  ProcessId,
                                                  _In_ BOOLEAN Allocate);

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperFreeMemoryOnTargetProcess(_In_ UINT32   ProcessId,
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

IMPORT_EXPORT_VMM BOOLEAN
MemoryMapperCheckIfPdeIsLargePageOnTargetProcess(_In_ PVOID Va);

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

IMPORT_EXPORT_VMM VOID
PoolManagerShowPreAllocatedPools();

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
BroadcastDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption);

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
BroadcastEnableMovControlRegisterExitingAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption);

IMPORT_EXPORT_VMM VOID
BroadcastDisableMovToControlRegistersExitingAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption);

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
