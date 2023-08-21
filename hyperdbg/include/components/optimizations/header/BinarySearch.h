/**
 * @file BinarySearch.h
 * @author Mohammad K. Fallah (mkf1980@gmail.com)
 * @brief The header file for array management routines (Binary Search)
 * @details
 * @version 0.5
 * @date 2023-07-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
BinarySearchPrintArray(UINT64 ArrayPtr[], UINT32 NumberOfItems);

BOOLEAN
BinarySearchPerformSearchItem(UINT64 ArrayPtr[], UINT32 NumberOfItems, UINT32 * ResultIndex, UINT64 Key);
