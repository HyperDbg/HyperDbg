/**
 * @file InlineAsm.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The definition of functions written in Assembly
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// ====================  Kernel Test Functions ====================
// File : AsmKernelSideTests.asm
//

/**
 * @brief Tests with test tags wrapper
 *
 */
extern unsigned long long
AsmTestWrapperWithTestTags(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);
