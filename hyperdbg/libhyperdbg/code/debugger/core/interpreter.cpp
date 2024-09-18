/**
 * @file interpreter.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The hyperdbg command interpreter and driver connector
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern CommandType              g_CommandsList;

extern BOOLEAN g_ShouldPreviousCommandBeContinued;
extern BOOLEAN g_IsCommandListInitialized;
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_ExecutingScript;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsDebuggeeRunning;
extern BOOLEAN g_BreakPrintingOutput;
extern BOOLEAN g_IsInterpreterOnString;
extern BOOLEAN g_IsInterpreterPreviousCharacterABackSlash;
extern BOOLEAN g_RtmSupport;

extern UINT32 g_VirtualAddressWidth;
extern UINT32 g_InterpreterCountOfOpenCurlyBrackets;
extern ULONG  g_CurrentRemoteCore;

extern string g_ServerPort;
extern string g_ServerIp;

class CommandParser
{
public:
    /**
     * @brief Parse the input string (commands)
     * @param input
     *
     * @return std::vector<CommandToken>
     */
    std::vector<CommandToken> Parse(const std::string & ConstInput)
    {
        std::vector<CommandToken> tokens;
        std::string               current;
        bool                      InQuotes   = FALSE;
        int                       IdxBracket = 0;

        //
        // mainly for removing \ from escaped chars
        //
        std::string input = ConstInput;
        for (size_t i = 0; i < input.length(); ++i)
        {
            char c = input[i];

            if (c == '/') // start comment parse
            {
                //
                // if we're in a script bracket, should we skip? it'll be handled later?
                //
                if (!InQuotes)
                {
                    size_t j = i;
                    char   c2 = input[++j];

                    if (c2 == '/') // start to look for comments
                    {
                        //
                        // assuming " }" as the end of a line comment aka //
                        //
                        size_t CloseBrktPos;
                        for (CloseBrktPos = input.find("}", i); CloseBrktPos != std::string::npos; )
                        {
                             CloseBrktPos = input.find("}", CloseBrktPos);
                             if (input[CloseBrktPos - 1] == '\\')
                             {
                                 input.erase(CloseBrktPos - 1, 1);
                                 CloseBrktPos += 1;
                             }
                             else
                             {
                                 break;
                             }
                        }

                        size_t NewLineSrtPos = input.find("\\n", i); // "\\n" entered by user
                        size_t NewLineChrPos = input.find('\n', i);

                        vector<size_t> PosVec;
                        PosVec.push_back(CloseBrktPos);
                        PosVec.push_back(NewLineSrtPos);
                        PosVec.push_back(NewLineChrPos); // *** check fo npos

                        auto min = min_element(PosVec.begin(), PosVec.end());

                        if (CloseBrktPos != std::string::npos && input[CloseBrktPos-1] != '\\')
                        {
                            //
                            // here we could get the comment but for now we just skip
                            //
                            std::string comment(input.substr(i, CloseBrktPos - i));

                            //
                            // append comments to be passed to script engine
                            //
                            if (IdxBracket)
                            {
                                current += comment;
                            }

                            //
                            // forward the buffer
                            //
                            i = i + (CloseBrktPos - i) - 1;

                            continue;
                        }
                        else
                        {
                            bool IsNewLineEsc = false;
                            if (NewLineSrtPos != std::string::npos)
                            {
                                IsNewLineEsc = input[NewLineSrtPos - 1] == '\\';
                            }

                            if (NewLineSrtPos != std::string::npos && !IsNewLineEsc) // is not escaped
                            {
                                //
                                // here we could get the comment but for now we just skip
                                //
                                std::string comment(input.substr(i, NewLineSrtPos - i));

                                //
                                // append comments to be passed to script engine
                                //
                                if (IdxBracket)
                                {
                                    current += comment;
                                }

                                //
                                // forward the buffer
                                //
                                i = i + (NewLineSrtPos - i) + 1; // +1 for "\n". the "continue;" will also go past another time.

                                continue;
                            }
                            else if (NewLineChrPos != std::string::npos)
                            {
                                //
                                // here we could get the comment but for now we just skip
                                //
                                std::string comment(input.substr(i, NewLineChrPos - i));

                                //
                                // append comments to be passed to script engine
                                //
                                if (IdxBracket)
                                {
                                    current += comment;
                                }

                                //
                                // forward the buffer
                                //
                                i = i + (NewLineChrPos - i);

                                continue; // go past '\n'
                            }
                            else
                            {
                                //
                                // no "\\n" nor '\n' found so we just mark the chars as comment till end of string
                                //
                                std::string comment(input.substr(i, input.size()));

                                // fix the escaped newline
                                if (IsNewLineEsc)
                                {
                                    size_t start_pos = 0;
                                    while ((start_pos = comment.find("\\\\n", start_pos)) != std::string::npos)
                                    {
                                        comment.replace(start_pos, 3, "\\n");
                                        start_pos += 2; // Handles case where 'to' is a substring of 'from'
                                    }

                                    IsNewLineEsc = false;
                                }

                                //
                                // append comments to be passed to script engine
                                //
                                if (IdxBracket)
                                {
                                    current += comment;
                                }

                                //
                                // forward the buffer
                                //
                                i = i + (input.size() - i);

                                continue;
                            }
                        }
                    }
                    else if (c2 == '*')
                    {
                        size_t EndPose = input.find("*/", i+2); // +2 for cases like /*/

                        if (EndPose != std::string::npos)
                        {
                            //
                            // here we could get the comment but for now we just skip
                            //
                            std::string comment(input.substr(i, EndPose - i + 2)); // */ is two bytes long

                            //
                            // append comments to be passed to script engine
                            //
                            if (IdxBracket)
                            {
                                current += comment;
                            }

                            //
                            // forward the buffer
                            //
                            i = (i + (EndPose - i)) + 1; // +1 for /

                            continue;
                        }
                        else
                        {
                            // error: comment not closed
                        }
                    }
                }
            }

            if (InQuotes)
            {
                if (c == '"' && input[i - 1] != '\\') //&& !IdxBracket)
                {
                    InQuotes = FALSE;

                    //
                    // if the quoted text is not within brackets, regard it as a StringLiteral token
                    //
                    if (!IdxBracket)
                    {
                        AddStringToken(tokens, current, TRUE); // TRUE for StringLiteral type
                        current.clear();
                        continue; // dont add " char
                    }
                    else
                    {
                        current += c;
                        continue; // dont add " char
                    }
                    //
                    // if we are indeed within brackets, we continue to add the '"' char to the current buffer
                    //
                }
            }

            if (IdxBracket)
            {
                if (c == '}' && input[i - 1] != '\\' && IdxBracket && !InQuotes) // not closing }
                {
                    IdxBracket--;
                }

                if (c == '}' && input[i - 1] != '\\' && !IdxBracket) // is closing }
                {
                    AddBracketStringToken(tokens, current);
                    current.clear();

                    continue;
                }

                if (c == '{' && input[i - 1] != '\\' && !InQuotes)
                {
                    if (i) // check if this { is the first char to avoid out of range check
                    {
                        if (input[i - 1] != '\\')
                        {
                            IdxBracket++;
                        }
                    }
                    else
                    {
                        IdxBracket++;
                    }
                }
            }

            if (c == ' ' && !InQuotes && !IdxBracket) // finding seperator space char
            {
                if (!current.empty() && current != " ")
                {
                    AddToken(tokens, current);
                    current.clear();

                    continue;
                }
                continue; // avoid adding extra space char
            }
            else if (c == '"') //&& !IdxBracket)
            {
                if (i) // check if this " is the first char to avoid out of range check
                {
                    if (input[i - 1] != ' ' && !IdxBracket && !current.empty()) // is prev cmd adjacent to "
                    {
                        AddStringToken(tokens, current);
                        current.clear();
                    }

                    if (input[i - 1] != '\\')
                    {
                        InQuotes = TRUE;
                        if (!IdxBracket)
                        {
                            continue; // don't include '"' in string
                        }
                    }
                }
                else
                {
                    InQuotes = TRUE;
                    if (!IdxBracket)
                    {
                        continue; // don't include '"' in string
                    }
                }
            }
            else if (c == '{' && !InQuotes && !IdxBracket) // first {
            {
                if (i) // check if this { is the first char to avoid out of range check
                {
                    if (input[i - 1] != '\\')
                    {
                        if (input[i - 1] != ' ') // in case '{' is adjacent to previous command like "command{"
                        {
                            AddToken(tokens, current);
                            current.clear();
                        }

                        IdxBracket++;
                        continue; // don't include '{' in string
                    }
                }
                else
                {
                    IdxBracket++;
                    continue; // don't include '{' in string
                }
            }

            //
            // ignore astray \n
            //
            if (c == '\\' && !InQuotes)
            {
                if (current.empty() && input[i + 1] == 'n')
                {
                    i++;
                    continue;
                }
            }

            current += c;
            
        }

        if (!current.empty() && current != " ")
        {
            AddToken(tokens, current);
        }

        if (IdxBracket)
        {
            // error: script bracket not closed
        }

        if (InQuotes)
        {
            // error: Quote not closed
        }
        
        return tokens;
    }

    /**
     * @brief Function to convert CommandParsingTokenType to a string
     * @param Type
     *
     * @return std::string
     */
    std::string TokenTypeToString(CommandParsingTokenType Type)
    {
        switch (Type)
        {
        case CommandParsingTokenType::Num:
            return "Num";

        case CommandParsingTokenType::String:
            return "String";

        case CommandParsingTokenType::StringLiteral:
            return "StringLiteral";

        case CommandParsingTokenType::BracketString:
            return "BracketString";

        default:
            return "Unknown";
        }
    }

    /**
     * @brief Function to print the elements of a vector of Tokens
     * @param Tokens
     *
     * @return VOID
     */
    VOID PrintTokens(const std::vector<CommandToken> & Tokens)
    {
        //
        // get len of longest string
        //
        const int sz      = 200; // size
        int       g_s1Len = 0, g_s2Len = 0, g_s3Len = 0;
        int       s1 = 0, s2 = 0, s3 = 0;
        char      LineToPrint1[sz], LineToPrint2[sz], LineToPrint3[sz];

        for (const auto & Token : Tokens)
        {
            s1 = snprintf(LineToPrint1, sz, "CommandParsingTokenType: %s ", TokenTypeToString(std::get<0>(Token)).c_str());
            s2 = snprintf(LineToPrint2, sz, ", Value 1: '%s'", std::get<1>(Token).c_str());
            s3 = snprintf(LineToPrint3, sz, ", Value 2 (lower): '%s'", std::get<2>(Token).c_str());

            if (s1 > g_s1Len)
                g_s1Len = s1;

            if (s2 > g_s2Len)
                g_s2Len = s2;

            if (s3 > g_s3Len)
                g_s3Len = s3;
        }

        for (const auto & Token : Tokens)
        {
            auto CaseSensitiveText = std::get<1>(Token);
            auto LowerCaseText     = std::get<2>(Token);

            if (std::get<0>(Token) == CommandParsingTokenType::BracketString ||
                std::get<0>(Token) == CommandParsingTokenType::String ||
                std::get<0>(Token) == CommandParsingTokenType::StringLiteral)
            {
                //
                // Replace \n with \\n
                //

                //
                // Search for \n and replace with \\n
                //
                std::string::size_type pos = 0;
                while ((pos = CaseSensitiveText.find("\n", pos)) != std::string::npos)
                {
                    CaseSensitiveText.replace(pos, 1, "\\n");
                    pos += 2; // Move past the newly added characters
                }

                //
                // Do the same for lower case text
                //
                pos = 0;
                while ((pos = LowerCaseText.find("\n", pos)) != std::string::npos)
                {
                    LowerCaseText.replace(pos, 1, "\\n");
                    pos += 2; // Move past the newly added characters
                }
            }

            snprintf(LineToPrint1, sz, "CommandParsingTokenType: %s ", TokenTypeToString(std::get<0>(Token)).c_str());
            snprintf(LineToPrint2, sz, ", Value 1: '%s'", CaseSensitiveText.c_str());
            snprintf(LineToPrint3, sz, ", Value 2 (lower): '%s'", LowerCaseText.c_str());

            ShowMessages("%-*s %-*s %-*s\n", // - for left align
                         g_s1Len,
                         LineToPrint1,
                         g_s2Len,
                         LineToPrint2,
                         g_s3Len,
                         LineToPrint3);

            /*ShowMessages("CommandParsingTokenType: %s , Value 1: '%s', Value 2 (lower): '%s'\n",
                         TokenTypeToString(std::get<0>(Token)).c_str(),
                         CaseSensitiveText.c_str(),
                         LowerCaseText.c_str());*/
        }
    }

private:
    /**
     * @brief Convert a string to lowercase
     * @param str
     *
     * @return std::string
     */
    std::string ToLower(const std::string & str) const
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);

        return result;
    }

    /**
     * @brief Add Token
     * @param tokens
     * @param str
     *
     * @return VOID
     */
    VOID AddToken(std::vector<CommandToken> & tokens, std::string & str)
    {
        auto   tmp    = str;
        UINT64 tmpNum = 0;

        //
        // Trim the string
        //
        Trim(tmp);

        if (ConvertStringToUInt64(tmp, &tmpNum)) // tmp will be modified to actual number
        {
            tokens.emplace_back(CommandParsingTokenType::Num, tmp, ToLower(tmp));
        }
        else
        {
            AddStringToken(tokens, str);
        }
    }

    /**
     * @brief Add String Token
     * @param tokens
     * @param str
     *
     * @return VOID
     */
    VOID AddStringToken(std::vector<CommandToken> & tokens, std::string & str, BOOL isLiteral = FALSE)
    {
        auto tmp = str;

        //
        // Trim the string
        //
        Trim(tmp);

        //
        // If the string is empty, we don't need to add it
        //
        if (tmp.empty())
            return;
        if (isLiteral)
        {
            tokens.emplace_back(CommandParsingTokenType::StringLiteral, tmp, ToLower(tmp));
        }
        else
        {
            tokens.emplace_back(CommandParsingTokenType::String, tmp, ToLower(tmp));
        }
    }

    /**
     * @brief Add Bracket String Token
     * @param tokens
     * @param str
     *
     * @return VOID
     */
    VOID AddBracketStringToken(std::vector<CommandToken> & tokens, const std::string & str)
    {
        tokens.emplace_back(CommandParsingTokenType::BracketString, str, ToLower(str));
    }
};

/**
 * @brief Find the position of the first difference between two strings
 * @param Str1 The first string
 * @param Str2 The second string
 *
 * @return The position of the first difference, or -1 if the strings are equal
 */
int
FindDifferencePosition(const char * Str1, const char * Str2)
{
    int i = 0;

    //
    // Loop until a difference is found or until the end of any string is reached
    //
    while (Str1[i] != '\0' && Str2[i] != '\0')
    {
        if (Str1[i] != Str2[i])
        {
            return i; // Return the position of the first mismatch
        }
        i++;
    }

    //
    // If one string ends before the other
    //
    if (Str1[i] != Str2[i])
    {
        return i;
    }

    //
    // If no difference is found, return -1
    //
    return -1;
}

/**
 * @brief Parse the command (used for testing purposes)
 *
 * @param Command The text of command
 * @param NumberOfTokens The number of tokens
 * @param TokensList The tokens list
 * @param FailedTokenNum The failed token number (if any)
 * @param FailedTokenPosition The failed token position (if any)
 *
 * @return BOOLEAN returns true if the command was parsed successfully and false if there was an error
 */
BOOLEAN
HyperDbgTestCommandParser(CHAR *   Command,
                          UINT32   NumberOfTokens,
                          CHAR **  TokensList,
                          UINT32 * FailedTokenNum,
                          UINT32 * FailedTokenPosition)
{
    CommandParser Parser;

    //
    // Convert to string
    //
    string CommandString(Command);

    //
    // Tokenize the command string
    //
    std::vector<CommandToken> Tokens = Parser.Parse(CommandString);

    //
    // Check if the number of tokens is correct
    //
    if (Tokens.size() != NumberOfTokens)
    {
        ShowMessages("err, the number of tokens is not correct\n");
        return FALSE;
    }

    //
    // Check if the tokens are correct
    //
    for (UINT32 i = 0; i < Tokens.size(); i++)
    {
        auto Token = Tokens.at(i);

        auto CaseSensitiveText = std::get<1>(Token);

        if (strcmp(CaseSensitiveText.c_str(), TokensList[i]) != 0)
        {
            //
            // Set the failed token number
            //
            *FailedTokenNum      = i;
            *FailedTokenPosition = FindDifferencePosition(CaseSensitiveText.c_str(), TokensList[i]);

            ShowMessages("err, the token is not correct\n");
            return FALSE;
        }
    }

    //
    // Everything is correct
    //
    return TRUE;
}

/**
 * @brief Parse the command and show tokens (used for testing purposes)
 *
 * @param Command The text of command
 *
 * @return VOID
 */
VOID
HyperDbgTestCommandParserShowTokens(CHAR * Command)
{
    CommandParser Parser;

    //
    // Convert to string
    //
    string CommandString(Command);

    //
    // Tokenize the command string
    //
    std::vector<CommandToken> Tokens = Parser.Parse(CommandString);

    Parser.PrintTokens(Tokens);
}

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return INT returns return zero if it was successful or non-zero if there was
 * error
 */
INT
HyperDbgInterpreter(CHAR * Command)
{
    BOOLEAN               HelpCommand       = FALSE;
    UINT64                CommandAttributes = NULL;
    CommandType::iterator Iterator;
    CommandParser         Parser;

    //
    // Check if it's the first command and whether the mapping of command is
    // initialized or not
    //
    if (!g_IsCommandListInitialized)
    {
        //
        // Initialize the debugger
        //
        InitializeDebugger();

        g_IsCommandListInitialized = TRUE;
    }

    //
    // Save the command into log open file
    //
    if (g_LogOpened && !g_ExecutingScript)
    {
        LogopenSaveToFile(Command);
        LogopenSaveToFile("\n");
    }

    //
    // Convert to string
    //
    string CommandString(Command);

    //
    // Tokenize the command string
    //
    auto Tokens = Parser.Parse(CommandString);

    //
    // Print the tokens
    //
    // Parser.PrintTokens(Tokens);

    //
    // Check if user entered an empty input
    //
    if (Tokens.empty())
    {
        ShowMessages("\n");
        return 0;
    }

    //
    // Get the first command (lower case)
    //
    string FirstCommand = GetLowerStringFromCommandToken(Tokens.front());

    //
    // Read the command's attributes
    //
    CommandAttributes = GetCommandAttributes(FirstCommand);

    //
    // Check if the command needs to be continued by pressing enter
    //
    if (CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER)
    {
        g_ShouldPreviousCommandBeContinued = TRUE;
    }
    else
    {
        g_ShouldPreviousCommandBeContinued = FALSE;
    }

    //
    // Check and send remote command and also we check whether this
    // is a command that should be handled in this command or we can
    // send it to the remote computer, it is because in a remote connection
    // still some of the commands should be handled in the local HyperDbg
    //
    if (g_IsConnectedToRemoteDebuggee &&
        !(CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION))
    {
        //
        // Check it here because generally, we use this variable in host
        // for showing the correct signature but we won't try to block
        // other commands, the only thing is events which is blocked
        // by the remote computer itself
        //
        if (g_BreakPrintingOutput)
        {
            g_BreakPrintingOutput = FALSE;
        }

        //
        // It's a connection over network (VMI-Mode)
        //
        RemoteConnectionSendCommand(Command, (UINT32)strlen(Command) + 1);

        ShowMessages("\n");

        //
        // Indicate that we sent the command to the target system
        //
        return 2;
    }
    else if (g_IsSerialConnectedToRemoteDebuggee &&
             !(CommandAttributes &
               DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE))
    {
        //
        // It's a connection over serial (Debugger-Mode)
        //

        if (CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN)
        {
            KdSendUserInputPacketToDebuggee(Command, (UINT32)strlen(Command) + 1, TRUE);

            //
            // Set the debuggee to show that it's running
            //
            KdSetStatusAndWaitForPause();
        }
        else
        {
            //
            // Disable the breakpoints and events while executing the command in the remote computer
            //
            KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_OFF_BPS_AND_EVENTS_FOR_COMMANDS_IN_REMOTE_COMPUTER);
            KdSendUserInputPacketToDebuggee(Command, (UINT32)strlen(Command) + 1, FALSE);
            KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_ON_BPS_AND_EVENTS_FOR_COMMANDS_IN_REMOTE_COMPUTER);
        }

        //
        // Indicate that we sent the command to the target system
        //
        return 2;
    }

    //
    // Detect whether it's a .help command or not
    //
    if (!FirstCommand.compare(".help") || !FirstCommand.compare("help") ||
        !FirstCommand.compare(".hh"))
    {
        if (Tokens.size() == 2)
        {
            //
            // Show that it's a help command
            //
            HelpCommand  = TRUE;
            FirstCommand = GetLowerStringFromCommandToken(Tokens.at(1));
        }
        else
        {
            ShowMessages("incorrect use of the '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(Tokens.at(0)).c_str());
            CommandHelpHelp();
            return 0;
        }
    }

    //
    // Start parsing commands
    //
    string CaseSensitiveCommandString(Command);
    Iterator = g_CommandsList.find(FirstCommand);

    if (Iterator == g_CommandsList.end())
    {
        //
        //  Command doesn't exist
        //
        if (!HelpCommand)
        {
            ShowMessages("err, couldn't resolve command at '%s'\n",
                         GetCaseSensitiveStringFromCommandToken(Tokens.front()).c_str());
        }
        else
        {
            ShowMessages("err, couldn't find the help for the command at '%s'\n",
                         GetCaseSensitiveStringFromCommandToken(Tokens.at(1)).c_str());
        }
    }
    else
    {
        if (HelpCommand)
        {
            Iterator->second.CommandHelpFunction();
        }
        else
        {
            //
            // Call the parser with tokens
            //
            Iterator->second.CommandFunctionNewParser(Tokens, CaseSensitiveCommandString);
        }
    }

    //
    // Save the command into log open file
    //
    if (g_LogOpened && !g_ExecutingScript)
    {
        LogopenSaveToFile("\n");
    }

    return 0;
}

/**
 * @brief Remove batch comments
 * @param CommandText
 * @details deprecated, not used anymore
 *
 * @return VOID
 */
VOID
InterpreterRemoveComments(char * CommandText)
{
    BOOLEAN IsComment         = FALSE;
    BOOLEAN IsOnBracketString = FALSE;
    BOOLEAN IsOnString        = FALSE;
    UINT32  LengthOfCommand   = (UINT32)strlen(CommandText);

    for (size_t i = 0; i < LengthOfCommand; i++)
    {
        if (IsComment)
        {
            if (CommandText[i] == '\n')
            {
                IsComment = FALSE;
            }
            else
            {
                if (CommandText[i] != '\0')
                {
                    memmove((void *)&CommandText[i], (const void *)&CommandText[i + 1], strlen(CommandText) - i);
                    i--;
                }
            }
        }
        else if (CommandText[i] == '#' && !IsOnString)
        {
            //
            // Comment detected
            //
            IsComment = TRUE;
            i--;
        }
        else if (CommandText[i] == '#' && !IsOnString)
        {
            //
            // Comment detected
            //
            IsComment = TRUE;
            i--;
        }
        else if (CommandText[i] == '"')
        {
            if (i != 0 && CommandText[i - 1] == '\\')
            {
                //
                // It's an escape character for : \"
                //
            }
            else if (IsOnString)
            {
                IsOnString = FALSE;
            }
            else
            {
                IsOnString = TRUE;
            }
        }
    }
}

/**
 * @brief Show signature of HyperDbg
 *
 * @return VOID
 */
VOID
HyperDbgShowSignature()
{
    if (g_IsConnectedToRemoteDebuggee)
    {
        //
        // Remote debugging over tcp (vmi-mode)
        //
        ShowMessages("[%s:%s] HyperDbg> ", g_ServerIp.c_str(), g_ServerPort.c_str());
    }
    else if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Debugging a special process
        //
        ShowMessages("%x:%x u%sHyperDbg> ",
                     g_ActiveProcessDebuggingState.ProcessId,
                     g_ActiveProcessDebuggingState.ThreadId,
                     g_ActiveProcessDebuggingState.Is32Bit ? "86" : "64");
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Remote debugging over serial (debugger-mode)
        //
        ShowMessages("%x: kHyperDbg> ", g_CurrentRemoteCore);
    }
    else
    {
        //
        // Anything other than above scenarios including local debugging
        // in vmi-mode
        //
        ShowMessages("HyperDbg> ");
    }
}

/**
 * @brief check for multi-line commands
 *
 * @param CurrentCommand
 * @param Reset
 * @return BOOLEAN return TRUE if the command needs extra input, otherwise
 * return FALSE
 */
BOOLEAN
CheckMultilineCommand(CHAR * CurrentCommand, BOOLEAN Reset)
{
    UINT32      CurrentCommandLen = 0;
    std::string CurrentCommandStr(CurrentCommand);

    if (Reset)
    {
        g_IsInterpreterOnString                    = FALSE;
        g_IsInterpreterPreviousCharacterABackSlash = FALSE;
        g_InterpreterCountOfOpenCurlyBrackets      = 0;
    }

    CurrentCommandLen = (UINT32)CurrentCommandStr.length();

    for (size_t i = 0; i < CurrentCommandLen; i++)
    {
        switch (CurrentCommandStr.at(i))
        {
        case '"':

            if (g_IsInterpreterPreviousCharacterABackSlash)
            {
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;
                break; // it's an escaped \" double-quote
            }

            if (g_IsInterpreterOnString)
                g_IsInterpreterOnString = FALSE;
            else
                g_IsInterpreterOnString = TRUE;

            break;

        case '{':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            if (!g_IsInterpreterOnString)
                g_InterpreterCountOfOpenCurlyBrackets++;

            break;

        case '}':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            if (!g_IsInterpreterOnString && g_InterpreterCountOfOpenCurlyBrackets > 0)
                g_InterpreterCountOfOpenCurlyBrackets--;

            break;

        case '\\':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE; // it's not a escape character (two backslashes \\ )
            else
                g_IsInterpreterPreviousCharacterABackSlash = TRUE;

            break;

        default:

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            break;
        }
    }

    if (g_IsInterpreterOnString == FALSE && g_InterpreterCountOfOpenCurlyBrackets == 0)
    {
        //
        // either the command is finished or it's a single
        // line command
        //
        return FALSE;
    }
    else
    {
        //
        // There still other lines, this command is incomplete
        //
        return TRUE;
    }
}

/**
 * @brief Some of commands like stepping commands (i, p, t) and etc.
 * need to be repeated when the user press enter, this function shows
 * whether we should continue the previous command or not
 *
 * @return TRUE means the command should be continued, FALSE means command
 * should be ignored
 */
BOOLEAN
ContinuePreviousCommand()
{
    BOOLEAN Result = g_ShouldPreviousCommandBeContinued;

    //
    // We should keep it false for the next command
    //
    g_ShouldPreviousCommandBeContinued = FALSE;

    if (Result)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Get Command Attributes
 *
 * @param FirstCommand just the first word of command (without other parameters)
 * @return BOOLEAN Mask of the command's attributes
 */
UINT64
GetCommandAttributes(const string & FirstCommand)
{
    CommandType::iterator Iterator;

    //
    // Some commands should not be passed to the remote system
    // and instead should be handled in the current debugger
    //

    Iterator = g_CommandsList.find(FirstCommand);

    if (Iterator == g_CommandsList.end())
    {
        //
        // Command doesn't exist, if it's not exists then it's better to handle
        // it locally, instead of sending it to the remote computer
        //
        return DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL;
    }
    else
    {
        return Iterator->second.CommandAttrib;
    }

    return NULL;
}

/**
 * @brief Initialize the debugger and adjust commands for the first run
 *
 * @return VOID
 */
VOID
InitializeDebugger()
{
    //
    // Initialize the mapping of functions
    //
    InitializeCommandsDictionary();

    //
    // Set the callback for symbol message handler
    //
    ScriptEngineSetTextMessageCallbackWrapper(ShowMessages);

    //
    // Register the CTRL+C and CTRL+BREAK Signals handler
    //
    if (!SetConsoleCtrlHandler(BreakController, TRUE))
    {
        ShowMessages("err, when registering CTRL+C and CTRL+BREAK Signals "
                     "handler\n");
        //
        // prefer to continue
        //
    }

    //
    // *** Check for feature indicators ***
    //

    //
    // Get x86 processor width for virtual address
    //
    g_VirtualAddressWidth = Getx86VirtualAddressWidth();

    //
    // Check if processor supports TSX (RTM)
    //
    g_RtmSupport = CheckCpuSupportRtm();

    //
    // Load default settings
    //
    CommandSettingsLoadDefaultValuesFromConfigFile();
}

/**
 * @brief Initialize commands and attributes
 *
 * @return VOID
 */
VOID
InitializeCommandsDictionary()
{
    g_CommandsList[".help"] = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandsList[".hh"]   = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandsList["help"]  = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};

    g_CommandsList["clear"] = {&CommandCls, &CommandClsHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandsList[".cls"]  = {&CommandCls, &CommandClsHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandsList["cls"]   = {&CommandCls, &CommandClsHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};

    g_CommandsList[".connect"] = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};
    g_CommandsList["connect"]  = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};

    g_CommandsList[".listen"] = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};
    g_CommandsList["listen"]  = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};

    g_CommandsList["g"]  = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};
    g_CommandsList["go"] = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};

    g_CommandsList[".attach"] = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};
    g_CommandsList["attach"]  = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};

    g_CommandsList[".detach"] = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};
    g_CommandsList["detach"]  = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};

    g_CommandsList[".start"] = {&CommandStart, &CommandStartHelp, DEBUGGER_COMMAND_START_ATTRIBUTES};
    g_CommandsList["start"]  = {&CommandStart, &CommandStartHelp, DEBUGGER_COMMAND_START_ATTRIBUTES};

    g_CommandsList[".restart"] = {&CommandRestart, &CommandRestartHelp, DEBUGGER_COMMAND_RESTART_ATTRIBUTES};
    g_CommandsList["restart"]  = {&CommandRestart, &CommandRestartHelp, DEBUGGER_COMMAND_RESTART_ATTRIBUTES};

    g_CommandsList[".switch"] = {&CommandSwitch, &CommandSwitchHelp, DEBUGGER_COMMAND_SWITCH_ATTRIBUTES};
    g_CommandsList["switch"]  = {&CommandSwitch, &CommandSwitchHelp, DEBUGGER_COMMAND_SWITCH_ATTRIBUTES};

    g_CommandsList[".kill"] = {&CommandKill, &CommandKillHelp, DEBUGGER_COMMAND_KILL_ATTRIBUTES};
    g_CommandsList["kill"]  = {&CommandKill, &CommandKillHelp, DEBUGGER_COMMAND_KILL_ATTRIBUTES};

    g_CommandsList[".process"]  = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};
    g_CommandsList[".process2"] = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};
    g_CommandsList["process"]   = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};
    g_CommandsList["process2"]  = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};

    g_CommandsList[".thread"]  = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};
    g_CommandsList[".thread2"] = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};
    g_CommandsList["thread"]   = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};
    g_CommandsList["thread2"]  = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};

    g_CommandsList["sleep"] = {&CommandSleep, &CommandSleepHelp, DEBUGGER_COMMAND_SLEEP_ATTRIBUTES};

    g_CommandsList["event"]  = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};
    g_CommandsList["events"] = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};

    g_CommandsList["setting"]   = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList["settings"]  = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList[".setting"]  = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList[".settings"] = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};

    g_CommandsList[".disconnect"] = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};
    g_CommandsList["disconnect"]  = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};

    g_CommandsList[".debug"] = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};
    g_CommandsList["debug"]  = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};

    g_CommandsList[".status"] = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_DOT_STATUS_ATTRIBUTES};
    g_CommandsList["status"]  = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_STATUS_ATTRIBUTES};

    g_CommandsList["load"]  = {&CommandLoad, &CommandLoadHelp, DEBUGGER_COMMAND_LOAD_ATTRIBUTES};
    g_CommandsList[".load"] = {&CommandLoad, &CommandLoadHelp, DEBUGGER_COMMAND_LOAD_ATTRIBUTES};

    g_CommandsList["exit"]  = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};
    g_CommandsList[".exit"] = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};

    g_CommandsList["flush"] = {&CommandFlush, &CommandFlushHelp, DEBUGGER_COMMAND_FLUSH_ATTRIBUTES};

    g_CommandsList["pause"]  = {&CommandPause, &CommandPauseHelp, DEBUGGER_COMMAND_PAUSE_ATTRIBUTES};
    g_CommandsList[".pause"] = {&CommandPause, &CommandPauseHelp, DEBUGGER_COMMAND_PAUSE_ATTRIBUTES};

    g_CommandsList["unload"] = {&CommandUnload, &CommandUnloadHelp, DEBUGGER_COMMAND_UNLOAD_ATTRIBUTES};

    g_CommandsList[".script"] = {&CommandScript, &CommandScriptHelp, DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES};
    g_CommandsList["script"]  = {&CommandScript, &CommandScriptHelp, DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES};

    g_CommandsList["output"] = {&CommandOutput, &CommandOutputHelp, DEBUGGER_COMMAND_OUTPUT_ATTRIBUTES};

    g_CommandsList["print"] = {&CommandPrint, &CommandPrintHelp, DEBUGGER_COMMAND_PRINT_ATTRIBUTES};

    g_CommandsList["?"]        = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandsList["eval"]     = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandsList["evaluate"] = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};

    g_CommandsList[".logopen"] = {&CommandLogopen, &CommandLogopenHelp, DEBUGGER_COMMAND_LOGOPEN_ATTRIBUTES};

    g_CommandsList[".logclose"] = {&CommandLogclose, &CommandLogcloseHelp, DEBUGGER_COMMAND_LOGCLOSE_ATTRIBUTES};

    g_CommandsList[".pagein"] = {&CommandPagein, &CommandPageinHelp, DEBUGGER_COMMAND_PAGEIN_ATTRIBUTES};
    g_CommandsList["pagein"]  = {&CommandPagein, &CommandPageinHelp, DEBUGGER_COMMAND_PAGEIN_ATTRIBUTES};

    g_CommandsList["test"] = {&CommandTest, &CommandTestHelp, DEBUGGER_COMMAND_TEST_ATTRIBUTES};

    g_CommandsList["cpu"] = {&CommandCpu, &CommandCpuHelp, DEBUGGER_COMMAND_CPU_ATTRIBUTES};

    g_CommandsList["wrmsr"] = {&CommandWrmsr, &CommandWrmsrHelp, DEBUGGER_COMMAND_WRMSR_ATTRIBUTES};

    g_CommandsList["rdmsr"] = {&CommandRdmsr, &CommandRdmsrHelp, DEBUGGER_COMMAND_RDMSR_ATTRIBUTES};

    g_CommandsList["!va2pa"] = {&CommandVa2pa, &CommandVa2paHelp, DEBUGGER_COMMAND_VA2PA_ATTRIBUTES};

    g_CommandsList["!pa2va"] = {&CommandPa2va, &CommandPa2vaHelp, DEBUGGER_COMMAND_PA2VA_ATTRIBUTES};

    g_CommandsList[".formats"] = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};
    g_CommandsList[".format"]  = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};

    g_CommandsList["!pte"] = {&CommandPte, &CommandPteHelp, DEBUGGER_COMMAND_PTE_ATTRIBUTES};

    g_CommandsList["~"]    = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};
    g_CommandsList["core"] = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};

    g_CommandsList["!monitor"] = {&CommandMonitor, &CommandMonitorHelp, DEBUGGER_COMMAND_MONITOR_ATTRIBUTES};

    g_CommandsList["!vmcall"] = {&CommandVmcall, &CommandVmcallHelp, DEBUGGER_COMMAND_VMCALL_ATTRIBUTES};

    g_CommandsList["!epthook"] = {&CommandEptHook, &CommandEptHookHelp, DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES};

    g_CommandsList["bp"] = {&CommandBp, &CommandBpHelp, DEBUGGER_COMMAND_BP_ATTRIBUTES};

    g_CommandsList["bl"] = {&CommandBl, &CommandBlHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["be"] = {&CommandBe, &CommandBeHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["bd"] = {&CommandBd, &CommandBdHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["bc"] = {&CommandBc, &CommandBcHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["!epthook2"] = {&CommandEptHook2, &CommandEptHook2Help, DEBUGGER_COMMAND_EPTHOOK2_ATTRIBUTES};

    g_CommandsList["!cpuid"] = {&CommandCpuid, &CommandCpuidHelp, DEBUGGER_COMMAND_CPUID_ATTRIBUTES};

    g_CommandsList["!msrread"] = {&CommandMsrread, &CommandMsrreadHelp, DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES};
    g_CommandsList["!msread"]  = {&CommandMsrread, &CommandMsrreadHelp, DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES};

    g_CommandsList["!msrwrite"] = {&CommandMsrwrite, &CommandMsrwriteHelp, DEBUGGER_COMMAND_MSRWRITE_ATTRIBUTES};

    g_CommandsList["!tsc"] = {&CommandTsc, &CommandTscHelp, DEBUGGER_COMMAND_TSC_ATTRIBUTES};

    g_CommandsList["!pmc"] = {&CommandPmc, &CommandPmcHelp, DEBUGGER_COMMAND_PMC_ATTRIBUTES};

    g_CommandsList["!crwrite"] = {&CommandCrwrite, &CommandCrwriteHelp, DEBUGGER_COMMAND_CRWRITE_ATTRIBUTES};

    g_CommandsList["!dr"] = {&CommandDr, &CommandDrHelp, DEBUGGER_COMMAND_DR_ATTRIBUTES};

    g_CommandsList["!ioin"] = {&CommandIoin, &CommandIoinHelp, DEBUGGER_COMMAND_IOIN_ATTRIBUTES};

    g_CommandsList["!ioout"] = {&CommandIoout, &CommandIooutHelp, DEBUGGER_COMMAND_IOOUT_ATTRIBUTES};
    g_CommandsList["!iout"]  = {&CommandIoout, &CommandIooutHelp, DEBUGGER_COMMAND_IOOUT_ATTRIBUTES};

    g_CommandsList["!exception"] = {&CommandException, &CommandExceptionHelp, DEBUGGER_COMMAND_EXCEPTION_ATTRIBUTES};

    g_CommandsList["!interrupt"] = {&CommandInterrupt, &CommandInterruptHelp, DEBUGGER_COMMAND_INTERRUPT_ATTRIBUTES};

    g_CommandsList["!syscall"]  = {&CommandSyscallAndSysret, &CommandSyscallHelp, DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES};
    g_CommandsList["!syscall2"] = {&CommandSyscallAndSysret, &CommandSyscallHelp, DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES};

    g_CommandsList["!sysret"]  = {&CommandSyscallAndSysret, &CommandSysretHelp, DEBUGGER_COMMAND_SYSRET_ATTRIBUTES};
    g_CommandsList["!sysret2"] = {&CommandSyscallAndSysret, &CommandSysretHelp, DEBUGGER_COMMAND_SYSRET_ATTRIBUTES};

    g_CommandsList["!mode"] = {&CommandMode, &CommandModeHelp, DEBUGGER_COMMAND_MODE_ATTRIBUTES};

    g_CommandsList["!trace"] = {&CommandTrace, &CommandTraceHelp, DEBUGGER_COMMAND_TRACE_ATTRIBUTES};

    g_CommandsList["!hide"] = {&CommandHide, &CommandHideHelp, DEBUGGER_COMMAND_HIDE_ATTRIBUTES};

    g_CommandsList["!unhide"] = {&CommandUnhide, &CommandUnhideHelp, DEBUGGER_COMMAND_UNHIDE_ATTRIBUTES};

    g_CommandsList["!measure"] = {&CommandMeasure, &CommandMeasureHelp, DEBUGGER_COMMAND_MEASURE_ATTRIBUTES};

    g_CommandsList["lm"] = {&CommandLm, &CommandLmHelp, DEBUGGER_COMMAND_LM_ATTRIBUTES};

    g_CommandsList["p"]  = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};
    g_CommandsList["pr"] = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};

    g_CommandsList["t"]  = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};
    g_CommandsList["tr"] = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};

    g_CommandsList["i"]  = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};
    g_CommandsList["ir"] = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};

    g_CommandsList["gu"] = {&CommandGu, &CommandGuHelp, DEBUGGER_COMMAND_GU_ATTRIBUTES};

    g_CommandsList["db"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dc"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dd"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dq"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!db"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dc"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dd"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dq"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u"]    = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u64"] = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u64"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u2"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u2"]   = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u32"] = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u32"]  = {&CommandReadMemoryAndDisassembler, &CommandReadMemoryAndDisassemblerHelp, DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};

    g_CommandsList["eb"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["ed"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["eq"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!eb"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!ed"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!eq"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};

    g_CommandsList["sb"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["sd"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["sq"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sb"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sd"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sq"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};

    g_CommandsList["r"] = {&CommandR, &CommandRHelp, DEBUGGER_COMMAND_R_ATTRIBUTES};

    g_CommandsList[".sympath"] = {&CommandSympath, &CommandSympathHelp, DEBUGGER_COMMAND_SYMPATH_ATTRIBUTES};
    g_CommandsList["sympath"]  = {&CommandSympath, &CommandSympathHelp, DEBUGGER_COMMAND_SYMPATH_ATTRIBUTES};

    g_CommandsList[".sym"] = {&CommandSym, &CommandSymHelp, DEBUGGER_COMMAND_SYM_ATTRIBUTES};
    g_CommandsList["sym"]  = {&CommandSym, &CommandSymHelp, DEBUGGER_COMMAND_SYM_ATTRIBUTES};

    g_CommandsList["x"] = {&CommandX, &CommandXHelp, DEBUGGER_COMMAND_X_ATTRIBUTES};

    g_CommandsList["prealloc"]      = {&CommandPrealloc, &CommandPreallocHelp, DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES};
    g_CommandsList["preallocate"]   = {&CommandPrealloc, &CommandPreallocHelp, DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES};
    g_CommandsList["preallocation"] = {&CommandPrealloc, &CommandPreallocHelp, DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES};

    g_CommandsList["preactivate"]   = {&CommandPreactivate, &CommandPreactivateHelp, DEBUGGER_COMMAND_PREACTIVATE_ATTRIBUTES};
    g_CommandsList["preactive"]     = {&CommandPreactivate, &CommandPreactivateHelp, DEBUGGER_COMMAND_PREACTIVATE_ATTRIBUTES};
    g_CommandsList["preactivation"] = {&CommandPreactivate, &CommandPreactivateHelp, DEBUGGER_COMMAND_PREACTIVATE_ATTRIBUTES};

    g_CommandsList["k"]  = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};
    g_CommandsList["kd"] = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};
    g_CommandsList["kq"] = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};

    g_CommandsList["dt"]  = {&CommandDtAndStruct, &CommandDtHelp, DEBUGGER_COMMAND_DT_ATTRIBUTES};
    g_CommandsList["!dt"] = {&CommandDtAndStruct, &CommandDtHelp, DEBUGGER_COMMAND_DT_ATTRIBUTES};

    g_CommandsList["struct"]    = {&CommandDtAndStruct, &CommandStructHelp, DEBUGGER_COMMAND_STRUCT_ATTRIBUTES};
    g_CommandsList["structure"] = {&CommandDtAndStruct, &CommandStructHelp, DEBUGGER_COMMAND_STRUCT_ATTRIBUTES};

    g_CommandsList[".pe"] = {&CommandPe, &CommandPeHelp, DEBUGGER_COMMAND_PE_ATTRIBUTES};

    g_CommandsList["!rev"] = {&CommandRev, &CommandRevHelp, DEBUGGER_COMMAND_REV_ATTRIBUTES};
    g_CommandsList["rev"]  = {&CommandRev, &CommandRevHelp, DEBUGGER_COMMAND_REV_ATTRIBUTES};

    g_CommandsList["!track"] = {&CommandTrack, &CommandTrackHelp, DEBUGGER_COMMAND_TRACK_ATTRIBUTES};
    g_CommandsList["track"]  = {&CommandTrack, &CommandTrackHelp, DEBUGGER_COMMAND_TRACK_ATTRIBUTES};

    g_CommandsList[".dump"] = {&CommandDump, &CommandDumpHelp, DEBUGGER_COMMAND_DUMP_ATTRIBUTES};
    g_CommandsList["dump"]  = {&CommandDump, &CommandDumpHelp, DEBUGGER_COMMAND_DUMP_ATTRIBUTES};
    g_CommandsList["!dump"] = {&CommandDump, &CommandDumpHelp, DEBUGGER_COMMAND_DUMP_ATTRIBUTES};

    //
    // hwdbg commands
    //
    g_CommandsList["!hw_clk"]      = {&CommandHwClk, &CommandHwClkHelp, DEBUGGER_COMMAND_HWDBG_HW_CLK_ATTRIBUTES};
    g_CommandsList["!hw_clock"]    = {&CommandHwClk, &CommandHwClkHelp, DEBUGGER_COMMAND_HWDBG_HW_CLK_ATTRIBUTES};
    g_CommandsList["!hwdbg_clock"] = {&CommandHwClk, &CommandHwClkHelp, DEBUGGER_COMMAND_HWDBG_HW_CLK_ATTRIBUTES};
    g_CommandsList["!hwdbg_clock"] = {&CommandHwClk, &CommandHwClkHelp, DEBUGGER_COMMAND_HWDBG_HW_CLK_ATTRIBUTES};

    g_CommandsList["a"]         = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["asm"]       = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["assemble"]  = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["assembly"]  = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["!a"]        = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["!asm"]      = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["!assemble"] = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
    g_CommandsList["!assembly"] = {&CommandAssemble, &CommandAssembleHelp, DEBUGGER_COMMAND_A_ATTRIBUTES};
}
