/**
 * @file settings.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief settings command
 * @details
 * @version 0.1
 * @date 2020-08-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_AutoUnpause;
extern BOOLEAN g_AutoFlush;
extern UINT32  g_DisassemblerSyntax;

/**
 * @brief help of settings command
 *
 * @return VOID
 */
VOID
CommandSettingsHelp()
{
    ShowMessages(
        "settings : query, set, or change a value for a sepcial settings.\n\n");
    ShowMessages("syntax : \tsettings [option name] [value (name | hex value | "
                 "on | off)]\n");
    ShowMessages("\t\te.g : settings autounpause\n");
    ShowMessages("\t\te.g : settings autounpause on\n");
    ShowMessages("\t\te.g : settings autounpause off\n");
    ShowMessages("\t\te.g : settings autoflush on\n");
    ShowMessages("\t\te.g : settings autoflush off\n");
    ShowMessages("\t\te.g : settings syntax intel\n");
    ShowMessages("\t\te.g : settings syntax att\n");
    ShowMessages("\t\te.g : settings syntax masm\n");
}

/**
 * @brief set the auto-flush mode to enabled and disable
 * and query the status of this mode
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID
CommandSettingsAutoFlush(vector<string> SplittedCommand)
{
    if (SplittedCommand.size() == 2)
    {
        //
        // It's a query
        //
        if (g_AutoFlush)
        {
            ShowMessages("auto-flush is enabled\n");
        }
        else
        {
            ShowMessages("auto-flush is disabled\n");
        }
    }
    else if (SplittedCommand.size() == 3)
    {
        //
        // The user tries to set a value as the autoflush
        //
        if (!SplittedCommand.at(2).compare("on"))
        {
            g_AutoFlush = TRUE;
            ShowMessages("set auto-flush to enabled\n");
        }
        else if (!SplittedCommand.at(2).compare("off"))
        {
            g_AutoFlush = FALSE;
            ShowMessages("set auto-flush to disabled\n");
        }
        else
        {
            //
            // Sth is incorrect
            //
            ShowMessages("incorrect use of 'settings', please use 'help settings' "
                         "for more details.\n");
            return;
        }
    }
    else
    {
        //
        // Sth is incorrect
        //
        ShowMessages("incorrect use of 'settings', please use 'help settings' "
                     "for more details.\n");
        return;
    }
}

/**
 * @brief set auto-unpause mode to enabled or disabled
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID
CommandSettingsAutoUpause(vector<string> SplittedCommand)
{
    if (SplittedCommand.size() == 2)
    {
        //
        // It's a query
        //
        if (g_AutoUnpause)
        {
            ShowMessages("auto-unpause is enabled\n");
        }
        else
        {
            ShowMessages("auto-unpause is disabled\n");
        }
    }
    else if (SplittedCommand.size() == 3)
    {
        //
        // The user tries to set a value as the autounpause
        //
        if (!SplittedCommand.at(2).compare("on"))
        {
            g_AutoUnpause = TRUE;
            ShowMessages("set auto-unpause to enabled\n");
        }
        else if (!SplittedCommand.at(2).compare("off"))
        {
            g_AutoUnpause = FALSE;
            ShowMessages("set auto-unpause to disabled\n");
        }
        else
        {
            //
            // Sth is incorrect
            //
            ShowMessages("incorrect use of 'settings', please use 'help settings' "
                         "for more details.\n");
            return;
        }
    }
    else
    {
        //
        // Sth is incorrect
        //
        ShowMessages("incorrect use of 'settings', please use 'help settings' "
                     "for more details.\n");
        return;
    }
}

/**
 * @brief set the syntax of !u !u2 u u2 command
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID
CommandSettingsSyntax(vector<string> SplittedCommand)
{
    if (SplittedCommand.size() == 2)
    {
        //
        // It's a query
        //
        if (g_DisassemblerSyntax == 1)
        {
            ShowMessages("disassembler syntax is : intel\n");
        }
        else if (g_DisassemblerSyntax == 2)
        {
            ShowMessages("disassembler syntax is : at&t\n");
        }
        else if (g_DisassemblerSyntax == 3)
        {
            ShowMessages("disassembler syntax is : masm\n");
        }
        else
        {
            ShowMessages("unknown syntax\n");
        }
    }
    else if (SplittedCommand.size() == 3)
    {
        //
        // The user tries to set a value as the syntax
        //
        if (!SplittedCommand.at(2).compare("intel"))
        {
            g_DisassemblerSyntax = 1;
            ShowMessages("set syntax to intel\n");
        }
        else if (!SplittedCommand.at(2).compare("att") ||
                 !SplittedCommand.at(2).compare("at&t"))
        {
            g_DisassemblerSyntax = 2;
            ShowMessages("set syntax to at&t\n");
        }
        else if (!SplittedCommand.at(2).compare("masm"))
        {
            g_DisassemblerSyntax = 3;
            ShowMessages("set syntax to masm\n");
        }
        else
        {
            //
            // Sth is incorrect
            //
            ShowMessages("incorrect use of 'settings', please use 'help settings' "
                         "for more details.\n");
            return;
        }
    }
    else
    {
        //
        // Sth is incorrect
        //
        ShowMessages("incorrect use of 'settings', please use 'help settings' "
                     "for more details.\n");
        return;
    }
}

/**
 * @brief settings command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSettings(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() <= 1)
    {
        ShowMessages("incorrect use of 'settings'\n\n");
        CommandSettingsHelp();
        return;
    }

    //
    // Interpret the field name
    //
    if (!SplittedCommand.at(1).compare("autounpause"))
    {
        CommandSettingsAutoUpause(SplittedCommand);
    }
    else if (!SplittedCommand.at(1).compare("syntax"))
    {
        CommandSettingsSyntax(SplittedCommand);
    }
    else if (!SplittedCommand.at(1).compare("autoflush"))
    {
        CommandSettingsAutoFlush(SplittedCommand);
    }
    else
    {
        //
        // optionm not found
        //
        ShowMessages("incorrect use of 'settings', please use 'help settings' "
                     "for more details.\n");
        return;
    }
}
