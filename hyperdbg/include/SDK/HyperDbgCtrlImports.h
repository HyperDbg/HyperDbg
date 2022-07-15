#pragma once

//
// Header file of HPRDBGCTRL
// Imports
//
extern "C" {

//
// VMM Module
//
__declspec(dllimport) int HyperDbgLoadVmm();
__declspec(dllimport) int HyperDbgUnloadVmm();
__declspec(dllimport) int HyperDbgInstallVmmDriver();
__declspec(dllimport) int HyperDbgUninstallVmmDriver();
__declspec(dllimport) int HyperDbgStopVmmDriver();

//
// General imports
//
__declspec(dllimport) int HyperDbgInterpreter(char * Command);
__declspec(dllimport) void HyperDbgShowSignature();
__declspec(dllimport) void HyperDbgSetTextMessageCallback(Callback handler);
__declspec(dllimport) void HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs);
__declspec(dllimport) bool HyperDbgContinuePreviousCommand();
__declspec(dllimport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}
