/**
 * @file exports.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief HyperDbg exported functions
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once


//////////////////////////////////////////////////
//					  Exports                   //
//////////////////////////////////////////////////

//
// Exports
//
extern "C" {
extern bool inline AsmVmxSupportDetection();
__declspec(dllexport) int __cdecl HyperdbgLoad();
__declspec(dllexport) int __cdecl HyperdbgUnload();
__declspec(dllexport) int __cdecl HyperdbgInstallDriver();
__declspec(dllexport) int __cdecl HyperdbgUninstallDriver();
__declspec(dllexport) void __stdcall HyperdbgSetTextMessageCallback(
    Callback handler);
__declspec(dllexport) int __cdecl HyperdbgInterpreter(const char *Command);
}
