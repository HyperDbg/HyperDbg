/**
 * @file BinarySearch.c
 * @author Mohammad K. Fallah (mkf1980@gmail.com)
 * @brief The file contains array management routines (Binary Search)
 * @details
 * @version 0.5
 * @date 2023-07-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief A utility function to print an array of size NumberOfItems
 *
 * @param ArrayPtr
 * @param NumberOfItems
 *
 * @return VOID
 */
VOID
BinarySearchPrintArray(UINT64 ArrayPtr[], UINT32 NumberOfItems)
{
    UINT32 i;

    for (i = 0; i < NumberOfItems; i++)
    {
        Log("%llx ", ArrayPtr[i]);
    }

    Log("\n");
}

/**
 * @brief A utility function to perform the binary search
 *
 * @param ArrayPtr
 * @param NumberOfItems
 * @param ResultIndex
 * @param Key
 *
 * @return BOOLEAN
 */
BOOLEAN
BinarySearchPerformSearchItem(UINT64 ArrayPtr[], UINT32 NumberOfItems, UINT32 * ResultIndex, UINT64 Key)
{
    UINT32 Position = 0;
    UINT32 Limit    = NumberOfItems;

    while (Position < Limit)
    {
        UINT32 TestPos = Position + ((Limit - Position) >> 1);

        if (ArrayPtr[TestPos] < Key)
            Position = TestPos + 1;
        else
            Limit = TestPos;
    }

    if (Position < NumberOfItems && ArrayPtr[Position] == Key)
    {
        //
        // Set the result position in the array
        //
        *ResultIndex = Position;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
