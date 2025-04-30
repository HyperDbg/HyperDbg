/**
 * @file InsertionSort.h
 * @author Mohammad K. Fallah (mkf1980@gmail.com)
 * @brief Headers for the file that contains array management routines (Insertion Sort)
 * @details
 * @version 0.6
 * @date 2023-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
InsertionSortInsertItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 MaxNumOfItems, UINT32 * Index, UINT64 Key);

BOOLEAN
InsertionSortDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index);
