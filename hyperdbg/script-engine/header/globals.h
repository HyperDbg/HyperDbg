/**
 * @file globals.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Global variables header
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef GLOBALS_H
#    define GLOBALS_H
#    define MAX_TEMP_COUNT 128

#endif // !GLOBALS_H

/**
 * @brief Instance information of the current hwdbg debuggee
 *
 */
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;

/**
 * @brief Shows whether the instance info is valid (received) or not
 *
 */
extern BOOLEAN g_HwdbgInstanceInfoIsValid;

/**
 * @brief Message handler function
 *
 */
extern PVOID g_MessageHandler;
