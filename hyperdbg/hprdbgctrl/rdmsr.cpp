/**
 * @file rdmsr.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief rdmsr command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of rdmsr command
 *
 * @return VOID
 */
VOID
CommandRdmsrHelp()
{
    ShowMessages("rdmsr : Reads a model-specific register (MSR).\n\n");
    ShowMessages("syntax : \trdmsr [rcx (hex value)] core [core index (hex value "
                 "- optional)]\n");
    ShowMessages("\t\te.g : rdmsr c0000082\n");
    ShowMessages("\t\te.g : rdmsr c0000082 core 2\n");
}

/**
 * @brief rdmsr command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRdmsr(vector<string> SplittedCommand, string Command)
{
    BOOL                           Status;
    BOOL                           IsNextCoreId = FALSE;
    BOOL                           SetMsr       = FALSE;
    DEBUGGER_READ_AND_WRITE_ON_MSR MsrReadRequest;
    ULONG                          ReturnedLength;
    UINT64                         Msr;
    UINT32                         CoreNumer = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;
    SYSTEM_INFO                    SysInfo;
    DWORD                          NumCPU;

    if (SplittedCommand.size() >= 5)
    {
        ShowMessages("incorrect use of 'rdmsr'\n\n");
        CommandRdmsrHelp();
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
                CommandRdmsrHelp();
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

        if (SetMsr || !ConvertStringToUInt64(Section, &Msr))
        {
            ShowMessages("please specify a correct hex value to be read\n\n");
            CommandRdmsrHelp();
            return;
        }
        SetMsr = TRUE;
    }

    //
    // Check if msr is set or not
    //
    if (!SetMsr)
    {
        ShowMessages("please specify a correct hex value to be read\n\n");
        CommandRdmsrHelp();
        return;
    }
    if (IsNextCoreId)
    {
        ShowMessages("please specify a correct hex value for core\n\n");
        CommandRdmsrHelp();
        return;
    }

    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    MsrReadRequest.ActionType = DEBUGGER_MSR_READ;
    MsrReadRequest.Msr        = Msr;
    MsrReadRequest.CoreNumber = CoreNumer;

    //
    // Find logical cores count
    //
    GetSystemInfo(&SysInfo);
    NumCPU = SysInfo.dwNumberOfProcessors;

    //
    // allocate buffer for transfering messages
    //
    UINT64 * OutputBuffer = (UINT64 *)malloc(sizeof(UINT64) * NumCPU);

    ZeroMemory(OutputBuffer, sizeof(UINT64) * NumCPU);

    Status = DeviceIoControl(
        g_DeviceHandle,                        // Handle to device
        IOCTL_DEBUGGER_READ_OR_WRITE_MSR,      // IO Control code
        &MsrReadRequest,                       // Input Buffer to driver.
        SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR, // Input buffer length
        OutputBuffer,                          // Output Buffer from driver.
        sizeof(UINT64) * NumCPU,               // Length of output buffer in bytes.
        &ReturnedLength,                       // Bytes placed in buffer.
        NULL                                   // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(OutputBuffer);
        return;
    }

    //
    // btw, %x is enough, no need to %llx
    //
    if (CoreNumer == DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES)
    {
        //
        // Show all cores
        //
        for (size_t i = 0; i < NumCPU; i++)
        {
            ShowMessages("core : 0x%x - msr[%llx] = %s\n", i, Msr, SeparateTo64BitValue((OutputBuffer[i])).c_str());
        }
    }
    else
    {
        //
        // Show for a single-core
        //
        ShowMessages("core : 0x%x - msr[%llx] = %s\n", CoreNumer, Msr, SeparateTo64BitValue((OutputBuffer[0])).c_str());
    }

    //
    // Free the buffer
    //
    free(OutputBuffer);
}
