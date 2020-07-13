/**
 * @file Transparency.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief try to hide the debugger from anti-debugging and anti-hypervisor methods
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

NTSTATUS
TransparentHideDebugger()
{
    //
    // Disable the transparent-mode
    //
    g_TransparentMode = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
TransparentUnhideDebugger()
{
    //
    // Disable the transparent-mode
    //
    g_TransparentMode = FALSE;

    return STATUS_SUCCESS;
}
