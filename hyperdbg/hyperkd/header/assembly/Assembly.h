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

//
// ====================  Kernel Test Functions ====================
// File : AsmDebugger.asm
//

/**
 * @brief default custom code handler for debugger
 *
 * @param Param1
 * @param Param2
 * @param Param3
 * @param Param4
 * @return unsigned long long
 */
extern void
AsmDebuggerCustomCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);

/**
 * @brief default condition code handler
 *
 * @param Param1
 * @param Param2
 * @param Param3
 * @return unsigned long long
 */
extern unsigned long long
AsmDebuggerConditionCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3);

/**
 * @brief Spin on thread
 *
 */
extern void
AsmDebuggerSpinOnThread();
