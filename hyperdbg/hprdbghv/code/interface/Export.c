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
    g_GuestState[CoreId].IncrementRip = TRUE;
}

/**
 * @brief Supress the incrementation of RIP
 *
 * @param CoreId Target core's ID
 * @return VOID
 */
VOID
VmFuncSuppressRipIncrement(UINT32 CoreId)
{
    g_GuestState[CoreId].IncrementRip = FALSE;
}

/**
 * @brief Supress unsetting MTF
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
 * @brief Set guest's interruptibility state
 * @param CoreId Target core's ID
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId)
{
    //
    // Broadcast NMI with the intention of halting cores
    //
    return VmxBroadcastNmi(&g_GuestState[CoreId], NMI_BROADCAST_ACTION_KD_HALT_CORE);
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
 * @brief Uninitialize VMM memories
 *
 * @return VOID
 */
VOID
VmFuncUninitializeMemory()
{
    //
    // Free g_GuestState
    //
    GlobalGuestStateFreeMemory();
}

/**
 * @brief Get the current vmx opeation state
 *
 * @return BOOLEAN
 */
BOOLEAN
VmFuncVmxGetCurrentExecutionMode()
{
    return VmxGetCurrentExecutionMode();
}
