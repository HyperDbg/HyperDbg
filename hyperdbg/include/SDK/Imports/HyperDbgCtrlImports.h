/**
 * @file HyperDbgCtrlImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from controller interface
 * @version 0.2
 * @date 2023-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Header file of HPRDBGCTRL
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

//
// VMM Module
//
__declspec(dllimport) int HyperDbgLoadVmm();
__declspec(dllimport) int HyperDbgUnloadVmm();
__declspec(dllimport) int HyperDbgInstallVmmDriver();
__declspec(dllimport) int HyperDbgUninstallVmmDriver();
__declspec(dllimport) int HyperDbgStopVmmDriver();

//
// Reversing Machine Module
//
__declspec(dllimport) int HyperDbgLoadReversingMachine();
__declspec(dllimport) int HyperDbgUnloadReversingMachine();
__declspec(dllimport) int HyperDbgInstallReversingMachineDriver();
__declspec(dllimport) int HyperDbgUninstallReversingMachineDriver();
__declspec(dllimport) int HyperDbgStopReversingMachineDriver();

//
// General imports
//
__declspec(dllimport) int HyperDbgInterpreter(char* Command);
__declspec(dllimport) void HyperDbgShowSignature();
__declspec(dllimport) void HyperDbgSetTextMessageCallback(Callback handler);
__declspec(dllimport) int HyperDbgScriptReadFileAndExecuteCommandline(int argc, char* argv[]);
__declspec(dllimport) bool HyperDbgContinuePreviousCommand();
__declspec(dllimport) bool HyperDbgCheckMultilineCommand(char* CurrentCommand, bool Reset);

#ifdef __cplusplus
}
#endif
