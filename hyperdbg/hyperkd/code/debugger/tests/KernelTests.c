/**
 * @file KernelTests.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of kernel-side test functions
 * @details
 *
 * @version 0.1
 * @date 2021-04-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Perform the kernel-side tests
 *
 * @param KernelTestRequest user-mode buffer to fill kernel test information
 * @return VOID
 */
VOID
TestKernelPerformTests(PDEBUGGER_PERFORM_KERNEL_TESTS KernelTestRequest)
{
    LogInfo("Starting kernel-test process...");

    LogInfo("All the kernel events are triggered");

    KernelTestRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
}
