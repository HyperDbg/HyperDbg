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
    ShowMessages("\n");
    ShowMessages("\t\te.g : !rev pattern\n");
    ShowMessages("\t\te.g : !rev reconstruct\n");
    ShowMessages("\t\te.g : !rev pattern pid 1c0\n");
    ShowMessages("\t\te.g : !rev reconstruct pid 1c0\n");
}

/**
 * @brief !rev command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRev(vector<string> SplittedCommand, string Command)
{
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevRequest         = {0};
    BOOLEAN                                      SetPid             = FALSE;
    UINT32                                       TargetPid          = NULL;
    BOOLEAN                                      IgnoreFirstCommand = TRUE;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE    Mode               = REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE    Type               = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN;

    //
    // Interpret command specific details
    //
    for (auto Section : SplittedCommand)
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
                // couldn't resolve or unkonwn parameter
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

    // ================================================================================

    //
    // Send the request to the hypervisor (kernel)
    //
    RevRequestService(&RevRequest);
}
