/**
 * @file restart.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .restart command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT32                   g_ProcessIdOfLatestStartingProcess;
extern std::wstring             g_StartCommandPath;
extern std::wstring             g_StartCommandPathAndArguments;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
;

/**
 * @brief help of the .restart command
 *
 * @return VOID
 */
VOID
CommandRestartHelp()
{
    ShowMessages(".restart : restarts the previously started process "
                 "(using '.start' command).\n\n");

    ShowMessages(
        "syntax : \t.restart \n");
}

/**
 * @brief .restart command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandRestart(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandRestartHelp();
        return;
    }

    //
    // Check if the .start command is previously called or not
    //
    if (g_StartCommandPath.empty())
    {
        ShowMessages("nothing to restart, did you use the '.start' command before?\n");
        return;
    }

    //
    // Check to kill the current active process (if exists)
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // kill the process, we will restart the process even if we didn't
        // successfully killed the active process
        //
        UdKillProcess(g_ActiveProcessDebuggingState.ProcessId);
    }
    else if (g_ProcessIdOfLatestStartingProcess != NULL)
    {
        UdKillProcess(g_ProcessIdOfLatestStartingProcess);

        //
        // No longer the last process exists
        //
        g_ProcessIdOfLatestStartingProcess = NULL;
    }

    //
    // Perform run of the target file
    //
    if (g_StartCommandPathAndArguments.empty())
    {
        //
        // TEMPORARY LINUX SHIM (same as in pe.cpp/dump.cpp): the path is a
        // std::wstring of native wchar_t (4 bytes on Linux), but UdAttachToProcess
        // takes a 2-byte HyperDbg WCHAR * (UINT16). The cast is a bogus 2-byte
        // reinterpretation on Linux, acceptable only because the user-debugger
        // path is still stubbed there. On Windows WCHAR == wchar_t, so it is a
        // plain correct pointer. TODO(Linux): convert wchar_t -> 2-byte WCHAR
        // properly once the Linux user-debugger path is real.
        //
        UdAttachToProcess(NULL,
                          (WCHAR *)g_StartCommandPath.c_str(),
                          NULL,
                          FALSE);
    }
    else
    {
        //
        // TEMPORARY LINUX SHIM (same as in pe.cpp/dump.cpp): the path/arguments
        // are std::wstring of native wchar_t (4 bytes on Linux), but
        // UdAttachToProcess takes 2-byte HyperDbg WCHAR * (UINT16). The casts are
        // a bogus 2-byte reinterpretation on Linux, acceptable only because the
        // user-debugger path is still stubbed there. On Windows WCHAR == wchar_t,
        // so they are plain correct pointers. TODO(Linux): convert wchar_t ->
        // 2-byte WCHAR properly once the Linux user-debugger path is real.
        //
        UdAttachToProcess(NULL,
                          (WCHAR *)g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str(),
                          FALSE);
    }
}
