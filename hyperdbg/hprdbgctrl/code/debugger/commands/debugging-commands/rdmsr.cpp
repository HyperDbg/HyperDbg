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
typedef BOOL (WINAPI *glpie_t)(
    LOGICAL_PROCESSOR_RELATIONSHIP,
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
    PDWORD);

/**
 * @brief classic way to get number of cores in windows
 * 
 * @return size_t
 */
static size_t
get_windows_compatible_n_cores()
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    return SysInfo.dwNumberOfProcessors;
}

/*
 * Windows NUMA support is particular
 * https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex#remarks
 *
 * New api here:
 * https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support
 */
static size_t
get_windows_numa_n_cores()
{
    uint8_t * buffer   = NULL;
    size_t    n_cores  = 0;
    DWORD     length   = 0;
    HMODULE   kernel32 = GetModuleHandleW(L"kernel32.dll");

    glpie_t GetLogicalProcessorInformationEx = (glpie_t)GetProcAddress(kernel32, "GetLogicalProcessorInformationEx");
    if (!GetLogicalProcessorInformationEx)
    {
        return 0;
    }

    GetLogicalProcessorInformationEx(RelationAll, NULL, &length);
    if (length < 1 || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return 0;
    }

    buffer = (uint8_t *)malloc(length);
    if (!buffer || !GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &length))
    {
        free(buffer);
        return 0;
    }

    for (DWORD offset = 0; offset < length;)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buffer + offset);
        offset += info->Size;
        if (info->Relationship != RelationProcessorCore)
        {
            continue;
        }
        for (WORD group = 0; group < info->Processor.GroupCount; ++group)
        {
            for (KAFFINITY mask = info->Processor.GroupMask[group].Mask; mask != 0; mask >>= 1)
            {
                n_cores += mask & 1;
            }
        }
    }
    free(buffer);
    return n_cores;
}

/**
 * @brief rdmsr command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRdmsr(vector<string> SplitCommand, string Command)
{
    BOOL                           Status;
    DWORD                          NumCPU;
    DEBUGGER_READ_AND_WRITE_ON_MSR MsrReadRequest;
    ULONG                          ReturnedLength;
    UINT64                         Msr;
    BOOL                           IsNextCoreId   = FALSE;
    BOOL                           SetMsr         = FALSE;
    UINT32                         CoreNumer      = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;
    BOOLEAN                        IsFirstCommand = TRUE;

    if (SplitCommand.size() >= 5)
    {
        ShowMessages("incorrect use of the 'rdmsr'\n\n");
        CommandRdmsrHelp();
        return;
    }

    for (auto Section : SplitCommand)
    {
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
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

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    MsrReadRequest.ActionType = DEBUGGER_MSR_READ;
    MsrReadRequest.Msr        = Msr;
    MsrReadRequest.CoreNumber = CoreNumer;

    //
    // Find logical cores count
    //
    size_t n_cores = get_windows_numa_n_cores();
    NumCPU = n_cores > 0 ? n_cores : get_windows_compatible_n_cores();

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
        sizeof(UINT64) * NumCPU,               // Length of output buffer in bytes.
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
