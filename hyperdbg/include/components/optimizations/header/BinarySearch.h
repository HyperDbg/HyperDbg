/**
 * @file BinarySearch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The header file for array management routines
 * @details
 * @version 0.5
 * @date 2023-07-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once

//////////////////////////////////////////////////
//				Test Constants					//
//////////////////////////////////////////////////

/**
 * @brief Maximum number of array for test cases
 *
 */
#define MAX_NUM_OF_ARRAY 10

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
ArrayManagementInsert(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT64 Key);

BOOLEAN
ArrayManagementDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index);

VOID
ArrayManagementPrintArray(UINT64 ArrayPtr[], UINT32 NumberOfItems);

BOOLEAN
ArrayManagementBinarySearch(UINT64 ArrayPtr[], UINT32 NumberOfItems, UINT32 * ResultIndex, UINT64 Key);
