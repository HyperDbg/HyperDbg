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
//				 Global Variables				//
//////////////////////////////////////////////////

/**
 * @brief The variable to store reversing machine's process Id
 *
 */
UINT32 ReversingMachineProcessId;

/**
 * @brief The variable to store reversing machine's thread Id
 *
 */
UINT32 ReversingMachineThreadId;

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

VOID
ReversingMachineHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineHandleMtfCallback(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingMachineRestoreNormalStateOnTargetProcess(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ReversingAddProcessThreadToTheWatchList(VIRTUAL_MACHINE_STATE * VCpu,
                                        UINT32                  ProcessId,
                                        UINT32                  ThreadId);
BOOLEAN
ReversingMachineInitialize(PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevServiceRequest);

BOOLEAN
ReversingMachineHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                         VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                         UINT64                                 GuestPhysicalAddr);
