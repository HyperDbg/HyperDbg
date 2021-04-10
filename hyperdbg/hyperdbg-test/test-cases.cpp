/**
 * @file test-cases.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @return VOID
 */
VOID
TestCase(std::vector<std::string> & TestCase)
{
    TestCase.push_back("!epthook [ExAllocatePoolWithTag] script { print(@rax); }");
    TestCase.push_back("!epthook2 [NtWriteFile] script { print(@rax); }");
    TestCase.push_back("!syscall script { print(@rax); }");
}
