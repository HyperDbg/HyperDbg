/**
 * @file sym.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .sym command
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
extern PMODULE_SYMBOL_DETAIL g_SymbolTable;
extern UINT32                g_SymbolTableSize;

/**
 * @brief help of .sym command
 *
 * @return VOID
 */
VOID
CommandSymHelp()
{
    ShowMessages(".sym : perfrom the symbol actions.\n\n");

    ShowMessages("syntax : \t.sym [ table | reload | load | unload ] [base (hex address)] "
                 "[path (string path to pdb)]\n");
    ShowMessages("\t\te.g : .sym table\n");
    ShowMessages("\t\te.g : .sym reload\n");
    ShowMessages("\t\te.g : .sym download\n");
    ShowMessages("\t\te.g : .sym load base fffff8077356000 path c:\\symbols\\my_dll.pdb\n");
    ShowMessages("\t\te.g : .sym unload\n");
    ShowMessages("\t\te.g : .sym unload win32k\n");
}

/**
 * @brief .sym command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSym(vector<string> SplittedCommand, string Command)
{
    WCHAR            ConfigPath[MAX_PATH] = {0};
    inipp::Ini<char> Ini;
    string           SymbolServer = "";
    UINT64           BaseAddress  = NULL;
    string           Delimiter    = "";
    string           PathToPdb    = "";

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of '.sym'\n\n");
        CommandSymHelp();
        return;
    }

    if (!SplittedCommand.at(1).compare("table"))
    {
        //
        // Validate params
        //
        if (SplittedCommand.size() != 2)
        {
            ShowMessages("incorrect use of '.sym'\n\n");
            CommandSymHelp();
            return;
        }

        //
        // Check if we found an already built symbol table
        //
        if (g_SymbolTable != NULL)
        {
            free(g_SymbolTable);

            g_SymbolTable     = NULL;
            g_SymbolTableSize = NULL;
        }

        //
        // Build the symbol table
        //
        SymbolBuildSymbolTable(&g_SymbolTable, &g_SymbolTableSize);

        //
        // Test, should be removed
        // show packet details
        //
        for (size_t i = 0; i < g_SymbolTableSize / sizeof(MODULE_SYMBOL_DETAIL); i++)
        {
            ShowMessages("is pdb details available? : %s\n", g_SymbolTable[i].IsSymbolDetailsFound ? "true" : "false");
            ShowMessages("is pdb a path instead of module name? : %s\n", g_SymbolTable[i].IsLocalSymbolPath ? "true" : "false");
            ShowMessages("base address : %llx\n", g_SymbolTable[i].BaseAddress);
            ShowMessages("file path : %s\n", g_SymbolTable[i].FilePath);
            ShowMessages("guid and age : %s\n", g_SymbolTable[i].ModuleSymbolGuidAndAge);
            ShowMessages("module symbol path/name : %s\n", g_SymbolTable[i].ModuleSymbolPath);
            ShowMessages("========================================================================\n");
        }
    }
    else if (!SplittedCommand.at(1).compare("reload") || !SplittedCommand.at(1).compare("download"))
    {
        //
        // Validate params
        //
        if (SplittedCommand.size() != 2)
        {
            ShowMessages("incorrect use of '.sym'\n\n");
            CommandSymHelp();
            return;
        }

        //
        // Check if we found an already built symbol table
        //
        if (g_SymbolTable != NULL)
        {
            free(g_SymbolTable);

            g_SymbolTable     = NULL;
            g_SymbolTableSize = NULL;
        }

        //
        // Build the symbol table
        //
        SymbolBuildSymbolTable(&g_SymbolTable, &g_SymbolTableSize);

        //
        // *** Read symbol path/server from config file ***
        //

        //
        // Get config file path
        //
        GetConfigFilePath(ConfigPath);

        if (!IsFileExistW(ConfigPath))
        {
            ShowMessages("please configure the symbol path before using this command. use 'help .sympath' for more information\n");
            return;
        }

        ifstream Is(ConfigPath);

        //
        // Read config file
        //
        Ini.parse(Is);

        //
        // Show config file
        //
        // Ini.generate(std::cout);

        inipp::get_value(Ini.sections["DEFAULT"], "SymbolServer", SymbolServer);

        Is.close();

        if (SymbolServer.empty())
        {
            ShowMessages("err, invalid config for symbol server/path\n");
            return;
        }

        //
        // Load and download available symbols
        //
        if (!SplittedCommand.at(1).compare("reload"))
        {
            ScriptEngineSymbolInitLoadWrapper(g_SymbolTable, g_SymbolTableSize, FALSE, SymbolServer.c_str());
        }
        else if (!SplittedCommand.at(1).compare("download"))
        {
            ScriptEngineSymbolInitLoadWrapper(g_SymbolTable, g_SymbolTableSize, TRUE, SymbolServer.c_str());
        }

        ShowMessages("symbol table successfully updated\n");
    }
    else if (!SplittedCommand.at(1).compare("unload"))
    {
        //
        // Validate params
        //
        if (SplittedCommand.size() != 2 && SplittedCommand.size() != 3)
        {
            ShowMessages("incorrect use of '.sym'\n\n");
            CommandSymHelp();
            return;
        }

        if (SplittedCommand.size() == 2)
        {
            //
            // unload without any parameters, means that unload
            // all the symbols
            //
            ScriptEngineUnloadAllSymbolsWrapper();
        }
        else
        {
            //
            // Size is 3 there is module name
            //
            ScriptEngineUnloadModuleSymbolWrapper((char *)SplittedCommand.at(2).c_str());
        }
    }
    else if (!SplittedCommand.at(1).compare("load"))
    {
        //
        // Validate params
        //
        if (SplittedCommand.size() < 6)
        {
            ShowMessages("incorrect use of '.sym'\n\n");
            CommandSymHelp();
            return;
        }

        if (!SplittedCommand.at(2).compare("base"))
        {
            if (!ConvertStringToUInt64(SplittedCommand.at(3), &BaseAddress))
            {
                ShowMessages("please add a valid hex address to be used as the base address\n\n");
                CommandSymHelp();
                return;
            }

            //
            // Base address is now valid, check if next parameter is path
            //
            if (SplittedCommand.at(4).compare("path"))
            {
                ShowMessages("incorrect use of '.sym'\n\n");
                CommandSymHelp();
                return;
            }

            //
            // The rest of command is pdb path
            //
            Delimiter = "path ";
            PathToPdb = Command.substr(Command.find(Delimiter) + 5, Command.size());

            //
            // Check if pdb file exists or not
            //
            if (!IsFileExistA(PathToPdb.c_str()))
            {
                ShowMessages("pdb file not found\n");
                return;
            }

            ShowMessages("loading module symbol at '%s'\n", PathToPdb.c_str());

            //
            // Load the pdb file (the validation of pdb file is checked into pdb
            // parsing functions)
            //
            ScriptEngineLoadFileSymbolWrapper(BaseAddress, PathToPdb.c_str());
        }
        else
        {
            ShowMessages("incorrect use of '.sym'\n\n");
            CommandSymHelp();
            return;
        }
    }
    else
    {
        ShowMessages("unknown parameter at '%s'\n\n", SplittedCommand.at(1).c_str());
        CommandSymHelp();
        return;
    }
}
