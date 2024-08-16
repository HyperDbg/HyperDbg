/**
 * @file test-semanitc-scripts.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Perfrom test on semantic scripts
 * @details
 * @version 0.11
 * @date 2024-08-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Test semantic scripts
 *
 * @return BOOLEAN
 */
BOOLEAN
TestSemanticScripts()
{
    BOOLEAN overallResult      = FALSE;
    int     testNum            = 0;
    CHAR    filePath[MAX_PATH] = {0};

    //
    // Parse the semantic script test cases from the file
    // Setup the path for the filenames
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
    // Connect to the debugger
    //
    if (!hyperdbg_u_connect_remote_debugger_using_named_pipe(TEST_DEFAULT_NAMED_PIPE, TRUE))
    {
        cout << "[-] Could not connect to the debugger" << endl;
        return FALSE;
    }

    //
    // Run a test command
    //
    if (hyperdbg_u_run_command((CHAR *)".process") != 0)
    {
        cout << "[-] Could not run the test command" << endl;
        return FALSE;
    }

    return overallResult;
}
