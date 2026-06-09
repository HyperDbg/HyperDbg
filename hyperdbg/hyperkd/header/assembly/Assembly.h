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
 * @param Param1
 * @param Param2
 * @param Param3
 * @param Param4
 * @return UINT64
 */
extern UINT64
AsmTestWrapperWithTestTags(UINT64 Param1, UINT64 Param2, UINT64 Param3, UINT64 Param4);

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
 * @return VOID
 */
extern VOID
AsmDebuggerCustomCodeHandler(UINT64 Param1, UINT64 Param2, UINT64 Param3, UINT64 Param4);

/**
 * @brief default condition code handler
 *
 * @param Param1
 * @param Param2
 * @param Param3
 * @return UINT64
 */
extern UINT64
AsmDebuggerConditionCodeHandler(UINT64 Param1, UINT64 Param2, UINT64 Param3);

/**
 * @brief Spin on thread
 *
 */
extern VOID
AsmDebuggerSpinOnThread();
