/**
 * @file UserExecTrap.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for the user-mode execution traps' routines
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
//				    Definitions  				//
//////////////////////////////////////////////////

/**
 * @brief maximum number of processes for a simultaneous user-mode execution trap
 *
 */
#define MAXIMUM_NUMBER_OF_PROCESSES_FOR_USER_MODE_EXEC_THREAD 100

//////////////////////////////////////////////////
//				    Structures  				//
//////////////////////////////////////////////////

/**
 * @brief The status user-mode execution traps for processes
 *
 */
typedef struct _USER_MODE_EXECUTION_TRAP_STATE
{
    UINT32 NumberOfItems;
    UINT64 InterceptionProcessIds[MAXIMUM_NUMBER_OF_PROCESSES_FOR_USER_MODE_EXEC_THREAD];

} USER_MODE_EXECUTION_TRAP_STATE, *PUSER_MODE_EXECUTION_TRAP_STATE;

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

VOID
UserExecTrapHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecTrapHandleMtfCallback(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecTrapChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecHandleRestoringToNormalState(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecTrapRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecTrapRestoreNormalStateOnTargetProcess(VIRTUAL_MACHINE_STATE * VCpu);

VOID
UserExecTrapUninitialize();

BOOLEAN
UserExecTrapInitialize();

BOOLEAN
UserExecTrapHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                     VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                     UINT64                                 GuestPhysicalAddr);

BOOLEAN
UserExecTrapAddProcessToWatchingList(UINT32 ProcessId);

BOOLEAN
UserExecTrapRemoveProcessFromWatchingList(UINT32 ProcessId);
