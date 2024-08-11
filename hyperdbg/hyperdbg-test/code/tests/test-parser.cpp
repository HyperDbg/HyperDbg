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

/**
 * @brief Test command parser
 *
 * @return BOOLEAN
 */
BOOLEAN
TestCommandParser()
{
    hyperdbg_u_test_command_parser((CHAR *)"Salam");
    return TRUE;
}
