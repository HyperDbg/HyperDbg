/**
 * @file tests.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for test functions
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Constants                   //
//////////////////////////////////////////////////

/**
 * @brief exe name of test process
 * 
 */
#define TEST_PROCESS_NAME "hyperdbg-test.exe"

//////////////////////////////////////////////////
//					Functions                   //
//////////////////////////////////////////////////

BOOLEAN
CreateProcessAndOpenPipeConnection(PVOID   KernelInformation,
                                   UINT32  KernelInformationSize,
                                   PHANDLE ConnectionPipeHandle,
                                   PHANDLE ThreadHandle,
                                   PHANDLE ProcessHandle);
VOID
CloseProcessAndClosePipeConnection(HANDLE ConnectionPipeHandle,
                                   HANDLE ThreadHandle,
                                   HANDLE ProcessHandle);
