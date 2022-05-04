/**
 * @file test-cases.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief test cases
 * @details
 * @version 0.1
 * @date 2021-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Create test cases
 * @param std::vector<std::string> &
 * 
 * @return BOOLEAN
 */
BOOLEAN
TestCase(std::vector<std::string> & TestCases)
{
    string Line;

    //
    // Read the test case files
    //
    std::ifstream File(TEST_CASE_FILE_NAME);

    if (File.is_open())
    {
        while (std::getline(File, Line))
        {
            TestCases.push_back(Line);
        }
        File.close();
    }
    else
    {
        //
        // The file can't be opened
        //
        printf("err, test case file not found (%s) \npress enter to continue", TEST_CASE_FILE_NAME);
        _getch();

        return FALSE;
    }

    return TRUE;
}
