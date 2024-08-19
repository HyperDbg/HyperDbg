/**
 * @file rdmsr.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @brief help of the rdmsr command
 *
 * @return VOID
 */
VOID
CommandRdmsrHelp()
{
    ShowMessages("rdmsr : reads a model-specific register (MSR).\n\n");

    ShowMessages("syntax : \trdmsr [Msr (hex)] [core CoreNumber (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : rdmsr c0000082\n");
    ShowMessages("\t\te.g : rdmsr c0000082 core 2\n");
}

/// defines the GetLogicalProcessorInformationEx function
typedef BOOL(WINAPI * glpie_t)(
    LOGICAL_PROCESSOR_RELATIONSHIP,
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
    PDWORD);

/**
 * @brief classic way to get number of cores in windows
 *
 * @return SIZE_T
 */
static SIZE_T
GetWindowsCompatibleNumberOfCores()
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    return (SIZE_T)SysInfo.dwNumberOfProcessors;
}

/*
 * @brief Windows NUMA support is particular
 * https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex#remarks
 *
 * New api here:
 * https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support
 *
 * @return SIZE_T
 */
static SIZE_T
GetWindowsNumaNumberOfCores()
{
    uint8_t * Buffer   = NULL;
    SIZE_T    NumCores = 0;
    DWORD     Length   = 0;
    HMODULE   Kernel32 = GetModuleHandleW(L"kernel32.dll");

    glpie_t GetLogicalProcessorInformationEx = (glpie_t)GetProcAddress(Kernel32, "GetLogicalProcessorInformationEx");
    if (!GetLogicalProcessorInformationEx)
    {
        return 0;
    }

    GetLogicalProcessorInformationEx(RelationAll, NULL, &Length);
    if (Length < 1 || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return 0;
    }

    Buffer = (uint8_t *)malloc(Length);
    if (!Buffer || !GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Buffer, &Length))
    {
        free(Buffer);
        return 0;
    }

    for (DWORD Offset = 0; Offset < Length;)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(Buffer + Offset);
        Offset += Info->Size;
        if (Info->Relationship != RelationProcessorCore)
        {
            continue;
        }
        for (WORD group = 0; group < Info->Processor.GroupCount; ++group)
        {
            for (KAFFINITY Mask = Info->Processor.GroupMask[group].Mask; Mask != 0; Mask >>= 1)
            {
                NumCores += Mask & 1;
            }
        }
    }
    free(Buffer);
    return NumCores;
}

/**
 * @brief rdmsr command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandRdmsr(vector<CommandToken> CommandTokens, string Command)
{
    BOOL                           Status;
    SIZE_T                         NumCPU;
    DEBUGGER_READ_AND_WRITE_ON_MSR MsrReadRequest;
    ULONG                          ReturnedLength;
    UINT64                         Msr;
    BOOL                           IsNextCoreId   = FALSE;
    BOOL                           SetMsr         = FALSE;
    UINT32                         CoreNumer      = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;
    BOOLEAN                        IsFirstCommand = TRUE;

    if (CommandTokens.size() >= 5)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandRdmsrHelp();
        return;
    }

    for (auto Section : CommandTokens)
    {
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
            continue;
        }

        if (IsNextCoreId)
        {
            if (!ConvertTokenToUInt32(Section, &CoreNumer))
            {
                ShowMessages("please specify a correct hex value for core id\n\n");
                CommandRdmsrHelp();
                return;
            }
            IsNextCoreId = FALSE;
            continue;
        }

        if (CompareLowerCaseStrings(Section, "core"))
        {
            IsNextCoreId = TRUE;
            continue;
        }

        if (SetMsr || !ConvertTokenToUInt64(Section, &Msr))
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

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    MsrReadRequest.ActionType = DEBUGGER_MSR_READ;
    MsrReadRequest.Msr        = Msr;
    MsrReadRequest.CoreNumber = CoreNumer;

    //
    // Find logical cores count
    //
    SIZE_T NumCores = GetWindowsNumaNumberOfCores();
    NumCPU          = NumCores > 0 ? NumCores : GetWindowsCompatibleNumberOfCores();

    //
    // allocate buffer for transferring messages
    //
    UINT64 * OutputBuffer = (UINT64 *)malloc(sizeof(UINT64) * NumCPU);

    ZeroMemory(OutputBuffer, sizeof(UINT64) * NumCPU);

    Status = DeviceIoControl(
        g_DeviceHandle,                        // Handle to device
        IOCTL_DEBUGGER_READ_OR_WRITE_MSR,      // IO Control Code (IOCTL)
        &MsrReadRequest,                       // Input Buffer to driver.
        SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR, // Input buffer length
        OutputBuffer,                          // Output Buffer from driver.
        (DWORD)(sizeof(UINT64) * NumCPU),      // Length of output buffer in bytes.
        &ReturnedLength,                       // Bytes placed in buffer.
        NULL                                   // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code (%x), either msr index or core id is invalid\n",
                     GetLastError());
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
        for (SIZE_T i = 0; i < NumCPU; i++)
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
