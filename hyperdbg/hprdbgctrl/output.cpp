/**
 * @file output.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief output command
 * @details
 * @version 0.1
 * @date 2020-11-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern LIST_ENTRY g_OutputSources;
extern BOOLEAN    g_OutputSourcesInitialized;

/**
 * @brief help of output command
 *
 * @return VOID
 */
VOID
CommandOutputHelp()
{
    ShowMessages("output : create an output instance that can be used in event "
                 "forwarding.\n\n");
    ShowMessages("syntax : \toutput [create|open|close] [type "
                 "(file|namedpipe|tcp)] [name|address]\n");
    ShowMessages("\t\te.g : output create MyOutputName1 file "
                 "c:\\users\\sina\\desktop\\output.txt\n");
    ShowMessages("\t\te.g : output create MyOutputName2 tcp 192.168.1.10:8080\n");
    ShowMessages("\t\te.g : output create MyOutputName3 namedpipe "
                 "\\\\.\\Pipe\\HyperDbgOutput\n");
    ShowMessages("\t\te.g : output open MyOutputName1\n");
    ShowMessages("\t\te.g : output close MyOutputName1\n");
}

/**
 * @brief output command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandOutput(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_EVENT_FORWARDING     EventForwardingObject;
    DEBUGGER_EVENT_FORWARDING_TYPE Type;
    DEBUGGER_OUTPUT_SOURCE_STATUS  Status;
    string                         DetailsOfSource;
    UINT32                         IndexToShowList;
    PLIST_ENTRY                    TempList          = 0;
    BOOLEAN                        OutputSourceFound = FALSE;
    HANDLE                         SourceHandle      = INVALID_HANDLE_VALUE;
    SOCKET                         Socket            = NULL;

    //
    // Check if the user needs a list of outputs or not
    //
    if (SplittedCommand.size() == 1)
    {
        IndexToShowList = 0;

        if (g_OutputSourcesInitialized)
        {
            TempList = &g_OutputSources;

            while (&g_OutputSources != TempList->Blink)
            {
                TempList = TempList->Blink;

                PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails =
                    CONTAINING_RECORD(TempList, DEBUGGER_EVENT_FORWARDING, OutputSourcesList);

                //
                // Increase index
                //
                IndexToShowList++;

                string TempStateString = "";
                string TempTypeString  = "";

                if (CurrentOutputSourceDetails->State ==
                    EVENT_FORWARDING_STATE_NOT_OPENED)
                {
                    TempStateString = "not opened";
                }
                else if (CurrentOutputSourceDetails->State ==
                         EVENT_FORWARDING_STATE_OPENED)
                {
                    TempStateString = "opened    ";
                }
                else if (CurrentOutputSourceDetails->State ==
                         EVENT_FORWARDING_CLOSED)
                {
                    TempStateString = "closed    ";
                }

                if (CurrentOutputSourceDetails->Type == EVENT_FORWARDING_NAMEDPIPE)
                {
                    TempTypeString = "namedpipe";
                }
                else if (CurrentOutputSourceDetails->Type == EVENT_FORWARDING_FILE)
                {
                    TempTypeString = "file     ";
                }
                else if (CurrentOutputSourceDetails->Type == EVENT_FORWARDING_TCP)
                {
                    TempTypeString = "tcp      ";
                }

                ShowMessages("%d  %s   %s\t%s\n", IndexToShowList, TempTypeString.c_str(), TempStateString.c_str(), CurrentOutputSourceDetails->Name);
            }
        }
        else
        {
            ShowMessages("output forwarding list is empty.\n\n");
        }

        return;
    }

    if (SplittedCommand.size() <= 2)
    {
        ShowMessages("incorrect use of 'output'\n\n");
        CommandOutputHelp();
        return;
    }

    //
    // Check if it's a create, open, or close
    //
    if (!SplittedCommand.at(1).compare("create"))
    {
        //
        // It's a create
        //

        //
        // check if the parameters are okay for a create or not
        //
        if (SplittedCommand.size() <= 4)
        {
            ShowMessages("incorrect use of 'output'\n\n");
            CommandOutputHelp();
            return;
        }

        //
        // Check for the type of the output source
        //
        if (!SplittedCommand.at(3).compare("file"))
        {
            Type = EVENT_FORWARDING_FILE;
        }
        else if (!SplittedCommand.at(3).compare("namedpipe"))
        {
            Type = EVENT_FORWARDING_NAMEDPIPE;
        }
        else if (!SplittedCommand.at(3).compare("tcp"))
        {
            Type = EVENT_FORWARDING_TCP;
        }
        else
        {
            ShowMessages("incorrect type near '%s'\n\n",
                         SplittedCommand.at(3).c_str());
            CommandOutputHelp();
            return;
        }

        //
        // Check to make sure that the name doesn't exceed the maximum character
        //
        if (SplittedCommand.at(2).size() >=
            MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME)
        {
            ShowMessages("name of the output cannot exceed form %d.\n\n",
                         MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME);
            CommandOutputHelp();
            return;
        }

        //
        // Search to see if there is another output source with the
        // same name which we don't want to create two or more output
        // sources with the same name
        //

        if (g_OutputSourcesInitialized)
        {
            TempList = &g_OutputSources;

            while (&g_OutputSources != TempList->Flink)
            {
                TempList = TempList->Flink;

                PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails =
                    CONTAINING_RECORD(TempList, DEBUGGER_EVENT_FORWARDING, OutputSourcesList);

                if (strcmp(CurrentOutputSourceDetails->Name,
                           SplittedCommand.at(2).c_str()) == 0)
                {
                    //
                    // Indicate that we found this item
                    //
                    OutputSourceFound = TRUE;

                    //
                    // No need to search through the list anymore
                    //
                    break;
                }
            }

            //
            // Check whether the entered name already exists
            //
            if (OutputSourceFound)
            {
                ShowMessages("err, the name you entered, already exists, please choose "
                             "another name.\n\n");
                return;
            }
        }

        //
        // try to open the source and get the handle
        //

        DetailsOfSource = Command.substr(Command.find(SplittedCommand.at(3)) +
                                             SplittedCommand.at(3).size() + 1,
                                         Command.size());

        SourceHandle = ForwardingCreateOutputSource(Type, DetailsOfSource, &Socket);

        //
        // Check if it's a valid handle or not
        //
        if (SourceHandle == INVALID_HANDLE_VALUE)
        {
            ShowMessages(
                "err, invalid address or cannot open or find the address.\n\n");
            return;
        }

        //
        // allocate the buffer for storing the event forwarding details
        //
        EventForwardingObject =
            (PDEBUGGER_EVENT_FORWARDING)malloc(sizeof(DEBUGGER_EVENT_FORWARDING));

        if (EventForwardingObject == NULL)
        {
            ShowMessages("err, in allocating memory for event forwarding.\n\n");
            return;
        }

        RtlZeroMemory(EventForwardingObject, sizeof(DEBUGGER_EVENT_FORWARDING));

        //
        // Set the state
        //
        EventForwardingObject->State = EVENT_FORWARDING_STATE_NOT_OPENED;

        //
        // Set the type
        //
        EventForwardingObject->Type = Type;

        //
        // Get a new tag
        //
        EventForwardingObject->OutputUniqueTag = ForwardingGetNewOutputSourceTag();

        //
        // Set the handle or in the case of TCP, set the socket
        //
        if (Type == EVENT_FORWARDING_TCP)
        {
            EventForwardingObject->Socket = Socket;
        }
        else
        {
            EventForwardingObject->Handle = SourceHandle;
        }

        //
        // Move the name of the output source to the buffer
        //
        strcpy_s(EventForwardingObject->Name, SplittedCommand.at(2).c_str());

        //
        // Check if list is initialized or not
        //
        if (!g_OutputSourcesInitialized)
        {
            g_OutputSourcesInitialized = TRUE;
            InitializeListHead(&g_OutputSources);
        }

        //
        // Add the source to the trace list
        //
        InsertHeadList(&g_OutputSources,
                       &(EventForwardingObject->OutputSourcesList));
    }
    else if (!SplittedCommand.at(1).compare("open"))
    {
        //
        // It's an open
        //
        if (!g_OutputSourcesInitialized)
        {
            ShowMessages("err, the name you entered, not found.\n\n");
            return;
        }
        //
        // Now we should find the corresponding object in the memory and
        // pass it to the global open functions
        //
        TempList = &g_OutputSources;

        while (&g_OutputSources != TempList->Flink)
        {
            TempList = TempList->Flink;

            PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails = CONTAINING_RECORD(
                TempList,
                DEBUGGER_EVENT_FORWARDING,
                OutputSourcesList);

            if (strcmp(CurrentOutputSourceDetails->Name,
                       SplittedCommand.at(2).c_str()) == 0)
            {
                //
                // Indicate that we found this item
                //
                OutputSourceFound = TRUE;

                //
                // Open the output
                //
                Status = ForwardingOpenOutputSource(CurrentOutputSourceDetails);

                if (Status == DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED)
                {
                    ShowMessages("err, the name you entered was already closed.\n\n");
                    return;
                }
                else if (Status == DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_OPENED)
                {
                    ShowMessages("err, the name you entered was already opened.\n\n");
                    return;
                }
                else if (Status !=
                         DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_OPENED)
                {
                    ShowMessages("err, unable to open the output source.\n\n");
                    return;
                }

                //
                // No need to search through the list anymore
                //
                break;
            }
        }

        if (!OutputSourceFound)
        {
            ShowMessages("err, the name you entered, not found.\n\n");
            return;
        }
    }
    else if (!SplittedCommand.at(1).compare("close"))
    {
        //
        // It's a close
        //
        if (!g_OutputSourcesInitialized)
        {
            ShowMessages("err, the name you entered, not found.\n\n");
            return;
        }

        //
        // Now we should find the corresponding object in the memory and
        // pass it to the global close functions
        //
        TempList = &g_OutputSources;

        while (&g_OutputSources != TempList->Flink)
        {
            TempList = TempList->Flink;

            PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails = CONTAINING_RECORD(
                TempList,
                DEBUGGER_EVENT_FORWARDING,
                OutputSourcesList);

            if (strcmp(CurrentOutputSourceDetails->Name,
                       SplittedCommand.at(2).c_str()) == 0)
            {
                //
                // Indicate that we found this item
                //
                OutputSourceFound = TRUE;

                //
                // Close the output if it's not already closed
                //
                Status = ForwardingCloseOutputSource(CurrentOutputSourceDetails);

                if (Status == DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED)
                {
                    ShowMessages("err, the name you entered was already closed.\n\n");
                    return;
                }
                else if (Status == DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR)
                {
                    ShowMessages("err, unable to close the source.\n\n");
                    return;
                }
                else if (Status !=
                         DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED)
                {
                    ShowMessages("err, unable to close the source.\n\n");
                    return;
                }

                //
                // No need to search through the list anymore
                //
                break;
            }
        }

        if (!OutputSourceFound)
        {
            ShowMessages("err, the name you entered, not found.\n\n");
            return;
        }
    }
    else
    {
        //
        // Invalid argument
        //
        ShowMessages("incorrect option at '%s'\n\n", SplittedCommand.at(1).c_str());
        CommandOutputHelp();
        return;
    }
}
