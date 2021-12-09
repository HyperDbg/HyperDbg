/**
 * @file hyperdbg-cli.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
#include "Definition.h"
#include "Configuration.h"

#pragma comment(lib, "HPRDBGCTRL.lib")

using namespace std;

//
// Header file of HPRDBGCTRL
// Imports
//
extern "C" {
__declspec(dllimport) int HyperdbgLoadVmm();
__declspec(dllimport) int HyperdbgUnload();
__declspec(dllimport) int HyperdbgInstallVmmDriver();
__declspec(dllimport) int HyperdbgUninstallDriver();
__declspec(dllimport) int HyperdbgStopDriver();
__declspec(dllimport) int HyperdbgInterpreter(char * Command);
__declspec(dllimport) void HyperdbgShowSignature();
__declspec(dllimport) bool HyperdbgContinuePreviousCommand();
__declspec(dllimport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
__declspec(dllimport) void HyperdbgSetTextMessageCallback(Callback handler);
}

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
    bool   ExitFromDebugger = false;
    string PreviousCommand;
    bool   Reset = false;

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
            char ScriptBuffer[MAX_PATH + 10] = {0};

            //
            // Executing the script
            //
            sprintf_s(ScriptBuffer, sizeof(ScriptBuffer), ".script %s", argv[2]);
            HyperdbgInterpreter(ScriptBuffer);
            printf("\n");
        }
        else
        {
            printf("invalid command line options passed to HyperDbg !\n");
            return 1;
        }
    }

    while (!ExitFromDebugger)
    {
        HyperdbgShowSignature();

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
            HyperdbgContinuePreviousCommand())
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

        int CommandExecutionResult = HyperdbgInterpreter((char *)CurrentCommand.c_str());

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
