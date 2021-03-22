/**
 * @file ioout.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief help of !ioout command
 *
 * @return VOID
 */
VOID
CommandIooutHelp()
{
    ShowMessages("!ioout : Detects the execution of OUT (I/O instructions) "
                 "instructions.\n\n");
    ShowMessages("syntax : \t!ioout [port (hex value) - if not specific means "
                 "all ports] core [core index (hex value)] pid [process id (hex "
                 "value)] condition {[assembly "
                 "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
                 "(hex value)] \n");

    ShowMessages("\t\te.g : !ioout\n");
    ShowMessages("\t\te.g : !ioout 0x64\n");
    ShowMessages("\t\te.g : !ioout pid 400\n");
    ShowMessages("\t\te.g : !ioout core 2 pid 400\n");
}

/**
 * @brief !ioout command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandIoout(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionScript          = NULL;
    UINT32                         EventLength;
    UINT32                         ActionBreakToDebuggerLength = 0;
    UINT32                         ActionCustomCodeLength      = 0;
    UINT32                         ActionScriptLength          = 0;
    UINT64                         SpecialTarget               = DEBUGGER_EVENT_ALL_IO_PORTS;
    BOOLEAN                        GetPort                     = FALSE;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            OUT_INSTRUCTION_EXECUTION,
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength))
    {
        CommandIooutHelp();
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    //
    for (auto Section : SplittedCommand)
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
                // Unkonwn parameter
                //
                ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                CommandIooutHelp();
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
            // Unkonwn parameter
            //
            ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
            CommandIooutHelp();
            return;
        }
    }

    //
    // Set the target I/O port
    //
    Event->OptionalParam1 = SpecialTarget;

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
