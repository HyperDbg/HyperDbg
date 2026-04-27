/**
 * @file CrossVmcalls.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines relating to cross (standalone) VMCALLs
 * @details
 * @version 0.19
 * @date 2026-04-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Get the guest state of IA32_DEBUGCTL on the target core from VMCS
 * using VMCALL
 *
 * @return UINT64
 */
UINT64
CrossVmcallGetDebugctlVmcallOnTargetCore()
{
    UINT64 DebugctlValue;
    AsmVmxVmcall(VMCALL_GET_VMCS_DEBUGCTL, (UINT64)&DebugctlValue, NULL64_ZERO, NULL64_ZERO);
    return DebugctlValue;
}

/**
 * @brief Set the guest state of IA32_DEBUGCTL on the target core from VMCS using VMCALL
 * @param Value
 *
 * @return VOID
 */
VOID
CrossVmcallSetDebugctlVmcallOnTargetCore(UINT64 Value)
{
    AsmVmxVmcall(VMCALL_SET_VMCS_DEBUGCTL, Value, NULL64_ZERO, NULL64_ZERO);
}

/**
 * @brief Set the guest state of MSR_LBR_SELECT on the target core from VMCS using VMCALL
 * @param FilterOptions
 *
 * @return VOID
 */
VOID
CrossVmcallSetLbrSelectVmcallOnTargetCore(UINT64 FilterOptions)
{
    AsmVmxVmcall(VMCALL_SET_MSR_LBR_SELECT, FilterOptions, NULL64_ZERO, NULL64_ZERO);
}

/**
 * @brief Set LOAD DEBUG CONTROLS on Vm-entry controls on the target core from VMCS using VMCALL
 *
 * @param Set Set or unset
 *
 * @return VOID
 */
VOID
CrossVmcallSetLoadDebugControlsVmcallOnTargetCore(BOOLEAN Set)
{
    if (Set)
    {
        AsmVmxVmcall(VMCALL_SET_VM_ENTRY_LOAD_DEBUG_CONTROLS, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);
    }
    else
    {
        AsmVmxVmcall(VMCALL_UNSET_VM_ENTRY_LOAD_DEBUG_CONTROLS, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);
    }
}

/**
 * @brief Set SAVE DEBUG CONTROLS on Vm-exit controls on the target core from VMCS using VMCALL
 *
 * @param Set Set or unset
 *
 * @return VOID
 */
VOID
CrossVmcallSetSaveDebugControlsVmcallOnTargetCore(BOOLEAN Set)
{
    if (Set)
    {
        AsmVmxVmcall(VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);
    }
    else
    {
        AsmVmxVmcall(VMCALL_UNSET_VM_EXIT_SAVE_DEBUG_CONTROLS, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);
    }
}
