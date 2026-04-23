
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
 * @brief The flag indicating whether the hypertrace module callbacks is initialized or not
 *
 */
BOOLEAN g_HyperTraceCallbacksInitialized;

/**
 * @brief The flag indicating whether the initialization is being done for hypervisor environment or not
 *
 */
BOOLEAN g_InitForHypervisorEnvironment;

/**
 * @brief The flag indicating whether the hypertrace LBR tracing is initialized or not
 *
 */
BOOLEAN g_LastBranchRecordEnabled;

/**
 * @brief Core specific state
 *
 */
LBR_IOCTL_REQUEST * g_LbrRequestState;
