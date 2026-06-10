/**
 * @file core.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief show and change processor
 * @details
 * @version 0.1
 * @date 2021-01-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ULONG   g_CurrentRemoteCore;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the ~ command
 *
 * @return VOID
 */
VOID
CommandCoreHelp()
{
    ShowMessages("~ : shows and changes the operating processor.\n\n");

    ShowMessages("syntax : \t~\n");
    ShowMessages("syntax : \t~ [CoreNumber (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : ~ \n");
    ShowMessages("\t\te.g : ~ 2 \n");
}

/**
 * @brief ~ command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandCore(vector<CommandToken> CommandTokens, string Command)
{
    UINT32 TargetCore = 0;

    if (CommandTokens.size() != 1 && CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandCoreHelp();
        return;
    }

    //
    // Check if it's connected to a remote debuggee or not
    //
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, you're not connected to any debuggee\n");
        return;
    }

    if (CommandTokens.size() == 1)
    {
        ShowMessages("current processor : 0x%x\n", g_CurrentRemoteCore);
    }
    else if (CommandTokens.size() == 2)
    {
        if (!ConvertTokenToUInt32(CommandTokens.at(1), &TargetCore))
        {
            ShowMessages("please specify a correct hex value for the core that you "
                         "want to operate on it\n\n");
            CommandCoreHelp();
            return;
        }

        //
        // Send the changing core packet
        //
        KdSendSwitchCorePacketToDebuggee(TargetCore);
    }
}
