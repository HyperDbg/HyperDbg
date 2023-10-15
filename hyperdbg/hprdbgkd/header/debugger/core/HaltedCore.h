/**
 * @file HaltedCore.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for the implementation of applying events in halted cores
 * @details
 * @version 0.7
 * @date 2023-09-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			       	 Tasks  	         		//
//////////////////////////////////////////////////

/**
 * @brief Halted core task for testing purpose
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_TEST 0x00000001

/**
 * @brief Halted core task for running VMCALLs
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_RUN_VMCALL 0x00000002

/**
 * @brief Halted core task for setting process interception
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_SET_PROCESS_INTERCEPTION 0x00000003

/**
 * @brief Halted core task for setting thread interception
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_SET_THREAD_INTERCEPTION 0x00000004

//////////////////////////////////////////////////
//			    	 Functions  	      		//
//////////////////////////////////////////////////

BOOLEAN
HaltedCoreBroadcastTaskAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                UINT32                      TargetTask,
                                BOOLEAN                     LockAgainAfterTask,
                                BOOLEAN                     Synchronize,
                                PVOID                       Context);

VOID
HaltedCoreRunTaskOnSingleCore(UINT32  TargetCoreId,
                              UINT32  TargetTask,
                              BOOLEAN LockAgainAfterTask,
                              PVOID   Context);

VOID
HaltedCorePerformTargetTask(PROCESSOR_DEBUGGING_STATE * DbgState,
                            UINT32                      TargetTask,
                            PVOID                       Context);
