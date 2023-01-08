/**
 * @file Export.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hypervisor
 * @version 0.1
 * @date 2022-12-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
VmFuncPerformRipIncrement(UINT32 CoreId);

VOID
VmFuncSuppressRipIncrement(UINT32 CoreId);

VOID
VmFuncChangeMtfUnsettingState(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncChangeIgnoreOneMtfState(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncSetMonitorTrapFlag(BOOLEAN Set);

VOID
VmFuncRegisterMtfBreak(UINT32 CoreId);

VOID
VmFuncUnRegisterMtfBreak(UINT32 CoreId);

VOID
VmFuncSetLoadDebugControls(BOOLEAN Set);

VOID
VmFuncSetSaveDebugControls(BOOLEAN Set);

VOID
VmFuncSetPmcVmexit(BOOLEAN Set);

VOID
VmFuncSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

VOID
VmFuncSetMovToCr3Vmexit(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncWriteExceptionBitmap(UINT32 BitmapMask);

VOID
VmFuncSetInterruptWindowExiting(BOOLEAN Set);

VOID
VmFuncSetNmiWindowExiting(BOOLEAN Set);

VOID
VmFuncSetNmiExiting(BOOLEAN Set);

VOID
VmFuncSetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

VOID
VmFuncUnsetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

VOID
VmFuncSetExternalInterruptExiting(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncSetRdtscExiting(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncSetMovDebugRegsExiting(UINT32 CoreId, BOOLEAN Set);

VOID
VmFuncInjectPendingExternalInterrupts(UINT32 CoreId);

VOID
VmFuncSetRflags(UINT64 Rflags);

VOID
VmFuncSetRip(UINT64 Rip);

VOID
VmFuncSetInterruptibilityState(UINT64 InterruptibilityState);

VOID
VmFuncCheckAndEnableExternalInterrupts(UINT32 CoreId);

VOID
VmFuncDisableExternalInterruptsAndInterruptWindow(UINT32 CoreId);

UINT16
VmFuncGetCsSelector();

UINT32
VmFuncReadExceptionBitmap();

UINT64
VmFuncGetLastVmexitRip(UINT32 CoreId);

UINT64
VmFuncGetRflags();

UINT64
VmFuncGetRip();

UINT64
VmFuncGetInterruptibilityState();

BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId);
