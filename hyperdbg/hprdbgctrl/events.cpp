/**
 * @file events.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief events commands
 * @details
 * @version 0.1
 * @date 2020-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN    g_EventTraceInitialized;
extern BOOLEAN    g_BreakPrintingOutput;
extern BOOLEAN    g_AutoFlush;
extern BOOLEAN    g_IsConnectedToRemoteDebuggee;
extern BOOLEAN    g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of events command
 *
 * @return VOID
 */
VOID
CommandEventsHelp()
{
    ShowMessages("events : show active and disabled events\n");
    ShowMessages("syntax : \tevents [e|d|c] [event number (hex value) | all]\n");
    ShowMessages("e : enable\n");
    ShowMessages("d : disable\n");
    ShowMessages("c : clear\n");
    ShowMessages("Note : If you specify 'all' then [e|d|c] will be applied to "
                 "all of the events.\n\n");

    ShowMessages("\te.g : events \n");
    ShowMessages("\te.g : events e 12\n");
    ShowMessages("\te.g : events d 10\n");
    ShowMessages("\te.g : events c 10\n");
}

/**
 * @brief events command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandEvents(vector<string> SplittedCommand, string Command)
{
    DEBUGGER_MODIFY_EVENTS_TYPE RequestedAction;
    UINT64                      RequestedTag;

    //
    // Validate the parameters (size)
    //
    if (SplittedCommand.size() != 1 && SplittedCommand.size() != 3)
    {
        ShowMessages("incorrect use of '%s'\n\n", SplittedCommand.at(0));
        CommandEventsHelp();
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        if (!g_EventTraceInitialized)
        {
            ShowMessages("no active/disabled events \n");
            return;
        }

        CommandEventsShowEvents();

        //
        // No need to continue any further
        //
        return;
    }

    //
    // Validate second argument as it's not just a simple
    // events without any parameter
    //
    if (!SplittedCommand.at(1).compare("e"))
    {
        RequestedAction = DEBUGGER_MODIFY_EVENTS_ENABLE;
    }
    else if (!SplittedCommand.at(1).compare("d"))
    {
        RequestedAction = DEBUGGER_MODIFY_EVENTS_DISABLE;
    }
    else if (!SplittedCommand.at(1).compare("c"))
    {
        RequestedAction = DEBUGGER_MODIFY_EVENTS_CLEAR;
    }
    else
    {
        //
        // unknown second command
        //
        ShowMessages("incorrect use of '%s'\n\n", SplittedCommand.at(0));
        CommandEventsHelp();
        return;
    }

    //
    // Validate third argument as it's not just a simple
    // events without any parameter
    //
    if (!SplittedCommand.at(2).compare("all"))
    {
        RequestedTag = DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG;
    }
    else if (!ConvertStringToUInt64(SplittedCommand.at(2), &RequestedTag))
    {
        ShowMessages(
            "please specify a correct hex value for tag id (event number)\n\n");
        CommandEventsHelp();
        return;
    }

    //
    // Send the request to the kernel, we add it to a constant
    // that's because we want start tags from that constant
    //
    if (RequestedTag != DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
    {
        RequestedTag = RequestedTag + DebuggerEventTagStartSeed;
    }

    //
    // Perform event related tasks
    //
    CommandEventsModifyAndQueryEvents(RequestedTag, RequestedAction);
}

/**
 * @brief Check the kernel whether the event is enabled or disabled
 *
 * @param Tag the tag of the target event
 * @return BOOLEAN if the event was enabled and false if event was
 * disabled
 */
BOOLEAN
CommandEventQueryEventState(UINT64 Tag)
{
    BOOLEAN IsEnabled;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's a remote debugger in Debugger Mode
        //
        if (KdSendEventQueryAndModifyPacketToDebuggee(
                Tag,
                DEBUGGER_MODIFY_EVENTS_QUERY_STATE,
                &IsEnabled))
        {
            return IsEnabled;
        }
        else
        {
            ShowMessages("err, unable to get the state of the event\n");
            return FALSE;
        }
    }
    else
    {
        //
        // It's a local debugging in VMI Mode
        //
        return CommandEventsModifyAndQueryEvents(
            Tag,
            DEBUGGER_MODIFY_EVENTS_QUERY_STATE);
    }
    //
    // By default, disabled, even if there was an error
    //
    return FALSE;
}

/**
 * @brief print every active and disabled events
 * @details this function will not show cleared events
 * @return VOID
 */
VOID
CommandEventsShowEvents()
{
    //
    // It's an events without any argument so we have to show
    // all the currently active events
    //
    PLIST_ENTRY                    TempList         = 0;
    PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail    = {0};
    BOOLEAN                        IsThereAnyEvents = FALSE;

    TempList = &g_EventTrace;
    while (&g_EventTrace != TempList->Blink)
    {
        TempList = TempList->Blink;

        CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL, CommandsEventList);

        ShowMessages("%x\t(%s)\t    %s\n",
                     CommandDetail->Tag - DebuggerEventTagStartSeed,
                     // CommandDetail->IsEnabled ? "enabled" : "disabled",
                     CommandEventQueryEventState(CommandDetail->Tag)
                         ? "enabled"
                         : "disabled", /* Query is live now */
                     CommandDetail->CommandStringBuffer);

        if (!IsThereAnyEvents)
        {
            IsThereAnyEvents = TRUE;
        }
    }

    if (!IsThereAnyEvents)
    {
        ShowMessages("no active/disabled events \n");
    }
}

/**
 * @brief Disable a special event
 *
 * @param Tag the tag of the target event
 * @return BOOLEAN if the operation was successful then it returns
 * true otherwise it returns false
 */
BOOLEAN
CommandEventDisableEvent(UINT64 Tag)
{
    PLIST_ENTRY                    TempList      = 0;
    BOOLEAN                        Result        = FALSE;
    PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail = {0};

    TempList = &g_EventTrace;
    while (&g_EventTrace != TempList->Blink)
    {
        TempList = TempList->Blink;

        CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL, CommandsEventList);

        if (CommandDetail->Tag == Tag ||
            Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
        {
            //
            // Put it to FALSE, to indicate that it's not active
            //
            CommandDetail->IsEnabled = FALSE;

            if (!Result)
            {
                Result = TRUE;
            }

            if (Tag != DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
            {
                //
                // Only, one command exist with a tag, so we need to return as we
                // find it
                //
                return TRUE;
            }
        }
    }

    //
    // Not found
    //
    return Result;
}

/**
 * @brief enables a special event
 *
 * @param Tag the tag of the target event
 * @return BOOLEAN if the operation was successful then it returns
 * true otherwise it returns false
 */
BOOLEAN
CommandEventEnableEvent(UINT64 Tag)
{
    PLIST_ENTRY                    TempList      = 0;
    BOOLEAN                        Result        = FALSE;
    PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail = {0};

    TempList = &g_EventTrace;
    while (&g_EventTrace != TempList->Blink)
    {
        TempList = TempList->Blink;

        CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL, CommandsEventList);

        if (CommandDetail->Tag == Tag ||
            Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
        {
            //
            // Put it to TRUE, to indicate that it's active
            //
            CommandDetail->IsEnabled = TRUE;

            if (!Result)
            {
                Result = TRUE;
            }

            if (Tag != DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
            {
                //
                // Only, one command exist with a tag, so we need to return as we
                // find it
                //
                return TRUE;
            }
        }
    }

    //
    // Not found
    //
    return Result;
}

/**
 * @brief disable and remove a special event
 *
 * @param Tag the tag of the target event
 * @return BOOLEAN if the operation was successful then it returns
 * true otherwise it returns false
 */
BOOLEAN
CommandEventClearEvent(UINT64 Tag)
{
    PLIST_ENTRY                    TempList      = 0;
    BOOLEAN                        Result        = FALSE;
    PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail = {0};

    TempList = &g_EventTrace;
    while (&g_EventTrace != TempList->Blink)
    {
        TempList = TempList->Blink;

        CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL, CommandsEventList);

        if (CommandDetail->Tag == Tag ||
            Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
        {
            //
            // Remove it from the list
            //
            RemoveEntryList(&CommandDetail->CommandsEventList);

            //
            // Free the buffer for string of command
            //
            free(CommandDetail->CommandStringBuffer);

            //
            // Free the event it self
            //
            free(CommandDetail);

            if (!Result)
            {
                Result = TRUE;
            }

            if (Tag != DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
            {
                //
                // Only, one command exist with a tag, so we need to return as we
                // find it
                //
                return TRUE;
            }
        }
    }

    //
    // Not found
    //
    return Result;
}

/**
 * @brief Handle events after modification
 *
 * @param Tag the tag of the target event
 * @param ModifyEventRequest Results
 * @return VOID
 */
VOID
CommandEventsHandleModifiedEvent(
    UINT64                  Tag,
    PDEBUGGER_MODIFY_EVENTS ModifyEventRequest)
{
    if (ModifyEventRequest->KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Successfull, nothing to show but we should also
        // do the same (change) the user-mode structures
        // that hold the event data
        //
        if (ModifyEventRequest->TypeOfAction == DEBUGGER_MODIFY_EVENTS_ENABLE)
        {
            if (!CommandEventEnableEvent(Tag))
            {
                ShowMessages("error, the event was successfully, "
                             "(enabled|disabled|cleared) but "
                             "can't apply it to the user-mode structures.\n");
            }
        }
        else if (ModifyEventRequest->TypeOfAction ==
                 DEBUGGER_MODIFY_EVENTS_DISABLE)
        {
            if (!CommandEventDisableEvent(Tag))
            {
                ShowMessages("error, the event was successfully, "
                             "(enabled|disabled|cleared) but "
                             "can't apply it to the user-mode structures.\n");
            }
            else
            {
                //
                // The action was applied successfully
                //
                if (g_BreakPrintingOutput)
                {
                    //
                    // It is because we didn't query the target debuggee auto-flush
                    // variable
                    //
                    if (!g_IsConnectedToRemoteDebuggee)
                    {
                        if (!g_AutoFlush)
                        {
                            ShowMessages(
                                "auto-flush mode is disabled, if there is still "
                                "messages or buffers in the kernel, you continue to see "
                                "the messages when you run 'g' until the kernel "
                                "buffers are empty. you can run 'settings autoflush "
                                "on' and after disabling and clearing events, "
                                "kernel buffers will be flushed automatically.\n");
                        }
                        else
                        {
                            //
                            // We should flush buffers here
                            //
                            CommandFlushRequestFlush();
                        }
                    }
                }
            }
        }
        else if (ModifyEventRequest->TypeOfAction ==
                 DEBUGGER_MODIFY_EVENTS_CLEAR)
        {
            if (!CommandEventClearEvent(Tag))
            {
                ShowMessages("error, the event was successfully, "
                             "(enabled|disabled|cleared) but "
                             "can't apply it to the user-mode structures.\n");
            }
            else
            {
                //
                // The action was applied successfully
                //
                if (g_BreakPrintingOutput)
                {
                    //
                    // It is because we didn't query the target debuggee auto-flush
                    // variable
                    //
                    if (!g_IsConnectedToRemoteDebuggee)
                    {
                        if (!g_AutoFlush)
                        {
                            ShowMessages(
                                "auto-flush mode is disabled, if there is still "
                                "messages or buffers in the kernel, you continue to see "
                                "the messages when you run 'g' until the kernel "
                                "buffers are empty. you can run 'settings autoflush "
                                "on' and after disabling and clearing events, "
                                "kernel buffers will be flushed automatically.\n");
                        }
                        else
                        {
                            //
                            // We should flush buffers here
                            //
                            CommandFlushRequestFlush();
                        }
                    }
                }
            }
        }
        else if (ModifyEventRequest->TypeOfAction ==
                 DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
        {
            //
            // Nothing to show
            //
        }
        else
        {
            ShowMessages(
                "error, the event was successfully, (enabled|disabled|cleared) but "
                "can't apply it to the user-mode structures.\n");
        }
    }
    else
    {
        //
        // Interpret error
        //
        ShowErrorMessage(ModifyEventRequest->KernelStatus);
        return;
    }
}

/**
 * @brief modify a special event (enable/disable/clear) and send the
 * request directly to the kernel
 * @details if you pass DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG as the
 * tag then it will be applied to all the active/disabled events in the
 * kernel
 *
 * @param Tag the tag of the target event
 * @param TypeOfAction whether its a enable/disable/clear
 * @return BOOLEAN Shows whether the event is enabled or disabled
 */
BOOLEAN
CommandEventsModifyAndQueryEvents(UINT64                      Tag,
                                  DEBUGGER_MODIFY_EVENTS_TYPE TypeOfAction)
{
    BOOLEAN                Status;
    ULONG                  ReturnedLength;
    DEBUGGER_MODIFY_EVENTS ModifyEventRequest = {0};

    //
    // Check if there is no event, then we shouldn't apply the
    // enable, disable, or clear commands, this command also
    // checks for DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG to
    // see if at least one tag exists or not; however, it's not
    // necessary as the kernel will check for the validity of
    // tag too, but let's not send it to kernel as we can prevent
    // invalid requests from user-mode too
    //
    if (!IsTagExist(Tag))
    {
        if (Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
        {
            ShowMessages("err, there is no event\n");
        }
        else
        {
            ShowMessages("err, tag id is invalid\n");
        }
        return FALSE;
    }

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Remote debuggee Debugger Mode
        //
        KdSendEventQueryAndModifyPacketToDebuggee(Tag, TypeOfAction, NULL);
    }
    else
    {
        //
        // Local debugging VMI-Mode
        //

        //
        // Check if debugger is loaded or not
        //
        if (!g_DeviceHandle)
        {
            ShowMessages(
                "Handle not found, probably the driver is not loaded. Did you "
                "use 'load' command?\n");
            return FALSE;
        }

        //
        // Fill the structure to send it to the kernel
        //
        ModifyEventRequest.Tag          = Tag;
        ModifyEventRequest.TypeOfAction = TypeOfAction;

        //
        // Send the request to the kernel
        //

        Status =
            DeviceIoControl(g_DeviceHandle,                // Handle to device
                            IOCTL_DEBUGGER_MODIFY_EVENTS,  // IO Control code
                            &ModifyEventRequest,           // Input Buffer to driver.
                            SIZEOF_DEBUGGER_MODIFY_EVENTS, // Input buffer length
                            &ModifyEventRequest,           // Output Buffer from driver.
                            SIZEOF_DEBUGGER_MODIFY_EVENTS, // Length of output
                                                           // buffer in bytes.
                            &ReturnedLength,               // Bytes placed in buffer.
                            NULL                           // synchronous call
            );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return FALSE;
        }

        //
        // Perform further actions
        //
        CommandEventsHandleModifiedEvent(Tag, &ModifyEventRequest);

        if (TypeOfAction == DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
        {
            return ModifyEventRequest.IsEnabled;
        }
    }

    //
    // in all the cases except query state it shows whether the operation was
    // successful or not
    //
    return TRUE;
}
