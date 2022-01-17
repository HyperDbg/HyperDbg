#include "..\hprdbghv\pch.h"
#include "platform/CrossApi.h"

/**
 * @brief 
 * @param NumberOfBytes 
 * @return 
*/
PVOID
CrsAllocateContiguousZeroedMemory(_In_ SIZE_T NumberOfBytes)
{
    PVOID            Result          = NULL;
    PHYSICAL_ADDRESS MaxPhysicalAddr = {.QuadPart = MAXULONG64};

    Result = MmAllocateContiguousMemory(NumberOfBytes, MaxPhysicalAddr);
    if (Result != NULL)
        RtlSecureZeroMemory(Result, NumberOfBytes);

    return Result;
}