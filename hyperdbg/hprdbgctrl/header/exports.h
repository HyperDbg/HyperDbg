/**
 * @file exports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

__declspec(dllexport) int HyperdbgInterpreter(char * Command);
__declspec(dllexport) int HyperdbgLoadVmm();
__declspec(dllexport) int HyperdbgUnload();
__declspec(dllexport) int HyperdbgInstallVmmDriver();
__declspec(dllexport) int HyperdbgUninstallDriver();
__declspec(dllexport) int HyperdbgStopDriver();
__declspec(dllexport) void HyperdbgShowSignature();
__declspec(dllexport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllexport) void HyperDbgScriptReadFileAndExecuteCommand(vector<string> & PathAndArgs);
__declspec(dllexport) bool HyperdbgContinuePreviousCommand();
__declspec(dllexport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}
