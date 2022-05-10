/**
 * @file monitor.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
    ShowMessages("!monitor : monitors address range for read and writes.\n\n");

    ShowMessages("syntax : \t!monitor [Mode (string)] [FromAddress (hex)] "
                 "[ToAddress (hex)] [pid ProcessId (hex)] [core CoreId (hex)] "
                 "[imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [condition { Condition (hex) }] "
                 "[code { Code (hex) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !monitor rw fffff801deadb000 fffff801deadbfff\n");
    ShowMessages("\t\te.g : !monitor rw nt!Kd_DEFAULT_Mask Kd_DEFAULT_Mask+5\n");
    ShowMessages("\t\te.g : !monitor r fffff801deadb000 fffff801deadbfff pid 400\n");
    ShowMessages("\t\te.g : !monitor w fffff801deadb000 fffff801deadbfff core 2 pid 400\n");
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
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    UINT64                             TargetAddress;
    UINT64                             OptionalParam1 = 0; // Set the 'from' target address
    UINT64                             OptionalParam2 = 0; // Set the 'to' target address
    BOOLEAN                            SetFrom        = FALSE;
    BOOLEAN                            SetTo          = FALSE;
    BOOLEAN                            SetMode        = FALSE;
    vector<string>                     SplittedCommandCaseSensitive {Split(Command, ' ')};
    UINT32                             IndexInCommandCaseSensitive = 0;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

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
            &SplittedCommandCaseSensitive,
            HIDDEN_HOOK_READ_AND_WRITE,
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength,
            &EventParsingErrorCause))
    {
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    for (auto Section : SplittedCommand)
    {
        IndexInCommandCaseSensitive++;

        if (!Section.compare("!monitor"))
        {
            continue;
        }
        else if (!Section.compare("r") && !SetMode)
        {
            Event->EventType = HIDDEN_HOOK_READ;
            SetMode          = TRUE;
        }
        else if (!Section.compare("w") && !SetMode)
        {
            Event->EventType = HIDDEN_HOOK_WRITE;
            SetMode          = TRUE;
        }
        else if ((!Section.compare("rw") || !Section.compare("wr")) && !SetMode)
        {
            Event->EventType = HIDDEN_HOOK_READ_AND_WRITE;
            SetMode          = TRUE;
        }
        else
        {
            //
            // It's probably address
            //
            if (!SetFrom)
            {
                if (!SymbolConvertNameOrExprToAddress(
                        SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                        &OptionalParam1))
                {
                    //
                    // couldn't resolve or unkonwn parameter
                    //
                    ShowMessages("err, couldn't resolve error at '%s'\n\n",
                                 SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());
                    CommandMonitorHelp();

                    FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                    return;
                }
                SetFrom = TRUE;
            }
            else if (!SetTo)
            {
                if (!SymbolConvertNameOrExprToAddress(
                        SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                        &OptionalParam2))
                {
                    //
                    // Couldn't resolve or unkonwn parameter
                    //
                    ShowMessages("err, couldn't resolve error at '%s'\n\n",
                                 SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());

                    CommandMonitorHelp();

                    FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                    return;
                }
                SetTo = TRUE;
            }
            else
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
                CommandMonitorHelp();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
        }
    }

    //
    // Check for invalid order of address
    //
    if (OptionalParam1 > OptionalParam2)
    {
        //
        // 'from' is greater than 'to'
        //
        ShowMessages("please choose the 'from' value first, then choose the 'to' "
                     "value\n");

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Check if user set the mode of !monitor or not
    //
    if (!SetMode)
    {
        ShowMessages("please specify the attribute(s) that you want to monitor (r, w, rw)\n");

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Set the optional parameters
    //
    Event->OptionalParam1 = OptionalParam1;
    Event->OptionalParam2 = OptionalParam2;

    //
    // Send the ioctl to the kernel for event registration
    //
    if (!SendEventToKernel(Event, EventLength))
    {
        //
        // There was an error, probably the handle was not initialized
        // we have to free the Action before exit, it is because, we
        // already freed the Event and string buffers
        //

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Add the event to the kernel
    //
    if (!RegisterActionToEvent(Event,
                               ActionBreakToDebugger,
                               ActionBreakToDebuggerLength,
                               ActionCustomCode,
                               ActionCustomCodeLength,
                               ActionScript,
                               ActionScriptLength))
    {
        //
        // There was an error
        //

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }
}
