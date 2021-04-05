/**
 * @file KernelTests.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief Collect the kernel-side debugging informations
 * 
 * @param InfoRequest user-mode buffer to fill kernel information
 * @return VOID 
 */
VOID
TestKernelGetInformation(PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION InfoRequest)
{
    //
    // Fill kernel functions information
    //
    InfoRequest->AddressOfExAllocatePoolWithTag = ExAllocatePoolWithTag;

    //
    // Indicate that it's successful
    //
    InfoRequest->KernelResult = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
}
