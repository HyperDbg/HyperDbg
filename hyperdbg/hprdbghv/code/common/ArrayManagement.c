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
    UINT32 i;

    if (*NumberOfItems >= MAX_NUM_OF_ARRAY)
    {
        //
        // Array list is full
        //
        return FALSE;
    }

    i = *NumberOfItems;

    //
    // Move elements of Arr[0..i-1], that are greater than Key,
    // to one position ahead of their current position
    //
    while (i > 0 && ArrayPtr[i - 1] > Key)
    {
        ArrayPtr[i] = ArrayPtr[i - 1];
        i           = i - 1;
    }
    ArrayPtr[i] = Key;
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
ArrayManagementDeleteItem(UINT64 ArrayPtr[], UINT32 * NumberOfItems, UINT32 Index)
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
 * @param Key
 * @param NumberOfItems
 * @param ResultIndex
 *
 * @return BOOLEAN
 */
BOOLEAN
ArrayManagementBinarySearch(UINT64 ArrayPtr[], UINT64 Key, UINT32 NumberOfItems, UINT32 * ResultIndex)
{
    UINT32 TempLow  = 0;
    UINT32 TempHigh = NumberOfItems;

    *ResultIndex = NULL;

    //
    // Becasue the high is the count of items, not index of the last item
    //
    TempHigh--;

    //
    // Repeat until the pointers low and high meet each other
    //
    while (TempLow <= TempHigh)
    {
        UINT32 Mid = (TempHigh + TempLow) / 2;

        if (ArrayPtr[Mid] == Key)
        {
            //
            // Set the result
            //
            *ResultIndex = Mid;

            //
            // Found
            //
            return TRUE;
        }

        if (ArrayPtr[Mid] < Key)
            TempLow = Mid + 1;

        else
            TempHigh = Mid - 1;
    }

    //
    // Not found
    //
    return FALSE;
}

/**
 * @brief Example of using array management functions
 *
 * @return VOID
 */
VOID
ArrayManagementExample()
{
    UINT64 Arr[MAX_NUM_OF_ARRAY] = {0};
    UINT32 NumberOfItems         = 0;
    UINT32 Index;

    ArrayManagementInsert(Arr, &NumberOfItems, 12);
    ArrayManagementInsert(Arr, &NumberOfItems, 11);
    ArrayManagementInsert(Arr, &NumberOfItems, 13);
    ArrayManagementInsert(Arr, &NumberOfItems, 5);
    ArrayManagementInsert(Arr, &NumberOfItems, 6);
    ArrayManagementInsert(Arr, &NumberOfItems, 8);

    //
    // Search for item equal to 15
    //
    BOOLEAN result = ArrayManagementBinarySearch(Arr, 13, NumberOfItems, &Index);

    if (result)
    {
        LogInfo("Index found: %d", Index);

        ArrayManagementDeleteItem(Arr, &NumberOfItems, Index);
    }
    else
    {
        LogInfo("Index not found!");
    }

    ArrayManagementPrintArray(Arr, NumberOfItems);

    return 0;
}
