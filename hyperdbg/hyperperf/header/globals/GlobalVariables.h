
/**
 * @file GlobalVariables.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Definition for global variables
 * @details
 * @version 0.21
 * @date 2026-06-22
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
HYPERPERF_CALLBACKS g_Callbacks;

/**
 * @brief The flag indicating whether the hyperperf module callbacks is initialized or not
 *
 */
BOOLEAN g_HyperPerfCallbacksInitialized;

/**
 * @brief The flag indicating whether the initialization is being done for hypervisor environment or not
 *
 */
BOOLEAN g_RunningOnHypervisorEnvironment;
