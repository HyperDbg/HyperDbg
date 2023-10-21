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

/**
 * @brief Halted core task for changing MSR Bitmap Read
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_READ 0x00000005

/**
 * @brief Halted core task for changing MSR Bitmap Write
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_WRITE 0x00000006

/**
 * @brief Halted core task for changing I/O Bitmaps (A & B)
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_CHANGE_IO_BITMAP 0x00000007

/**
 * @brief Halted core task for enabling rdpmc exiting
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_SET_RDPMC_EXITING 0x00000008

/**
 * @brief Halted core task for enabling rdtsc/rdtscp exiting
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_SET_RDTSC_EXITING 0x00000009

/**
 * @brief Halted core task for enabling mov to debug registers exiting
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_DEBUG_REGS_EXITING 0x0000000a

/**
 * @brief Halted core task for setting exception bitmap
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_SET_EXCEPTION_BITMAP 0x0000000b

/**
 * @brief Halted core task for enabling external interrupt exiting
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_ENABLE_EXTERNAL_INTERRUPT_EXITING 0x0000000c

/**
 * @brief Halted core task for enabling mov to CR exiting
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_CONTROL_REGS_EXITING 0x0000000d

/**
 * @brief Halted core task for enabling syscall hook using EFER SCE bit
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_ENABLE_SYSCALL_HOOK_EFER 0x0000000e

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
