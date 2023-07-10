/**
 * @file ReversingMachine.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for the reversing machine's routines
 * @details
 *
 * @version 0.4
 * @date 2023-07-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

VOID
ReversingMachineHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, UINT64 NewCr3);

VOID
ReversingMachineHandleMtfCallback(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
ReversingMachineInitialize(PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevServiceRequest);

BOOLEAN
ReversingMachineHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                         VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                         UINT64                                 GuestPhysicalAddr);

VOID
ReversingMachineRestoreNormalStateOnTargetProcess(VIRTUAL_MACHINE_STATE * VCpu);
