/**
 * @file exception.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !exception command
 * @details
 * @version 0.1
 * @date 2020-06-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !exception command
 *
 * @return VOID
 */
VOID
CommandExceptionHelp()
{
    ShowMessages("!exception : monitors the first 32 entry of IDT (starting from "
                 "zero).\n\n");

    ShowMessages(
        "syntax : \t!exception [IdtIndex (hex)] [pid ProcessId (hex)] "
        "[core CoreId (hex)] [imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
        "[script { Script (string) }] [condition { Condition (hex) }] [code { Code (hex) }]\n");

    ShowMessages("\nnote: monitoring page-faults (entry 0xe) is implemented differently.\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !exception\n");
    ShowMessages("\t\te.g : !exception 0xe\n");
    ShowMessages("\t\te.g : !exception pid 400\n");
    ShowMessages("\t\te.g : !exception core 2 pid 400\n");
}

/**
 * @brief !exception command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandException(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    UINT64                             SpecialTarget               = DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES;
    BOOLEAN                            GetEntry                    = FALSE;
    vector<string>                     SplittedCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            &SplittedCommandCaseSensitive,
            EXCEPTION_OCCURRED,
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
        if (!Section.compare("!exception"))
        {
            continue;
        }
        else if (!GetEntry)
        {
            //
            // It's probably an index
            //
            if (!ConvertStringToUInt64(Section, &SpecialTarget))
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
                CommandExceptionHelp();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
            else
            {
                //
                // Check if entry is valid or not (start from zero)
                //
                if (SpecialTarget >= 31)
                {
                    //
                    // Entry is invalid (this command is designed for just first 32
                    // entries)
                    //
                    ShowMessages("the entry should be between 0x0 to 0x1f or first 32 "
                                 "entries'\n\n");
                    CommandExceptionHelp();

                    FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                    return;
                }
                GetEntry = TRUE;
            }
        }
        else
        {
            //
            // Unkonwn parameter
            //
            ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
            CommandExceptionHelp();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }

    //
    // Set the target exception (if not specific then it means all exceptions)
    //
    Event->OptionalParam1 = SpecialTarget;

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
