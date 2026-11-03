/**
 * @file hyperdbg-cli.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Main HyperDbg Cli source code
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#ifdef _WIN32
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

//
// Include the editor's STL headers before the SDK. WdkTypes.h defines the
// __in/__out SAL macros as empty macros on Linux, and libstdc++ 16 uses those
// identifiers internally in <memory>.
//
#include "line-editor.h"

#include "SDK/HyperDbgSdk.h"
#include "SDK/imports/user/HyperDbgLibImports.h"

using namespace std;

//
// The editor lives for the whole process so that the history is still saved
// when a command calls exit(0) (e.g. the 'exit' command), which bypasses
// automatic objects in main(). LineEditor's destructor saves the history.
//
static hyperdbg::LineEditor g_LineEditor;

/**
 * @brief CLI main function
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @return int zero on success, 1 on failure
 */
int
main(int argc, char * argv[])
{
    BOOLEAN ExitFromDebugger = FALSE;
    string  PreviousCommand;

    //
    // Set console output code page to UTF-8
    //
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    printf("HyperDbg Debugger [version: %s, build: %s]\n", CompleteVersion, BuildVersion);
    printf("Please visit https://docs.hyperdbg.org for more information...\n");
    printf("HyperDbg is released under the GNU Public License v3 (GPLv3).\n\n");

#if BETA_VERSION == 1
    printf("Notice: This is a beta release and may contain bugs or stability issues. ");
    printf("If you encounter any problems, please report them and consider using the previous stable release.\n\n");
#endif

    //
    // Load the persistent command history. Failure is not fatal: the current
    // session still gets history, it just will not be restored next time.
    // The editor saves it automatically when the process exits.
    //
    const std::filesystem::path HistoryPath = hyperdbg::DefaultHistoryFilePath();
    g_LineEditor.SetHistoryPath(HistoryPath);
    g_LineEditor.LoadHistory(HistoryPath);

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

    const string ContinuationPrompt = "> ";

    while (!ExitFromDebugger)
    {
        CHAR Signature[MAXIMUM_PROMPT_LENGTH] = {0};

        //
        // The Signature changes after commands such as .connect, .attach and
        // .process, so resolve it again before every command.
        //
        hyperdbg_u_get_signature(Signature, static_cast<UINT32>(sizeof(Signature)));

        const string InitialPrompt = Signature;
        string       CurrentCommand;
        BOOLEAN      Reset     = TRUE;
        BOOLEAN      Cancelled = FALSE;
        BOOLEAN      EndOfFile = FALSE;

        for (;;)
        {
            string                       TempCommand;
            const string &               Prompt     = Reset ? InitialPrompt : ContinuationPrompt;
            hyperdbg::LINE_EDITOR_RESULT ReadResult = g_LineEditor.ReadLine(Prompt, TempCommand);

            if (ReadResult == hyperdbg::LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE ||
                ReadResult == hyperdbg::LINE_EDITOR_RESULT::LINE_EDITOR_ERROR)
            {
                EndOfFile = TRUE;
                break;
            }

            if (ReadResult == hyperdbg::LINE_EDITOR_RESULT::LINE_EDITOR_CANCELLED)
            {
                //
                // Abandon the whole command. Reset the multiline parser as
                // well; the user may have pressed Ctrl+C in the middle of a
                // string or an open curly brace.
                //
                hyperdbg_u_check_multiline_command((CHAR *)"", TRUE);
                Cancelled = TRUE;
                break;
            }

            //
            // Check for multi-line commands
            //
            if (hyperdbg_u_check_multiline_command((CHAR *)TempCommand.c_str(), Reset) == TRUE)
            {
                //
                // It's a multi-line command
                //
                Reset = FALSE;

                //
                // Save the command with a space separator
                //
                CurrentCommand += TempCommand + "\n";

                //
                // Get next command
                //
                continue;
            }

            //
            // Reset for future commands
            //
            Reset = TRUE;

            //
            // Either the multi-line is finished or it's a
            // single line command
            //
            CurrentCommand += TempCommand;
            break;
        }

        if (EndOfFile)
        {
            break;
        }

        if (Cancelled)
        {
            continue;
        }

        if (!CurrentCommand.compare("") && hyperdbg_u_continue_previous_command())
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

        //
        // Some library commands (including 'exit') call exit(0) directly.
        // Persist the history before running the command so a command that
        // terminates the process cannot bypass the editor's destructor.
        //
        if (!CurrentCommand.empty() && !HistoryPath.empty())
        {
            g_LineEditor.SaveHistory(HistoryPath);
        }

        INT CommandExecutionResult = hyperdbg_u_run_command((CHAR *)CurrentCommand.c_str());

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
