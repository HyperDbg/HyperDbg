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
