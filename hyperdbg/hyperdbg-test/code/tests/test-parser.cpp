/**
 * @file test-parser.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Perfrom test on command parser
 * @details
 * @version 0.11
 * @date 2024-08-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

typedef char ** CHAR_PTR_PTR; // Define CHAR_PTR_PTR as a char**

/**
 * @brief Create an array of strings from a vector of strings
 * @param testCases The vector of strings to copy
 *
 * @return A pointer to the array of strings
 */
CHAR_PTR_PTR
createTestCaseArray(const std::vector<std::string> & testCases)
{
    //
    // Allocate memory for the array of pointers (size: number of test cases)
    //
    CHAR_PTR_PTR testCaseArray = new char *[testCases.size()];

    //
    // Allocate memory for each string and copy the content
    //
    for (size_t i = 0; i < testCases.size(); ++i)
    {
        testCaseArray[i] = new char[testCases[i].length() + 1]; // +1 for the null terminator
        std::strcpy(testCaseArray[i], testCases[i].c_str());
    }

    return testCaseArray;
}

/**
 * @brief Free the memory allocated for the test case array
 * @param testCaseArray The array of pointers to free
 * @param size The size of the array
 *
 * @return VOID
 */
VOID
freeTestCaseArray(CHAR_PTR_PTR testCaseArray, size_t size)
{
    //
    // Free each string
    //
    for (size_t i = 0; i < size; ++i)
    {
        delete[] testCaseArray[i];
    }

    //
    // Free the array of pointers
    //
    delete[] testCaseArray;
}

/**
 * @brief Parse the test cases from the file
 * @param filename The name of the file to parse
 *
 * @return A vector of pairs, where each pair contains a command and a vector of tokens
 */
std::vector<std::pair<std::string, std::vector<std::string>>>
parseTestCases(const std::string & filename)
{
    std::ifstream                                                 file(filename);
    std::string                                                   line;
    std::string                                                   command;
    std::string                                                   currentToken;
    std::vector<std::string>                                      tokens;
    std::vector<std::pair<std::string, std::vector<std::string>>> testCases;
    bool                                                          isCommand  = false;
    bool                                                          addNewline = false;

    const std::string tokenDelimiter   = "----------------------------------";
    const std::string commandDelimiter = "_____________________________________________________________";

    while (std::getline(file, line))
    {
        if (line == commandDelimiter)
        {
            //
            // No new line is needed after this token
            //
            addNewline = false;

            //
            // Save the previous command and its tokens if any
            //
            if (!command.empty())
            {
                if (!currentToken.empty())
                {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
                testCases.push_back({command, tokens});
                command.clear();
                tokens.clear();
            }

            //
            // is command is true since a command is started
            //
            isCommand = true;
        }
        else if (line == tokenDelimiter)
        {
            //
            // No new line is needed after this token
            //
            addNewline = false;

            //
            // not in command anymore
            //
            isCommand = false;

            //
            // If we're in the middle of collecting a token, save it
            //
            if (!currentToken.empty())
            {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        }
        else
        {
            //
            // Accumulate lines for the command or token
            //
            if (isCommand)
            {
                if (addNewline)
                    command += "\n";
                command += line;
            }
            else
            {
                if (addNewline)
                    currentToken += "\n";
                currentToken += line;
            }

            addNewline = true;
        }
    }

    //
    // Store the last command and tokens if any
    //
    if (!command.empty())
    {
        if (!currentToken.empty())
        {
            tokens.push_back(currentToken);
        }
        testCases.push_back({command, tokens});
    }

    return testCases;
}

/**
 * @brief Show parsed command and tokens
 * @param testCases A vector of pairs, where each pair contains a command and a vector of tokens
 *
 * @return VOID
 */
VOID
ShowParsedCommandAndTokens(const std::pair<std::string, std::vector<std::string>> & testCase)
{
    //
    // Output the parsed test case
    //

    string showingCommand = testCase.first;

    std::string::size_type pos = 0;
    while ((pos = showingCommand.find("\n", pos)) != std::string::npos)
    {
        showingCommand.replace(pos, 1, "\\n");
        pos += 2; // Move past the newly added characters
    }

    std::cout << "Command: \"" << showingCommand << "\"" << std::endl;
    std::cout << "____________________________________\n";

    std::cout << "Tokens: " << std::endl;

    for (const auto & token : testCase.second)
    {
        string showingToken = token;

        pos = 0;
        while ((pos = showingToken.find("\n", pos)) != std::string::npos)
        {
            showingToken.replace(pos, 1, "\\n");
            pos += 2; // Move past the newly added characters
        }

        std::cout << "  - \"" << showingToken << "\"" << std::endl;
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
    int  testNum            = 0;
    CHAR filePath[MAX_PATH] = {0};

    //
    // Parse the test cases from the file
    // Setup the path for the filename
    //
    if (!hyperdbg_u_setup_path_for_filename(COMMAND_PARSER_TEST_CASES_FILE, filePath, MAX_PATH, TRUE))
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
    auto testCases = parseTestCases(filePath);

    //
    // Perfom testing test cases with parsed file
    //
    cout << "Perfom testing test cases with parsed file:" << endl;

    //
    // Output the parsed test cases
    //
    for (const auto & testCase : testCases)
    {
        testNum++;

        //
        // Create CHAR**
        //
        CHAR_PTR_PTR testCaseArray = createTestCaseArray(testCase.second);

        //
        // Check token with actual parser
        //
        if (hyperdbg_u_test_command_parser((CHAR *)testCase.first.c_str(), (UINT32)testCase.second.size(), testCaseArray))
        {
            cout << "[+] Test number " << testNum << " Passed " << endl;
        }
        else
        {
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
            hyperdbg_u_test_command_parser_show_tokens((CHAR *)testCase.first.c_str());

            cout << "\n============================================================" << endl;

            cout << "\nThe parsed command and tokens (From file):" << endl;
            ShowParsedCommandAndTokens(testCase);

            cout << "\n[-] Test number " << testNum << " Failed " << endl;
            cout << "============================================================\n"
                 << endl;

            break;
        }

        //
        // Clean up memory
        //
        freeTestCaseArray(testCaseArray, testCases.size());
    }

    return TRUE;
}
