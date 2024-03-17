/**
 * @file rev.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !rev command
 * @details
 * @version 0.2
 * @date 2023-03-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN      g_IsSerialConnectedToRemoteDebugger;
extern std::wstring g_StartCommandPath;
extern std::wstring g_StartCommandPathAndArguments;
extern BOOLEAN      g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of the !rev command
 *
 * @return VOID
 */
VOID
CommandRevHelp()
{
    ShowMessages("!rev : uses the reversing machine module in order to reconstruct the programmer/memory assumptions.\n\n");

    ShowMessages("syntax : \t!rev [config] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t!rev [path Path (string)] [Parameters (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !rev path c:\\reverse eng\\my_file.exe\n");
    ShowMessages("\t\te.g : !rev pattern\n");
    ShowMessages("\t\te.g : !rev reconstruct\n");
    ShowMessages("\t\te.g : !rev pattern pid 1c0\n");
    ShowMessages("\t\te.g : !rev reconstruct pid 1c0\n");
}

/**
 * @brief !rev command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRev(vector<string> SplitCommand, string Command)
{
    /*
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
        // Remove !rev from it
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
                          TRUE);
    }
    else
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str(),
                          TRUE);
    }

    ///////////////////////////////////////////////////////////////////////////////
    return;

    */

    REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevRequest         = {0};
    BOOLEAN                                      SetPid             = FALSE;
    UINT32                                       TargetPid          = NULL;
    BOOLEAN                                      IgnoreFirstCommand = TRUE;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE    Mode               = REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE    Type               = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN;

    //
    // Interpret command specific details
    //
    for (auto Section : SplitCommand)
    {
        if (!Section.compare("!rev") && IgnoreFirstCommand)
        {
            IgnoreFirstCommand = FALSE;
            continue;
        }
        else if (!Section.compare("pid") && !SetPid)
        {
            SetPid = TRUE;
        }
        else if (SetPid)
        {
            if (!ConvertStringToUInt32(Section, &TargetPid))
            {
                //
                // couldn't resolve or unknown parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n\n",
                             Section.c_str());
                CommandRevHelp();
                return;
            }
            SetPid = FALSE;
        }
        else if (!Section.compare("pattern"))
        {
            Type = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_PATTERN;
        }
        else if (!Section.compare("reconstruct"))
        {
            Type = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_RECONSTRUCT;
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         Section.c_str());
            CommandRevHelp();
            return;
        }
    }

    if (SetPid)
    {
        ShowMessages("err, please enter a valid process id in hex format, "
                     "or if you want to use it in decimal format, add '0n' "
                     "prefix to the number\n");
        return;
    }

    RevRequest.ProcessId = TargetPid;

    // ================================================================================

    //
    // Send the request to the hypervisor (kernel)
    //
    RevRequestService(&RevRequest);
}
