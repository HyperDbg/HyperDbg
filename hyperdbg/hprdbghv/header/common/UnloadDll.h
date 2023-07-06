/**
 * @file UnloadDll.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for unloading DLL in the target Windows
 *
 * @version 0.4
 * @date 2023-07-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//	     		Exported Functions  			//
//////////////////////////////////////////////////

__declspec(dllexport) NTSTATUS DllInitialize(_In_ PUNICODE_STRING RegistryPath);

__declspec(dllexport) NTSTATUS DllUnload(void);
