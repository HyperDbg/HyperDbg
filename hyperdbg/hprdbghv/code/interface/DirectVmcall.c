/**
 * @file DirectVmcall.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Direct VMCALL routines
 * @details
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines for test direct VMCALL
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallTest(UINT32                     CoreId,
                 DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_TEST, DirectVmcallOptions);
}

/**
 * @brief routines for test direct VMCALL
 * @details Should be called from VMX root-mode
 *
 * @param CoreId
 * @param DirectVmcallOptions
 *
 * @return NTSTATUS
 */
NTSTATUS
DirectVmcallChangeMsrBitmapRead(UINT32                     CoreId,
                                DIRECT_VMCALL_PARAMETERS * DirectVmcallOptions)
{
    //
    // Call the VMCALL handler (directly)
    //
    VmxVmcallDirectVmcallHandler(&g_GuestState[CoreId], VMCALL_CHANGE_MSR_BITMAP_READ, DirectVmcallOptions);
}
