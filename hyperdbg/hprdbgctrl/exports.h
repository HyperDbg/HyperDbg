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
__declspec(dllexport) int HyperdbgLoadVmm();
__declspec(dllexport) int HyperdbgUnload();
__declspec(dllexport) int HyperdbgInstallVmmDriver();
__declspec(dllexport) int HyperdbgUninstallDriver();
__declspec(dllexport) int HyperdbgStopDriver();
__declspec(dllexport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllexport) int HyperdbgInterpreter(const char * Command);
__declspec(dllexport) void HyperdbgShowSignature();
}
