/**
 * @file sympath.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .sympath command
 * @details
 * @version 0.1
 * @date 2021-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//

/**
 * @brief help of the .sympath command
 *
 * @return VOID
 */
VOID
CommandSympathHelp()
{
    ShowMessages(".sympath : shows and sets the symbol server and path.\n\n");

    ShowMessages("syntax : \t.sympath\n");
    ShowMessages("syntax : \t.sympath [SymServer (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .sympath\n");
    ShowMessages("\t\te.g : .sympath SRV*c:\\Symbols*https://msdl.microsoft.com/download/symbols \n");
}

/**
 * @brief .sympath command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandSympath(vector<CommandToken> CommandTokens)
{
    string SymbolServer = "";
    string Token;

    if (CommandTokens.size() == 1)
    {
        //
        // Show the current symbol path
        //
        if (!CommandSettingsGetValueFromConfigFile("SymbolServer", SymbolServer))
        {
            ShowMessages("symbol server is not configured, please use '.help .sympath'\n");
        }
        else
        {
            ShowMessages("current symbol server is : %s\n", SymbolServer.c_str());
        }
    }
    else if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandSympathHelp();
        return;
    }
    else
    {
        //
        // Save the symbol path
        //

        std::string TargetSymPath = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1));

        //
        // *** validate the symbols ***
        //

        //
        // Check if the string contains '*'
        //
        char Delimiter = '*';
        if (TargetSymPath.find(Delimiter) != std::string::npos)
        {
            //
            // Found
            //
            Token = TargetSymPath.substr(0, TargetSymPath.find(Delimiter));

            //
            // using transform() function and ::tolower in STL
            //
            transform(Token.begin(), Token.end(), Token.begin(), ::tolower);

            //
            // Check if it starts with srv
            //
            if (!Token.compare("srv"))
            {
                //
                // Save the config
                //
                CommandSettingsSetValueFromConfigFile("SymbolServer", TargetSymPath);

                //
                // Show the message
                //
                ShowMessages("symbol server/path is configured successfully\n");
                ShowMessages("use '.sym load', '.sym reload', or '.sym download' to load pdb files\n");
            }
            else
            {
                ShowMessages("symbol path is invalid\n\n");
                CommandSympathHelp();
                return;
            }
        }
        else
        {
            ShowMessages("symbol path is invalid\n\n");
            CommandSympathHelp();
            return;
        }
    }
}
