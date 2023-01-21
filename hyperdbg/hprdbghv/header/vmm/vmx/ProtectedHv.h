/**
 * @file ProtectedHv.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for protected hypervisor resources
 * @details Protected Hypervisor Routines are those resource that
 * are used in different parts of the debugger or hypervisor,
 * these resources need extra checks to avoid integrity problems
 *
 * @version 0.1
 * @date 2021-10-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                   Functions                  //
//////////////////////////////////////////////////

//
// Exception Bitmap Functions
//

VOID
ProtectedHvSetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex);

VOID
ProtectedHvUnsetExceptionBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IdtIndex);

VOID
ProtectedHvResetExceptionBitmapToClearEvents(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands(VIRTUAL_MACHINE_STATE * VCpu);

//
// External-interrupt Exiting Functions
//

VOID
ProtectedHvSetExternalInterruptExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

VOID
ProtectedHvExternalInterruptExitingForDisablingInterruptCommands(VIRTUAL_MACHINE_STATE * VCpu);

//
// RDTSC/P Exiting Functions
//

VOID
ProtectedHvSetRdtscExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

VOID
ProtectedHvDisableRdtscExitingForDisablingTscCommands(VIRTUAL_MACHINE_STATE * VCpu);

//
// Mov to HW Debug Regs Exiting Functions
//

VOID
ProtectedHvSetMovDebugRegsExiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);

VOID
ProtectedHvDisableMovDebugRegsExitingForDisablingDrCommands(VIRTUAL_MACHINE_STATE * VCpu);

//
// Mov to Control Regs Exiting Functions
//

VOID
ProtectedHvDisableMovControlRegsExitingForDisablingCrCommands(VIRTUAL_MACHINE_STATE * VCpu, UINT64 ControlRegister, UINT64 MaskRegister);

//
// Mov to CR0/4 Exiting Functions
//

VOID
ProtectedHvSetMov2CrExiting(BOOLEAN Set, UINT64 ControlRegister, UINT64 MaskRegister);

//
// Mov to CR3 Exiting Functions
//

VOID
ProtectedHvSetMov2Cr3Exiting(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN Set);
