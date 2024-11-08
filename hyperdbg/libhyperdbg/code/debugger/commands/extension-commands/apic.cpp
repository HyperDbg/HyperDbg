/**
 * @file apic.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !apic command
 * @details
 * @version 0.11
 * @date 2024-11-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the !apic command
 *
 * @return VOID
 */
VOID
CommandApicHelp()
{
    ShowMessages("!apic : shows the details of Local APIC in both xAPIC and x2APIC modes.\n\n");

    ShowMessages("syntax : \t!apic\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !apic\n");
}

/**
 * @brief Send APIC requests
 *
 * @param ApicType
 * @param ApicBuffer
 * @param ExpectedRequestSize
 *
 * @return VOID
 */
BOOLEAN
CommandApicSendRequest(DEBUGGER_APIC_REQUEST_TYPE ApicType,
                       PVOID                      ApicBuffer,
                       UINT32                     ExpectedRequestSize)
{
    BOOL                   Status;
    ULONG                  ReturnedLength;
    PDEBUGGER_APIC_REQUEST ApicRequest;
    UINT32                 RequestSize = 0;

    RequestSize = SIZEOF_DEBUGGER_APIC_REQUEST + ExpectedRequestSize;

    //
    // Allocate buffer to fill the request
    //
    ApicRequest = (PDEBUGGER_APIC_REQUEST)malloc(RequestSize);

    if (ApicRequest == NULL)
    {
        //
        // Unable to allocate buffer
        //
        return FALSE;
    }

    RtlZeroMemory(ApicRequest, RequestSize);

    //
    // Set the APIC type to local apic
    // Note that the APIC mode (xAPIC and x2APIC) is determined in the debugger
    //
    ApicRequest->ApicType = ApicType;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendApicActionPacketsToDebuggee(ApicRequest, RequestSize))
        {
            return FALSE;
        }
        else
        {
            RtlCopyMemory(ApicBuffer, (PVOID)(((CHAR *)ApicRequest) + sizeof(DEBUGGER_APIC_REQUEST)), ExpectedRequestSize);
            return TRUE;
        }
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                // Handle to device
            IOCTL_PERFROM_ACTIONS_ON_APIC, // IO Control Code (IOCTL)
            ApicRequest,                   // Input Buffer to driver.
            SIZEOF_DEBUGGER_APIC_REQUEST,  // Input buffer length
            ApicRequest,                   // Output Buffer from driver.
            RequestSize,                   // Length of output
                                           // buffer in bytes.
            &ReturnedLength,               // Bytes placed in buffer.
            NULL                           // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return FALSE;
        }

        if (ApicRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            //
            // Fill the request buffer
            //
            RtlCopyMemory(ApicBuffer, (PVOID)(((CHAR *)ApicRequest) + sizeof(DEBUGGER_APIC_REQUEST)), ExpectedRequestSize);
            return TRUE;
        }
        else
        {
            //
            // An err occurred, no results
            //
            ShowErrorMessage(ApicRequest->KernelStatus);
            return FALSE;
        }
    }
}

/**
 * @brief Request to get Local APIC
 * @param LocalApic
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgGetLocalApic(PLAPIC_PAGE LocalApic)
{
    return CommandApicSendRequest(DEBUGGER_APIC_REQUEST_TYPE_READ_LOCAL_APIC, LocalApic, sizeof(LAPIC_PAGE));
}

/**
 * @brief !apic command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandApic(vector<CommandToken> CommandTokens, string Command)
{
    UINT8      i = 0, j = 0;
    UINT32     k         = 0;
    UINT32     Reg       = 0;
    LAPIC_PAGE LocalApic = {0};

    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandApicHelp();
        return;
    }

    //
    // Get the local APIC results
    //
    if (HyperDbgGetLocalApic(&LocalApic) == FALSE)
    {
        return;
    }

    //
    // Show different fields of the Local APIC
    //
    ShowMessages("***  PHYSICAL LAPIC ID = %u, Ver = 0x%x, MaxLvtEntry = %d, DirectedEOI = P%d/E%d, (SW: '%s')\n"
                 "     -> TPR = 0x%08x,  PPR = 0x%08x\n"
                 "     -> LDR = 0x%08x,  SVR = 0x%08x,  Err = 0x%08x\n"
                 "     -> LVT_INT0 = 0x%08x,  LVT_INT1 = 0x%08x\n"
                 "     -> LVT_CMCI = 0x%08x,  LVT_PMCR = 0x%08x\n"
                 "     -> LVT_TMR  = 0x%08x,  LVT_TSR  = 0x%08x\n"
                 "     -> LVT_ERR  = 0x%08x\n"
                 "     -> InitialCount = 0x%08x, CurrentCount = 0x%08x, DivideConfig = 0x%08x\n",
                 LocalApic.Id >> 24,
                 LocalApic.Version,
                 (LocalApic.Version & 0xFF0000) >> 16,
                 (LocalApic.Version >> 24) & 1,
                 (LocalApic.SpuriousInterruptVector >> 12) & 1,
                 (LocalApic.SpuriousInterruptVector & LAPIC_SVR_FLAG_SW_ENABLE) != 0 ? "Enabled" : "Disabled",
                 LocalApic.TPR,
                 LocalApic.ProcessorPriority,
                 LocalApic.LogicalDestination,
                 LocalApic.SpuriousInterruptVector,
                 LocalApic.ErrorStatus,
                 LocalApic.LvtLINT0,
                 LocalApic.LvtLINT1,
                 LocalApic.LvtCmci,
                 LocalApic.LvtPerfMonCounters,
                 LocalApic.LvtTimer,
                 LocalApic.LvtThermalSensor,
                 LocalApic.LvtError,
                 LocalApic.InitialCount,
                 LocalApic.CurrentCount,
                 LocalApic.DivideConfiguration);

    //
    // Print the ISR, TMR and IRR
    //
    ShowMessages("ISR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApic.ISR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (0 != (Reg & k))
            {
                ShowMessages("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }
    ShowMessages("\n");

    ShowMessages("TMR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApic.TMR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                ShowMessages("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    ShowMessages("\n");

    ShowMessages("IRR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApic.IRR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                ShowMessages("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    ShowMessages("\n");
}
