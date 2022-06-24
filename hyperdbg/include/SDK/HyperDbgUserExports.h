/**
 * @file HyperDbgUserExports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK Header Files
 * @details This file contains definitions of exported user-mode functions in HyperDbg
 * @version 0.2
 * @date 2022-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Header file of HPRDBGCTRL
// Imports
//
extern "C" {
__declspec(dllimport) int HyperdbgLoadVmm();
__declspec(dllimport) int HyperdbgUnload();
__declspec(dllimport) int HyperdbgInstallVmmDriver();
__declspec(dllimport) int HyperdbgUninstallDriver();
__declspec(dllimport) int HyperdbgStopDriver();
__declspec(dllimport) int HyperdbgInterpreter(char * Command);
__declspec(dllimport) void HyperdbgShowSignature();
__declspec(dllimport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllimport) void HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs);
__declspec(dllimport) bool HyperdbgContinuePreviousCommand();
__declspec(dllimport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}
