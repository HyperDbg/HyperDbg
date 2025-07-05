/**
 * @file ExecTrap.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for the user-mode, kernel-mode execution traps' routines
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
 * @brief maximum number of processes for a simultaneous user-mode, kernel-mode execution trap
 *
 */
#define MAXIMUM_NUMBER_OF_PROCESSES_FOR_USER_KERNEL_EXEC_THREAD 100

//////////////////////////////////////////////////
//				      Locks 	    			//
//////////////////////////////////////////////////

/**
 * @brief The lock for modifying the list of processes for user-mode, kernel-mode execution traps
 *
 */
volatile LONG ExecTrapProcessListLock;

//////////////////////////////////////////////////
//				    Structures  				//
//////////////////////////////////////////////////

/**
 * @brief The status user-mode, kernel-mode execution traps for processes
 *
 */
typedef struct _USER_KERNEL_EXECUTION_TRAP_STATE
{
    UINT32 NumberOfItems;
    UINT64 InterceptionProcessIds[MAXIMUM_NUMBER_OF_PROCESSES_FOR_USER_KERNEL_EXEC_THREAD];

} USER_KERNEL_EXECUTION_TRAP_STATE, *PUSER_KERNEL_EXECUTION_TRAP_STATE;

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

VOID
ExecTrapHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ExecTrapChangeToUserDisabledMbecEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ExecTrapChangeToKernelDisabledMbecEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ExecTrapRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ExecTrapHandleMoveToAdjustedTrapState(VIRTUAL_MACHINE_STATE * VCpu, DEBUGGER_EVENT_MODE_TYPE TargetMode);

VOID
ExecTrapUninitialize();

BOOLEAN
ExecTrapInitialize();

VOID
ExecTrapApplyMbecConfiguratinFromKernelSide(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
ExecTrapHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                 VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification);

BOOLEAN
ExecTrapAddProcessToWatchingList(UINT32 ProcessId);

BOOLEAN
ExecTrapRemoveProcessFromWatchingList(UINT32 ProcessId);
