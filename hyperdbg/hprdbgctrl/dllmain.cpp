/**
 * @file dllmain.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Defines the entry point for the DLL application
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Dll Main Entry
 *
 * @param hModule
 * @param ul_reason_for_call
 * @param lpReserved
 * @return BOOL DllMain
 */
BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
