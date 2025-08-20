/**
 * @file test-semanitc-scripts.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Perform test on semantic scripts
 * @details
 * @version 0.11
 * @date 2024-08-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

namespace fs = std::filesystem;

/**
 * @brief Read directory of semantic test cases and run each of them
 *
 * @param ScriptSemanticPath Path to the semantic test cases
 *
 * @return VOID
 */
VOID
ReadDirectoryAndTestSemanticTestcases(const char * ScriptSemanticPath)
{
    //
    // Iterate through the directory
    //
    try
    {
        for (const auto & entry : fs::directory_iterator(ScriptSemanticPath))
        {
            //
            // Check if the entry is a file
            //
            if (entry.is_regular_file())
            {
                //
                // Get the file path
                //
                std::string filePath = entry.path().string();

                //
                // Output the file name
                //
                // std::cout << "Test case file: " << entry.path().filename().string() << std::endl;

                //
                // Open the file and read its contents
                //
                std::ifstream file(filePath);
                if (file.is_open())
                {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());

                    //
                    // Display the content of the file
                    //
                    std::cout << "Executing file " << entry.path().filename().string() << std::endl;

                    // std::cout << content << std::endl;

                    //
                    // Run the test case command
                    //
                    hyperdbg_u_run_command((CHAR *)content.c_str());

                    std::cout << "--------------------------------------------" << std::endl;

                    //
                    // Close the file
                    //
                    file.close();
                }
                else
                {
                    std::cerr << "Could not open file: " << filePath << std::endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error & e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

/**
 * @brief Test semantic scripts
 *
 * @return BOOLEAN
 */
BOOLEAN
TestSemanticScripts()
{
    int  testNum           = 0;
    CHAR dirPath[MAX_PATH] = {0};

    //
    // Parse the semantic script test cases from the file
    // Setup the path for the filenames
    //
    if (!hyperdbg_u_setup_path_for_filename(SCRIPT_SEMANTIC_TEST_CASE_DIRECTORY, dirPath, MAX_PATH, FALSE))
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
    // Run test cases
    //
    ReadDirectoryAndTestSemanticTestcases(dirPath);

    //
    // Close the connection
    //
    hyperdbg_u_debug_close_remote_debugger();

    return TRUE;
}
