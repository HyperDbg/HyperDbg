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

//
// Global Variables
//
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;
extern BOOLEAN                    g_HwdbgInstanceInfoIsValid;

/**
 * @brief Create test cases for hwdbg
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgTestCreateTestCases()
{
    //
    // Perform test with default file path and initial BRAM buffer size
    //
    /* hwdbg_script_run_script("script here",
                            HWDBG_TEST_READ_INSTANCE_INFO_PATH,
                            HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH,
                            DEFAULT_INITIAL_BRAM_BUFFER_SIZE);
                            */

    return TRUE;
}
