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

#ifdef HYPERDBG_HPRDBGCTRL
#    define IMPORT_EXPORT_CTRL __declspec(dllexport)
#else
#    define IMPORT_EXPORT_CTRL __declspec(dllimport)
#endif

//
// Header file of HPRDBGCTRL
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

//
// Support Detection
//
IMPORT_EXPORT_CTRL bool HyperDbgVmxSupportDetection();
IMPORT_EXPORT_CTRL void HyperDbgReadVendorString(char *);

//
// VMM Module
//
IMPORT_EXPORT_CTRL int HyperDbgLoadVmm();
IMPORT_EXPORT_CTRL int HyperDbgUnloadVmm();
IMPORT_EXPORT_CTRL int HyperDbgInstallVmmDriver();
IMPORT_EXPORT_CTRL int HyperDbgUninstallVmmDriver();
IMPORT_EXPORT_CTRL int HyperDbgStopVmmDriver();

//
// General imports
//
IMPORT_EXPORT_CTRL int HyperDbgInterpreter(char * Command);
IMPORT_EXPORT_CTRL void HyperDbgShowSignature();
IMPORT_EXPORT_CTRL void HyperDbgSetTextMessageCallback(Callback handler);
IMPORT_EXPORT_CTRL int HyperDbgScriptReadFileAndExecuteCommandline(int argc, char * argv[]);
IMPORT_EXPORT_CTRL bool HyperDbgContinuePreviousCommand();
IMPORT_EXPORT_CTRL bool HyperDbgCheckMultilineCommand(char * CurrentCommand, bool Reset);

#ifdef __cplusplus
}
#endif
