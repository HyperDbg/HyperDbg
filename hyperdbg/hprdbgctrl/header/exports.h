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

//
// VMM Module
//
__declspec(dllexport) int HyperDbgLoadVmm();
__declspec(dllexport) int HyperDbgUnloadVmm();
__declspec(dllexport) int HyperDbgInstallVmmDriver();
__declspec(dllexport) int HyperDbgUninstallVmmDriver();
__declspec(dllexport) int HyperDbgStopVmmDriver();

//
// General exports
//
__declspec(dllexport) int HyperDbgInterpreter(char * Command);
__declspec(dllexport) void HyperDbgShowSignature();
__declspec(dllexport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllexport) void HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs);
__declspec(dllexport) bool HyperDbgContinuePreviousCommand();
__declspec(dllexport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}
