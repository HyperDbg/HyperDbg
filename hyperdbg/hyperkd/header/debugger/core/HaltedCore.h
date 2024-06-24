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

/**
 * @brief Halted core task for invalidating EPT (All Contexts)
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_INVEPT_ALL_CONTEXTS 0x0000000f

/**
 * @brief Halted core task for invalidating EPT (A Single Context)
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_INVEPT_SINGLE_CONTEXT 0x00000010

/**
 * @brief Halted core task for unsetting exception bitmap on VMCS
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_UNSET_EXCEPTION_BITMAP 0x00000011

/**
 * @brief Halted core task for restoring a single EPT entry and invalidating EPT cache
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_UNHOOK_SINGLE_PAGE 0x00000012

/**
 * @brief Halted core task for disabling external interrupt exiting only to clear !interrupt commands
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS 0x00000013

/**
 * @brief Halted core task for resetting MSR Bitmap Read
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_READ 0x00000014

/**
 * @brief Halted core task for resetting MSR Bitmap Write
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_WRITE 0x00000015

/**
 * @brief Halted core task for resetting exception bitmap on VMCS
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS 0x00000016

/**
 * @brief Halted core task for resetting I/O Bitmaps (A & B)
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_RESET_IO_BITMAP 0x00000017

/**
 * @brief Halted core task for clearing rdtsc exiting bit ONLY in the case of disabling
 * the events for !tsc command
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS 0x00000018

/**
 * @brief Halted core task for disabling rdpmc exiting in primary cpu-based controls
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_UNSET_RDPMC_EXITING 0x00000019

/**
 * @brief Halted core task for disabling syscall hook using EFER SCE bit
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_DISABLE_SYSCALL_HOOK_EFER 0x0000001a

/**
 * @brief Halted core task for clearing mov 2 hw dr exiting bit ONLY in the case of
 * disabling the events for !dr command
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS 0x0000001b

/**
 * @brief Halted core task for clearing mov 2 cr exiting bit ONLY in the case of disabling
 * the events for !crwrite command
 *
 */
#define DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS 0x0000001c

//////////////////////////////////////////////////
//			    	 Functions  	      		//
//////////////////////////////////////////////////

BOOLEAN
HaltedCoreBroadcastTaskAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                UINT64                      TargetTask,
                                BOOLEAN                     LockAgainAfterTask,
                                BOOLEAN                     Synchronize,
                                PVOID                       Context);

VOID
HaltedCoreRunTaskOnSingleCore(UINT32  TargetCoreId,
                              UINT64  TargetTask,
                              BOOLEAN LockAgainAfterTask,
                              PVOID   Context);

VOID
HaltedCorePerformTargetTask(PROCESSOR_DEBUGGING_STATE * DbgState,
                            UINT64                      TargetTask,
                            PVOID                       Context);
