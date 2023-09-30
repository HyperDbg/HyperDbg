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
//			    	 Functions  	      		//
//////////////////////////////////////////////////

VOID
HaltedCorePerformTargetTask(PROCESSOR_DEBUGGING_STATE * DbgState,
                            UINT32                      TargetTask);

VOID
HaltedCoreBroadcasTaskToAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                 UINT32                      TargetTask,
                                 BOOLEAN                     LockAgainAfterTask);
