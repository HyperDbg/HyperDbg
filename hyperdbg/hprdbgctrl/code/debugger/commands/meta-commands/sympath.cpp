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
 * @brief help of .sympath command
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
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSympath(vector<string> SplittedCommand, string Command)
{
    inipp::Ini<char> Ini;
    inipp::Ini<char> Ini2;
    WCHAR            ConfigPath[MAX_PATH] = {0};
    string           Token;

    //
    // Get config file path
    //
    GetConfigFilePath(ConfigPath);

    if (SplittedCommand.size() == 1)
    {
        ifstream Is(ConfigPath);

        //
        // Show the current symbol path
        //

        //
        // Read config file
        //
        Ini.parse(Is);

        //
        // Show config file
        //
        // Ini.generate(std::cout);

        string SymbolServer = "";
        inipp::get_value(Ini.sections["DEFAULT"], "SymbolServer", SymbolServer);

        Is.close();

        if (!SymbolServer.empty())
        {
            ShowMessages("current symbol server is : %s\n", SymbolServer.c_str());
        }
        else
        {
            ShowMessages("symbol server is not configured, please use '.help .sympath'\n");
        }
    }
    else
    {
        //
        // Save the symbol path
        //

        //
        // Trim the command
        //
        Trim(Command);

        //
        // Remove .sympath from it
        //
        Command.erase(0, 8);

        //
        // Trim it again
        //
        Trim(Command);

        //
        // *** validate the symbols ***
        //

        //
        // Check if the string contains '*'
        //
        char Delimiter = '*';
        if (Command.find(Delimiter) != std::string::npos)
        {
            //
            // Found
            //
            Token = Command.substr(0, Command.find(Delimiter));
            // using transform() function and ::tolower in STL
            transform(Token.begin(), Token.end(), Token.begin(), ::tolower);

            //
            // Check if it starts with srv
            //
            if (!Token.compare("srv"))
            {
                ifstream Is(ConfigPath);

                //
                // Show the current symbol path
                //

                //
                // Read config file
                //
                Ini.parse(Is);

                Is.close();

                //
                // Save the config
                //
                Ini.sections["DEFAULT"]["SymbolServer"] = Command.c_str();
                Ini.interpolate();

                //
                // Test, show the config
                //
                // Ini.generate(std::cout);

                //
                // Save the config
                //
                ofstream Os(ConfigPath);

                Ini.generate(Os);

                Os.close();

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
