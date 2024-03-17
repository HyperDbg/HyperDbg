/**
 * @file ioout.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !ioout command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !ioout command
 *
 * @return VOID
 */
VOID
CommandIooutHelp()
{
    ShowMessages("!ioout : detects the execution of OUT (I/O instructions) "
                 "instructions.\n\n");

    ShowMessages("syntax : \t!ioout [Port (hex)] [pid ProcessId (hex)] "
                 "[core CoreId (hex)] [imm IsImmediate (yesno)] [sc EnableShortCircuiting (onoff)] "
                 "[stage CallingStage (prepostall)] [buffer PreAllocatedBuffer (hex)] [script { Script (string) }] "
                 "[condition { Condition (hex) }] [code { Code (hex) }] [output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !ioout\n");
    ShowMessages("\t\te.g : !ioout 0x64\n");
    ShowMessages("\t\te.g : !ioout pid 400\n");
    ShowMessages("\t\te.g : !ioout core 2 pid 400\n");
}

/**
 * @brief !ioout command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandIoout(vector<string> SplitCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    UINT64                             SpecialTarget               = DEBUGGER_EVENT_ALL_IO_PORTS;
    BOOLEAN                            GetPort                     = FALSE;
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplitCommand,
            &SplitCommandCaseSensitive,
            OUT_INSTRUCTION_EXECUTION,
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
    //
    for (auto Section : SplitCommand)
    {
        if (!Section.compare("!ioout"))
        {
            continue;
        }
        else if (!GetPort)
        {
            //
            // It's probably an I/O port
            //
            if (!ConvertStringToUInt64(Section, &SpecialTarget))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
                CommandIooutHelp();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
            else
            {
                GetPort = TRUE;
            }
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
            CommandIooutHelp();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }

    //
    // Set the target I/O port
    //
    Event->Options.OptionalParam1 = SpecialTarget;

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
