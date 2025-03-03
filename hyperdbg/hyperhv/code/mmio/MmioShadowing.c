/**
 * @file MmioShadowing.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for MMIO shadowing
 *
 * @version 0.14
 * @date 2025-02-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Apply memory modifications to the target device MMIO range
 *
 * @param VCpu The virtual processor's state
 * @param HookedPage The details of hooked pages
 *
 * @return VOID
 */
VOID
MmioShadowingApplyPageModification(VIRTUAL_MACHINE_STATE * VCpu,
                                   PEPT_HOOKED_PAGE_DETAIL HookedPage)
{
    UNREFERENCED_PARAMETER(VCpu);

    UINT32 OffsetOfAccessedMemory;
    UINT64 WrittenValue = NULL64_ZERO; // Initialize it by zero because we might not use the entire 64-bit

    //
    // Compute the offset of accessed memory in the redirected page (VA)
    //
    OffsetOfAccessedMemory = (UINT32)(HookedPage->LastContextState.VirtualAddress - (UINT64)PAGE_ALIGN(HookedPage->LastContextState.VirtualAddress));

    //
    // Get the written value in the redirected (shadow) page
    //
    if (HookedPage->LastContextState.MmioOperandWidth <= sizeof(UINT64))
    {
        memcpy(&WrittenValue,
               &HookedPage->FakePageContents[OffsetOfAccessedMemory],
               HookedPage->LastContextState.MmioOperandWidth);
    }
    else
    {
        LogInfo("Warning, the operand width of the MMIO instruction is bigger than 64 bit, the written value is not correct");
    }

    //
    // Create a log from modified (redirected) page
    //
    LogInfo("Original MMIO Base PA: %llx, accessed PA : %llx, accessed VA: %llx, redirected size: %x, written value: %llx",
            HookedPage->PhysicalBaseAddress,
            HookedPage->LastContextState.PhysicalAddress,
            HookedPage->LastContextState.VirtualAddress,
            HookedPage->LastContextState.MmioOperandWidth,
            WrittenValue);

    //
    // Apply the written value to the target device
    //
    switch (HookedPage->LastContextState.MmioOperandWidth)
    {
    case sizeof(BYTE):
        MemoryMapperWriteMmioMemory(HookedPage->LastContextState.PhysicalAddress, (UINT8)WrittenValue, PAGE_CACHE_POLICY_UC, MMIO_ACCESS_SIZE_1_BYTE);
        break;

    case sizeof(WORD):
        MemoryMapperWriteMmioMemory(HookedPage->LastContextState.PhysicalAddress, (UINT16)WrittenValue, PAGE_CACHE_POLICY_UC, MMIO_ACCESS_SIZE_2_BYTES);
        break;

    case sizeof(DWORD):
        MemoryMapperWriteMmioMemory(HookedPage->LastContextState.PhysicalAddress, (UINT32)WrittenValue, PAGE_CACHE_POLICY_UC, MMIO_ACCESS_SIZE_4_BYTES);
        break;

    case sizeof(QWORD):
        MemoryMapperWriteMmioMemory(HookedPage->LastContextState.PhysicalAddress, (UINT64)WrittenValue, PAGE_CACHE_POLICY_UC, MMIO_ACCESS_SIZE_8_BYTES);
        break;

    default:
        LogInfo("Err, Not supported MMIO operand width");
        break;
    }
}
