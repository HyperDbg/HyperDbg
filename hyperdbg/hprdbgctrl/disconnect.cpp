/**
 * @file disconnect.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .disconnect command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern HANDLE  g_RemoteDebuggeeListeningThread;

/**
 * @brief help of .disconnect command
 *
 * @return VOID
 */
VOID
CommandDisconnectHelp()
{
    ShowMessages(".disconnect : disconnect from a debugging session (it won't "
                 "unload the modules).\n\n");
    ShowMessages("syntax : \t.disconnect\n");
}

/**
 * @brief .disconnect command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDisconnect(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.disconnect'\n\n");
        CommandDisconnectHelp();
        return;
    }

    if (!g_IsConnectedToHyperDbgLocally && !g_IsConnectedToRemoteDebuggee)
    {
        ShowMessages("you're not connected to any instance of HyperDbg, did you "
                     "use '.connect'? \n");
        return;
    }

    //
    // Disconnect the session
    //
    g_IsConnectedToHyperDbgLocally = FALSE;

    //
    // This computer is connected to a remote system
    //
    if (g_IsConnectedToRemoteDebuggee)
    {
        //
        // We should kill the thread that was listening for the
        // remote commands and close the connection
        //
        TerminateThread(g_RemoteDebuggeeListeningThread, 0);
        CloseHandle(g_RemoteDebuggeeListeningThread);

        RemoteConnectionCloseTheConnectionWithDebuggee();

        g_IsConnectedToRemoteDebuggee = FALSE;
    }

    ShowMessages("successfully disconnected\n");
}
