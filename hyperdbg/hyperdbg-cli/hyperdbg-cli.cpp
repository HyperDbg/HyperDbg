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
#include "details.h"

#pragma comment(lib, "HPRDBGCTRL.lib")

using namespace std;

//
// Global variables
//
bool IsOnString                    = false;
bool IsPreviousCharacterABackSlash = false;
int  CountOfOpenCurlyBrackets      = 0;

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
__declspec(dllimport) void HyperdbgSetTextMessageCallback(Callback handler);
}

/**
 * @brief check for multi-line commands
 * 
 * @param CurrentCommand 
 * @return bool return true if the command needs extra input, otherwise
 * return false
 */
bool
CheckMultilineCommand(std::string CurrentCommand)
{
    for (size_t i = 0; i < CurrentCommand.length(); i++)
    {
        switch (CurrentCommand.at(i))
        {
        case '"':

            if (IsPreviousCharacterABackSlash)
            {
                IsPreviousCharacterABackSlash = false;
                break; // it's an escaped \" double-quote
            }

            if (IsOnString)
                IsOnString = false;
            else
                IsOnString = true;

            break;

        case '{':

            if (IsPreviousCharacterABackSlash)
                IsPreviousCharacterABackSlash = false;

            if (!IsOnString)
                CountOfOpenCurlyBrackets++;

            break;

        case '}':

            if (IsPreviousCharacterABackSlash)
                IsPreviousCharacterABackSlash = false;

            if (!IsOnString && CountOfOpenCurlyBrackets > 0)
                CountOfOpenCurlyBrackets--;

            break;

        case '\\':

            if (IsPreviousCharacterABackSlash)
                IsPreviousCharacterABackSlash = false; // it's not a escape character (two backslashes \\ )
            else
                IsPreviousCharacterABackSlash = true;

            break;

        default:

            if (IsPreviousCharacterABackSlash)
                IsPreviousCharacterABackSlash = false;

            break;
        }
    }

    if (IsOnString == FALSE && CountOfOpenCurlyBrackets == 0)
    {
        //
        // either the command is finished or it's a single
        // line command
        //
        return false;
    }
    else
    {
        //
        // There still other lines, this command is incomplete
        //
        return true;
    }
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

    printf("HyperDbg Debugger [core version: v%s]\n", Version);
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

        string CurrentCommand         = "";
        IsOnString                    = false;
        IsPreviousCharacterABackSlash = false;
        CountOfOpenCurlyBrackets      = 0;

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
        if (CheckMultilineCommand(TempCommand) == true)
        {
            //
            // It's a multi-line command
            //

            //
            // Save the command with a space separator
            //
            CurrentCommand += TempCommand + " ";

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
