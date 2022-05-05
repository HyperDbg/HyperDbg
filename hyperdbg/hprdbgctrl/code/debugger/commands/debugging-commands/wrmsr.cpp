/**
 * @file wrmsr.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief wrmsr command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of wrmsr command
 *
 * @return VOID
 */
VOID
CommandWrmsrHelp()
{
    ShowMessages("wrmsr : writes on a model-specific register (MSR).\n\n");

    ShowMessages("syntax : \twrmsr [Msr (hex)] [Value (hex)] [core CoreNumber (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : wrmsr c0000082 fffff8077356f010\n");
    ShowMessages("\t\te.g : wrmsr c0000082 @rcx\n");
    ShowMessages("\t\te.g : wrmsr c0000082 @rcx+@rdx+12\n");
    ShowMessages("\t\te.g : wrmsr c0000082 fffff8077356f010 core 2\n");
}

/**
 * @brief wrmsr command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandWrmsr(vector<string> SplittedCommand, string Command)
{
    BOOL                           Status;
    BOOL                           IsNextCoreId = FALSE;
    BOOL                           SetMsr       = FALSE;
    BOOL                           SetValue     = FALSE;
    DEBUGGER_READ_AND_WRITE_ON_MSR MsrWriteRequest;
    UINT64                         Msr;
    UINT64                         Value     = 0;
    UINT32                         CoreNumer = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;

    if (SplittedCommand.size() >= 6)
    {
        ShowMessages("incorrect use of 'wrmsr'\n\n");
        CommandWrmsrHelp();
        return;
    }

    for (auto Section : SplittedCommand)
    {
        if (!Section.compare(SplittedCommand.at(0)))
        {
            continue;
        }

        if (IsNextCoreId)
        {
            if (!ConvertStringToUInt32(Section, &CoreNumer))
            {
                ShowMessages("please specify a correct hex value for core id\n\n");
                CommandWrmsrHelp();
                return;
            }
            IsNextCoreId = FALSE;
            continue;
        }

        if (!Section.compare("core"))
        {
            IsNextCoreId = TRUE;
            continue;
        }

        if (!SetMsr)
        {
            if (!ConvertStringToUInt64(Section, &Msr))
            {
                ShowMessages("please specify a correct hex value to be read\n\n");
                CommandWrmsrHelp();
                return;
            }
            else
            {
                //
                // Means that the MSR is set, next we should read value
                //
                SetMsr = TRUE;
                continue;
            }
        }

        if (SetMsr)
        {
            if (!SymbolConvertNameOrExprToAddress(Section, &Value))
            {
                ShowMessages(
                    "please specify a correct hex value or an expression to put on the msr\n\n");
                CommandWrmsrHelp();
                return;
            }
            else
            {
                SetValue = TRUE;
                continue;
            }
        }
    }

    //
    // Check if msr is set or not
    //
    if (!SetMsr)
    {
        ShowMessages("please specify a correct hex value to write\n\n");
        CommandWrmsrHelp();
        return;
    }
    if (!SetValue)
    {
        ShowMessages("please specify a correct hex value to put on msr\n\n");
        CommandWrmsrHelp();
        return;
    }
    if (IsNextCoreId)
    {
        ShowMessages("please specify a correct hex value for core\n\n");
        CommandWrmsrHelp();
        return;
    }

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    MsrWriteRequest.ActionType = DEBUGGER_MSR_WRITE;
    MsrWriteRequest.Msr        = Msr;
    MsrWriteRequest.CoreNumber = CoreNumer;
    MsrWriteRequest.Value      = Value;

    Status = DeviceIoControl(
        g_DeviceHandle,                        // Handle to device
        IOCTL_DEBUGGER_READ_OR_WRITE_MSR,      // IO Control code
        &MsrWriteRequest,                      // Input Buffer to driver.
        SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR, // Input buffer length
        NULL,                                  // Output Buffer from driver.
        NULL,                                  // Length of output buffer in bytes.
        NULL,                                  // Bytes placed in buffer.
        NULL                                   // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code (%x), either msr index or core id is invalid\n",
                     GetLastError());
        return;
    }

    ShowMessages("\n");
}
