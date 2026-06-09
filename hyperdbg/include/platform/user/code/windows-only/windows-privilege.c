/**
 * @file windows-privilege.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of Windows only APIs for adjusting privileges
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Is privileges already adjusted
 */
BOOLEAN g_PrivilegesAlreadyAdjusted = FALSE;

/**
 * @brief Adjust kernel debug privilege
 *
 * @return BOOLEAN return TRUE if it was successful or FALSE if there
 */
BOOLEAN
WindowsSetDebugPrivilege()
{
#ifdef _WIN32 // Windows
    BOOL   Status;
    HANDLE Token;

    //
    // Check if we already adjusted the privilege
    //
    if (g_PrivilegesAlreadyAdjusted)
    {
        return TRUE;
    }

    //
    // Enable Debug privilege
    //
    Status = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token);
    if (!Status)
    {
        ShowMessages("err, OpenProcessToken failed (%x)\n", GetLastError());
        return FALSE;
    }

    Status = SetPrivilege(Token, SE_DEBUG_NAME, TRUE);
    if (!Status)
    {
        CloseHandle(Token);
        return FALSE;
    }

    //
    // Indicate that the privilege is already adjusted
    //
    g_PrivilegesAlreadyAdjusted = TRUE;

    CloseHandle(Token);
    return TRUE;

#elif defined(__linux__) // Linux
    return TRUE; // No need to adjust privileges on Linux
#endif
}
