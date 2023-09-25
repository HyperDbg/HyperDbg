/**
 * @file Definition.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for global definitions
 * @details This file contains definitions that are use in both user mode and
 * kernel mode Means that if you change the following files, structures or
 * enums, then these settings apply to both usermode and kernel mode
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                Config File                  //
//////////////////////////////////////////////////

/**
 * @brief Config file name for HyperDbg
 *
 */
#define CONFIG_FILE_NAME L"config.ini"

//////////////////////////////////////////////////
//                   Installer                  //
//////////////////////////////////////////////////

/**
 * @brief name of HyperDbg's VMM driver
 *
 */
#define VMM_DRIVER_NAME "hprdbghv"

/**
 * @brief name of HyperDbg's VMM driver
 *
 */
#define KERNEL_DEBUGGER_DRIVER_NAME "hprdbgkd"

//////////////////////////////////////////////////
//				   Test Cases                   //
//////////////////////////////////////////////////

/**
 * @brief Test cases file name
 */
#define TEST_CASE_FILE_NAME "test-cases.txt"

/**
 * @brief Test cases file name
 */
#define SCRIPT_ENGINE_TEST_CASES_DIRECTORY "script-test-cases"

/**
 * @brief Maximum test cases to communicate between debugger and debuggee process
 */
#define TEST_CASE_MAXIMUM_NUMBER_OF_KERNEL_TEST_CASES 200

/**
 * @brief Maximum buffer to communicate between debugger and debuggee process
 */
#define TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE sizeof(DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION) * TEST_CASE_MAXIMUM_NUMBER_OF_KERNEL_TEST_CASES

//////////////////////////////////////////////////
//				Delay Speeds                    //
//////////////////////////////////////////////////

/**
 * @brief The speed delay for showing messages from kernel-mode
 * to user-mode in  VMI-mode, using a lower value causes the
 * HyperDbg to show messages faster but you should keep in mind,
 *  not to eat all of the CPU
 */
#define DefaultSpeedOfReadingKernelMessages 30
