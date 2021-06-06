/**
 * @file sympath.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
    ShowMessages(".sympath : show and set the symbol server and path.\n\n");

    ShowMessages("syntax : \t.sympath [server, path]\n");
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
    ifstream         Is(CONFIG_FILE_NAME);
    ofstream         Os(CONFIG_FILE_NAME);
    string           SymbolServer = "";

    if (SplittedCommand.size() == 1)
    {
        //
        // Show the current symbol path
        //

        //
        // Read config file
        //
        Ini.parse(Is);

        inipp::get_value(Ini.sections["DEFAULT"], "SymbolServer", SymbolServer);

        Is.close();

        ShowMessages("current SymbolServer is : %s\n", SymbolServer.c_str());
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
        // Open file
        //

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
        Ini.generate(Os);

        Os.close();
    }
}
