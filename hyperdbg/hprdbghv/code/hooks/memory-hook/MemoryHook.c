/**
 * @file MemoryHook.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of memory hooks functions
 * @details
 *
 * @version 0.2
 * @date 2023-02-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize the memory hooking mechanism
 *
 * @return VOID
 */
BOOLEAN
MemoryHookInitialize()
{
    PVMM_EPT_PAGE_TABLE PageTable;

    //
    // Allocate a secondary page-table
    //
    PageTable = EptAllocateAndCreateIdentityPageTable();
}
