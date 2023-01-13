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

#define EXPORT_FUNC __declspec(dllexport)
#define IMPORT_FUNC __declspec(dllimport)

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

EXPORT_FUNC VOID
VmFuncPerformRipIncrement(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncSuppressRipIncrement(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncChangeMtfUnsettingState(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncChangeIgnoreOneMtfState(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetMonitorTrapFlag(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncRegisterMtfBreak(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncUnRegisterMtfBreak(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncSetLoadDebugControls(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetSaveDebugControls(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetPmcVmexit(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetMovControlRegsExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

EXPORT_FUNC VOID
VmFuncSetMovToCr3Vmexit(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncWriteExceptionBitmap(UINT32 BitmapMask);

EXPORT_FUNC VOID
VmFuncSetInterruptWindowExiting(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetNmiWindowExiting(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetNmiExiting(BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

EXPORT_FUNC VOID
VmFuncUnsetExceptionBitmap(UINT32 CoreId, UINT32 IdtIndex);

EXPORT_FUNC VOID
VmFuncSetExternalInterruptExiting(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetRdtscExiting(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncSetMovDebugRegsExiting(UINT32 CoreId, BOOLEAN Set);

EXPORT_FUNC VOID
VmFuncInjectPendingExternalInterrupts(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncSetRflags(UINT64 Rflags);

EXPORT_FUNC VOID
VmFuncSetRip(UINT64 Rip);

EXPORT_FUNC VOID
VmFuncSetInterruptibilityState(UINT64 InterruptibilityState);

EXPORT_FUNC VOID
VmFuncCheckAndEnableExternalInterrupts(UINT32 CoreId);

EXPORT_FUNC VOID
VmFuncDisableExternalInterruptsAndInterruptWindow(UINT32 CoreId);

EXPORT_FUNC UINT16
VmFuncGetCsSelector();

EXPORT_FUNC UINT32
VmFuncReadExceptionBitmap();

EXPORT_FUNC UINT64
VmFuncGetLastVmexitRip(UINT32 CoreId);

EXPORT_FUNC UINT64
VmFuncGetRflags();

EXPORT_FUNC UINT64
VmFuncGetRip();

EXPORT_FUNC UINT64
VmFuncGetInterruptibilityState();

EXPORT_FUNC BOOLEAN
VmFuncNmiHaltCores(UINT32 CoreId);
