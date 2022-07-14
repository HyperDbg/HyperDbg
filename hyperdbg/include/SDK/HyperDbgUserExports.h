#pragma once

//
// Header file of HPRDBGCTRL
// Imports
//
extern "C" {

//
// VMM Module
//
__declspec(dllimport) int HyperdbgLoadVmm();
__declspec(dllimport) int HyperdbgUnloadVmm();
__declspec(dllimport) int HyperdbgInstallVmmDriver();
__declspec(dllimport) int HyperdbgUninstallVmmDriver();
__declspec(dllimport) int HyperdbgStopVmmDriver();

//
// General imports
//
__declspec(dllimport) int HyperdbgInterpreter(char * Command);
__declspec(dllimport) void HyperdbgShowSignature();
__declspec(dllimport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllimport) void HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs);
__declspec(dllimport) bool HyperdbgContinuePreviousCommand();
__declspec(dllimport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}
