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

/**
 * @brief events list (for debugger)
 *
 */
DEBUGGER_CORE_EVENTS * g_Events;

/**
 * @brief Holds the requests to pause the break of debuggee until
 * a special event happens
 *
 */
DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT g_IgnoreBreaksToDebugger;

/**
 * @brief Holds the state of hardware debug register for step-over
 *
 */
HARDWARE_DEBUG_REGISTER_DETAILS g_HardwareDebugRegisterDetailsForStepOver;

/**
 * @brief Process switch to EPROCESS or Process ID
 *
 */
DEBUGGEE_REQUEST_TO_CHANGE_PROCESS g_ProcessSwitch;

/**
 * @brief Thread switch to ETHREAD or Thread ID
 *
 */
DEBUGGEE_REQUEST_TO_CHANGE_THREAD g_ThreadSwitch;
