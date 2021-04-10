/**
 * @file interrupt.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !interrupt command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !interrupt command
 *
 * @return VOID
 */
VOID
CommandInterruptHelp()
{
    ShowMessages("!interrupt : Monitors the external interrupt (IDT >= 32).\n\n");
    ShowMessages("syntax : \t!interrupt [entry index (hex value) - should be "
                 "selected] core [core index (hex value)] pid [process id (hex "
                 "value)] condition {[assembly in hex]} code {[assembly in hex]} "
                 "buffer [pre-require buffer - (hex value)] \n");
    ShowMessages("\nNote : The index should be greater than 0x20 (32) and less "
                 "than 0xFF (255) - starting from zero.\n");
    ShowMessages("\t\te.g : !interrupt 0x2f\n");
    ShowMessages("\t\te.g : !interrupt 0x2f pid 400\n");
    ShowMessages("\t\te.g : !interrupt 0x2f core 2 pid 400\n");
}

/**
 * @brief !interrupt command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandInterrupt(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionScript          = NULL;
    UINT32                         EventLength;
    UINT32                         ActionBreakToDebuggerLength = 0;
    UINT32                         ActionCustomCodeLength      = 0;
    UINT32                         ActionScriptLength          = 0;
    UINT64                         SpecialTarget               = 0;
    BOOLEAN                        GetEntry                    = FALSE;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            EXTERNAL_INTERRUPT_OCCURRED,
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength))
    {
        CommandInterruptHelp();
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    //
    for (auto Section : SplittedCommand)
    {
        if (!Section.compare("!interrupt"))
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
                ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                CommandInterruptHelp();
                return;
            }
            else
            {
                //
                // Check if entry is valid or not
                //
                if (!(SpecialTarget >= 32 && SpecialTarget <= 0xff))
                {
                    //
                    // Entry is invalid (this command is designed for just entries
                    // between 32 to 255)
                    //
                    ShowMessages("The entry should be between 0x20 to 0xFF or the "
                                 "entries between 32 to 255.\n\n");
                    CommandInterruptHelp();
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
            ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
            CommandInterruptHelp();
            return;
        }
    }

    if (SpecialTarget == 0)
    {
        //
        // The user didn't set the target interrupt, even though it's possible to
        // get all interrupts but it makes the system not resposive so it's wrong
        // to trigger event on all interrupts and we're not going to support it
        //
        ShowMessages("Please specify an interrupt index to monitor, HyperDbg "
                     "doesn't support to trigger events on all interrupts because "
                     "it's not reasonable and make the system unresponsive\n");
        CommandInterruptHelp();
        return;
    }

    //
    // Set the target interrupt
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
