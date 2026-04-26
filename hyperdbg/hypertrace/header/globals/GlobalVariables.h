
/**
 * @file GlobalVariables.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Definition for global variables
 * @details
 * @version 0.19
 * @date 2026-04-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			   Global Variables     			//
//////////////////////////////////////////////////

/**
 * @brief List of callbacks
 *
 */
HYPERTRACE_CALLBACKS g_Callbacks;

/**
 * @brief The flag indicating whether the hypertrace module callbacks is initialized or not
 *
 */
BOOLEAN g_HyperTraceCallbacksInitialized;

/**
 * @brief The flag indicating whether the initialization is being done for hypervisor environment or not
 *
 */
BOOLEAN g_RunningOnHypervisorEnvironment;

/**
 * @brief The flag indicating whether the hypertrace LBR tracing is initialized or not
 *
 */
BOOLEAN g_LastBranchRecordEnabled;

/**
 * @brief The flag indicating whether the hypertrace Processor Trace is initialized or not
 *
 */
BOOLEAN g_ProcessorTraceEnabled;

/**
 * @brief This will be a dynamically allocated array to hold LBR states for each core
 *
 */
LBR_STACK_ENTRY * g_LbrStateList;
