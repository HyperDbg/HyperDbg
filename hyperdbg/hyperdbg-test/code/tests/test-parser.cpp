/**
 * @file test-parser.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Perform test on command parser
 * @details
 * @version 0.11
 * @date 2024-08-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

typedef CHAR ** CHAR_PTR_PTR; // Define CHAR_PTR_PTR as CHAR**

/**
 * @brief Create an array of strings from a vector of strings
 * @param TestCases The vector of strings to copy
 *
 * @return A pointer to the array of strings
 */
CHAR_PTR_PTR
CreateTestCaseArray(const std::vector<std::string> & TestCases)
{
    //
    // Allocate memory for the array of pointers (size: number of test cases)
    //
    CHAR_PTR_PTR TestCaseArray = (CHAR_PTR_PTR)malloc(TestCases.size() * sizeof(UINT64));

    //
    // Allocate memory for each string and copy the content
    //
    for (SIZE_T i = 0; i < TestCases.size(); ++i)
    {
        TestCaseArray[i] = (CHAR *)malloc(TestCases[i].length() + 1); // +1 for the null terminator

        if (TestCaseArray[i] == NULL)
        {
            return NULL;
        }
        std::strcpy(TestCaseArray[i], TestCases[i].c_str());
    }

    return TestCaseArray;
}

/**
 * @brief Free the memory allocated for the test case array
 * @param TestCaseArray The array of pointers to free
 * @param Size The size of the array
 *
 * @return VOID
 */
VOID
FreeTestCaseArray(CHAR_PTR_PTR TestCaseArray, SIZE_T Size)
{
    //
    // Free each string
    //
    for (SIZE_T i = 0; i < Size; ++i)
    {
        free(TestCaseArray[i]);
    }

    //
    // Free the array of pointers
    //
    free(TestCaseArray);
}

/**
 * @brief Parse the test cases from the file
 * @param Filename The name of the file to parse
 *
 * @return A vector of pairs, where each pair contains a command and a vector of tokens
 */
std::vector<std::pair<std::string, std::vector<std::string>>>
ParseTestCases(const std::string & Filename)
{
    std::ifstream                                                 file(Filename);
    std::string                                                   Line;
    std::string                                                   Command;
    std::string                                                   CurrentToken;
    std::vector<std::string>                                      Tokens;
    std::vector<std::pair<std::string, std::vector<std::string>>> TestCases;
    BOOLEAN                                                       IsCommand  = FALSE;
    BOOLEAN                                                       AddNewline = FALSE;

    const std::string TokenDelimiter   = "----------------------------------";
    const std::string CommandDelimiter = "_____________________________________________________________";

    while (std::getline(file, Line))
    {
        if (Line == CommandDelimiter)
        {
            //
            // No new line is needed after this token
            //
            AddNewline = FALSE;

            //
            // Save the previous command and its tokens if any
            //
            if (!Command.empty())
            {
                if (!CurrentToken.empty())
                {
                    Tokens.push_back(CurrentToken);
                    CurrentToken.clear();
                }
                TestCases.push_back({Command, Tokens});
                Command.clear();
                Tokens.clear();
            }

            //
            // is command is true since a command is started
            //
            IsCommand = TRUE;
        }
        else if (Line == TokenDelimiter)
        {
            //
            // No new line is needed after this token
            //
            AddNewline = FALSE;

            //
            // not in command anymore
            //
            IsCommand = FALSE;

            //
            // If we're in the middle of collecting a token, save it
            //
            if (!CurrentToken.empty())
            {
                Tokens.push_back(CurrentToken);
                CurrentToken.clear();
            }
        }
        else
        {
            //
            // Accumulate lines for the command or token
            //
            if (IsCommand)
            {
                if (AddNewline)
                    Command += "\n";
                Command += Line;
            }
            else
            {
                if (AddNewline)
                    CurrentToken += "\n";
                CurrentToken += Line;
            }

            AddNewline = TRUE;
        }
    }

    //
    // Store the last command and tokens if any
    //
    if (!Command.empty())
    {
        if (!CurrentToken.empty())
        {
            Tokens.push_back(CurrentToken);
        }
        TestCases.push_back({Command, Tokens});
    }

    return TestCases;
}

/**
 * @brief Count the number of occurrences of the substring "\\n" up to a specified position
 * @param Str The string to search
 * @param Limit The position to search up to
 *
 * @return INT32 The number of occurrences of the substring "\\n"
 */
INT32
CountBackslashNUpToPosition(const std::string & Str, std::size_t Limit)
{
    INT32                  Count  = 0;
    std::string::size_type Pos    = 0;
    std::string            Target = "\\n";

    //
    // Limit the string to search within the specified range
    //
    while ((Pos = Str.find(Target, Pos)) != std::string::npos && Pos < Limit)
    {
        ++Count;
        Pos += Target.length(); // Move past the current occurrence
    }

    return Count;
}

/**
 * @brief Show parsed command and tokens
 * @param TestCase A pair containing a command and a vector of tokens
 * @param FailedTokenNum The number of the failed token
 * @param FailedTokenPosition The position of the failed token
 *
 * @return VOID
 */
VOID
ShowParsedCommandAndTokens(const std::pair<std::string,
                                           std::vector<std::string>> & TestCase,
                           UINT32                                      FailedTokenNum,
                           UINT32                                      FailedTokenPosition)
{
    UINT32 TokenNum = 0;

    //
    // Output the parsed test case
    //

    string ShowingCommand = TestCase.first;

    std::string::size_type Pos = 0;
    while ((Pos = ShowingCommand.find("\n", Pos)) != std::string::npos)
    {
        ShowingCommand.replace(Pos, 1, "\\n");
        Pos += 2; // Move past the newly added characters
    }

    std::cout << "Command: \"" << ShowingCommand << "\"" << std::endl;
    std::cout << "____________________________________\n";

    std::cout << "Expected Tokens: " << std::endl;

    for (const auto & Token : TestCase.second)
    {
        string ShowingToken = Token;

        Pos = 0;
        while ((Pos = ShowingToken.find("\n", Pos)) != std::string::npos)
        {
            ShowingToken.replace(Pos, 1, "\\n");
            Pos += 2; // Move past the newly added characters
        }

        if (TokenNum == FailedTokenNum)
        {
            std::cout << "  x ";
        }
        else
        {
            std::cout << "  - ";
        }

        std::cout << "\"" << ShowingToken << "\"" << std::endl;

        if (TokenNum == FailedTokenNum)
        {
            std::cout << "     ";
            INT32 CountOfSpaces = CountBackslashNUpToPosition(ShowingToken, FailedTokenPosition);

            CountOfSpaces += FailedTokenPosition;

            for (INT32 i = 0; i < CountOfSpaces; i++)
            {
                std::cout << " ";
            }
            std::cout << "^" << std::endl;
        }

        TokenNum++;
    }
}

/**
 * @brief Test command parser
 *
 * @return BOOLEAN
 */
BOOLEAN
TestCommandParser()
{
    BOOLEAN OverallResult       = TRUE;
    INT32   TestNum             = 0;
    CHAR    FilePath[MAX_PATH]  = {0};
    UINT32  FailedTokenNum      = 0;
    UINT32  FailedTokenPosition = 0;

    //
    // Parse the test cases from the file
    // Setup the path for the filename
    //
    if (!hyperdbg_u_setup_path_for_filename(COMMAND_PARSER_TEST_CASES_FILE, FilePath, MAX_PATH, TRUE))
    {
        //
        // Error could not find the test case files
        //
        cout << "[-] Could not find the test case files" << endl;
        return FALSE;
    }

    //
    // Parse the test cases from the file
    //
    auto TestCases = ParseTestCases(FilePath);

    //
    // Perform testing test cases with parsed file
    //
    cout << "Perform testing test cases with parsed file:" << endl;

    //
    // Output the parsed test cases
    //
    for (const auto & TestCase : TestCases)
    {
        TestNum++;

        //
        // Create CHAR**
        //
        CHAR_PTR_PTR TestCaseArray = CreateTestCaseArray(TestCase.second);

        //
        // Check token with actual parser
        //
        if (hyperdbg_u_test_command_parser((CHAR *)TestCase.first.c_str(),
                                           (UINT32)TestCase.second.size(),
                                           TestCaseArray,
                                           &FailedTokenNum,
                                           &FailedTokenPosition))
        {
            cout << "[+] Test number " << TestNum << " Passed " << endl;
        }
        else
        {
            //
            // Set overall result to FALSE since one of the test cases failed
            //
            OverallResult = FALSE;

            //
            // Show parsed command and tokens
            //
            cout << "\n============================================================" << endl;
            cout << "\n*********************          " << endl;
            cout << "*** Error details ***          " << endl;
            cout << "*********************          " << endl;
            cout << "\nParsed tokens from HyperDbg main command parser:\n"
                 << endl;

            //
            // Show tokens
            //
            hyperdbg_u_test_command_parser_show_tokens((CHAR *)TestCase.first.c_str());

            cout << "\n============================================================" << endl;

            cout << "\nThe parsed command and tokens (From file):" << endl;
            ShowParsedCommandAndTokens(TestCase, FailedTokenNum, FailedTokenPosition);

            cout << "\n[-] Test number " << TestNum << " Failed " << endl;
            cout << "============================================================\n"
                 << endl;

            break;
        }

        //
        // Clean up memory
        //
        FreeTestCaseArray(TestCaseArray, TestCase.second.size());
    }

    return OverallResult;
}
