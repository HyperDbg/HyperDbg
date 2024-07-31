/**
 * @file msrwrite.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !msrwrite command
 * @details
 * @version 0.1
 * @date 2020-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !msrwrite command
 *
 * @return VOID
 */
VOID
CommandMsrwriteHelp()
{
    ShowMessages("!msrwrite : detects the execution of wrmsr instructions.\n\n");

    ShowMessages("syntax : \t!msrwrite [Msr (hex)] [pid ProcessId (hex)] [core CoreId (hex)] "
                 "[imm IsImmediate (yesno)] [sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] "
                 "[buffer PreAllocatedBuffer (hex)] [script { Script (string) }] [asm condition { Condition (assembly/hex) }] "
                 "[asm code { Code (assembly/hex) }] [output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !msrwrite\n");
    ShowMessages("\t\te.g : !msrwrite 0xc0000082\n");
    ShowMessages("\t\te.g : !msrwrite pid 400\n");
    ShowMessages("\t\te.g : !msrwrite core 2 pid 400\n");
    ShowMessages("\t\te.g : !msrwrite script { printf(\"msr write with the 'ecx' register equal to: %%llx\\n\", $context); }\n");
    ShowMessages("\t\te.g : !msrwrite asm code { nop; nop; nop }\n");
}

/**
 * @brief !msrwrite command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandMsrwrite(vector<CommandToken> CommandTokens, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    UINT64                             SpecialTarget               = DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS;
    BOOLEAN                            GetAddress                  = FALSE;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &CommandTokens,
            WRMSR_INSTRUCTION_EXECUTION,
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
    // Interpret command specific details (if any), it is because we can use
    // special msr bitmap here
    //
    for (auto Section : CommandTokens)
    {
        if (CompareLowerCaseStrings(Section, "!msrwrite"))
        {
            continue;
        }
        else if (!GetAddress)
        {
            //
            // It's probably an msr
            //
            if (!ConvertTokenToUInt64(Section, &SpecialTarget))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(Section).c_str());
                CommandMsrwriteHelp();

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
            ShowMessages("unknown parameter '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(Section).c_str());
            CommandMsrwriteHelp();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }

    //
    // Set the target msr (if not specific then it means all msrs)
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
