/**
 * @file main.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief perform tests
 * @details
 * @version 0.11
 * @date 2024-08-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Main function of test process
 *
 * @param argc
 * @param argv
 * @return int
 */
int
main(int argc, char * argv[])
{
    // if (argc != 2)
    // {
    //     printf("you should not test functionalities directly, instead use 'test' "
    //            "command from HyperDbg...\n");
    //     return 1;
    // }

    //
    // # Test case 1
    // Testing command parser
    //
    TestCommandParser();

    return 0;
}
