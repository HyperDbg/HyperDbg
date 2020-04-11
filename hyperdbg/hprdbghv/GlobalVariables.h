/**
 * @file GlobalVariables.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Here we put global variables that are used more or less in all part of our hypervisor (not all of them)
 * @details Note : All the global variables are not here, just those that
 * will be used in all project. Special use global variables are located 
 * in their corresponding headers
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once
#include <ntddk.h>
#include "Vmx.h"
#include "Logging.h"
#include "PoolManager.h"

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

/**
 * @brief Save the state and variables related to each to logical core
 * 
 */
VIRTUAL_MACHINE_STATE * g_GuestState;

/**
 * @brief Save the state and variables related to EPT
 * 
 */
EPT_STATE * g_EptState;

/**
 * @brief Save the state of the thread that waits for messages to deliver to user-mode
 * 
 */
NOTIFY_RECORD * g_GlobalNotifyRecord;

/**
 * @brief Support for execute-only pages (indicating that data accesses are
 *  not allowed while instruction fetches are allowed)
 * 
 */
BOOLEAN g_ExecuteOnlySupport;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 * 
 */
BOOLEAN g_AllowIOCTLFromUsermode;
