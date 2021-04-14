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
 * @brief Perform the kernel-side tests
 * 
 * @param KernelTestRequest user-mode buffer to fill kernel test information
 * @return VOID 
 */
VOID
TestKernelPerformTests(PDEBUGGER_PERFORM_KERNEL_TESTS KernelTestRequest)
{
    UINT64 TempPool = 0;

    //
    // Call wrapper for ExAllocatePoolWithTag
    //
    TempPool = AsmTestExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

    if (TempPool != NULL)
    {
        //
        // Free the previous pool
        //
        //ExFreePoolWithTag(TempPool, POOLTAG);
    }

    LogInfo("hello test");
    KernelTestRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
}

/**
 * @brief Collect the kernel-side debugging informations
 * 
 * @param InfoRequest user-mode buffer to fill kernel information
 * @return UINT32 Filled entries 
 */
UINT32
TestKernelGetInformation(PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION InfoRequest)
{
    UINT32 Index = 0;

    //
    // Zero the memory
    //
    RtlZeroMemory(&InfoRequest[0], TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // *** Fill kernel functions information ***
    //

    // ------------------------------------------------------

    Index                    = 0;
    InfoRequest[Index].Value = ExAllocatePoolWithTag;
    memcpy(&InfoRequest[Index].Tag, "ExAllocatePoolWithTag", strlen("ExAllocatePoolWithTag") + 1);

    // ------------------------------------------------------

    Index                    = 1;
    InfoRequest[Index].Value = NtReadFile;
    memcpy(&InfoRequest[Index].Tag, "NtReadFile", strlen("NtReadFile") + 1);

    // ------------------------------------------------------

    Index                    = 2;
    InfoRequest[Index].Value = NtWriteFile;
    memcpy(&InfoRequest[Index].Tag, "NtWriteFile", strlen("NtWriteFile") + 1);

    // ------------------------------------------------------

    //
    // Check maximum index
    //
    if (Index > TEST_CASE_MAXIMUM_NUMBER_OF_KERNEL_TEST_CASES)
    {
        LogError("err, test cases are above the supported buffers");
        return 0;
    }

    //
    // Return the index (add +1 because we start from zero)
    //
    return Index + 1;
}
