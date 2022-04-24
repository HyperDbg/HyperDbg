/**
 * @file debugger.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpret general fields
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Global Variables
//
extern UINT64                   g_EventTag;
extern LIST_ENTRY               g_EventTrace;
extern BOOLEAN                  g_EventTraceInitialized;
extern BOOLEAN                  g_BreakPrintingOutput;
extern BOOLEAN                  g_AutoUnpause;
extern BOOLEAN                  g_OutputSourcesInitialized;
extern LIST_ENTRY               g_OutputSources;
extern BOOLEAN                  g_IsConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsConnectedToRemoteDebugger;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebugger;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief shows the error message
 *
 * @param Error
 * @return BOOLEAN
 */
BOOLEAN
ShowErrorMessage(UINT32 Error)
{
    switch (Error)
    {
    case DEBUGGER_ERROR_TAG_NOT_EXISTS:
        ShowMessages("err, tag not found (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_ACTION_TYPE:
        ShowMessages("err, invalid action type (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO:
        ShowMessages("err, action buffer size is zero (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EVENT_TYPE_IS_INVALID:
        ShowMessages("err, the event type is invalid (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_CREATE_EVENT:
        ShowMessages("err, unable to create event (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_ADDRESS:
        ShowMessages("err, invalid address (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_CORE_ID:
        ShowMessages("err, invalid core id (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES:
        ShowMessages("err, exception index exceed first 32 entries (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID:
        ShowMessages("err, interrupt index is not valid (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER:
        ShowMessages("err, unable to hide or unhide debugger (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_DEBUGGER_ALREADY_UHIDE:
        ShowMessages("err, debugger already unhide (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER:
        ShowMessages("err, edit memeory request has invalid parameters (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS:
        ShowMessages("err, edit memory request has invalid address based on "
                     "current process layout, the address might be valid but not "
                     "present in the ram (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS:
        ShowMessages("err, edit memory request has invalid address based on other "
                     "process layout (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TAG:
        ShowMessages("err, modify event with invalid tag (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TYPE_OF_ACTION:
        ShowMessages("err, modify event with invalid type of action (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_STEPPING_INVALID_PARAMETER:
        ShowMessages("err, invalid parameter passsed to stepping core. (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_STEPPINGS_EITHER_THREAD_NOT_FOUND_OR_DISABLED:
        ShowMessages(
            "err, the target thread not found or the thread is disabled (%x)\n",
            Error);
        break;

    case DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_BAUDRATE:
        ShowMessages("err, invalid baud rate (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT:
        ShowMessages("err, invalid serial port (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_CORE_IN_REMOTE_DEBUGGE:
        ShowMessages("err, invalid core selected in switching cores (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_PREPARING_DEBUGGEE_UNABLE_TO_SWITCH_TO_NEW_PROCESS:
        ShowMessages("err, unable to switch to the new process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_PREPARING_DEBUGGEE_TO_RUN_SCRIPT:
        ShowMessages("err, unable to run the script on remote debuggee (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_REGISTER_NUMBER:
        ShowMessages("err, invalid register number (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_WITHOUT_CONTINUE:
        ShowMessages("err, maximum number of breakpoints are used, you need to "
                     "send an ioctl to re-allocate new pre-allocated buffers or "
                     "configure HyperDbg to pre-allocated more buffers by "
                     "configuring MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_BREAKPOINT_ALREADY_EXISTS_ON_THE_ADDRESS:
        ShowMessages("err, breakpoint already exists on target address, you can "
                     "use 'bl' command to view a list of breakpoints (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND:
        ShowMessages("err, breakpoint id is invalid (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_BREAKPOINT_ALREADY_DISABLED:
        ShowMessages("err, breakpoint already disabled (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_BREAKPOINT_ALREADY_ENABLED:
        ShowMessages("err, breakpoint already enabled (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_MEMORY_TYPE_INVALID:
        ShowMessages("err, the memory type is invalid (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_PROCESS_ID:
        ShowMessages("err, the process id is invalid, make sure to enter the "
                     "process id in hex format (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EVENT_IS_NOT_APPLIED:
        ShowMessages("err, the event is not applied (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER:
        ShowMessages("err, either the process id or the _EPROCESS is invalid or "
                     "cannot get the details based on the provided parameters (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER:
        ShowMessages("err, either the thread id, _ETHREAD, or _EPROCESS is invalid or "
                     "cannot get the details based on the provided parameters (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_FOR_A_SINGLE_PAGE_IS_HIT:
        ShowMessages("err, the maximum breakpoint for a single page is hit, "
                     "you cannot apply more breakpoints in this page (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY:
        ShowMessages("err, the pre-allocated buffer is empty, usually this buffer will be "
                     "filled at the next IOCTL when the debugger is continued (%x)\n"
                     "please visit the documentation for the 'prealloc' command or use "
                     "'.help prealloc' to to reserve more pre-allocated pools\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES:
        ShowMessages("err, could not convert 2MB large page to 4KB pages (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS:
        ShowMessages("err, failed to get the PML1 entry of the target address (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_EPT_MULTIPLE_HOOKS_IN_A_SINGLE_PAGE:
        ShowMessages("err, the page modification is not applied, make sure that you don't "
                     "put multiple EPT Hooks or Monitors on a single page (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_COULD_NOT_BUILD_THE_EPT_HOOK:
        ShowMessages("err, could not build the EPT hook (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_COULD_NOT_FIND_ALLOCATION_TYPE:
        ShowMessages("err, could not find the allocation type (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_TEST_QUERY_INDEX:
        ShowMessages("err, invalid index specified for test query command (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS:
        ShowMessages("err, unable to attach to the target process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS_ENTRYPOINT_NOT_REACHED:
        ShowMessages("err, unable to remove hooks as the entrypoint of user-mode "
                     "process is not reached yet (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS:
        ShowMessages("err, unable to remove hooks (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_FUNCTIONS_FOR_INITIALIZING_PEB_ADDRESSES_ARE_NOT_INITIALIZED:
        ShowMessages("err, the routines for getting the PEB is not correctly "
                     "initialized (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_DETECT_32_BIT_OR_64_BIT_PROCESS:
        ShowMessages("err, unable to detect whether the process was 32-bit "
                     "or 64-bit (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_KILL_THE_PROCESS:
        ShowMessages("err, unable to kill the process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_INVALID_THREAD_DEBUGGING_TOKEN:
        ShowMessages("err, invalid thread debugging token (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_PAUSE_THE_PROCESS_THREADS:
        ShowMessages("err, unable to pause the threads of the process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_AN_ALREADY_ATTACHED_PROCESS:
        ShowMessages("err, the user debugger is already attached to this "
                     "process, please use the '.switch' command to switch "
                     "to this process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_THE_USER_DEBUGGER_NOT_ATTACHED_TO_THE_PROCESS:
        ShowMessages("err, the user debugger is not already attached to "
                     "the process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_DETACH_AS_THERE_ARE_PAUSED_THREADS:
        ShowMessages("err, the user debugger is not able to detach from "
                     "this process as there are paused threads in the "
                     "target process, please make sure to remove all "
                     "the events and continue the target process, then "
                     "perform the detach again (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_SWITCH_PROCESS_ID_OR_THREAD_ID_IS_INVALID:
        ShowMessages("err, unable to switch to the process id or thread id "
                     "as the target process id or thread id is not found in "
                     "the attached threads list, please view the list of "
                     "processes and threads by using the '.switch list' command (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_SWITCH_THERE_IS_NO_THREAD_ON_THE_PROCESS:
        ShowMessages("err, unable to switch to the process as the process doesn't "
                     "contain an active intercepted thread (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_GET_MODULES_OF_THE_PROCESS:
        ShowMessages("err, unable to get user-mode modules of the process (%x)\n",
                     Error);
        break;

    case DEBUGGER_ERROR_UNABLE_TO_GET_CALLSTACK:
        ShowMessages("err, unable to get the callstack (%x)\n",
                     Error);
        break;

    default:
        ShowMessages("err, error not found (%x)\n",
                     Error);
        return FALSE;
        break;
    }

    return TRUE;
}

/**
 * @brief Get ntoskrnl.exe base in the kernel
 *
 * @return UINT64 Base address of ntoskrnl.exe
 */
UINT64
DebuggerGetNtoskrnlBase()
{
    NTSTATUS             Status                  = STATUS_UNSUCCESSFUL;
    UINT64               NtoskrnlBase            = NULL;
    PRTL_PROCESS_MODULES Modules                 = NULL;
    ULONG                SysModuleInfoBufferSize = 0;

    //
    // Get required size of "RTL_PROCESS_MODULES" buffer
    //
    Status = NtQuerySystemInformation(SystemModuleInformation, NULL, NULL, &SysModuleInfoBufferSize);

    Modules = (PRTL_PROCESS_MODULES)malloc(SysModuleInfoBufferSize);

    NtQuerySystemInformation(SystemModuleInformation, Modules, SysModuleInfoBufferSize, NULL);

    for (int i = 0; i < Modules->NumberOfModules; i++)
    {
        if (!strcmp((const char *)Modules->Modules[i].FullPathName +
                        Modules->Modules[i].OffsetToFileName,
                    "ntoskrnl.exe"))
        {
            NtoskrnlBase = (UINT64)Modules->Modules[i].ImageBase;
            break;
        }
    }

    free(Modules);

    return NtoskrnlBase;
}

/**
 * @brief pauses the debuggee
 *
 * @return BOOLEAN shows whether the pause was successful or not, if successful
 * then when it returns true the debuggee is not paused anymore (continued)
 */
BOOLEAN
DebuggerPauseDebuggee()
{
    BOOLEAN                        StatusIoctl    = 0;
    ULONG                          ReturnedLength = 0;
    DEBUGGER_PAUSE_PACKET_RECEIVED PauseRequest   = {0};

    //
    // Send a pause IOCTL
    //
    StatusIoctl =
        DeviceIoControl(g_DeviceHandle,                        // Handle to device
                        IOCTL_PAUSE_PACKET_RECEIVED,           // IO Control code
                        &PauseRequest,                         // Input Buffer to driver.
                        SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED, // Input buffer
                                                               // length
                        &PauseRequest,                         // Output Buffer from driver.
                        SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED, // Length of output
                                                               // buffer in bytes.
                        &ReturnedLength,                       // Bytes placed in buffer.
                        NULL                                   // synchronous call
        );

    if (!StatusIoctl)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    if (PauseRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Nothing to show, the request was successfully processed
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(PauseRequest.Result);
        return FALSE;
    }
    return FALSE;
}

/**
 * @brief Shows whether the debugger is connected to a debugger
 * or debuggee connected to a debugger
 * @details we use this function to avoid connecting to a remote machine whem\n
 * debuggee or debugger is already connected to an instance
 *
 * @return BOOLEAN
 */
BOOLEAN
IsConnectedToAnyInstanceOfDebuggerOrDebuggee()
{
    if (g_DeviceHandle)
    {
        ShowMessages("err, the current system is already connected to the local "
                     "debugging, use '.disconnect' to disconnect\n");
        return TRUE;
    }
    else if (g_IsConnectedToRemoteDebuggee)
    {
        ShowMessages("err, the current system is already connected to remote "
                     "machine (debuggee), use '.disconnect' to disconnect from the "
                     "remote machine\n");
        return TRUE;
    }
    else if (g_IsConnectedToRemoteDebugger)
    {
        ShowMessages("err, the current system is already connected to remote "
                     "machine (debugger), use '.disconnect' to disconnect from the "
                     "remote machine from debugger\n");
        return TRUE;
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages(
            "err, the current system is already connected to remote "
            "machine (debuggee), use '.debug close' to disconnect from the "
            "remote machine\n");
        return TRUE;
    }
    else if (g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages(
            "err, the current system is already connected to remote "
            "machine (debugger), use '.debug close' to disconnect from the "
            "remote machine from debugger\n");
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Check whether the tag exists or not, if the tag is
 * DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG then if we find just one event, it
 * also means that the tag is found
 *
 * @param Tag
 * @return BOOLEAN
 */
BOOLEAN
IsTagExist(UINT64 Tag)
{
    PLIST_ENTRY                    TempList      = 0;
    PDEBUGGER_GENERAL_EVENT_DETAIL CommandDetail = {0};

    if (!g_EventTraceInitialized)
    {
        return FALSE;
    }

    TempList = &g_EventTrace;
    while (&g_EventTrace != TempList->Blink)
    {
        TempList = TempList->Blink;

        CommandDetail = CONTAINING_RECORD(TempList, DEBUGGER_GENERAL_EVENT_DETAIL, CommandsEventList);

        if (CommandDetail->Tag == Tag ||
            Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
        {
            return TRUE;
        }
    }

    //
    // Not found
    //
    return FALSE;
}

/**
 * @brief Interpret script (if an event has script)
 * @details If this function returns true then it means that there is a
 * script in this command split and the details are returned in the
 * input structure
 *
 * @param SplittedCommand the initialized command that are splitted by space
 * @param SplittedCommandCaseSensitive case sensitive splitted command
 * @param BufferAddress the address that the allocated buffer will be saved on
 * it
 * @param BufferLength the length of the buffer
 * @return BOOLEAN shows whether the interpret was successful (true) or not
 * successful (false)
 */
BOOLEAN
InterpretScript(vector<string> * SplittedCommand,
                vector<string> * SplittedCommandCaseSensitive,
                PBOOLEAN         ScriptSyntaxErrors,
                PUINT64          BufferAddress,
                PUINT32          BufferLength,
                PUINT32          Pointer,
                PUINT64          ScriptCodeBuffer)
{
    BOOLEAN        IsTextVisited = FALSE;
    BOOLEAN        IsInState     = FALSE;
    BOOLEAN        IsEnded       = FALSE;
    string         AppendedFinalBuffer;
    vector<string> SaveBuffer;
    vector<int>    IndexesToRemove;
    UCHAR *        FinalBuffer;
    int            Index            = 0;
    int            NewIndexToRemove = 0;
    int            OpenBracket      = 0;
    size_t         CountOfOpenBrackets;
    size_t         CountOfCloseBrackets;
    UINT32         IndexInCommandCaseSensitive          = 0;
    vector<string> SplittedCommandCaseSensitiveInstance = *SplittedCommandCaseSensitive;
    string         TempStr;

    //
    // Indicate that there is an error at first
    //
    *ScriptSyntaxErrors = TRUE;

    for (auto Section : *SplittedCommand)
    {
        IndexInCommandCaseSensitive++;
        Index++;

        if (IsInState)
        {
            if (OpenBracket == 0 && Section.find('{') == string::npos)
            {
                //
                // Check if the buffer is ended or not
                //
                if (!Section.compare("}"))
                {
                    //
                    // Save to remove this string from the command
                    //
                    IndexesToRemove.push_back(Index);
                    IsEnded = TRUE;
                    break;
                }

                //
                // Check if the script is end or not
                //
                if (HasEnding(Section, "}"))
                {
                    //
                    // Save to remove this string from the command
                    //
                    IndexesToRemove.push_back(Index);

                    //
                    // remove the last character and append it to the ConditionBuffer
                    //
                    TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1);
                    SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                    IsEnded = TRUE;
                    break;
                }
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            //
            // Check if script contains bracket "{"
            //
            if (Section.find('{') != string::npos)
            {
                //
                // We find a { between the script
                //

                //
                // Check the count of brackets in the string and add it to OpenBracket
                //
                size_t CountOfBrackets = count(Section.begin(), Section.end(), '{');

                //
                // Add it to the open brackets
                //
                OpenBracket += CountOfBrackets;
            }

            //
            // Check if script contains bracket "}"
            //
            if (Section.find('}') != string::npos)
            {
                //
                // We find a } between the script
                //

                //
                // Check the count of brackets in the string and add it to OpenBracket
                //
                size_t CountOfBrackets = count(Section.begin(), Section.end(), '}');

                //
                // Add it to the open brackets
                //
                OpenBracket -= CountOfBrackets;

                if (OpenBracket < 0)
                {
                    OpenBracket = 0;
                    IsEnded     = TRUE;
                    TempStr     = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1);
                    SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);
                    break;
                }
            }

            //
            // Add the text as the script screen
            //
            SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1));

            //
            // We want to stay in this condition
            //
            continue;
        }

        if (IsTextVisited && !Section.compare("{"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsInState = TRUE;
            continue;
        }

        if (IsTextVisited && Section.rfind('{', 0) == 0)
        {
            //
            // Section starts with {
            //

            //
            // Check if script contains bracket "{"
            //
            if (Section.find('{') != string::npos)
            {
                //
                // We find a { between the script
                //

                //
                // Check the count of brackets in the string and add it to OpenBracket
                //
                size_t CountOfBrackets = count(Section.begin(), Section.end(), '{');

                //
                // Add it to the open brackets (-1 because script starts with { which
                // is not related to our interpretation of script)
                //
                OpenBracket += CountOfBrackets - 1;
            }

            //
            // Check if it ends with }
            //
            if (OpenBracket == 0 && HasEnding(Section, "}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 1);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded = TRUE;
                break;
            }

            //
            // Check if script contains bracket "}"
            //
            if (Section.find('}') != string::npos)
            {
                //
                // We find a } between the script
                //

                //
                // Check the count of brackets in the string and add it to OpenBracket
                //
                size_t CountOfBrackets = count(Section.begin(), Section.end(), '}');

                //
                // Add it to the open brackets
                //
                OpenBracket -= CountOfBrackets;

                if (OpenBracket < 0)
                {
                    OpenBracket = 0;
                    IsEnded     = TRUE;
                    TempStr     = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1);
                    SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);
                    break;
                }
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 1));

            IsInState = TRUE;
            continue;
        }

        //
        // Check if it's a script
        //
        if (!Section.compare("script"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited = TRUE;
            continue;
        }

        //
        // Check if it's a script with script{
        //
        if (!Section.compare("script{"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited = TRUE;
            IsInState     = TRUE;
            continue;
        }

        //
        // It's a script
        //
        if (Section.rfind("script{", 0) == 0)
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited        = TRUE;
            IsInState            = TRUE;
            CountOfOpenBrackets  = count(Section.begin(), Section.end(), '{');
            CountOfCloseBrackets = count(Section.begin(), Section.end(), '}');

            //
            // Check if script contains bracket "{"
            //
            if (Section.find('{') != string::npos)
            {
                //
                // We find a { between the script
                //

                //
                // Add it to the open brackets (-1 because script starts with { which
                // is not related to our interpretation of script)
                //
                OpenBracket += CountOfOpenBrackets - 1;
            }

            if (CountOfOpenBrackets == CountOfCloseBrackets ||
                (OpenBracket == 0 && HasEnding(Section, "}")))
            {
                //
                // remove the last character and first character append it to the
                // ConditionBuffer
                //
                TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 7);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded     = TRUE;
                OpenBracket = 0;
                break;
            }
            else
            {
                //
                // Section starts with script{
                //
                SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 7));

                //
                // Check if script contains bracket "}"
                //
                if (Section.find('}') != string::npos)
                {
                    //
                    // We find a } between the script
                    //

                    //
                    // Check the count of brackets in the string and add it to OpenBracket
                    //
                    size_t CountOfBrackets = count(Section.begin(), Section.end(), '}');

                    //
                    // Add it to the open brackets
                    //
                    OpenBracket -= CountOfBrackets;

                    if (OpenBracket < 0)
                    {
                        OpenBracket = 0;
                        IsEnded     = TRUE;
                    }
                }

                continue;
            }
        }
    }

    //
    // Now we have everything in script buffer
    // Check to see if it is empty or not
    //
    if (SaveBuffer.size() == 0)
    {
        //
        // Nothing in condition buffer, return zero
        //
        return FALSE;
    }

    //
    // Check if we see that all the '{' ended with '}' or not
    //
    if (OpenBracket != 0)
    {
        //
        // Not all open brackets close with a }
        //
        return FALSE;
    }

    //
    // Check if we see '}' at the end
    //
    if (!IsEnded)
    {
        //
        // Nothing in condition buffer, return zero
        //
        return FALSE;
    }

    //
    // If we reach here then there is sth in script buffer
    //
    for (auto Section : SaveBuffer)
    {
        AppendedFinalBuffer.append(Section);
        AppendedFinalBuffer.append(" ");
    }

    if (AppendedFinalBuffer.rfind("file:", 0) == 0)
    {
        //
        // It's a file script
        //
        std::ifstream     t(AppendedFinalBuffer.erase(0, 5).c_str());
        std::stringstream buffer;
        buffer << t.rdbuf();
        AppendedFinalBuffer = buffer.str();
        if (AppendedFinalBuffer.empty())
        {
            ShowMessages("err, either script file is not found or it's empty\n");

            //
            // There was an error
            //
            *ScriptSyntaxErrors = TRUE;

            //
            // return TRUE to show that this item contains an script
            //
            return TRUE;
        }
    }

    // ShowMessages("script : %s\n", AppendedFinalBuffer.c_str());

    //
    // Run script engine handler
    //
    PVOID CodeBuffer =
        ScriptEngineParseWrapper((char *)AppendedFinalBuffer.c_str(), TRUE);

    if (CodeBuffer == NULL)
    {
        //
        // There was an error
        //
        *ScriptSyntaxErrors = TRUE;

        //
        // return TRUE to show that this item contains an script
        //
        return TRUE;
    }
    else
    {
        //
        // There is no syntax error
        //
        *ScriptSyntaxErrors = FALSE;
    }

    //
    // Print symbols (test)
    //
    // PrintSymbolBufferWrapper(CodeBuffer);

    //
    // Set the buffer and length
    //
    *BufferAddress    = ScriptEngineWrapperGetHead(CodeBuffer);
    *BufferLength     = ScriptEngineWrapperGetSize(CodeBuffer);
    *Pointer          = ScriptEngineWrapperGetPointer(CodeBuffer);
    *ScriptCodeBuffer = (UINT64)CodeBuffer;

    //
    // Removing the script indexes from the command
    //
    NewIndexToRemove = 0;

    for (auto IndexToRemove : IndexesToRemove)
    {
        NewIndexToRemove++;

        SplittedCommand->erase(SplittedCommand->begin() +
                               (IndexToRemove - NewIndexToRemove));
        SplittedCommandCaseSensitive->erase(SplittedCommandCaseSensitive->begin() +
                                            (IndexToRemove - NewIndexToRemove));
    }

    return TRUE;
}

/**
 * @brief Interpret conditions (if an event has condition) and custom code
 * @details If this function returns true then it means that there is a condtion
 * or code buffer in this command split and the details are returned in the
 * input structure
 *
 * @param SplittedCommand the initialized command that are splitted by space
 * @param SplittedCommandCaseSensitive the initialized command that are splitted
 * by space case sensitive
 * @param IsConditionBuffer is it a condition buffer or a custom code buffer
 * @param BufferAddress the address that the allocated buffer will be saved on
 * it
 * @param BufferLength the length of the buffer
 * @return BOOLEAN shows whether the interpret was successful (true) or not
 * successful (false)
 */
BOOLEAN
InterpretConditionsAndCodes(vector<string> * SplittedCommand,
                            vector<string> * SplittedCommandCaseSensitive,
                            BOOLEAN          IsConditionBuffer,
                            PUINT64          BufferAddress,
                            PUINT32          BufferLength)
{
    BOOLEAN        IsTextVisited = FALSE;
    BOOLEAN        IsInState     = FALSE;
    BOOLEAN        IsEnded       = FALSE;
    string         Temp;
    string         TempStr;
    string         AppendedFinalBuffer;
    vector<string> SaveBuffer;
    vector<CHAR>   ParsedBytes;
    vector<int>    IndexesToRemove;
    UCHAR *        FinalBuffer;
    int            NewIndexToRemove = 0;
    int            Index            = 0;

    for (auto Section : *SplittedCommand)
    {
        Index++;

        if (IsInState)
        {
            //
            // Check if the buffer is ended or not
            //
            if (!Section.compare("}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);
                IsEnded = TRUE;
                break;
            }

            //
            // Check if the condition is end or not
            //
            if (HasEnding(Section, "}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                //
                // remove the last character and append it to the ConditionBuffer
                //
                SaveBuffer.emplace_back(Section.begin(), Section.begin() + Section.size() - 1);

                IsEnded = TRUE;
                break;
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            //
            // Add the codes into condition bi
            //
            SaveBuffer.push_back(Section);

            //
            // We want to stay in this condition
            //
            continue;
        }

        if (IsTextVisited && !Section.compare("{"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsInState = TRUE;
            continue;
        }

        if (IsTextVisited && Section.rfind('{', 0) == 0)
        {
            //
            // Section starts with {
            //

            //
            // Check if it ends with }
            //
            if (HasEnding(Section, "}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                TempStr = Section.erase(0, 1);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded = TRUE;
                break;
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            SaveBuffer.push_back(Section.erase(0, 1));

            IsInState = TRUE;
            continue;
        }

        if (IsConditionBuffer)
        {
            if (!Section.compare("condition"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                continue;
            }
        }
        else
        {
            //
            // It's code
            //
            if (!Section.compare("code"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                continue;
            }
        }

        if (IsConditionBuffer)
        {
            if (!Section.compare("condition{"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                IsInState     = TRUE;
                continue;
            }
        }
        else
        {
            //
            // It's code
            //
            if (!Section.compare("code{"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                IsInState     = TRUE;
                continue;
            }
        }

        if (IsConditionBuffer)
        {
            if (Section.rfind("condition{", 0) == 0)
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                IsInState     = TRUE;

                if (!HasEnding(Section, "}"))
                {
                    //
                    // Section starts with condition{
                    //
                    SaveBuffer.push_back(Section.erase(0, 10));
                    continue;
                }
                else
                {
                    //
                    // remove the last character and first character append it to the
                    // ConditionBuffer
                    //
                    TempStr = Section.erase(0, 10);
                    SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                    IsEnded = TRUE;
                    break;
                }
            }
        }
        else
        {
            //
            // It's a code
            //
            if (Section.rfind("code{", 0) == 0)
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                IsTextVisited = TRUE;
                IsInState     = TRUE;

                if (!HasEnding(Section, "}"))
                {
                    //
                    // Section starts with code{
                    //
                    SaveBuffer.push_back(Section.erase(0, 5));
                    continue;
                }
                else
                {
                    //
                    // remove the last character and first character append it to the
                    // ConditionBuffer
                    //
                    TempStr = Section.erase(0, 5);
                    SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                    IsEnded = TRUE;
                    break;
                }
            }
        }
    }

    //
    // Now we have everything in condition buffer
    // Check to see if it is empty or not
    //
    if (SaveBuffer.size() == 0)
    {
        //
        // Nothing in condition buffer, return zero
        //
        return FALSE;
    }

    //
    // Check if we see '}' at the end
    //
    if (!IsEnded)
    {
        //
        // Nothing in condition buffer, return zero
        //
        return FALSE;
    }

    //
    // Append a 'ret' at the end of the buffer
    //
    SaveBuffer.push_back("c3");

    //
    // If we reach here then there is sth in condition buffer
    //
    for (auto Section : SaveBuffer)
    {
        //
        // Check if the section is started with '0x'
        //
        if (Section.rfind("0x", 0) == 0 || Section.rfind("0X", 0) == 0 ||
            Section.rfind("\\x", 0) == 0 || Section.rfind("\\X", 0) == 0)
        {
            Temp = Section.erase(0, 2);
        }
        else if (Section.rfind('x', 0) == 0 || Section.rfind('X', 0) == 0)
        {
            Temp = Section.erase(0, 1);
        }
        else
        {
            Temp = std::move(Section);
        }

        //
        // replace \x s
        //
        ReplaceAll(Temp, "\\x", "");

        //
        // check if the buffer is aligned to 2
        //
        if (Temp.size() % 2 != 0)
        {
            //
            // Add a zero to the start of the buffer
            //
            Temp.insert(0, 1, '0');
        }

        if (!IsHexNotation(Temp))
        {
            ShowMessages("please enter condition code in a hex notation\n");
            return FALSE;
        }
        AppendedFinalBuffer.append(Temp);
    }

    //
    // Convert it to vectored bytes
    //
    ParsedBytes = HexToBytes(AppendedFinalBuffer);

    //
    // Convert to a contigues buffer
    //
    FinalBuffer = (unsigned char *)malloc(ParsedBytes.size());
    std::copy(ParsedBytes.begin(), ParsedBytes.end(), FinalBuffer);

    //
    // Set the buffer and length
    //
    *BufferAddress = (UINT64)FinalBuffer;
    *BufferLength  = ParsedBytes.size();

    //
    // Removing the code or condition indexes from the command
    //
    NewIndexToRemove = 0;
    for (auto IndexToRemove : IndexesToRemove)
    {
        NewIndexToRemove++;

        SplittedCommand->erase(SplittedCommand->begin() +
                               (IndexToRemove - NewIndexToRemove));
        SplittedCommandCaseSensitive->erase(SplittedCommandCaseSensitive->begin() +
                                            (IndexToRemove - NewIndexToRemove));
    }

    return TRUE;
}

/**
 * @brief Interpret output (if an event has special output)
 * @details If this function returns true then it means that there is a special
 * input that needs to be considered for this event (other than just printing)
 * like sending over network, save to file, and send over a namedpipe
 *
 * @param SplittedCommand the initialized command that are splitted by space
 * @param SplittedCommandCaseSensitive the initialized command that are splitted
 * by space case sensitive
 * @param BufferAddress the address that the allocated buffer will be saved on
 * it
 * @param BufferLength the length of the buffer
 * @return BOOLEAN shows whether the interpret was successful (true) or not
 * successful (false)
 */
BOOLEAN
InterpretOutput(vector<string> * SplittedCommand,
                vector<string> * SplittedCommandCaseSensitive,
                vector<string> & InputSources)
{
    BOOLEAN        IsTextVisited = FALSE;
    BOOLEAN        IsInState     = FALSE;
    BOOLEAN        IsEnded       = FALSE;
    string         AppendedFinalBuffer;
    vector<string> SaveBuffer;
    vector<int>    IndexesToRemove;
    string         Token;
    string         TempStr;
    int            NewIndexToRemove                     = 0;
    int            Index                                = 0;
    char           Delimiter                            = ',';
    size_t         Pos                                  = 0;
    vector<string> SplittedCommandCaseSensitiveInstance = *SplittedCommandCaseSensitive;
    UINT32         IndexInCommandCaseSensitive          = 0;

    for (auto Section : *SplittedCommand)
    {
        IndexInCommandCaseSensitive++;
        Index++;

        if (IsInState)
        {
            //
            // Check if the buffer is ended or not
            //
            if (!Section.compare("}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);
                IsEnded = TRUE;
                break;
            }

            //
            // Check if the output is end or not
            //
            if (HasEnding(Section, "}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                //
                // remove the last character and append it to the output buffer
                //
                TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded = TRUE;
                break;
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            //
            // Add the codes into buffer buffer
            //
            SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1));

            //
            // We want to stay in this condition
            //
            continue;
        }

        if (IsTextVisited && !Section.compare("{"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsInState = TRUE;
            continue;
        }

        if (IsTextVisited && Section.rfind('{', 0) == 0)
        {
            //
            // Section starts with {
            //

            //
            // Check if it ends with }
            //
            if (HasEnding(Section, "}"))
            {
                //
                // Save to remove this string from the command
                //
                IndexesToRemove.push_back(Index);

                TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 1);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded = TRUE;
                break;
            }

            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 1));

            IsInState = TRUE;
            continue;
        }

        if (!Section.compare("output"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited = TRUE;
            continue;
        }

        if (!Section.compare("output{"))
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited = TRUE;
            IsInState     = TRUE;
            continue;
        }

        if (Section.rfind("output{", 0) == 0)
        {
            //
            // Save to remove this string from the command
            //
            IndexesToRemove.push_back(Index);

            IsTextVisited = TRUE;
            IsInState     = TRUE;

            if (!HasEnding(Section, "}"))
            {
                //
                // Section starts with output{
                //
                SaveBuffer.push_back(SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 7));
                continue;
            }
            else
            {
                //
                // remove the last character and first character append it to the
                // Output
                //
                TempStr = SplittedCommandCaseSensitiveInstance.at(IndexInCommandCaseSensitive - 1).erase(0, 7);
                SaveBuffer.emplace_back(TempStr.begin(), TempStr.begin() + TempStr.size() - 1);

                IsEnded = TRUE;
                break;
            }
        }
    }

    //
    // Now we have everything in buffer buffer
    // Check to see if it is empty or not
    //
    if (SaveBuffer.size() == 0)
    {
        //
        // Nothing in output buffer, return zero
        //
        return FALSE;
    }

    //
    // Check if we see '}' at the end
    //
    if (!IsEnded)
    {
        //
        // Nothing in output buffer, return zero
        //
        return FALSE;
    }

    //
    // If we reach here then there is sth in condition buffer
    //
    for (auto Section : SaveBuffer)
    {
        AppendedFinalBuffer.append(Section);
        AppendedFinalBuffer.append(" ");
    }

    //
    // Check if we see multiple sources or it's just one single output
    //
    if (AppendedFinalBuffer.find(Delimiter) != std::string::npos)
    {
        //
        // Delimiter found !
        //
        while ((Pos = AppendedFinalBuffer.find(Delimiter)) != string::npos)
        {
            Token = AppendedFinalBuffer.substr(0, Pos);
            Trim(Token);

            if (!Token.empty())
            {
                InputSources.push_back(Token);
            }

            AppendedFinalBuffer.erase(0, Pos + sizeof(Delimiter) / sizeof(char));
        }

        if (!AppendedFinalBuffer.empty())
        {
            InputSources.push_back(AppendedFinalBuffer);
        }
    }
    else
    {
        //
        // Delimiter not found !
        //
        InputSources.push_back(AppendedFinalBuffer);
    }

    //
    // Removing the code or condition indexes from the command
    //
    NewIndexToRemove = 0;
    for (auto IndexToRemove : IndexesToRemove)
    {
        NewIndexToRemove++;

        SplittedCommand->erase(SplittedCommand->begin() +
                               (IndexToRemove - NewIndexToRemove));
        SplittedCommandCaseSensitive->erase(SplittedCommandCaseSensitive->begin() +
                                            (IndexToRemove - NewIndexToRemove));
    }

    return TRUE;
}

/**
 * @brief Register the event to the kernel
 *
 * @param Event the event structure to send
 * @param EventBufferLength the buffer length of event
 * @return BOOLEAN if the request was successful then true
 * if the request was not successful then false
 */
BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32                         EventBufferLength)
{
    BOOL                                  Status;
    ULONG                                 ReturnedLength;
    DEBUGGER_EVENT_AND_ACTION_REG_BUFFER  ReturnedBuffer = {0};
    PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER TempRegResult;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's a debuggger, we should send the events buffer directly
        // from here
        //

        //
        // Send the buffer from here
        //
        TempRegResult =
            KdSendRegisterEventPacketToDebuggee(Event, EventBufferLength);

        //
        // Move the buffer to local buffer
        //
        memcpy(&ReturnedBuffer, TempRegResult, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));
    }
    else
    {
        //
        // It's either a debuggee or a local debugging instance
        //

        if (!g_DeviceHandle)
        {
            ShowMessages(
                "handle not found, probably the driver is not loaded. Did you "
                "use 'load' command?\n");
            return FALSE;
        }

        //
        // Send IOCTL
        //

        Status =
            DeviceIoControl(g_DeviceHandle,                               // Handle to device
                            IOCTL_DEBUGGER_REGISTER_EVENT,                // IO Control code
                            Event,                                        // Input Buffer to driver.
                            EventBufferLength,                            // Input buffer length
                            &ReturnedBuffer,                              // Output Buffer from driver.
                            sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                          // of
                                                                          // output
                                                                          // buffer
                                                                          // in
                                                                          // bytes.
                            &ReturnedLength,                              // Bytes placed in buffer.
                            NULL                                          // synchronous call
            );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return FALSE;
        }
    }

    if (ReturnedBuffer.IsSuccessful && ReturnedBuffer.Error == 0)
    {
        //
        // The returned buffer shows that this event was
        // successful and now we can add it to the list of
        // events
        //
        InsertHeadList(&g_EventTrace, &(Event->CommandsEventList));

        //
        // Check for auto-unpause mode
        //
        if (!g_IsSerialConnectedToRemoteDebuggee &&
            !g_IsSerialConnectedToRemoteDebugger &&
            g_BreakPrintingOutput &&
            g_AutoUnpause)
        {
            //
            // Allow debugger to show its contents
            //

            //
            // Set the g_BreakPrintingOutput to FALSE
            //
            g_BreakPrintingOutput = FALSE;

            //
            // If it's a remote debugger then we send the remote debuggee a 'g'
            //
            if (g_IsConnectedToRemoteDebuggee)
            {
                RemoteConnectionSendCommand("g", strlen("g") + 1);
            }

            ShowMessages("\n");
        }
    }
    else
    {
        //
        // It was not successful, we have to free the buffers for command
        // string and buffer for the event itself
        //
        free(Event->CommandStringBuffer);
        free(Event);

        //
        // Show the error
        //
        if (ReturnedBuffer.Error != 0)
        {
            ShowErrorMessage(ReturnedBuffer.Error);
        }
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Register the action to the event
 *
 * @param Action the action buffer structure
 * @param ActionsBufferLength length of action buffer
 * @return BOOLEAN BOOLEAN if the request was successful then true
 * if the request was not successful then false
 */
BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_ACTION ActionBreakToDebugger,
                      UINT32                   ActionBreakToDebuggerLength,
                      PDEBUGGER_GENERAL_ACTION ActionCustomCode,
                      UINT32                   ActionCustomCodeLength,
                      PDEBUGGER_GENERAL_ACTION ActionScript,
                      UINT32                   ActionScriptLength)
{
    BOOL                                  Status;
    ULONG                                 ReturnedLength;
    DEBUGGER_EVENT_AND_ACTION_REG_BUFFER  ReturnedBuffer = {0};
    PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER TempAddingResult;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's action(s) in debugger mode
        //

        //
        // Send Break to debugger packet to debuggee
        //
        if (ActionBreakToDebugger != NULL)
        {
            //
            // Send the add action to event from here
            //
            TempAddingResult = KdSendAddActionToEventPacketToDebuggee(
                ActionBreakToDebugger,
                ActionBreakToDebuggerLength);

            //
            // Move the buffer to local buffer
            //
            memcpy(&ReturnedBuffer, TempAddingResult, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionBreakToDebugger);
        }

        //
        // Send custom code packet to debuggee
        //
        if (ActionCustomCode != NULL)
        {
            //
            // Send the add action to event from here
            //
            TempAddingResult = KdSendAddActionToEventPacketToDebuggee(
                ActionCustomCode,
                ActionCustomCodeLength);

            //
            // Move the buffer to local buffer
            //
            memcpy(&ReturnedBuffer, TempAddingResult, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionCustomCode);
        }

        //
        // Send custom code packet to debuggee
        //
        if (ActionScript != NULL)
        {
            //
            // Send the add action to event from here
            //
            TempAddingResult = KdSendAddActionToEventPacketToDebuggee(
                ActionScript,
                ActionScriptLength);

            //
            // Move the buffer to local buffer
            //
            memcpy(&ReturnedBuffer, TempAddingResult, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionScript);
        }
    }
    else
    {
        //
        // It's either a local debugger to in vmi-mode remote conntection
        //

        if (!g_DeviceHandle)
        {
            ShowMessages(
                "handle not found, probably the driver is not loaded. Did you "
                "use 'load' command?\n");
            return FALSE;
        }

        //
        // Send IOCTLs
        //

        //
        // Send Break to debugger ioctl
        //
        if (ActionBreakToDebugger != NULL)
        {
            Status = DeviceIoControl(
                g_DeviceHandle,                               // Handle to device
                IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT,           // IO Control code
                ActionBreakToDebugger,                        // Input Buffer to driver.
                ActionBreakToDebuggerLength,                  // Input buffer length
                &ReturnedBuffer,                              // Output Buffer from driver.
                sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                              // of
                                                              // output
                                                              // buffer
                                                              // in
                                                              // bytes.
                &ReturnedLength,                              // Bytes placed in buffer.
                NULL                                          // synchronous call
            );

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionBreakToDebugger);

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }
        }

        //
        // Send custom code ioctl
        //
        if (ActionCustomCode != NULL)
        {
            Status = DeviceIoControl(
                g_DeviceHandle,                               // Handle to device
                IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT,           // IO Control code
                ActionCustomCode,                             // Input Buffer to driver.
                ActionCustomCodeLength,                       // Input buffer length
                &ReturnedBuffer,                              // Output Buffer from driver.
                sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                              // of
                                                              // output
                                                              // buffer
                                                              // in
                                                              // bytes.
                &ReturnedLength,                              // Bytes placed in buffer.
                NULL                                          // synchronous call
            );

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionCustomCode);

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }
        }

        //
        // Send custom code ioctl
        //
        if (ActionScript != NULL)
        {
            Status = DeviceIoControl(
                g_DeviceHandle,                               // Handle to device
                IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT,           // IO Control code
                ActionScript,                                 // Input Buffer to driver.
                ActionScriptLength,                           // Input buffer length
                &ReturnedBuffer,                              // Output Buffer from driver.
                sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                              // of
                                                              // output
                                                              // buffer
                                                              // in
                                                              // bytes.
                &ReturnedLength,                              // Bytes placed in buffer.
                NULL                                          // synchronous call
            );

            //
            // The action buffer is allocated once using malloc and
            // is not used anymore, we have to free it
            //
            free(ActionScript);

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * @brief Get the New Debugger Event Tag object and increase the
 * global variable for tag
 *
 * @return UINT64
 */
UINT64
GetNewDebuggerEventTag()
{
    return g_EventTag++;
}

/**
 * @brief Interpret general event fields
 *
 * @param SplittedCommand the commands that was splitted by space
 * @param SplittedCommandCaseSensitive the commands that was splitted by space
 * case sensitive
 * @param EventType type of event
 * @param EventDetailsToFill a pointer address that will be filled
 * by event detail buffer
 * @param EventBufferLength a pointer the receives the buffer length
 * of the event
 * @param ActionDetailsToFill a pointer address that will be filled
 * by action detail buffer
 * @param ActionBufferLength a pointer the receives the buffer length
 * of the action
 * @param ReasonForErrorInParsing reason that interpretation failed, null
 * if the returns true
 * @return BOOLEAN If this function returns true then it means that there
 * was no error in parsing the general event details
 */
BOOLEAN
InterpretGeneralEventAndActionsFields(
    vector<string> *                    SplittedCommand,
    vector<string> *                    SplittedCommandCaseSensitive,
    DEBUGGER_EVENT_TYPE_ENUM            EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL *    EventDetailsToFill,
    PUINT32                             EventBufferLength,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillBreakToDebugger,
    PUINT32                             ActionBufferLengthBreakToDebugger,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillCustomCode,
    PUINT32                             ActionBufferLengthCustomCode,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillScript,
    PUINT32                             ActionBufferLengthScript,
    PDEBUGGER_EVENT_PARSING_ERROR_CAUSE ReasonForErrorInParsing)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL TempEvent;
    PDEBUGGER_GENERAL_ACTION       TempActionBreak                = NULL;
    PDEBUGGER_GENERAL_ACTION       TempActionScript               = NULL;
    PDEBUGGER_GENERAL_ACTION       TempActionCustomCode           = NULL;
    UINT32                         LengthOfCustomCodeActionBuffer = 0;
    UINT32                         LengthOfScriptActionBuffer     = 0;
    UINT32                         LengthOfBreakActionBuffer      = 0;
    UINT64                         ConditionBufferAddress;
    UINT32                         ConditionBufferLength = 0;
    vector<string>                 ListOfOutputSources;
    UINT64                         CodeBufferAddress;
    UINT32                         CodeBufferLength = 0;
    UINT64                         ScriptBufferAddress;
    UINT64                         ScriptCodeBuffer     = 0;
    BOOLEAN                        HasScriptSyntaxError = 0;
    UINT32                         ScriptBufferLength   = 0;
    UINT32                         ScriptBufferPointer  = 0;
    UINT32                         LengthOfEventBuffer  = 0;
    string                         CommandString;
    BOOLEAN                        HasConditionBuffer              = FALSE;
    BOOLEAN                        HasOutputPath                   = FALSE;
    BOOLEAN                        HasCodeBuffer                   = FALSE;
    BOOLEAN                        HasScript                       = FALSE;
    BOOLEAN                        IsNextCommandPid                = FALSE;
    BOOLEAN                        IsNextCommandCoreId             = FALSE;
    BOOLEAN                        IsNextCommandBufferSize         = FALSE;
    BOOLEAN                        IsNextCommandImmediateMessaging = FALSE;
    BOOLEAN                        ImmediateMessagePassing         = UseImmediateMessagingByDefaultOnEvents;
    UINT32                         CoreId;
    UINT32                         ProcessId;
    UINT32                         IndexOfValidSourceTags;
    UINT32                         RequestBuffer = 0;
    PLIST_ENTRY                    TempList;
    BOOLEAN                        OutputSourceFound;
    vector<int>                    IndexesToRemove;
    vector<UINT64>                 ListOfValidSourceTags;
    int                            NewIndexToRemove = 0;
    int                            Index            = 0;

    //
    // Create a command string to show in the history
    //
    for (auto Section : *SplittedCommandCaseSensitive)
    {
        CommandString.append(Section);
        CommandString.append(" ");
    }

    //
    // Compute the size of buffer + 1 null for the end of buffer
    //

    UINT64 BufferOfCommandStringLength = CommandString.size() + 1;

    //
    // Allocate Buffer and zero for command to the buffer
    //
    PVOID BufferOfCommandString = malloc(BufferOfCommandStringLength);

    RtlZeroMemory(BufferOfCommandString, BufferOfCommandStringLength);

    //
    // Copy the string to the buffer
    //
    memcpy(BufferOfCommandString, CommandString.c_str(), CommandString.size());

    //
    // Check if there is a condition buffer in the command
    //
    if (!InterpretConditionsAndCodes(SplittedCommand, SplittedCommandCaseSensitive, TRUE, &ConditionBufferAddress, &ConditionBufferLength))
    {
        //
        // Indicate condition is not available
        //
        HasConditionBuffer = FALSE;

        //
        // ShowMessages("\nNo condition!\n");
        //
    }
    else
    {
        //
        // Indicate condition is available
        //
        HasConditionBuffer = TRUE;

        /*
    ShowMessages(
        "\n========================= Condition =========================\n");

    ShowMessages(
        "\nUINT64  DebuggerCheckForCondition(PGUEST_REGS Regs_RCX, PVOID "
        "Context_RDX)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler64((unsigned char *)ConditionBufferAddress, 0x0,
                           ConditionBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
      */
    }

    //
    // Check if there is a code buffer in the command
    //
    if (!InterpretConditionsAndCodes(SplittedCommand, SplittedCommandCaseSensitive, FALSE, &CodeBufferAddress, &CodeBufferLength))
    {
        //
        // Indicate code is not available
        //
        HasCodeBuffer = FALSE;
        //
        // ShowMessages("\nNo custom code!\n");
        //
    }
    else
    {
        //
        // Indicate code is available
        //
        HasCodeBuffer = TRUE;

        /*
    ShowMessages(
        "\n=========================    Code    =========================\n");
    ShowMessages("\nPVOID DebuggerRunCustomCodeFunc(PVOID "
                 "PreAllocatedBufferAddress_RCX, "
                 "PGUEST_REGS Regs_RDX, PVOID Context_R8)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler64((unsigned char *)CodeBufferAddress, 0x0,
                           CodeBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
        */
    }

    //
    // Check if there is a Script block in the command
    //
    if (!InterpretScript(SplittedCommand,
                         SplittedCommandCaseSensitive,
                         &HasScriptSyntaxError,
                         &ScriptBufferAddress,
                         &ScriptBufferLength,
                         &ScriptBufferPointer,
                         &ScriptCodeBuffer))
    {
        //
        // Indicate code is not available
        //
        HasScript = FALSE;
        //
        // ShowMessages("\nNo script!\n");
        //
    }
    else
    {
        //
        // Check if there is a syntax error
        //
        if (HasScriptSyntaxError)
        {
            free(BufferOfCommandString);

            *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_SCRIPT_SYNTAX_ERROR;
            return FALSE;
        }

        //
        // Indicate code is available
        //
        HasScript = TRUE;
    }

    //
    // Check if there is a output path in the command
    //
    if (!InterpretOutput(SplittedCommand, SplittedCommandCaseSensitive, ListOfOutputSources))
    {
        //
        // Indicate output is not available
        //
        HasOutputPath = FALSE;

        //
        // ShowMessages("\nNo condition!\n");
        //
    }
    else
    {
        //
        // Check for empty input
        //
        if (ListOfOutputSources.size() == 0)
        {
            //
            // No input !
            //
            free(BufferOfCommandString);

            ShowMessages("err, no input found\n");
            *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_NO_INPUT;
            return FALSE;
        }

        //
        // Check whether the size exceed maximum size or not
        //
        if (ListOfOutputSources.size() >
            DebuggerOutputSourceMaximumRemoteSourceForSingleEvent)
        {
            free(BufferOfCommandString);

            ShowMessages(
                "err, based on this build of HyperDbg, the maximum input sources for "
                "a single event is 0x%x sources but you entered 0x%x sources\n",
                DebuggerOutputSourceMaximumRemoteSourceForSingleEvent,
                ListOfOutputSources.size());
            *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_MAXIMUM_INPUT_REACHED;
            return FALSE;
        }

        //
        // Check whether the output source initialized or not
        //
        if (!g_OutputSourcesInitialized)
        {
            free(BufferOfCommandString);

            ShowMessages("err, the name you entered, not found. Did you use "
                         "'output' commmand to create it?\n");
            *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_OUTPUT_NAME_NOT_FOUND;
            return FALSE;
        }

        //
        // Convert output sources to the corresponding tags
        //
        for (auto item : ListOfOutputSources)
        {
            TempList          = 0;
            OutputSourceFound = FALSE;

            //
            // Now we should find the corresponding object in the list
            // of output sources and save its tags
            //
            TempList = &g_OutputSources;

            while (&g_OutputSources != TempList->Flink)
            {
                TempList = TempList->Flink;

                PDEBUGGER_EVENT_FORWARDING CurrentOutputSourceDetails =
                    CONTAINING_RECORD(TempList, DEBUGGER_EVENT_FORWARDING, OutputSourcesList);

                if (strcmp(CurrentOutputSourceDetails->Name,
                           RemoveSpaces(item).c_str()) == 0)
                {
                    //
                    // Check to see the source state, whether it is closed or not
                    //
                    if (CurrentOutputSourceDetails->State == EVENT_FORWARDING_CLOSED)
                    {
                        free(BufferOfCommandString);

                        ShowMessages("err, output source already closed\n");
                        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_OUTPUT_SOURCE_ALREADY_CLOSED;
                        return FALSE;
                    }

                    //
                    // Indicate that we found this item
                    //
                    OutputSourceFound = TRUE;

                    //
                    // Save the tag into a list which will be used later
                    //
                    ListOfValidSourceTags.push_back(
                        CurrentOutputSourceDetails->OutputUniqueTag);

                    //
                    // No need to search through the list anymore
                    //
                    break;
                }
            }

            if (!OutputSourceFound)
            {
                free(BufferOfCommandString);

                ShowMessages("err, the name you entered, not found. Did you use "
                             "'output' commmand to create it?\n");
                *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_OUTPUT_NAME_NOT_FOUND;
                return FALSE;
            }
        }
        //
        // Indicate output is available
        //
        HasOutputPath = TRUE;
    }

    //
    // Create action and event based on previously parsed buffers
    // (DEBUGGER_GENERAL_ACTION)
    //
    // Allocate the buffer (with ConditionBufferLength and CodeBufferLength)
    //

    /*

  Layout of Buffer :

   ________________________________
  |                                |
  |  DEBUGGER_GENERAL_EVENT_DETAIL |
  |                                |
  |________________________________|
  |                                |
  |       Condition Buffer         |
  |                                |
  |________________________________|

   */

    /*
   ________________________________
  |                                |
  |     DEBUGGER_GENERAL_ACTION    |
  |                                |
  |________________________________|
  |                                |
  |     Condition Custom Code      |
  |       or Script Buffer         |
  |________________________________|

  */

    LengthOfEventBuffer =
        sizeof(DEBUGGER_GENERAL_EVENT_DETAIL) + ConditionBufferLength;

    TempEvent = (PDEBUGGER_GENERAL_EVENT_DETAIL)malloc(LengthOfEventBuffer);
    RtlZeroMemory(TempEvent, LengthOfEventBuffer);

    //
    // Check if buffer is availabe
    //
    if (TempEvent == NULL)
    {
        //
        // The heap is not available
        //
        free(BufferOfCommandString);

        ShowMessages("err, allocation error\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_ALLOCATION_ERROR;
        return FALSE;
    }

    //
    // Event is enabled by default when created
    //
    TempEvent->IsEnabled = TRUE;

    //
    // Get a new tag for it
    //
    TempEvent->Tag = GetNewDebuggerEventTag();

    //
    // Set the core Id and Process Id to all cores and all
    // processes, next time we check whether the user needs
    // a special core or a special process then we change it
    //
    TempEvent->CoreId = DEBUGGER_EVENT_APPLY_TO_ALL_CORES;

    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("notice: as you're debugging a user-mode application, "
                     "this event will only trigger on your current debugging process "
                     "(pid:%x). If you want the event from the entire system, "
                     "add 'pid all' to the event\n",
                     g_ActiveProcessDebuggingState.ProcessId);

        TempEvent->ProcessId = g_ActiveProcessDebuggingState.ProcessId;
    }
    else
    {
        TempEvent->ProcessId = DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES;
    }

    //
    // Set the event type
    //
    TempEvent->EventType = EventType;

    //
    // Get the current time
    //
    TempEvent->CreationTime = time(0);

    //
    // Set buffer string command
    //
    TempEvent->CommandStringBuffer = BufferOfCommandString;

    //
    // Fill the buffer of condition for event
    //
    if (HasConditionBuffer)
    {
        memcpy((PVOID)((UINT64)TempEvent + sizeof(DEBUGGER_GENERAL_EVENT_DETAIL)),
               (PVOID)ConditionBufferAddress,
               ConditionBufferLength);

        //
        // Set the size of the buffer for event condition
        //
        TempEvent->ConditionBufferSize = ConditionBufferLength;
    }

    //
    // Fill the buffer of custom code for action
    //
    if (HasCodeBuffer)
    {
        //
        // Allocate the Action (THIS ACTION BUFFER WILL BE FREED WHEN WE SENT IT TO
        // THE KERNEL AND RETURNED FROM THE KERNEL AS WE DON'T NEED IT ANYMORE)
        //
        LengthOfCustomCodeActionBuffer =
            sizeof(DEBUGGER_GENERAL_ACTION) + CodeBufferLength;

        TempActionCustomCode =
            (PDEBUGGER_GENERAL_ACTION)malloc(LengthOfCustomCodeActionBuffer);

        RtlZeroMemory(TempActionCustomCode, LengthOfCustomCodeActionBuffer);

        memcpy(
            (PVOID)((UINT64)TempActionCustomCode + sizeof(DEBUGGER_GENERAL_ACTION)),
            (PVOID)CodeBufferAddress,
            CodeBufferLength);
        //
        // Set the action Tag
        //
        TempActionCustomCode->EventTag = TempEvent->Tag;

        //
        // Set the action type
        //
        TempActionCustomCode->ActionType = RUN_CUSTOM_CODE;

        //
        // Set the action buffer size
        //
        TempActionCustomCode->CustomCodeBufferSize = CodeBufferLength;

        //
        // Increase the count of actions
        //
        TempEvent->CountOfActions = TempEvent->CountOfActions + 1;
    }

    //
    // Fill the buffer of script for action
    //
    if (HasScript)
    {
        //
        // Allocate the Action (THIS ACTION BUFFER WILL BE FREED WHEN WE SENT IT TO
        // THE KERNEL AND RETURNED FROM THE KERNEL AS WE DON'T NEED IT ANYMORE)
        //
        LengthOfScriptActionBuffer =
            sizeof(DEBUGGER_GENERAL_ACTION) + ScriptBufferLength;
        TempActionScript =
            (PDEBUGGER_GENERAL_ACTION)malloc(LengthOfScriptActionBuffer);

        RtlZeroMemory(TempActionScript, LengthOfScriptActionBuffer);

        memcpy((PVOID)((UINT64)TempActionScript + sizeof(DEBUGGER_GENERAL_ACTION)),
               (PVOID)ScriptBufferAddress,
               ScriptBufferLength);
        //
        // Set the action Tag
        //
        TempActionScript->EventTag = TempEvent->Tag;

        //
        // Set the action type
        //
        TempActionScript->ActionType = RUN_SCRIPT;

        //
        // Set the action buffer size and pointer
        //
        TempActionScript->ScriptBufferSize    = ScriptBufferLength;
        TempActionScript->ScriptBufferPointer = ScriptBufferPointer;

        //
        // Increase the count of actions
        //
        TempEvent->CountOfActions = TempEvent->CountOfActions + 1;

        //
        // Free the buffer of script related functions
        //
        ScriptEngineWrapperRemoveSymbolBuffer((PVOID)ScriptCodeBuffer);
    }

    //
    // If this action didn't contain a buffer for custom code and
    // a buffer for script then it's a break to debugger
    //
    if (!HasCodeBuffer && !HasScript)
    {
        //
        // Allocate the Action (THIS ACTION BUFFER WILL BE FREED WHEN WE SENT IT TO
        // THE KERNEL AND RETURNED FROM THE KERNEL AS WE DON'T NEED IT ANYMORE)
        //
        LengthOfBreakActionBuffer = sizeof(DEBUGGER_GENERAL_ACTION);

        TempActionBreak =
            (PDEBUGGER_GENERAL_ACTION)malloc(LengthOfBreakActionBuffer);

        RtlZeroMemory(TempActionBreak, LengthOfBreakActionBuffer);

        //
        // Set the action Tag
        //
        TempActionBreak->EventTag = TempEvent->Tag;

        //
        // Set the action type
        //
        TempActionBreak->ActionType = BREAK_TO_DEBUGGER;

        //
        // Increase the count of actions
        //
        TempEvent->CountOfActions = TempEvent->CountOfActions + 1;
    }

    //
    // Interpret rest of the command
    //
    for (auto Section : *SplittedCommand)
    {
        Index++;
        if (IsNextCommandBufferSize)
        {
            if (!ConvertStringToUInt32(Section, &RequestBuffer))
            {
                free(BufferOfCommandString);
                free(TempEvent);

                if (TempActionBreak != NULL)
                {
                    free(TempActionBreak);
                }
                if (TempActionScript != NULL)
                {
                    free(TempActionScript);
                }
                if (TempActionCustomCode != NULL)
                {
                    free(TempActionCustomCode);
                }

                ShowMessages("err, buffer size is invalid\n");
                *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
                return FALSE;
            }
            else
            {
                //
                // Set the specific requested buffer size
                //
                if (TempActionBreak != NULL)
                {
                    TempActionBreak->PreAllocatedBuffer = RequestBuffer;
                }
                if (TempActionScript != NULL)
                {
                    TempActionScript->PreAllocatedBuffer = RequestBuffer;
                }
                if (TempActionCustomCode != NULL)
                {
                    TempActionCustomCode->PreAllocatedBuffer = RequestBuffer;
                }
            }
            IsNextCommandBufferSize = FALSE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (IsNextCommandImmediateMessaging)
        {
            if (!Section.compare("yes"))
            {
                ImmediateMessagePassing = TRUE;
            }
            else if (!Section.compare("no"))
            {
                ImmediateMessagePassing = FALSE;
            }
            else
            {
                //
                // err, not token recognized error
                //
                free(BufferOfCommandString);
                free(TempEvent);

                if (TempActionBreak != NULL)
                {
                    free(TempActionBreak);
                }
                if (TempActionScript != NULL)
                {
                    free(TempActionScript);
                }
                if (TempActionCustomCode != NULL)
                {
                    free(TempActionCustomCode);
                }

                ShowMessages("err, immediate messaging token is invalid\n");
                *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
                return FALSE;
            }

            IsNextCommandImmediateMessaging = FALSE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (IsNextCommandPid)
        {
            if (!Section.compare("all"))
            {
                TempEvent->ProcessId = DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES;
            }
            else if (!ConvertStringToUInt32(Section, &ProcessId))
            {
                free(BufferOfCommandString);
                free(TempEvent);

                if (TempActionBreak != NULL)
                {
                    free(TempActionBreak);
                }
                if (TempActionScript != NULL)
                {
                    free(TempActionScript);
                }
                if (TempActionCustomCode != NULL)
                {
                    free(TempActionCustomCode);
                }

                ShowMessages("err, pid is invalid\n");
                *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
                return FALSE;
            }
            else
            {
                //
                // Set the specific process id
                //
                TempEvent->ProcessId = ProcessId;
            }

            IsNextCommandPid = FALSE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (IsNextCommandCoreId)
        {
            if (!ConvertStringToUInt32(Section, &CoreId))
            {
                free(BufferOfCommandString);
                free(TempEvent);

                if (TempActionBreak != NULL)
                {
                    free(TempActionBreak);
                }
                if (TempActionScript != NULL)
                {
                    free(TempActionScript);
                }
                if (TempActionCustomCode != NULL)
                {
                    free(TempActionCustomCode);
                }

                ShowMessages("err, core id is invalid\n");
                *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
                return FALSE;
            }
            else
            {
                //
                // Set the specific core id
                //
                TempEvent->CoreId = CoreId;
            }
            IsNextCommandCoreId = FALSE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (!Section.compare("pid"))
        {
            IsNextCommandPid = TRUE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }
        if (!Section.compare("core"))
        {
            IsNextCommandCoreId = TRUE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (!Section.compare("imm"))
        {
            //
            // the next commnad is immediate messaging indicator
            //
            IsNextCommandImmediateMessaging = TRUE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }

        if (!Section.compare("buffer"))
        {
            IsNextCommandBufferSize = TRUE;

            //
            // Add index to remove it from the command
            //
            IndexesToRemove.push_back(Index);

            continue;
        }
    }

    //
    // Additional validation
    //
    if (IsNextCommandCoreId)
    {
        free(BufferOfCommandString);
        free(TempEvent);

        if (TempActionBreak != NULL)
        {
            free(TempActionBreak);
        }
        if (TempActionScript != NULL)
        {
            free(TempActionScript);
        }
        if (TempActionCustomCode != NULL)
        {
            free(TempActionCustomCode);
        }

        ShowMessages("err, please specify a value for 'core'\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
        return FALSE;
    }

    if (IsNextCommandPid)
    {
        free(BufferOfCommandString);
        free(TempEvent);

        if (TempActionBreak != NULL)
        {
            free(TempActionBreak);
        }
        if (TempActionScript != NULL)
        {
            free(TempActionScript);
        }
        if (TempActionCustomCode != NULL)
        {
            free(TempActionCustomCode);
        }

        ShowMessages("err, please specify a value for 'pid'\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
        return FALSE;
    }

    if (IsNextCommandBufferSize)
    {
        free(BufferOfCommandString);
        free(TempEvent);

        if (TempActionBreak != NULL)
        {
            free(TempActionBreak);
        }
        if (TempActionScript != NULL)
        {
            free(TempActionScript);
        }
        if (TempActionCustomCode != NULL)
        {
            free(TempActionCustomCode);
        }

        ShowMessages("err, please specify a value for 'buffer'\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_FORMAT_ERROR;
        return FALSE;
    }

    //
    // It's not possible to break to debugger in vmi-mode
    //
    if (!g_IsSerialConnectedToRemoteDebuggee && TempActionBreak != NULL)
    {
        free(BufferOfCommandString);
        free(TempEvent);

        if (TempActionBreak != NULL)
        {
            free(TempActionBreak);
        }
        if (TempActionScript != NULL)
        {
            free(TempActionScript);
        }
        if (TempActionCustomCode != NULL)
        {
            free(TempActionCustomCode);
        }

        ShowMessages(
            "err, it's not possible to break to the debugger in VMI Mode. "
            "You should operate in Debugger Mode to break and get the "
            "full control of the system. Still, you can use 'script' and run "
            "'custom code' in your local debugging (VMI Mode)\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_ATTEMPT_TO_BREAK_ON_VMI_MODE;
        return FALSE;
    }

    //
    // We do not support non-immediate message passing if the user
    // specified a special output
    //
    if (!ImmediateMessagePassing && HasOutputPath)
    {
        free(BufferOfCommandString);
        free(TempEvent);

        if (TempActionBreak != NULL)
        {
            free(TempActionBreak);
        }
        if (TempActionScript != NULL)
        {
            free(TempActionScript);
        }
        if (TempActionCustomCode != NULL)
        {
            free(TempActionCustomCode);
        }

        ShowMessages("err, non-immediate message passing is not supported in "
                     "'output-forwarding mode'\n");
        *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_IMMEDIATE_MESSAGING_IN_EVENT_FORWARDING_MODE;
        return FALSE;
    }

    //
    // Set the specific requested immediate message passing
    //
    if (TempActionBreak != NULL)
    {
        TempActionBreak->ImmediateMessagePassing = ImmediateMessagePassing;
    }
    if (TempActionScript != NULL)
    {
        TempActionScript->ImmediateMessagePassing = ImmediateMessagePassing;
    }
    if (TempActionCustomCode != NULL)
    {
        TempActionCustomCode->ImmediateMessagePassing = ImmediateMessagePassing;
    }

    //
    // Set the tags into the event list
    //
    IndexOfValidSourceTags = 0;
    for (auto item : ListOfValidSourceTags)
    {
        TempEvent->OutputSourceTags[IndexOfValidSourceTags] = item;

        //
        // Increase index
        //
        IndexOfValidSourceTags++;
    }

    //
    // If it has output then we should indicate it in event's object
    //
    if (HasOutputPath)
    {
        TempEvent->HasCustomOutput = TRUE;
    }

    //
    // Fill the address and length of event before release
    //
    *EventDetailsToFill = TempEvent;
    *EventBufferLength  = LengthOfEventBuffer;

    //
    // Fill the address and length of action before release
    //
    if (TempActionBreak != NULL)
    {
        *ActionDetailsToFillBreakToDebugger = TempActionBreak;
        *ActionBufferLengthBreakToDebugger  = LengthOfBreakActionBuffer;
    }
    if (TempActionScript != NULL)
    {
        *ActionDetailsToFillScript = TempActionScript;
        *ActionBufferLengthScript  = LengthOfScriptActionBuffer;
    }
    if (TempActionCustomCode != NULL)
    {
        *ActionDetailsToFillCustomCode = TempActionCustomCode;
        *ActionBufferLengthCustomCode  = LengthOfCustomCodeActionBuffer;
    }

    //
    // Remove the command that we interpreted above from the command
    //
    for (auto IndexToRemove : IndexesToRemove)
    {
        NewIndexToRemove++;
        SplittedCommand->erase(SplittedCommand->begin() +
                               (IndexToRemove - NewIndexToRemove));
        SplittedCommandCaseSensitive->erase(SplittedCommandCaseSensitive->begin() +
                                            (IndexToRemove - NewIndexToRemove));
    }

    //
    // Check if list is initialized or not
    //
    if (!g_EventTraceInitialized)
    {
        InitializeListHead(&g_EventTrace);
        g_EventTraceInitialized = TRUE;
    }

    //
    // Add the event to the trace list
    // UPDATE : if we add it here, then if the event was not
    // successful then still the event shows this event, so
    // we have to add it lated
    //
    // InsertHeadList(&g_EventTrace, &(TempEvent->CommandsEventList));

    //
    // Everything is ok, let's return TRUE
    //
    *ReasonForErrorInParsing = DEBUGGER_EVENT_PARSING_ERROR_CAUSE_SUCCESSFUL_NO_ERROR;
    return TRUE;
}
