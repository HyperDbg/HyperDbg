/**
 * @file hyperdbg-cli.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Main HyperDbg Cli source coede
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <Windows.h>
#include <string>
#include <conio.h>
#include <iostream>
#include <vector>

#include "SDK/HyperDbgSdk.h"
#include "SDK/Imports/HyperDbgCtrlImports.h"

using namespace std;

#pragma comment(lib, "HPRDBGCTRL.lib")

/**
 * @brief CLI main function
 *
 * @param argc
 * @param argv
 * @return int
 */
int
main(int argc, char * argv[])
{
    bool           ExitFromDebugger = false;
    string         PreviousCommand;
    bool           Reset = false;
    vector<string> Args;

    printf("HyperDbg Debugger [version: %s, build: %s]\n", CompleteVersion, BuildVersion);
    printf("Please visit https://docs.hyperdbg.org for more information...\n");
    printf("HyperDbg is released under the GNU Public License v3 (GPLv3).\n\n");

    if (argc != 1)
    {
        //
        // User-passed arguments to the debugger
        //
        if (!strcmp(argv[1], "--script"))
        {
            //
            //
            //
            for (size_t i = 2; i < argc; i++)
            {
                std::string TempStr(argv[i]);
                Args.push_back(TempStr);
            }

            //
            // Check if the target path and args for script is not empty
            //
            if (!Args.empty())
            {
                HyperDbgScriptReadFileAndExecuteCommand(Args);
                printf("\n");
            }
            else
            {
                printf("err, invalid command line options passed to the HyperDbg!\n");
                return 1;
            }
        }
        else
        {
            printf("err, invalid command line options passed to the HyperDbg!\n");
            return 1;
        }
    }

    while (!ExitFromDebugger)
    {
        HyperDbgShowSignature();

        string CurrentCommand = "";

        //
        // Clear multiline
        //
        Reset = true;

    GetMultiLinecCommand:

        string TempCommand = "";

        getline(cin, TempCommand);

        if (cin.fail() || cin.eof())
        {
            cin.clear(); // reset cin state

            printf("\n\n");

            //
            // probably sth like CTRL+C pressed
            //
            continue;
        }

        //
        // Check for multi-line commands
        //
        if (HyperDbgCheckMultilineCommand(TempCommand, Reset) == true)
        {
            //
            // It's a multi-line command
            //
            Reset = false;

            //
            // Save the command with a space separator
            //
            CurrentCommand += TempCommand + "\n";

            //
            // Show a small signature
            //
            printf("> ");

            //
            // Get next command
            //
            goto GetMultiLinecCommand;
        }
        else
        {
            //
            // Reset for future commands
            //
            Reset = true;

            //
            // Either the multi-line is finished or it's a
            // single line command
            //
            CurrentCommand += TempCommand;
        }

        if (!CurrentCommand.compare("") &&
            HyperDbgContinuePreviousCommand())
        {
            //
            // Retry the previous command
            //
            CurrentCommand = PreviousCommand;
        }
        else
        {
            //
            // Save previous command
            //
            PreviousCommand = CurrentCommand;
        }

        int CommandExecutionResult = HyperDbgInterpreter((char *)CurrentCommand.c_str());

        //
        // if the debugger encounters an exit state then the return will be 1
        //
        if (CommandExecutionResult == 1)
        {
            //
            // Exit from the debugger
            //
            ExitFromDebugger = true;
        }
        if (CommandExecutionResult != 2)
        {
            printf("\n");
        }
    }

    return 0;
}
