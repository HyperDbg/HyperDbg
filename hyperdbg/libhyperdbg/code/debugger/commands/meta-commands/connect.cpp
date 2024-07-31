/**
 * @file connect.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .connect command
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
extern BOOLEAN g_IsConnectedToRemoteDebugger;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;
extern string  g_ServerPort;
extern string  g_ServerIp;

/**
 * @brief help of the .connect command
 *
 * @return VOID
 */
VOID
CommandConnectHelp()
{
    ShowMessages(".connect : connects to a remote or local machine to start "
                 "debugging.\n\n");

    ShowMessages("syntax : \t.connect [local]\n");
    ShowMessages("syntax : \t.connect [Ip (string)] [Port (decimal)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .connect local\n");
    ShowMessages("\t\te.g : .connect 192.168.1.5 50000\n");
}

/**
 * @brief Connect to local debugger
 *
 * @return VOID
 */
VOID
ConnectLocalDebugger()
{
    g_IsConnectedToHyperDbgLocally = TRUE;
}

/**
 * @brief Connect to remote debugger
 *
 * @return BOOLEAN
 */
BOOLEAN
ConnectRemoteDebugger(const CHAR * Ip, const CHAR * Port)
{
    //
    // Validate IP and Port

    if (!ValidateIP(Ip))
    {
        return FALSE;
    }

    if (Port != NULL)
    {
        if (!IsNumber(Port) || stoi(Port) > 65535 || stoi(Port) < 0)
        {
            return FALSE;
        }

        //
        // connect to remote debugger
        //
        g_ServerIp   = Ip;
        g_ServerPort = Port;
        RemoteConnectionConnect(Ip, Port);
    }
    else
    {
        //
        // connect to remote debugger (default port)
        //
        g_ServerIp   = Ip;
        g_ServerPort = DEFAULT_PORT;
        RemoteConnectionConnect(Ip, DEFAULT_PORT);
    }

    return TRUE;
}

/**
 * @brief .connect command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandConnect(vector<CommandToken> CommandTokens, string Command)
{
    string Ip;
    string Port;

    if (g_IsConnectedToHyperDbgLocally || g_IsConnectedToRemoteDebuggee ||
        g_IsConnectedToRemoteDebugger)
    {
        ShowMessages("you're connected to a debugger, please use '.disconnect' "
                     "command\n");
        return;
    }

    if (g_IsSerialConnectedToRemoteDebuggee ||
        g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages("you're connected to a an instance of HyperDbg, please use "
                     "'.debug close' command\n");
        return;
    }

    if (CommandTokens.size() == 1)
    {
        //
        // Means that user entered just a connect so we have to
        // ask to connect to what ?
        //
        ShowMessages("incorrect use of the '.connect'\n\n");
        CommandConnectHelp();
        return;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "local") && CommandTokens.size() == 2)
    {
        //
        // connect to local debugger
        //
        ShowMessages("local debugging (vmi-mode)\n");

        ConnectLocalDebugger();

        return;
    }
    else if (CommandTokens.size() == 3 || CommandTokens.size() == 2)
    {
        Ip = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1));

        if (CommandTokens.size() == 3)
        {
            Port = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2));
        }

        //
        // means that probably wants to connect to a remote
        // system, let's first check the if the parameters are
        // valid
        //
        if (!ValidateIP(Ip))
        {
            ShowMessages("incorrect ip address\n");
            return;
        }

        if (CommandTokens.size() == 3)
        {
            if (!IsNumber(Port) || stoi(Port) > 65535 || stoi(Port) < 0)
            {
                ShowMessages("incorrect port\n");
                return;
            }

            //
            // connect to remote debugger
            //
            ConnectRemoteDebugger(Ip.c_str(), Port.c_str());
        }
        else
        {
            //
            // connect to remote debugger (default port)
            //
            ConnectRemoteDebugger(Ip.c_str(), NULL);
        }
    }
    else
    {
        ShowMessages("incorrect use of the '.connect'\n\n");
        CommandConnectHelp();
        return;
    }
}
