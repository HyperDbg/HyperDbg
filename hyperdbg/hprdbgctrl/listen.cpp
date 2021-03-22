/**
 * @file listen.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .listen command
 * @details
 * @version 0.1
 * @date 2020-08-21
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
extern BOOLEAN g_IsConnectedToRemoteDebugger;

/**
 * @brief help of listen command
 *
 * @return VOID
 */
VOID
CommandListenHelp()
{
    ShowMessages(".listen : listen for a client to connect to HyperDbg (works as "
                 "a guest server).\n\n");

    ShowMessages("note : \tif you don't specify port then HyperDbg uses the "
                 "default port (%s)\n",
                 DEFAULT_PORT);

    ShowMessages("syntax : \t.listen [port]\n");
    ShowMessages("\t\te.g : .listen\n");
    ShowMessages("\t\te.g : .listen 50000\n");
}

/**
 * @brief listen command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandListen(vector<string> SplittedCommand, string Command)
{
    string port;

    if (SplittedCommand.size() >= 3)
    {
        //
        // Means that user entered invalid parameters
        //
        ShowMessages("incorrect use of '.listen'\n\n");
        CommandListenHelp();
        return;
    }

    if (g_IsConnectedToHyperDbgLocally || g_IsConnectedToRemoteDebuggee ||
        g_IsConnectedToRemoteDebugger)
    {
        ShowMessages("you're connected to a debugger, please use '.disconnect' "
                     "command.\n");
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        //
        // listen on default port
        //
        ShowMessages("listening on %s ...\n", DEFAULT_PORT);
        RemoteConnectionListen(DEFAULT_PORT);

        return;
    }
    else if (SplittedCommand.size() == 2)
    {
        port = SplittedCommand.at(1);

        //
        // means that probably wants to listen
        // on a specific port, let's see if the
        // port is valid or not
        //
        if (!IsNumber(port) || stoi(port) > 65535 || stoi(port) < 0)
        {
            ShowMessages("incorrect port\n");
            return;
        }

        //
        // listen on the port
        //
        ShowMessages("listening on %s ...\n", port.c_str());
        RemoteConnectionListen(port.c_str());
    }
    else
    {
        ShowMessages("incorrect use of '.listen'\n\n");
        CommandListenHelp();
        return;
    }
}
