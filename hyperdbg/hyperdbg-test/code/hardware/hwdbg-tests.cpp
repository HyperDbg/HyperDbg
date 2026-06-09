/**
 * @file hwdbg-tests.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Test cases for testing hwdbg
 * @details
 * @version 0.11
 * @date 2024-09-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

namespace fs = std::filesystem;

/**
 * @brief function to comment each line by adding ';' at the start
 * @param Content the content to comment out
 *
 * @return std::string
 */
std::string
CommentContent(const std::string & Content)
{
    std::istringstream Iss(Content);
    std::string        Line;
    std::string        CommentedContent;

    while (std::getline(Iss, Line))
    {
        CommentedContent += Line + "\n; ";
    }

    return CommentedContent;
}

/**
 * @brief Read directory of hwdbg script test cases and run each of them
 *
 * @param HwdbgScriptTestCasesPath Path to the hwdbg script test cases
 *
 * @return BOOLEAN
 */
BOOLEAN
ReadDirectoryAndCreateHwdbgTestCases(const CHAR * HwdbgScriptTestCasesPath)
{
    CHAR TempFilePath[MAX_PATH] = {0};

    //
    // Iterate through the directory
    //
    try
    {
        for (const auto & entry : fs::directory_iterator(HwdbgScriptTestCasesPath))
        {
            //
            // Check if the entry is a file
            //
            if (entry.is_regular_file())
            {
                //
                // Get the file path
                //
                std::string FilePath = entry.path().string();

                //
                // Output the file name
                //
                // std::cout << "Test case file: " << entry.path().filename().string() << std::endl;

                //
                // Open the file and read its contents
                //
                std::ifstream File(FilePath);
                if (File.is_open())
                {
                    std::string Content((std::istreambuf_iterator<char>(File)),
                                        std::istreambuf_iterator<char>());

                    //
                    // Display the content of the file
                    //
                    std::cout << "Reading script test case file " << entry.path().filename().string() << std::endl;

                    // std::cout << content << std::endl;

                    std::string CompiledVersionFilePath = HWDBG_SCRIPT_TEST_CASE_COMPILED_SCRIPTS_DIRECTORY "\\" + entry.path().filename().string() + ".hex.txt";

                    //
                    // Run the test case command
                    //
                    printf("File content: %s\n", Content.c_str());

                    if (!hwdbg_script_run_script(Content.c_str(),
                                                 HWDBG_TEST_READ_INSTANCE_INFO_PATH,
                                                 CompiledVersionFilePath.c_str(),
                                                 DEFAULT_INITIAL_BRAM_BUFFER_SIZE))
                    {
                        std::cout << "[-] Could not run the script: " << FilePath << std::endl;
                        return FALSE;
                    }

                    //
                    // Now open the compiled_version_file_path file and add content at the top
                    //

                    //
                    // Parse the hwdbg compiled test cases from the file
                    //
                    if (!hyperdbg_u_setup_path_for_filename(CompiledVersionFilePath.c_str(), TempFilePath, MAX_PATH, FALSE))
                    {
                        //
                        // Error could not find the test case files
                        //
                        std::cout << "[-] Could not find the compiled version of the hwdbg test case file" << endl;
                        return FALSE;
                    }

                    std::ifstream CompiledFile(TempFilePath);
                    if (CompiledFile.is_open())
                    {
                        //
                        // Read the existing content of the compiled file
                        //
                        std::string CompiledContent((std::istreambuf_iterator<char>(CompiledFile)),
                                                    std::istreambuf_iterator<char>());
                        CompiledFile.close(); // Close the file after reading

                        //
                        // Comment the content
                        //
                        std::string CommentedContent = CommentContent(Content);

                        //
                        // Concatenate the new content (prepend the original content)
                        //
                        std::string NewContent = "; The raw script file is available at: " HWDBG_SCRIPT_TEST_CASE_COMPILED_SCRIPTS_DIRECTORY "\\" +
                                                 entry.path().filename().string() +
                                                 "\n;\n; !hw script " +
                                                 CommentedContent +
                                                 "\n" +
                                                 CompiledContent;

                        //
                        // Write the new content back to the file (overwriting it)
                        //
                        std::ofstream CompiledFileOut(TempFilePath);
                        if (CompiledFileOut.is_open())
                        {
                            CompiledFileOut << NewContent;
                            CompiledFileOut.close();
                        }
                        else
                        {
                            std::cerr << "Could not open file for writing: " << CompiledVersionFilePath << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "Could not open compiled file: " << CompiledVersionFilePath << std::endl;
                    }

                    std::cout << "--------------------------------------------" << std::endl;

                    //
                    // Close the file
                    //
                    File.close();
                }
                else
                {
                    std::cerr << "Could not open file: " << FilePath << std::endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error & e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return FALSE;
    }

    //
    // Return success
    //
    return TRUE;
}

/**
 * @brief Create test cases for hwdbg
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgTestCreateTestCases()
{
    INT32 TestNum           = 0;
    CHAR  dirPath[MAX_PATH] = {0};

    //
    // Parse the hwdbg test cases from the file
    // Setup the path for the filenames
    //
    if (!hyperdbg_u_setup_path_for_filename(HWDBG_SCRIPT_TEST_CASE_CODE_DIRECTORY, dirPath, MAX_PATH, FALSE))
    {
        //
        // Error could not find the test case files
        //
        cout << "[-] Could not find the hwdbg test case files" << endl;
        return FALSE;
    }

    //
    // Run test cases
    //
    return ReadDirectoryAndCreateHwdbgTestCases(dirPath);
}
