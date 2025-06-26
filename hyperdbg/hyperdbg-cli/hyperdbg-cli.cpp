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

//
// Environment headers
//
#include "platform/user/header/Environment.h"

#include <Windows.h>
#include <string>
#include <conio.h>
#include <iostream>
#include <vector>

#include "SDK/HyperDbgSdk.h"
#include "SDK/imports/user/HyperDbgLibImports.h"

using namespace std;

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
    BOOLEAN exit_from_debugger = FALSE;
    string  previous_command;
    BOOLEAN reset = FALSE;

    //
    // Set console output code page to UTF-8
    //
    SetConsoleOutputCP(CP_UTF8);

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
            // Handle the script
            //
            hyperdbg_u_script_read_file_and_execute_commandline(argc, argv);
        }
        else
        {
            printf("err, invalid command line options passed to the HyperDbg!\n");
            return 1;
        }
    }

    while (!exit_from_debugger)
    {
        hyperdbg_u_show_signature();

        string current_command = "";

        //
        // Clear multiline
        //
        reset = TRUE;

    GetMultiLinecCommand:

        string temp_command = "";

        getline(cin, temp_command);

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
        if (hyperdbg_u_check_multiline_command((CHAR *)temp_command.c_str(), reset) == TRUE)
        {
            //
            // It's a multi-line command
            //
            reset = FALSE;

            //
            // Save the command with a space separator
            //
            current_command += temp_command + "\n";

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
            reset = TRUE;

            //
            // Either the multi-line is finished or it's a
            // single line command
            //
            current_command += temp_command;
        }

        if (!current_command.compare("") && hyperdbg_u_continue_previous_command())
        {
            //
            // Retry the previous command
            //
            current_command = previous_command;
        }
        else
        {
            //
            // Save previous command
            //
            previous_command = current_command;
        }

        INT CommandExecutionResult = hyperdbg_u_run_command((CHAR *)current_command.c_str());

        //
        // if the debugger encounters an exit state then the return will be 1
        //
        if (CommandExecutionResult == 1)
        {
            //
            // Exit from the debugger
            //
            exit_from_debugger = true;
        }
        if (CommandExecutionResult != 2)
        {
            printf("\n");
        }
    }

    return 0;
}
