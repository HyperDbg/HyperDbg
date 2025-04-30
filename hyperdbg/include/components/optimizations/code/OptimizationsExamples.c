/**
 * @file OptimizationExamples.c
 * @author Mohammad K. Fallah (mkf1980@gmail.com)
 * @brief The file contains examples of using different optimization algorithms
 * @details
 * @version 0.6
 * @date 2023-08-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Maximum number of array for test cases
 *
 */
#define MAX_NUM_OF_ARRAY 10

/**
 * @brief Example of using array management functions for
 * insertion sort and binary search
 *
 * @return VOID
 */
VOID
OptimizationExampleInsertionSortAndBinarySearch()
{
    UINT64 Arr[MAX_NUM_OF_ARRAY] = {0};
    UINT32 NumberOfItems         = 0;
    UINT32 Index;

    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 12);
    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 11);
    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 13);
    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 5);
    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 6);
    InsertionSortInsertItem(Arr, &NumberOfItems, MAX_NUM_OF_ARRAY, &Index, 8);

    //
    // Search for item equal to 15
    //
    BOOLEAN Result = BinarySearchPerformSearchItem(Arr, NumberOfItems, &Index, 15);

    if (Result)
    {
        LogInfo("Index found: %d", Index);

        InsertionSortDeleteItem(Arr, &NumberOfItems, Index);
    }
    else
    {
        LogInfo("Index not found!");
    }

    BinarySearchPrintArray(Arr, NumberOfItems);

}
