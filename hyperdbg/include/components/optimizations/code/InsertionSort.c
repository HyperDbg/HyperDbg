/**
 * @file InsertionSort.c
 * @author Mohammad K. Fallah (mkf1980@gmail.com)
 * @brief The file contains array management routines (Insertion Sort)
 * @details
 * @version 0.6
 * @date 2023-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Function to implement insertion sort
 *
 * @param ArrayPtr
 * @param NumberOfItems
 * @param MaxNumOfItems
 * @param Index
 * @param Key
 *
 * @return BOOLEAN
 */
BOOLEAN
InsertionSortInsertItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 MaxNumOfItems, UINT32 * Index, UINT64 Key)
{
    UINT32 Idx;

    if (*NumberOfItems >= MaxNumOfItems)
    {
        //
        // Array list is full
        //
        return FALSE;
    }

    Idx = *NumberOfItems;

    //
    // Move elements of Arr[0..i-1], that are greater than Key,
    // to one position ahead of their current position
    //
    while (Idx > 0 && ArrayPtr[Idx - 1] > Key)
    {
        ArrayPtr[Idx] = ArrayPtr[Idx - 1];
        Idx           = Idx - 1;
    }
    ArrayPtr[Idx] = Key;
    *Index        = Idx;
    (*NumberOfItems)++;

    //
    // Successfully inserted
    //
    return TRUE;
}

/**
 * @brief Function to implement insertion sort
 *
 * @param ArrayPtr
 * @param NumberOfItems
 * @param Index
 *
 * @return BOOLEAN
 */
BOOLEAN
InsertionSortDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index)
{
    //
    // check if index is valid
    //
    if (Index >= *NumberOfItems)
    {
        return FALSE;
    }

    for (size_t i = Index + 1; i < *NumberOfItems; i++)
    {
        ArrayPtr[i - 1] = ArrayPtr[i];
    }

    (*NumberOfItems)--;

    return TRUE;
}
