/**
 * @file epthook2.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !epthook2 command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !epthook2 command
 *
 * @return VOID
 */
VOID
CommandEptHook2Help()
{
    ShowMessages("!epthook2 : puts a hidden-hook EPT (detours).\n\n");

    ShowMessages(
        "syntax : \t!epthook2 [Address (hex)] [pid ProcessId (hex)] "
        "[core CoreId (hex)] [imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
        "[script { Script (string) }] [condition { Condition (hex) }] [code { Code (hex) }] "
        "[output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !epthook2 nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : !epthook2 nt!ExAllocatePoolWithTag+5\n");
    ShowMessages("\t\te.g : !epthook2 fffff801deadb000\n");
    ShowMessages("\t\te.g : !epthook2 fffff801deadb000 pid 400\n");
    ShowMessages("\t\te.g : !epthook2 fffff801deadb000 core 2 pid 400\n");
}

/**
 * @brief !epthook2 command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandEptHook2(vector<string> SplitCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    BOOLEAN                            GetAddress                  = FALSE;
    UINT64                             OptionalParam1              = 0; // Set the target address
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    UINT32                             IndexInCommandCaseSensitive = 0;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    if (SplitCommand.size() < 2)
    {
        ShowMessages("incorrect use of the '!epthook2'\n");
        CommandEptHook2Help();
        return;
    }

    //
    // Interpret and fill the general event and action fields
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplitCommand,
            &SplitCommandCaseSensitive,
            HIDDEN_HOOK_EXEC_DETOURS,
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
    // Check here to make sure that the user didn't specified the calling stages for this ept hook
    //
    if (Event->EventStage != VMM_CALLBACK_CALLING_STAGE_PRE_EVENT_EMULATION)
    {
        ShowMessages("the utilization of 'post' or 'all' event calling stages is not meaningful "
                     "for the hidden hook; therefore, this command does not support them\n");

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    for (auto Section : SplitCommand)
    {
        IndexInCommandCaseSensitive++;

        if (!Section.compare("!epthook2"))
        {
            continue;
        }
        else if (!GetAddress)
        {
            //
            // It's probably address
            //
            if (!SymbolConvertNameOrExprToAddress(
                    SplitCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                    &OptionalParam1))
            {
                //
                // Couldn't resolve or unknown parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n\n",
                             SplitCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());
                CommandEptHook2Help();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
            else
            {
                GetAddress = TRUE;
            }
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
            CommandEptHook2Help();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }
    if (OptionalParam1 == 0)
    {
        ShowMessages("please choose an address to put the hook on it\n");

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Set the optional parameters
    //
    Event->Options.OptionalParam1 = OptionalParam1;

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
