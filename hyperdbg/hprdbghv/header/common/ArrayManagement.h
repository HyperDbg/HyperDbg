/**
 * @file ArrayManagement.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The header file for array management routines
 * @details
 * @version 0.4.1
 * @date 2023-07-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
ArrayManagementInsert(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT64 Key);

VOID
ArrayManagementDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index);

VOID
ArrayManagementPrintArray(UINT64 ArrayPtr[], UINT32 NumberOfItems);

UINT32
ArrayManagementBinarySearch(UINT64 ArrayPtr[], UINT32 High, UINT32 Low, UINT64 Key);
