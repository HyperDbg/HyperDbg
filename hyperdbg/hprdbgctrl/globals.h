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

BOOLEAN g_IsVmxOffProcessStart; // Show whether the vmxoff process start or not

HANDLE g_DeviceHandle;
