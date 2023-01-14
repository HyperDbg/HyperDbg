/**
 * @file Global.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for global variables
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/**
 * @brief Holder of script engines global variables
 *
 */
UINT64 * g_ScriptGlobalVariables;

/**
 * @brief Save the state and variables related to debugging on each to logical core
 *
 */
PROCESSOR_DEBUGGING_STATE * g_DbgState;

/**
 * @brief Determines whether the one application gets the handle or not
 * this is used to ensure that only one application can get the handle
 *
 */
BOOLEAN g_HandleInUse;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 *
 */
BOOLEAN g_AllowIOCTLFromUsermode;
