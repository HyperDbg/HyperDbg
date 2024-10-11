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
 * @param content
 *
 * @return std::string
 */
std::string
commentContent(const std::string & content)
{
    std::istringstream iss(content);
    std::string        line;
    std::string        commentedContent;

    while (std::getline(iss, line))
    {
        commentedContent += line + "\n; ";
    }

    return commentedContent;
}

/**
 * @brief Read directory of hwdbg script test cases and run each of them
 *
 * @param HwdbgScriptTestCasesPath Path to the hwdbg script test cases
 *
 * @return BOOLEAN
 */
BOOLEAN
ReadDirectoryAndCreateHwdbgTestCases(const char * HwdbgScriptTestCasesPath)
{
    CHAR tempFilePath[MAX_PATH] = {0};

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
                    std::cout << "Reading script test case file " << entry.path().filename().string() << std::endl;

                    // std::cout << content << std::endl;

                    std::string compiled_version_file_path = HWDBG_SCRIPT_TEST_CASE_COMPILED_SCRIPTS_DIRECTORY "\\" + entry.path().filename().string() + ".hex.txt";

                    //
                    // Run the test case command
                    //
                    printf("File content: %s\n", content.c_str());

                    if (!hwdbg_script_run_script(content.c_str(),
                                                 HWDBG_TEST_READ_INSTANCE_INFO_PATH,
                                                 compiled_version_file_path.c_str(),
                                                 DEFAULT_INITIAL_BRAM_BUFFER_SIZE))
                    {
                        std::cout << "[-] Could not run the script: " << filePath << std::endl;
                        return FALSE;
                    }

                    //
                    // Now open the compiled_version_file_path file and add content at the top
                    //

                    //
                    // Parse the hwdbg compiled test cases from the file
                    //
                    if (!hyperdbg_u_setup_path_for_filename(compiled_version_file_path.c_str(), tempFilePath, MAX_PATH, FALSE))
                    {
                        //
                        // Error could not find the test case files
                        //
                        std::cout << "[-] Could not find the compiled version of the hwdbg test case file" << endl;
                        return FALSE;
                    }

                    std::ifstream compiledFile(tempFilePath);
                    if (compiledFile.is_open())
                    {
                        //
                        // Read the existing content of the compiled file
                        //
                        std::string compiledContent((std::istreambuf_iterator<char>(compiledFile)),
                                                    std::istreambuf_iterator<char>());
                        compiledFile.close(); // Close the file after reading

                        //
                        // Comment the content
                        //
                        std::string commentedContent = commentContent(content);

                        //
                        // Concatenate the new content (prepend the original content)
                        //
                        std::string newContent = "; The raw script file is available at: " HWDBG_SCRIPT_TEST_CASE_COMPILED_SCRIPTS_DIRECTORY "\\" +
                                                 entry.path().filename().string() +
                                                 "\n;\n; !hw script " +
                                                 commentedContent +
                                                 "\n" +
                                                 compiledContent;

                        //
                        // Write the new content back to the file (overwriting it)
                        //
                        std::ofstream compiledFileOut(tempFilePath);
                        if (compiledFileOut.is_open())
                        {
                            compiledFileOut << newContent;
                            compiledFileOut.close();
                        }
                        else
                        {
                            std::cerr << "Could not open file for writing: " << compiled_version_file_path << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "Could not open compiled file: " << compiled_version_file_path << std::endl;
                    }

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
    int  testNum           = 0;
    CHAR dirPath[MAX_PATH] = {0};

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
