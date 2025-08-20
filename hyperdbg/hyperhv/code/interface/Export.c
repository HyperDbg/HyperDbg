/**
 * @file Export.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of exported functions from hypervisor
 * @details
 *
 * @version 0.1
 * @date 2022-12-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Perform the incrementation of RIP
 *
 * @param CoreId Target core's ID
 * @return VOID
 */
VOID
VmFuncPerformRipIncrement(UINT32 CoreId)
{
    HvPerformRipIncrement(&g_GuestState[CoreId]);
}

/**
 * @brief Suppress the incrementation of RIP
 *
 * @param CoreId Target core's ID
 * @return VOID
 */
VOID
VmFuncSuppressRipIncrement(UINT32 CoreId)
{
    HvSuppressRipIncrement(&g_GuestState[CoreId]);
}

/**
 * @brief Suppress unsetting MTF
 *
 * @param CoreId Target core's ID
 * @param Set State of setting or unsetting
 * @return VOID
 */
VOID
VmFuncChangeMtfUnsettingState(UINT32 CoreId, BOOLEAN Set)
{
    g_GuestState[CoreId].IgnoreMtfUnset = Set;
}

/**
 * @brief Change ignore one MTF state
 *
 * @param CoreId Target core's ID
 * @param Set State of setting or unsetting
 * @return VOID
 */
VOID
VmFuncChangeIgnoreOneMtfState(UINT32 CoreId, BOOLEAN Set)
{
    g_GuestState[CoreId].IgnoreOneMtf = Set;
}

/**
 * @brief Register for break in the case of an MTF
 *
 * @param CoreId Target core's ID
 *
 * @return VOID
 */
VOID
VmFuncRegisterMtfBreak(UINT32 CoreId)
{
    g_GuestState[CoreId].RegisterBreakOnMtf = TRUE;
}

/**
 * @brief Unregister for break in the case of an MTF
 *
 * @param CoreId Target core's ID
 *
 * @return VOID
 */
VOID
VmFuncUnRegisterMtfBreak(UINT32 CoreId)
{
    g_GuestState[CoreId].RegisterBreakOnMtf = FALSE;
}

/**
 * @brief Set the monitor trap flag
 *
 * @param Set Set or unset the MTFs
 * @return VOID
 */
VOID
VmFuncSetMonitorTrapFlag(BOOLEAN Set)
{
    HvSetMonitorTrapFlag(Set);
}

/**
 * @brief Set Rflag's trap flag
 *
 * @param Set Set or unset the TF
 * @return VOID
 */
VOID
VmFuncSetRflagTrapFlag(BOOLEAN Set)
{
    HvSetRflagTrapFlag(Set);
}

/**
 * @brief Set LOAD DEBUG CONTROLS on Vm-entry controls
 *
 * @param Set Set or unset
 * @return VOID
 */
VOID
VmFuncSetLoadDebugControls(BOOLEAN Set)
{
    HvSetLoadDebugControls(Set);
}

/**
 * @brief Set SAVE DEBUG CONTROLS on Vm-exit controls
 *
 * @param Set Set or unset
 * @return VOID
 */
VOID
VmFuncSetSaveDebugControls(BOOLEAN Set)
{
    HvSetSaveDebugControls(Set);
}

/**
 * @brief Set vm-exit for rdpmc instructions
 * @details Should be called in vmx-root
 *
 * @param Set Set or unset the vm-exits
 * @return VOID
 */
VOID
VmFuncSetPmcVmexit(BOOLEAN Set)
{
    HvSetPmcVmexit(Set);
}

/**
 * @brief Set vm-exit for mov-to-cr0/4
 * @details Should be called in vmx-root
 *
 * @param Set or unset the vm-exits
 * @param Control Register
 * @param Mask Register
 * @return VOID
 */
VOID
VmFuncSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister)
{
    HvSetMovControlRegsExiting(Set, ControlRegister, MaskRegister);
}

/**
 * @brief Set vm-exit for mov-to-cr3
 * @details Should be called in vmx-root
 *
 * @param CoreId target core id
 * @param Set Set or unset the vm-exits
 *
 * @return VOID
 */
VOID
VmFuncSetMovToCr3Vmexit(UINT32 CoreId, BOOLEAN Set)
{
    HvSetMovToCr3Vmexit(&g_GuestState[CoreId], Set);
}

/**
 * @brief Write on exception bitmap in VMCS
 * DO NOT CALL IT DIRECTLY, instead use HvSetExceptionBitmap
 * @details Should be called in vmx-root
 *
 * @param BitmapMask The content to write on exception bitmap
 * @return VOID
 */
VOID
VmFuncWriteExceptionBitmap(UINT32 BitmapMask)
{
    HvWriteExceptionBitmap(BitmapMask);
}

/**
 * @brief Read exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @return UINT32
 */
UINT32
VmFuncReadExceptionBitmap()
{
    return HvReadExceptionBitmap();
}

/**
 * @brief Set Interrupt-window exiting
 *
 * @param Set Set or unset the Interrupt-window exiting
 * @return VOID
 */
VOID
VmFuncSetInterruptWindowExiting(BOOLEAN Set)
{
    HvSetInterruptWindowExiting(Set);
}

/**
 * @brief Set NMI-window exiting
 *
 * @param Set Set or unset the NMI-window exiting
 * @return VOID
 */
VOID
VmFuncSetNmiWindowExiting(BOOLEAN Set)
{
    HvSetNmiWindowExiting(Set);
}

/**
 * @brief Set the NMI Exiting
 *
 * @param Set Set or unset the NMI Exiting
 * @return VOID
 */
VOID
VmFuncSetNmiExiting(BOOLEAN Set)
{
    HvSetNmiExiting(Set);
}

/**
 * @brief Set exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param CoreId Target core's ID
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
VmFuncSetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex)
{
    HvSetExceptionBitmap(&g_GuestState[CoreId], IdtIndex);
}

/**
 * @brief Unset exception bitmap in VMCS
 * @details Should be called in vmx-root
 *
 * @param CoreId Target core's ID
 * @param IdtIndex Interrupt Descriptor Table index of exception
 * @return VOID
 */
VOID
VmFuncUnsetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex)
{
    HvUnsetExceptionBitmap(&g_GuestState[CoreId], IdtIndex);
}

/**
 * @brief Set the External Interrupt Exiting
 *
 * @param CoreId Target core's ID
 * @param Set Set or unset the External Interrupt Exiting
 * @return VOID
 */
VOID
VmFuncSetExternalInterruptExiting(UINT32 CoreId, BOOLEAN Set)
{
    HvSetExternalInterruptExiting(&g_GuestState[CoreId], Set);
}

/**
 * @brief Set the RDTSC/P Exiting
 *
 * @param CoreId Target core's ID
 * @param Set Set or unset the RDTSC/P Exiting
 * @return VOID
 */
VOID
VmFuncSetRdtscExiting(UINT32 CoreId, BOOLEAN Set)
{
    HvSetRdtscExiting(&g_GuestState[CoreId], Set);
}

/**
 * @brief Set or unset the Mov to Debug Registers Exiting
 *
 * @param CoreId Target core's ID
 * @param Set Set or unset the Mov to Debug Registers Exiting
 * @return VOID
 */
VOID
VmFuncSetMovDebugRegsExiting(UINT32 CoreId, BOOLEAN Set)
{
    HvSetMovDebugRegsExiting(&g_GuestState[CoreId], Set);
}

/**
 * @brief get the last vm-exit RIP
 *
 * @param CoreId Target core's ID
 * @return UINT64
 */
UINT64
VmFuncGetLastVmexitRip(UINT32 CoreId)
{
    return g_GuestState[CoreId].LastVmexitRip;
}

/**
 * @brief Inject pending external interrupts
 *
 * @param CoreId Target core's ID
 * @return VOID
 */
VOID
VmFuncInjectPendingExternalInterrupts(UINT32 CoreId)
{
    HvInjectPendingExternalInterrupts(&g_GuestState[CoreId]);
}

/**
 * @brief Read CS selector
 *
 * @return UINT16
 */
UINT16
VmFuncGetCsSelector()
{
    return HvGetCsSelector();
}

/**
 * @brief Read guest's RFLAGS
 *
 * @return UINT64
 */
UINT64
VmFuncGetRflags()
{
    return HvGetRflags();
}

/**
 * @brief Set guest's RFLAGS
 * @param Rflags
 *
 * @return VOID
 */
VOID
VmFuncSetRflags(UINT64 Rflags)
{
    HvSetRflags(Rflags);
}

/**
 * @brief Read guest's RIP
 *
 * @return UINT64
 */
UINT64
VmFuncGetRip()
{
    return HvGetRip();
}

/**
 * @brief Set guest's RIP
 * @param Rip
 *
 * @return VOID
 */
VOID
VmFuncSetRip(UINT64 Rip)
{
    HvSetRip(Rip);
}

/**
 * @brief Read guest's interruptibility state
 *
 * @return UINT64
 */
UINT64
VmFuncGetInterruptibilityState()
{
    return HvGetInterruptibilityState();
}

/**
 * @brief Clear STI and MOV SS bits
 *
 * @return UINT64
 */
UINT64
VmFuncClearSteppingBits(UINT64 Interruptibility)
{
    return HvClearSteppingBits(Interruptibility);
}

/**
 * @brief Set guest's interruptibility state
 * @param InterruptibilityState
 *
 * @return VOID
 */
VOID
VmFuncSetInterruptibilityState(UINT64 InterruptibilityState)
{
    HvSetInterruptibilityState(InterruptibilityState);
}

/**
 * @brief Broadcast NMI requests
 * @param CoreId Target core's ID
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncNmiBroadcastRequest(UINT32 CoreId)
{
    //
    // Broadcast NMI requests
    //
    return VmxBroadcastNmi(&g_GuestState[CoreId], NMI_BROADCAST_ACTION_REQUEST);
}

/**
 * @brief Broadcast NMI requests for single-context EPT invalidation
 * @param CoreId Target core's ID
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncNmiBroadcastInvalidateEptSingleContext(UINT32 CoreId)
{
    //
    // Broadcast NMI requests
    //
    return VmxBroadcastNmi(&g_GuestState[CoreId], NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_SINGLE_CONTEXT);
}

/**
 * @brief Broadcast NMI requests for all contexts EPT invalidation
 * @param CoreId Target core's ID
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncNmiBroadcastInvalidateEptAllContexts(UINT32 CoreId)
{
    //
    // Broadcast NMI requests
    //
    return VmxBroadcastNmi(&g_GuestState[CoreId], NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_ALL_CONTEXTS);
}

/**
 * @brief Requests for single-context EPT invalidation
 * @param CoreId Target core's ID
 *
 * @return VOID
 */
VOID
VmFuncInvalidateEptSingleContext(UINT32 CoreId)
{
    EptInveptSingleContext(g_GuestState[CoreId].EptPointer.AsUInt);
}

/**
 * @brief Requests for all contexts EPT invalidation
 *
 * @return VOID
 */
VOID
VmFuncInvalidateEptAllContexts()
{
    //
    // Broadcast NMI requests
    //
    EptInveptAllContexts();
}

/**
 * @brief Check and enable external interrupts
 *
 * @param CoreId Target core's ID
 *
 * @return VOID
 */
VOID
VmFuncCheckAndEnableExternalInterrupts(UINT32 CoreId)
{
    HvCheckAndEnableExternalInterrupts(&g_GuestState[CoreId]);
}

/**
 * @brief Disable external-interrupts and interrupt window
 *
 * @param CoreId
 *
 * @return VOID
 */
VOID
VmFuncDisableExternalInterruptsAndInterruptWindow(UINT32 CoreId)
{
    HvDisableExternalInterruptsAndInterruptWindow(&g_GuestState[CoreId]);
}

/**
 * @brief Initializes hypervisor
 * @param VmmCallbacks
 *
 * @return BOOLEAN Shows whether the initialization was successful or not
 */
BOOLEAN
VmFuncInitVmm(VMM_CALLBACKS * VmmCallbacks)
{
    return HvInitVmm(VmmCallbacks);
}

/**
 * @brief Uninitialize Terminate Vmx on all logical cores
 *
 * @return VOID
 */
VOID
VmFuncUninitVmm()
{
    VmxPerformTermination();
}

/**
 * @brief Get the current VMX operation state
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncVmxGetCurrentExecutionMode()
{
    return VmxGetCurrentExecutionMode();
}

/**
 * @brief Get the current state of mode exec trap mechanism
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncQueryModeExecTrap()
{
    return g_ExecTrapInitialized;
}

/**
 * @brief Set triggering events for VMCALLs
 *
 * @param Set Set or unset the trigger
 * @return VOID
 */
VOID
VmFuncSetTriggerEventForVmcalls(BOOLEAN Set)
{
    g_TriggerEventForVmcalls = Set;
}

/**
 * @brief Set triggering events for CPUIDs
 *
 * @param Set Set or unset the trigger
 * @return VOID
 */
VOID
VmFuncSetTriggerEventForCpuids(BOOLEAN Set)
{
    g_TriggerEventForCpuids = Set;
}

/**
 * @brief Set trigger event for XSETBVs
 *
 * @param Set Set or unset the trigger
 * @return VOID
 */
VOID
VmFuncSetTriggerEventForXsetbvs(BOOLEAN Set)
{
    g_TriggerEventForXsetbvs = Set;
}

/**
 * @brief VMX-root compatible strlen
 * @param s A pointer to the string
 *
 * @return UINT32
 */
UINT32
VmFuncVmxCompatibleStrlen(const CHAR * s)
{
    return VmxCompatibleStrlen(s);
}

/**
 * @brief VMX-root compatible strlen
 * @param s A pointer to the string
 *
 * @return UINT32
 */
UINT32
VmFuncVmxCompatibleWcslen(const wchar_t * s)
{
    return VmxCompatibleWcslen(s);
}

/**
 * @brief VMX-root compatible micro sleep
 * @param Us Delay in micro seconds
 *
 * @return VOID
 */
VOID
VmFuncVmxCompatibleMicroSleep(UINT64 Us)
{
    VmxCompatibleMicroSleep(Us);
}

/**
 * @brief Inject #PF and configure CR2 register
 *
 * @param CoreId Target core's ID
 * @param Address Page-fault address
 * @param PageFaultCode Page-fault error code
 *
 * @return VOID
 */
VOID
VmFuncEventInjectPageFaultWithCr2(UINT32 CoreId, UINT64 Address, UINT32 PageFaultCode)
{
    EventInjectPageFaultWithCr2(&g_GuestState[CoreId], Address, PageFaultCode);
}

/**
 * @brief Inject a range of page-faults
 *
 * @param CoreId Target core's ID
 * @param AddressFrom Page-fault address (from)
 * @param AddressTo Page-fault address (to)
 * @param Address Page-fault address
 * @param PageFaultCode Page-fault error code
 *
 * @return VOID
 */
VOID
VmFuncEventInjectPageFaultRangeAddress(UINT32 CoreId,
                                       UINT64 AddressFrom,
                                       UINT64 AddressTo,
                                       UINT32 PageFaultCode)
{
    EventInjectPageFaultRangeAddress(&g_GuestState[CoreId], AddressFrom, AddressTo, PageFaultCode);
}

/**
 * @brief Inject interrupt/faults/exceptions
 *
 * @param InterruptionType
 * @param Vector
 * @param DeliverErrorCode
 * @param ErrorCode
 *
 * @return VOID
 */
VOID
VmFuncEventInjectInterruption(UINT32  InterruptionType,
                              UINT32  Vector,
                              BOOLEAN DeliverErrorCode,
                              UINT32  ErrorCode)
{
    EventInjectInterruption(InterruptionType,
                            Vector,
                            DeliverErrorCode,
                            ErrorCode);
}

/**
 * @brief Export for running VMX VMCALLs
 *
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @return NTSTATUS
 */
NTSTATUS
VmFuncVmxVmcall(unsigned long long VmcallNumber,
                unsigned long long OptionalParam1,
                unsigned long long OptionalParam2,
                unsigned long long OptionalParam3)
{
    return AsmVmxVmcall(VmcallNumber, OptionalParam1, OptionalParam2, OptionalParam3);
}

/**
 * @brief Export for initialize the VMX Broadcast mechanism
 *
 * @return VOID
 */
VOID
VmFuncVmxBroadcastInitialize()
{
    VmxBroadcastInitialize();
}

/**
 * @brief Export for uninitialize the VMX Broadcast mechanism
 *
 * @return VOID
 */
VOID
VmFuncVmxBroadcastUninitialize()
{
    VmxBroadcastUninitialize();
}

/**
 * @brief Inject #BP to the guest (Event Injection)
 *
 * @return VOID
 */
VOID
VmFuncEventInjectBreakpoint()
{
    EventInjectBreakpoint();
}

/**
 * @brief VMX-root compatible strcmp
 * @param Address1
 * @param Address2
 *
 * @return INT32
 */
INT32
VmFuncVmxCompatibleStrcmp(const CHAR * Address1, const CHAR * Address2)
{
    return VmxCompatibleStrcmp(Address1, Address2, NULL_ZERO, FALSE);
}

/**
 * @brief VMX-root compatible strncmp
 * @param Address1
 * @param Address2
 * @param Num
 *
 * @return INT32
 */
INT32
VmFuncVmxCompatibleStrncmp(const CHAR * Address1, const CHAR * Address2, SIZE_T Num)
{
    return VmxCompatibleStrcmp(Address1, Address2, Num, TRUE);
}

/**
 * @brief VMX-root compatible wcscmp
 * @param Address1
 * @param Address2
 *
 * @return INT32
 */
INT32
VmFuncVmxCompatibleWcscmp(const wchar_t * Address1, const wchar_t * Address2)
{
    return VmxCompatibleWcscmp(Address1, Address2, NULL_ZERO, FALSE);
}

/**
 * @brief VMX-root compatible wcsncmp
 * @param Address1
 * @param Address2
 * @param Num
 *
 * @return INT32
 */
INT32
VmFuncVmxCompatibleWcsncmp(const wchar_t * Address1, const wchar_t * Address2, SIZE_T Num)
{
    return VmxCompatibleWcscmp(Address1, Address2, Num, TRUE);
}

/**
 * @brief VMX-root compatible memcmp
 * @param Address1
 * @param Address2
 * @param Count
 *
 * @return INT32
 */
INT32
VmFuncVmxCompatibleMemcmp(const CHAR * Address1, const CHAR * Address2, size_t Count)
{
    return VmxCompatibleMemcmp(Address1, Address2, Count);
}

/**
 * @brief Enables MTF and adjust external interrupt state
 * @param UINT32 CoreId
 *
 * @return VOID
 */
VOID
VmFuncEnableMtfAndChangeExternalInterruptState(UINT32 CoreId)
{
    HvEnableMtfAndChangeExternalInterruptState(&g_GuestState[CoreId]);
}

/**
 * @brief Checks to enable and reinject previous interrupts
 *
 * @param UINT32 CoreId
 *
 * @return VOID
 */
VOID
VmFuncEnableAndCheckForPreviousExternalInterrupts(UINT32 CoreId)
{
    HvEnableAndCheckForPreviousExternalInterrupts(&g_GuestState[CoreId]);
}

/**
 * @brief Store the details Local APIC in xapic or x2apic modes
 * @param LocalApicBuffer
 * @param IsUsingX2APIC
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncApicStoreLocalApicFields(PLAPIC_PAGE LocalApicBuffer, PBOOLEAN IsUsingX2APIC)
{
    return ApicStoreLocalApicFields(LocalApicBuffer, IsUsingX2APIC);
}

/**
 * @brief Store the details of I/O APIC
 * @param IoApicPackets
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncApicStoreIoApicFields(IO_APIC_ENTRY_PACKETS * IoApicPackets)
{
    return ApicStoreIoApicFields(IoApicPackets);
}

/**
 * @brief Perform query for IDT entries
 *
 * @param IdtQueryRequest
 * @param ReadFromVmxRoot
 *
 * @return VOID
 */
VOID
VmFuncIdtQueryEntries(PINTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS IdtQueryRequest,
                      BOOLEAN                                     ReadFromVmxRoot)
{
    IdtEmulationQueryIdtEntriesRequest(IdtQueryRequest, ReadFromVmxRoot);
}

/**
 * @brief Perform actions related to System Management Interrupts (SMIs)
 *
 * @param SmiOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncSmmPerformSmiOperation(SMI_OPERATION_PACKETS * SmiOperationRequest,
                             BOOLEAN                 ApplyFromVmxRootMode)
{
    return SmmPerformSmiOperation(SmiOperationRequest, ApplyFromVmxRootMode);
}
