/**
 * @file start.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .start command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern std::wstring g_StartCommandPath;
extern std::wstring g_StartCommandPathAndArguments;
extern BOOLEAN      g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of .start command
 *
 * @return VOID
 */
VOID
CommandStartHelp()
{
    ShowMessages(".start : runs a user-mode process.\n\n");

    ShowMessages("syntax : \t.start [path Path (string)] [Parameters (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .start path c:\\users\\sina\\reverse eng\\my_file.exe\n");
}

/**
 * @brief .start command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandStart(vector<string> SplittedCommand, string Command)
{
    vector<string> PathAndArgs;
    string         Arguments = "";

    //
    // Disable user-mode debugger in this version
    //
#if ActivateUserModeDebugger == FALSE

    if (!g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages("The user-mode debugger is still in the beta version and not stable. "
                     "We decided to exclude it from this release and release it in future versions. "
                     "If you want to test the user-mode debugger in VMI Mode, you should build "
                     "HyperDbg with special instructions. \nPlease follow the steps here: "
                     "https://docs.hyperdbg.org/getting-started/build-and-install \n");
        return;
    }

#endif // !ActivateUserModeDebugger

    if (SplittedCommand.size() <= 2)
    {
        ShowMessages("incorrect use of '.start'\n\n");
        CommandStartHelp();
        return;
    }

    if (!SplittedCommand.at(1).compare("path"))
    {
        //
        // *** It's a run of target PE file ***
        //

        //
        // Trim the command
        //
        Trim(Command);

        //
        // Remove .start from it
        //
        Command.erase(0, 6);

        //
        // Remove path + space
        //
        Command.erase(0, 4 + 1);

        //
        // Trim it again
        //
        Trim(Command);

        //
        // Split Path and args
        //
        SplitPathAndArgs(PathAndArgs, Command);

        //
        // Convert path to wstring
        //
        StringToWString(g_StartCommandPath, PathAndArgs.at(0));

        if (PathAndArgs.size() != 1)
        {
            //
            // There are arguments to this command
            //

            for (auto item : PathAndArgs)
            {
                //
                // Append the arguments
                //
                // ShowMessages("Arg : %s\n", item.c_str());
                Arguments += item + " ";
            }

            //
            // Remove the latest space
            //
            Arguments.pop_back();

            //
            // Convert arguments to wstring
            //
            StringToWString(g_StartCommandPathAndArguments, Arguments);
        }
    }
    else
    {
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplittedCommand.at(1).c_str());
        CommandStartHelp();
        return;
    }

    //
    // Perform run of the target file
    //
    if (Arguments.empty())
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          NULL);
    }
    else
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str());
    }
}
