/**
 * @file ArrayManagement.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The file contains array management routines
 * @details
 * @version 0.4.1
 * @date 2023-07-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#define MAX_NUM_OF_ARRAY 10

/**
 * @brief Function to implement insertion sort
 *
 * @param ArrayPtr
 * @param NumberOfItems
 * @param Key
 *
 * @return BOOLEAN
 */
BOOLEAN
ArrayManagementInsert(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT64 Key)
{
    UINT32 Idx;

    if (*NumberOfItems >= MAX_NUM_OF_ARRAY)
    {
        //
        // Array list is full
        //
        return FALSE;
    }

    Idx = *NumberOfItems - 1;

    //
    // Move elements of arr[0..i-1], that are greater than key,
    // to one position ahead of their current position
    //
    while (ArrayPtr[Idx] > Key)
    {
        ArrayPtr[Idx + 1] = ArrayPtr[Idx];
        Idx               = Idx - 1;
    }
    ArrayPtr[Idx + 1] = Key;

    //
    // As it's inserted, we'll increment the number of Items
    //
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
 * @return VOID
 */
VOID
ArrayManagementDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index)
{
    for (size_t i = Index + 1; i < *NumberOfItems; i++)
    {
        ArrayPtr[i - 1] = ArrayPtr[i];
    }

    //
    // Increment the number of items
    //
    (*NumberOfItems)--;
}

/**
 * @brief A utility function to print an array of size NumberOfItems
 *
 * @param ArrayPtr
 * @param NumberOfItems
 *
 * @return VOID
 */
VOID
ArrayManagementPrintArray(UINT64 ArrayPtr[], UINT32 NumberOfItems)
{
    int i;
    for (i = 0; i < NumberOfItems; i++)
        Log("%d ", ArrayPtr[i]);
    Log("\n");
}

/**
 * @brief A utility function to perform the binary search
 *
 * @param ArrayPtr
 * @param High
 * @param Low
 * @param Key
 *
 * @return VOID
 */
UINT32
ArrayManagementBinarySearch(UINT64 ArrayPtr[], UINT32 High, UINT32 Low, UINT64 Key)
{
    int TempHigh = High;

    //
    // Becasue the high is the count of items, not index of the last item
    //
    TempHigh--;

    //
    // Repeat until the pointers low and high meet each other
    //
    while (Low <= TempHigh)
    {
        UINT32 Mid = Low + (TempHigh - Low) / 2;

        if (ArrayPtr[Mid] == Key)
            return Mid;

        if (ArrayPtr[Mid] < Key)
            Low = Mid + 1;

        else
            TempHigh = Mid - 1;
    }

    return -1;
}

/**
 * @brief Example of using array management functions
 *
 * @return VOID
 */
VOID
ArrayManagementExample()
{
    UINT64 Array[MAX_NUM_OF_ARRAY] = {0};
    UINT32 LowestIndex             = 0;
    UINT32 HighestIndex            = 0;
    UINT32 Index                   = 0;

    //
    // Inserting items into the list
    //
    ArrayManagementInsert(Array, &HighestIndex, 12);
    ArrayManagementInsert(Array, &HighestIndex, 11);
    ArrayManagementInsert(Array, &HighestIndex, 13);
    ArrayManagementInsert(Array, &HighestIndex, 5);
    ArrayManagementInsert(Array, &HighestIndex, 6);
    ArrayManagementInsert(Array, &HighestIndex, 8);

    //
    // Search for item equal to 15
    //
    Index = ArrayManagementBinarySearch(Array, HighestIndex, LowestIndex, 15);

    LogInfo("Index found: %d", Index);

    ArrayManagementDeleteItem(Array, &HighestIndex, Index);
    ArrayManagementPrintArray(Array, HighestIndex);

    return 0;
}
