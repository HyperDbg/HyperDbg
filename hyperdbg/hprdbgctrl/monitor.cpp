/**
 * @file monitor.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !monitor command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !monitor command
 *
 * @return VOID
 */
VOID
CommandMonitorHelp()
{
    ShowMessages("!monitor : Monitor address range for read and writes.\n\n");
    ShowMessages("syntax : \t!monitor [attrib (r, w, rw)] [From Virtual Address "
                 "(hex value)] [To Virtual Address (hex value)] core [core index "
                 "(hex value)] pid [process id (hex value)] condition {[assembly "
                 "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
                 "(hex value)] \n");

    ShowMessages("\t\te.g : !monitor rw fffff801deadb000 fffff801deadbfff\n");
    ShowMessages(
        "\t\te.g : !monitor r fffff801deadb000 fffff801deadbfff pid 400\n");
    ShowMessages("\t\te.g : !monitor w fffff801deadb000 fffff801deadbfff core 2 "
                 "pid 400\n");
}

/**
 * @brief !monitor command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandMonitor(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionScript          = NULL;
    UINT32                         EventLength;
    UINT32                         ActionBreakToDebuggerLength = 0;
    UINT32                         ActionCustomCodeLength      = 0;
    UINT32                         ActionScriptLength          = 0;
    UINT64                         TargetAddress;
    UINT64                         OptionalParam1 = 0; // Set the 'from' target address
    UINT64                         OptionalParam2 = 0; // Set the 'to' target address
    BOOLEAN                        SetFrom        = FALSE;
    BOOLEAN                        SetTo          = FALSE;

    if (SplittedCommand.size() < 4)
    {
        ShowMessages("incorrect use of '!monitor'\n");
        CommandMonitorHelp();
        return;
    }

    //
    // Interpret and fill the general event and action fields
    //
    // We use HIDDEN_HOOK_READ_AND_WRITE here but it might be changed to
    // HIDDEN_HOOK_READ or HIDDEN_HOOK_WRITE it is because we are not sure what
    // kind event the user need
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            HIDDEN_HOOK_READ_AND_WRITE,
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength))
    {
        CommandMonitorHelp();
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    for (auto Section : SplittedCommand)
    {
        if (!Section.compare("!monitor"))
        {
            continue;
        }
        else if (!Section.compare("r"))
        {
            Event->EventType = HIDDEN_HOOK_READ;
        }
        else if (!Section.compare("w"))
        {
            Event->EventType = HIDDEN_HOOK_WRITE;
        }
        else if (!Section.compare("rw") || !Section.compare("wr"))
        {
            Event->EventType = HIDDEN_HOOK_READ_AND_WRITE;
        }
        else
        {
            //
            // It's probably address
            //
            if (!SetFrom)
            {
                if (!ConvertStringToUInt64(Section, &OptionalParam1))
                {
                    //
                    // Unkonwn parameter
                    //
                    ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                    CommandMonitorHelp();
                    return;
                }
                SetFrom = TRUE;
            }
            else if (!SetTo)
            {
                if (!ConvertStringToUInt64(Section, &OptionalParam2))
                {
                    //
                    // Unkonwn parameter
                    //
                    ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                    CommandMonitorHelp();
                    return;
                }
                SetTo = TRUE;
            }
            else
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                CommandMonitorHelp();
                return;
            }
        }
    }
    if (OptionalParam1 > OptionalParam2)
    {
        //
        // 'from' is greater than 'to'
        //
        ShowMessages("Please choose the 'from' value first, then choose the 'to' "
                     "value.\n");
        return;
    }

    //
    // Set the optional parameters
    //
    Event->OptionalParam1 = OptionalParam1;
    Event->OptionalParam2 = OptionalParam2;

    //
    // Send the ioctl to the kernel for event registeration
    //
    if (!SendEventToKernel(Event, EventLength))
    {
        //
        // There was an error, probably the handle was not initialized
        // we have to free the Action before exit, it is because, we
        // already freed the Event and string buffers
        //
        if (ActionBreakToDebugger != NULL)
        {
            free(ActionBreakToDebugger);
        }
        if (ActionCustomCode != NULL)
        {
            free(ActionCustomCode);
        }
        if (ActionScript != NULL)
        {
            free(ActionScript);
        }
        return;
    }

    //
    // Add the event to the kernel
    //
    if (!RegisterActionToEvent(ActionBreakToDebugger, ActionBreakToDebuggerLength, ActionCustomCode, ActionCustomCodeLength, ActionScript, ActionScriptLength))
    {
        //
        // There was an error
        //
        return;
    }
}
