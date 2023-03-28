/**
 * @file ModeBasedExecHook.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hook headers for Mode-based execution
 * @details
 * @version 0.2
 * @date 2023-03-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

BOOLEAN
ModeBasedExecHookInitialize();

VOID
ModeBasedExecHookUninitialize();

VOID
ModeBasedExecHookHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, UINT64 NewCr3);

BOOLEAN
ModeBasedExecHookHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE * VCpu, VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification);

VOID
ModeBasedExecHookChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ModeBasedExecHookRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ModeBasedExecHookChangeToExecuteOnlyEptp(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
ModeBasedExecHookReversingMachineInitialize(PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevServiceRequest);
