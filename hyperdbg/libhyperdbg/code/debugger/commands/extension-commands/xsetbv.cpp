/**
 * @file xsetbv.cpp
 * @author unrustled.jimmies
 * @brief !xsetbv command
 * @details This command
 * @version 0.16
 * @date 2025-08-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !xsetbv command
 *
 * @return VOID
 */
VOID
CommandXsetbvHelp()
{
    ShowMessages("!xsetbv : monitors execution of xsetbv instructions.\n\n");

    ShowMessages("syntax : \t!xsetbv [Xcr (hex)] [pid ProcessId (hex)] [core CoreId (hex)] "
                 "[imm IsImmediate (yesno)] [sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] "
                 "[buffer PreAllocatedBuffer (hex)] [script { Script (string) }] [asm condition { Condition (assembly/hex) }] "
                 "[asm code { Code (assembly/hex) }] [output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !xsetbv\n");
    ShowMessages("\t\te.g : !xsetbv 0\n");
    ShowMessages("\t\te.g : !xsetbv pid 400\n");
    ShowMessages("\t\te.g : !xsetbv core 2 pid 400\n");
    ShowMessages("\t\te.g : !xsetbv script { printf(\"XSETBV instruction is executed with XCR index: %%llx\\n\", @rcx); }\n");
    ShowMessages("\t\te.g : !xsetbv asm code { nop; nop; nop }\n");
}

/**
 * @brief !xsetbv command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandXsetbv(vector<CommandToken> CommandTokens, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    BOOLEAN                            GetXcr                = FALSE;
    UINT32                             EventLength;
    UINT64                             SpecialTarget               = 0;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    if (!InterpretGeneralEventAndActionsFields(
            &CommandTokens,
            XSETBV_INSTRUCTION_EXECUTION,
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
    // Interpret command specific details (if any), of XCR index
    //
    for (auto Section : CommandTokens)
    {
        if (CompareLowerCaseStrings(Section, "!xsetbv"))
        {
            continue;
        }
        else if (!GetXcr)
        {
            //
            // It's probably an XCR index
            //
            if (!ConvertTokenToUInt64(Section, &SpecialTarget))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(Section).c_str());
                CommandXsetbvHelp();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
            else
            {
                //
                // A special XCR is set
                //
                GetXcr = TRUE;
            }
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("unknown parameter '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(Section).c_str());

            CommandXsetbvHelp();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }

    //
    // Set the target XCR (if not specific then it means all XCRs)
    //
    Event->Options.OptionalParam1 = GetXcr;

    if (GetXcr)
    {
        Event->Options.OptionalParam2 = SpecialTarget;
    }

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
