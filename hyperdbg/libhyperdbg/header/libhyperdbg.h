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

INT
HyperDbgCreateHandleFromVmmModule();

INT
HyperDbgUnloadVmm();

INT
HyperDbgInstallVmmDriver();

INT
HyperDbgUninstallVmmDriver();

INT
HyperDbgLoadVmmModule();

INT
HyperDbgStopVmmDriver();

INT
HyperDbgInterpreter(CHAR * Command);

BOOLEAN
HyperDbgTestCommandParser(CHAR *   Command,
                          UINT32   NumberOfTokens,
                          CHAR **  TokensList,
                          UINT32 * FailedTokenNum,
                          UINT32 * FailedTokenPosition);

VOID
HyperDbgTestCommandParserShowTokens(CHAR * Command);

INT
ScriptReadFileAndExecuteCommandline(INT argc, CHAR * argv[]);

VOID
HyperDbgShowSignature();

VOID
SetTextMessageCallback(PVOID Handler);

PVOID
SetTextMessageCallbackUsingSharedBuffer(PVOID Handler);

VOID
UnsetTextMessageCallback();
