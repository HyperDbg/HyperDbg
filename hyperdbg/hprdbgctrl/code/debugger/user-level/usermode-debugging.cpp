/**
 * @file usermode-debugging.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief control the user-mode debugging affairs
 * @details
 * @version 0.1
 * @date 2021-12-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief Attach to a target process
 * @param FileName
 * @param ProcessInformation
 * 
 * @return BOOLEAN
 */
BOOLEAN
UsermodeDebuggingAttach(WCHAR * FileName, PPROCESS_INFORMATION ProcessInformation)
{
    STARTUPINFOW StartupInfo;
    BOOL         CreateProcessResult;


    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);

    //
    // Create process suspended
    //
    CreateProcessResult = CreateProcessW(FileName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         CREATE_SUSPENDED,
                                         NULL,
                                         NULL,
                                         &StartupInfo,
                                         ProcessInformation);

    if (!CreateProcessResult)
    {
        ShowMessages("err, start process failed (%x)", GetLastError());
        return FALSE;
    }

    //
    // Get loaded exe's PEB (base address of loaded module)
    //

    return TRUE;
}
