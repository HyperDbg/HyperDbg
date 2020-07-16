/**
 * @file globals.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Global Variables for user-mode interface
 * @details
 * @version 0.1
 * @date 2020-07-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				 Global Variables               //
//////////////////////////////////////////////////

BOOLEAN g_IsConnectedToDebugger = FALSE;

BOOLEAN g_IsDebuggerModulesLoaded = FALSE;

UINT64 g_EventTag = DebuggerEventTagStartSeed;

BOOLEAN g_EventTraceInitialized = FALSE;

LIST_ENTRY g_EventTrace = {0};

TCHAR g_DriverLocation[MAX_PATH] = {0};

Callback g_MessageHandler = 0;

BOOLEAN g_IsVmxOffProcessStart; // Shows whether the vmxoff process start or not

HANDLE g_DeviceHandle;

BOOLEAN g_LogOpened = FALSE;

ofstream g_LogOpenFile;

BOOLEAN g_ExecutingScript = FALSE;
