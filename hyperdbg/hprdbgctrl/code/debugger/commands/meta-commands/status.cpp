/**
 * @file status.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .status command
 * @details
 * @version 0.1
 * @date 2020-08-23
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
 * @brief help of .status command
 *
 * @return VOID
 */
VOID
CommandStatusHelp()
{
    ShowMessages(".status | status : gets the status of current debugger in local "
                 "system (if you connected to a remote system then '.status' "
                 "shows the state of current debugger, while 'status' shows the "
                 "state of remote debuggee).\n\n");

    ShowMessages("syntax : \t.status\n");
    ShowMessages("syntax : \tstatus\n");
}

/**
 * @brief .status and status command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandStatus(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.status'\n\n");
        CommandStatusHelp();
    }

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Connected to a remote debugger (serial port)
        //
        ShowMessages("remote debugging - debugger ('debugger mode')\n");
    }
    else if (g_IsSerialConnectedToRemoteDebugger)
    {
        //
        // Connected to a remote debuggee (serial port)
        //
        ShowMessages("remote debugging - debuggee ('debugger mode')\n");
    }
    else if (g_IsConnectedToRemoteDebuggee)
    {
        //
        // Connected to a remote debuggee
        //
        ShowMessages("remote debugging ('vmi mode'), ip : %s:%s \n",
                     g_ServerIp.c_str(),
                     g_ServerPort.c_str());
    }
    else if (g_IsConnectedToHyperDbgLocally)
    {
        //
        // Connected to a local system
        //
        ShowMessages("local debugging ('vmi mode')\n");
    }
    else if (g_IsConnectedToRemoteDebugger)
    {
        //
        // It's computer connect to a remote machine (this
        // message shouldn't be showed)
        //
        ShowMessages("a remote debugger connected to this system in ('vmi "
                     "mode'), ip : %s:%s \n",
                     g_ServerIp.c_str(),
                     g_ServerPort.c_str());
    }
    else
    {
        //
        // we never should see this message
        //
        ShowMessages("err, you're not connected to any instance of HyperDbg\n");
    }
}
