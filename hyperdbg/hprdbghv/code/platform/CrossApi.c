/**
 * @file CrossApi.c
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief Implementation of cross APIs for different platforms
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Allocate a contiguous zeroed memory
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
CrsAllocateContiguousZeroedMemory(SIZE_T NumberOfBytes)
{
    PVOID            Result          = NULL;
    PHYSICAL_ADDRESS MaxPhysicalAddr = {.QuadPart = MAXULONG64};

    Result = MmAllocateContiguousMemory(NumberOfBytes, MaxPhysicalAddr);
    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);

    return Result;
}

/**
 * @brief Allocate a non-paged buffer
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
CrsAllocateNonPagedPool(SIZE_T NumberOfBytes)
{
    PVOID Result = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, POOLTAG);

    return Result;
}

/**
 * @brief Allocate a non-paged buffer (zeroed)
 *
 * @param NumberOfBytes
 * @return PVOID
 */
PVOID
CrsAllocateZeroedNonPagedPool(SIZE_T NumberOfBytes)
{
    PVOID Result = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, POOLTAG);

    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);

    return Result;
}

/**
 * @brief Free (dellocate) a non-paged buffer
 *
 * @param BufferAddress
 * @return VOID
 */
VOID
CrsFreePool(PVOID BufferAddress)
{
    ExFreePoolWithTag(BufferAddress, POOLTAG);
}
