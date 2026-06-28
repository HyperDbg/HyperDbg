/**
 * @file libhyperdbg.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for libhyperdbg
 * @details
 * @version 0.10
 * @date 2024-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            	    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
HyperDbgIsAnyModuleLoaded();

INT
HyperDbgUnloadAllModules();

INT
HyperDbgInitHyperTraceModule();

INT
HyperDbgUnloadVmm();

INT
HyperDbgUnloadHyperTrace();

INT
HyperDbgUnloadKd();

INT
HyperDbgInstallKdDriver();

INT
HyperDbgUninstallKdDriver();

INT
HyperDbgLoadKdModule();

INT
HyperDbgLoadVmmModule();

INT
HyperDbgLoadHyperTraceModule();

INT
HyperDbgLoadAllModules();

INT
HyperDbgStartKdDriver();

INT
HyperDbgStopKdDriver();

INT
HyperDbgInterpreter(CHAR * Command);

INT
HyperDbgScriptReadFileAndExecuteCommandline(INT argc, CHAR * argv[]);

GENERIC_PROCESSOR_VENDOR
HyperDbgGetProcessorVendor();

BOOLEAN
HyperDbgTestCommandParser(CHAR *   Command,
                          UINT32   NumberOfTokens,
                          CHAR **  TokensList,
                          UINT32 * FailedTokenNum,
                          UINT32 * FailedTokenPosition);

VOID
HyperDbgTestCommandParserShowTokens(CHAR * Command);

VOID
HyperDbgShowSignature();
