/**
 * @file KernelTests.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Kernel Test headers
 * @details 
 * @version 0.1
 * @date 2021-04-06
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
TestKernelPerformTests(PDEBUGGER_PERFORM_KERNEL_TESTS KernelTestRequest);

UINT32
TestKernelGetInformation(PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION InfoRequest);
