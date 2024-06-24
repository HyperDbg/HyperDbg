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
 * @brief help of the .start command
 *
 * @return VOID
 */
VOID
CommandStartHelp()
{
    ShowMessages(".start : runs a user-mode process.\n\n");

    ShowMessages("syntax : \t.start [path Path (string)] [Parameters (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .start path c:\\reverse eng\\my_file.exe\n");
}

/**
 * @brief .start command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandStart(vector<string> SplitCommand, string Command)
{
    vector<string> PathAndArgs;
    string         Arguments = "";

    //
    // Disable user-mode debugger in this version
    //
#if ActivateUserModeDebugger == FALSE

    if (!g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages("the user-mode debugger in VMI Mode is still in the beta version and not stable. "
                     "we decided to exclude it from this release and release it in future versions. "
                     "if you want to test the user-mode debugger in VMI Mode, you should build "
                     "HyperDbg with special instructions. But starting processes is fully supported "
                     "in the Debugger Mode.\n"
                     "(it's not recommended to use it in VMI Mode yet!)\n");
        return;
    }

#endif // !ActivateUserModeDebugger

    if (SplitCommand.size() <= 2)
    {
        ShowMessages("incorrect use of the '.start'\n\n");
        CommandStartHelp();
        return;
    }

    if (!SplitCommand.at(1).compare("path"))
    {
        //
        // *** It's a run of target PE file ***
        //

        //
        // Trim the command
        //
        Trim(Command);

        //
        // Remove '.start' or 'start' from it
        //
        Command.erase(0, SplitCommand.at(0).size());

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
                     SplitCommand.at(1).c_str());
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
                          NULL,
                          FALSE);
    }
    else
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str(),
                          FALSE);
    }
}
