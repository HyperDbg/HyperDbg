/**
 * @file tests.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief headers for test functions
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Test cases
//

#define DEBUGGER_TEST_ALL_COMMANDS 0x0
#define DEBUGGER_TEST_VMM_MONITOR_COMMAND 0x1

//
// Test commands
//
BOOLEAN TestMonitorCommand();
