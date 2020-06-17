/**
 * @file HiddenHooks.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of different EPT hidden hooks functions
 * @details All the R/W hooks, Execute hooks and hardware register simulators
 * are implemented here
 *  
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntifs.h>
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "Logging.h"
#include "Hooks.h"
#include "HypervisorRoutines.h"

/**
 * @brief Hook function that HooksExAllocatePoolWithTag
 * 
 * @param PoolType 
 * @param NumberOfBytes 
 * @param Tag 
 * @return PVOID 
 */
PVOID
ExAllocatePoolWithTagHook(
    POOL_TYPE PoolType,
    SIZE_T    NumberOfBytes,
    ULONG     Tag)
{
    LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x , Number Of Bytes = %d , Pool Type = %d ", Tag, NumberOfBytes, PoolType);
    return ExAllocatePoolWithTagOrig(PoolType, NumberOfBytes, Tag);
}

/**
 * @brief Make examples for testing hidden hooks
 * 
 * @return VOID 
 */
VOID
HiddenHooksTest()
{
    //
    // Hook Test
    //
    //EptPageHook(KeGetCurrentThread(), NULL, PsGetCurrentProcessId(), NULL, TRUE, TRUE, FALSE);
    //EptPageHook(ExAllocatePoolWithTag, ExAllocatePoolWithTagHook, PsGetCurrentProcessId(), (PVOID *)&ExAllocatePoolWithTagOrig, FALSE, FALSE, TRUE);
    //EptPageHook(ExAllocatePoolWithTag, AsmGeneralDetourHook, PsGetCurrentProcessId(), (PVOID *)&ExAllocatePoolWithTagOrig, FALSE, FALSE, TRUE);
    EptPageHook(NtCreateFile, AsmGeneralDetourHook, PsGetCurrentProcessId(), (PVOID *)&NtCreateFileOrig, FALSE, FALSE, TRUE);
    //
    // Unhook Tests
    //HvPerformPageUnHookSinglePage(ExAllocatePoolWithTag);
    //HvPerformPageUnHookAllPages();
    //
}
