/**
 * @file listening.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief Listening for remote connections
 * @details
 * @version 0.1
 * @date 2020-12-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern DEBUGGER_SYNCRONIZATION_EVENTS_STATE
                                            g_SyncronizationObjectsHandleTable[DEBUGGER_MAXIMUM_SYNCRONIZATION_OBJECTS];
extern BYTE                                 g_CurrentRunningInstruction[MAXIMUM_INSTR_SIZE];
extern OVERLAPPED                           g_OverlappedIoStructureForReadDebugger;
extern OVERLAPPED                           g_OverlappedIoStructureForWriteDebugger;
extern HANDLE                               g_SerialRemoteComPortHandle;
extern BOOLEAN                              g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                              g_IsDebuggeeRunning;
extern BOOLEAN                              g_IgnoreNewLoggingMessages;
extern BOOLEAN                              g_SharedEventStatus;
extern BOOLEAN                              g_IsRunningInstruction32Bit;
extern ULONG                                g_CurrentRemoteCore;
extern DEBUGGER_EVENT_AND_ACTION_REG_BUFFER g_DebuggeeResultOfRegisteringEvent;
extern DEBUGGER_EVENT_AND_ACTION_REG_BUFFER
    g_DebuggeeResultOfAddingActionsToEvent;

/**
 * @brief Check if the remote debuggee needs to pause the system
 * and also process the debuggee's messages
 *
 * @return BOOLEAN
 */
BOOLEAN
ListeningSerialPortInDebugger()
{
    PDEBUGGER_REMOTE_PACKET               TheActualPacket;
    PDEBUGGEE_PAUSED_PACKET               PausePacket;
    PDEBUGGEE_MESSAGE_PACKET              MessagePacket;
    PDEBUGGEE_CHANGE_CORE_PACKET          ChangeCorePacket;
    PDEBUGGEE_SCRIPT_PACKET               ScriptPacket;
    PDEBUGGEE_FORMATS_PACKET              FormatsPacket;
    PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER EventAndActionPacket;
    PDEBUGGER_MODIFY_EVENTS               EventModifyAndQueryPacket;
    PDEBUGGEE_CHANGE_PROCESS_PACKET       ChangeProcessPacket;
    PDEBUGGER_FLUSH_LOGGING_BUFFERS       FlushPacket;
    PDEBUGGEE_REGISTER_READ_DESCRIPTION   ReadRegisterPacket;
    PDEBUGGER_READ_MEMORY                 ReadMemoryPacket;
    PDEBUGGER_EDIT_MEMORY                 EditMemoryPacket;
    PDEBUGGEE_BP_PACKET                   BpPacket;
    PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET    ListOrModifyBreakpointPacket;
    PGUEST_REGS                           Regs;
    PGUEST_EXTRA_REGISTERS                ExtraRegs;
    unsigned char *                       MemoryBuffer;
StartAgain:

    CHAR   BufferToReceive[MaxSerialPacketSize] = {0};
    UINT32 LengthReceived                       = 0;

    //
    // Wait for handshake to complete or in other words
    // get the receive packet
    //
    if (!KdReceivePacketFromDebuggee(BufferToReceive, &LengthReceived))
    {
        if (LengthReceived == 0 && BufferToReceive[0] == NULL)
        {
            //
            // The remote computer (debuggee) closed the connection
            //
            ShowMessages("the remote connection is closed\n");

            if (g_IsDebuggeeRunning == FALSE && g_IsSerialConnectedToRemoteDebuggee)
            {
                ShowMessages("\n");
                HyperdbgShowSignature();
            }

            KdCloseConnection();
            return FALSE;
        }
        else
        {
            ShowMessages("err, invalid buffer received\n");
            goto StartAgain;
        }
    }

    //
    // Check for invalid close packets
    //
    if (LengthReceived == 1 && BufferToReceive[0] == NULL)
    {
        goto StartAgain;
    }

    TheActualPacket = (PDEBUGGER_REMOTE_PACKET)BufferToReceive;

    if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER)
    {
        //
        // Check checksum
        //
        if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
                                  LengthReceived - sizeof(BYTE)) !=
            TheActualPacket->Checksum)
        {
            ShowMessages("err checksum is invalid\n");
            goto StartAgain;
        }

        //
        // Check if the packet type is correct
        //
        if (TheActualPacket->TypeOfThePacket !=
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER)
        {
            //
            // sth wrong happened, the packet is not belonging to use
            // nothing to do, just wait again
            //
            ShowMessages("err, unknown packet received from the debuggee\n");
            goto StartAgain;
        }

        //
        // It's a HyperDbg packet
        //
        switch (TheActualPacket->RequestedActionOfThePacket)
        {
        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED:

            ShowMessages("Connected to debuggee %s !\n",
                         ((CHAR *)TheActualPacket) + sizeof(DEBUGGER_REMOTE_PACKET));
            ShowMessages("Press CTRL+C to pause the debuggee\n");

            //
            // Signal the event that the debugger started
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_STARTED_PACKET_RECEIVED]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_STARTED_PACKET_RECEIVED]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_LOGGING_MECHANISM:

            MessagePacket =
                (DEBUGGEE_MESSAGE_PACKET *)(((CHAR *)TheActualPacket) +
                                            sizeof(DEBUGGER_REMOTE_PACKET));

            //
            // We check g_IgnoreNewLoggingMessages here because we want to
            // avoid messages when the debuggee is halted
            //
            if (!g_IgnoreNewLoggingMessages)
            {
                ShowMessages("%s", MessagePacket->Message);
            }

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION:

            //
            // Pause logging mechanism
            //
            g_IgnoreNewLoggingMessages = TRUE;

            PausePacket = (DEBUGGEE_PAUSED_PACKET *)(((CHAR *)TheActualPacket) +
                                                     sizeof(DEBUGGER_REMOTE_PACKET));

            //
            // Debuggee is not running
            //
            g_IsDebuggeeRunning = FALSE;

            //
            // Set the current core
            //
            g_CurrentRemoteCore = PausePacket->CurrentCore;

            //
            // Save the current operating instruction and operating mode
            //
            RtlZeroMemory(g_CurrentRunningInstruction, MAXIMUM_INSTR_SIZE);
            memcpy(g_CurrentRunningInstruction, &PausePacket->InstructionBytesOnRip, MAXIMUM_INSTR_SIZE);

            g_IsRunningInstruction32Bit = PausePacket->Is32BitAddress;

            //
            // Check whether the pausing was because of triggering an event
            // or not
            //
            if (PausePacket->EventTag != NULL)
            {
                if (PausePacket->PausingReason ==
                    DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT)
                {
                    //
                    // It's a breakpoint id
                    //
                    ShowMessages("breakpoint 0x%x hit\n", PausePacket->EventTag);
                }
                else
                {
                    //
                    // It's an event tag
                    //
                    ShowMessages("event 0x%x triggered\n",
                                 PausePacket->EventTag - DebuggerEventTagStartSeed);
                }
            }

            if (PausePacket->PausingReason !=
                DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM)
            {
                if (!PausePacket->Is32BitAddress)
                {
                    HyperDbgDisassembler64(PausePacket->InstructionBytesOnRip,
                                           PausePacket->Rip,
                                           MAXIMUM_INSTR_SIZE,
                                           1,
                                           TRUE,
                                           &PausePacket->Rflags);
                }
                else
                {
                    HyperDbgDisassembler32(PausePacket->InstructionBytesOnRip,
                                           PausePacket->Rip,
                                           MAXIMUM_INSTR_SIZE,
                                           1,
                                           TRUE,
                                           &PausePacket->Rflags);
                }
            }

            switch (PausePacket->PausingReason)
            {
            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT:
            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT:
            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED:
            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED:
            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED:

                //
                // Unpause the debugger to get commands
                //
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_IS_DEBUGGER_RUNNING]
                        .IsOnWaitingState = FALSE;
                SetEvent(g_SyncronizationObjectsHandleTable
                             [DEBUGGER_SYNCRONIZATION_OBJECT_IS_DEBUGGER_RUNNING]
                                 .EventHandle);

                break;

            case DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM:

                //
                // Nothing
                //
                break;

            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_CORE_SWITCHED:

                //
                // Signal the event relating to receiving result of core change
                //
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_CORE_SWITCHING_RESULT]
                        .IsOnWaitingState = FALSE;
                SetEvent(g_SyncronizationObjectsHandleTable
                             [DEBUGGER_SYNCRONIZATION_OBJECT_CORE_SWITCHING_RESULT]
                                 .EventHandle);

                break;

            case DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED:

                //
                // Signal the event relating to result of command execution finished
                //
                ShowMessages("\n");
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_DEBUGGEE_FINISHED_COMMAND_EXECUTION]
                        .IsOnWaitingState = FALSE;
                SetEvent(
                    g_SyncronizationObjectsHandleTable
                        [DEBUGGER_SYNCRONIZATION_OBJECT_DEBUGGEE_FINISHED_COMMAND_EXECUTION]
                            .EventHandle);

                break;

            default:

                //
                // Signal the event relating to commands that are waiting for
                // the details of a halted debuggeee
                //
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_PAUSED_DEBUGGEE_DETAILS]
                        .IsOnWaitingState = FALSE;
                SetEvent(g_SyncronizationObjectsHandleTable
                             [DEBUGGER_SYNCRONIZATION_OBJECT_PAUSED_DEBUGGEE_DETAILS]
                                 .EventHandle);

                break;
            }

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_CORE:

            ChangeCorePacket =
                (DEBUGGEE_CHANGE_CORE_PACKET *)(((CHAR *)TheActualPacket) +
                                                sizeof(DEBUGGER_REMOTE_PACKET));

            if (ChangeCorePacket->Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                ShowMessages("current operating core changed to 0x%x\n",
                             ChangeCorePacket->NewCore);
            }
            else
            {
                ShowErrorMessage(ChangeCorePacket->Result);

                //
                // Signal the event relating to receiving result of core change
                //
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_CORE_SWITCHING_RESULT]
                        .IsOnWaitingState = FALSE;
                SetEvent(g_SyncronizationObjectsHandleTable
                             [DEBUGGER_SYNCRONIZATION_OBJECT_CORE_SWITCHING_RESULT]
                                 .EventHandle);
            }

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_PROCESS:

            ChangeProcessPacket =
                (DEBUGGEE_CHANGE_PROCESS_PACKET *)(((CHAR *)TheActualPacket) +
                                                   sizeof(DEBUGGER_REMOTE_PACKET));

            if (ChangeProcessPacket->Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                if (ChangeProcessPacket->GetRemotePid)
                {
                    ShowMessages("current process id : %x\n",
                                 ChangeProcessPacket->ProcessId);
                }
                else
                {
                    ShowMessages(
                        "press 'g' to continue the debuggee, if the pid is valid then "
                        "the debuggee will be automatically paused when it attached to "
                        "the target process\n");
                }
            }
            else
            {
                ShowErrorMessage(ChangeProcessPacket->Result);
            }

            //
            // Signal the event relating to receiving result of process change
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_PROCESS_SWITCHING_RESULT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_PROCESS_SWITCHING_RESULT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FLUSH:

            FlushPacket =
                (DEBUGGER_FLUSH_LOGGING_BUFFERS *)(((CHAR *)TheActualPacket) +
                                                   sizeof(DEBUGGER_REMOTE_PACKET));

            if (FlushPacket->KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // The amount of message that are deleted are the amount of
                // vmx-root messages and vmx non-root messages
                //
                ShowMessages("flushing buffers was successful, total %d messages were "
                             "cleared.\n",
                             FlushPacket->CountOfMessagesThatSetAsReadFromVmxNonRoot +
                                 FlushPacket->CountOfMessagesThatSetAsReadFromVmxRoot);
            }
            else
            {
                ShowErrorMessage(FlushPacket->KernelStatus);
            }

            //
            // Signal the event relating to receiving result of flushing
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_FLUSH_RESULT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_FLUSH_RESULT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_RUNNING_SCRIPT:

            ScriptPacket = (DEBUGGEE_SCRIPT_PACKET *)(((CHAR *)TheActualPacket) +
                                                      sizeof(DEBUGGER_REMOTE_PACKET));

            if (ScriptPacket->Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Nothing to do
                //
            }
            else
            {
                ShowErrorMessage(ScriptPacket->Result);
            }

            if (ScriptPacket->IsFormat)
            {
                //
                // Signal the event relating to receiving result of .formats command
                //
                g_SyncronizationObjectsHandleTable
                    [DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_FORMATS_RESULT]
                        .IsOnWaitingState = FALSE;
                SetEvent(g_SyncronizationObjectsHandleTable
                             [DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_FORMATS_RESULT]
                                 .EventHandle);
            }

            //
            // Signal the event relating to receiving result of running script
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_RUNNING_RESULT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_SCRIPT_RUNNING_RESULT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FORMATS:

            FormatsPacket =
                (DEBUGGEE_FORMATS_PACKET *)(((CHAR *)TheActualPacket) +
                                            sizeof(DEBUGGER_REMOTE_PACKET));

            if (FormatsPacket->Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Show .formats
                //
                CommandFormatsShowResults(FormatsPacket->Value);
            }
            else
            {
                ShowErrorMessage(FormatsPacket->Result);
            }

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT:

            EventAndActionPacket =
                (DEBUGGER_EVENT_AND_ACTION_REG_BUFFER *)(((CHAR *)TheActualPacket) +
                                                         sizeof(
                                                             DEBUGGER_REMOTE_PACKET));

            //
            // Move the buffer to the global variable
            //
            memcpy(&g_DebuggeeResultOfRegisteringEvent, EventAndActionPacket, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

            //
            // Signal the event relating to receiving result of register event
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_REGISTER_EVENT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_REGISTER_EVENT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT:

            EventAndActionPacket =
                (DEBUGGER_EVENT_AND_ACTION_REG_BUFFER *)(((CHAR *)TheActualPacket) +
                                                         sizeof(
                                                             DEBUGGER_REMOTE_PACKET));

            //
            // Move the buffer to the global variable
            //
            memcpy(&g_DebuggeeResultOfAddingActionsToEvent, EventAndActionPacket, sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

            //
            // Signal the event relating to receiving result of adding action to event
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_REGISTER_EVENT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_REGISTER_EVENT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT:

            EventModifyAndQueryPacket =
                (DEBUGGER_MODIFY_EVENTS *)(((CHAR *)TheActualPacket) +
                                           sizeof(DEBUGGER_REMOTE_PACKET));

            //
            // Set the result of query
            //
            if (EventModifyAndQueryPacket->KernelStatus !=
                DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // There was an error
                //
                ShowErrorMessage(EventModifyAndQueryPacket->KernelStatus);
            }
            else if (EventModifyAndQueryPacket->TypeOfAction ==
                     DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
            {
                //
                // Set the global state
                //
                g_SharedEventStatus = EventModifyAndQueryPacket->IsEnabled;
            }
            else
            {
                CommandEventsHandleModifiedEvent(EventModifyAndQueryPacket->Tag,
                                                 EventModifyAndQueryPacket);
            }

            //
            // Signal the event relating to receiving result of event query and
            // modification
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_MODIFY_AND_QUERY_EVENT]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_MODIFY_AND_QUERY_EVENT]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_REGISTERS:

            ReadRegisterPacket =
                (DEBUGGEE_REGISTER_READ_DESCRIPTION *)(((CHAR *)TheActualPacket) +
                                                       sizeof(
                                                           DEBUGGER_REMOTE_PACKET));

            if (ReadRegisterPacket->KernelStatus ==
                DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Show the result of reading registers like rax=0000000000018b01
                //
                if (ReadRegisterPacket->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
                {
                    Regs      = (GUEST_REGS *)(((CHAR *)ReadRegisterPacket) +
                                          sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION));
                    ExtraRegs = (GUEST_EXTRA_REGISTERS *)(((CHAR *)ReadRegisterPacket) +
                                                          sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) +
                                                          sizeof(GUEST_REGS));

                    RFLAGS Rflags = {0};
                    Rflags.Value  = ExtraRegs->RFLAGS;

                    ShowMessages(
                        "RAX=%016llx RBX=%016llx RCX=%016llx\n"
                        "RDX=%016llx RSI=% 016llx RDI=%016llx\n"
                        "RIP=%016llx RSP=%016llx RBP=%016llx\n"
                        "R8=%016llx  R9=%016llx  R10=%016llx\n"
                        "R11=%016llx R12=%016llx R13=%016llx\n"
                        "R14=%016llx R15=%016llx IOPL=%02x\n"
                        "%s  %s  %s  %s\n%s  %s  %s  %s  \n"
                        "CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n"
                        "RFLAGS=%016llx\n",
                        Regs->rax,
                        Regs->rbx,
                        Regs->rcx,
                        Regs->rdx,
                        Regs->rsi,
                        Regs->rdi,
                        ExtraRegs->RIP,
                        Regs->rsp,
                        Regs->rbp,
                        Regs->r8,
                        Regs->r9,
                        Regs->r10,
                        Regs->r11,
                        Regs->r12,
                        Regs->r13,
                        Regs->r14,
                        Regs->r15,
                        Rflags.IoPrivilegeLevel,
                        Rflags.OverflowFlag ? "OF 1" : "OF 0",
                        Rflags.DirectionFlag ? "DF 1" : "DF 0",
                        Rflags.InterruptEnableFlag ? "IF 1" : "IF 0",
                        Rflags.SignFlag ? "SF  1" : "SF  0",
                        Rflags.ZeroFlag ? "ZF 1" : "ZF 0",
                        Rflags.ParityFlag ? "PF 1" : "PF 0",
                        Rflags.CarryFlag ? "CF 1" : "CF 0",
                        Rflags.AuxiliaryCarryFlag ? "AXF 1" : "AXF 0",
                        ExtraRegs->CS,
                        ExtraRegs->SS,
                        ExtraRegs->DS,
                        ExtraRegs->ES,
                        ExtraRegs->FS,
                        ExtraRegs->GS,
                        ExtraRegs->RFLAGS);
                }
                else
                {
                    ShowMessages("%s=%016llx\n",
                                 RegistersNames[ReadRegisterPacket->RegisterID],
                                 ReadRegisterPacket->Value);
                }
            }
            else
            {
                ShowErrorMessage(ReadRegisterPacket->KernelStatus);
            }

            //
            // Signal the event relating to receiving result of reading registers
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_READ_REGISTERS]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_READ_REGISTERS]
                             .EventHandle);

            break;
        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_MEMORY:

            ReadMemoryPacket =
                (DEBUGGER_READ_MEMORY *)(((CHAR *)TheActualPacket) +
                                         sizeof(DEBUGGER_REMOTE_PACKET));

            if (ReadMemoryPacket->KernelStatus ==
                DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Show the result of reading memory like mem=0000000000018b01
                //
                MemoryBuffer = (unsigned char *)(((CHAR *)TheActualPacket) +
                                                 sizeof(DEBUGGER_REMOTE_PACKET) +
                                                 sizeof(DEBUGGER_READ_MEMORY));

                if (ReadMemoryPacket->Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE64)
                {
                    HyperDbgDisassembler64(MemoryBuffer, ReadMemoryPacket->Address, ReadMemoryPacket->ReturnLength, 0, FALSE, NULL);
                }
                else if (ReadMemoryPacket->Style ==
                         DEBUGGER_SHOW_COMMAND_DISASSEMBLE32)
                {
                    HyperDbgDisassembler32(MemoryBuffer, ReadMemoryPacket->Address, ReadMemoryPacket->ReturnLength, 0, FALSE, NULL);
                }
                else if (ReadMemoryPacket->Style == DEBUGGER_SHOW_COMMAND_DB)
                {
                    ShowMemoryCommandDB(
                        MemoryBuffer,
                        ReadMemoryPacket->Size,
                        ReadMemoryPacket->Address,
                        ReadMemoryPacket->MemoryType,
                        ReadMemoryPacket->ReturnLength);
                }
                else if (ReadMemoryPacket->Style == DEBUGGER_SHOW_COMMAND_DC)
                {
                    ShowMemoryCommandDC(
                        MemoryBuffer,
                        ReadMemoryPacket->Size,
                        ReadMemoryPacket->Address,
                        ReadMemoryPacket->MemoryType,
                        ReadMemoryPacket->ReturnLength);
                }
                else if (ReadMemoryPacket->Style == DEBUGGER_SHOW_COMMAND_DD)
                {
                    ShowMemoryCommandDD(
                        MemoryBuffer,
                        ReadMemoryPacket->Size,
                        ReadMemoryPacket->Address,
                        ReadMemoryPacket->MemoryType,
                        ReadMemoryPacket->ReturnLength);
                }
                else if (ReadMemoryPacket->Style == DEBUGGER_SHOW_COMMAND_DQ)
                {
                    ShowMemoryCommandDQ(
                        MemoryBuffer,
                        ReadMemoryPacket->Size,
                        ReadMemoryPacket->Address,
                        ReadMemoryPacket->MemoryType,
                        ReadMemoryPacket->ReturnLength);
                }
            }
            else
            {
                ShowErrorMessage(ReadMemoryPacket->KernelStatus);
            }

            //
            // Signal the event relating to receiving result of reading registers
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_READ_MEMORY]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_READ_MEMORY]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_EDITING_MEMORY:

            EditMemoryPacket =
                (DEBUGGER_EDIT_MEMORY *)(((CHAR *)TheActualPacket) +
                                         sizeof(DEBUGGER_REMOTE_PACKET));

            if (EditMemoryPacket->KernelStatus ==
                DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Show the result of reading memory like mem=0000000000018b01
                //
                
            }
            else
            {
                ShowErrorMessage(EditMemoryPacket->KernelStatus);
            }

            //
            // Signal the event relating to receiving result of reading registers
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_EDIT_MEMORY]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_EDIT_MEMORY]
                             .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_BP:

            BpPacket = (DEBUGGEE_BP_PACKET *)(((CHAR *)TheActualPacket) +
                                              sizeof(DEBUGGER_REMOTE_PACKET));

            if (BpPacket->Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Everything was okay, nothing to do
                //
            }
            else
            {
                ShowErrorMessage(BpPacket->Result);
            }

            //
            // Signal the event relating to receiving result of putting breakpoints
            //
            g_SyncronizationObjectsHandleTable[DEBUGGER_SYNCRONIZATION_OBJECT_BP]
                .IsOnWaitingState = FALSE;
            SetEvent(
                g_SyncronizationObjectsHandleTable[DEBUGGER_SYNCRONIZATION_OBJECT_BP]
                    .EventHandle);

            break;

        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_LIST_OR_MODIFY_BREAKPOINTS:

            ListOrModifyBreakpointPacket =
                (DEBUGGEE_BP_LIST_OR_MODIFY_PACKET *)(((CHAR *)TheActualPacket) +
                                                      sizeof(DEBUGGER_REMOTE_PACKET));

            if (ListOrModifyBreakpointPacket->Result ==
                DEBUGEER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // Everything was okay, nothing to do
                //
            }
            else
            {
                ShowErrorMessage(ListOrModifyBreakpointPacket->Result);
            }

            //
            // Signal the event relating to receiving result of modifying or listing
            // breakpoints
            //
            g_SyncronizationObjectsHandleTable
                [DEBUGGER_SYNCRONIZATION_OBJECT_LIST_OR_MODIFY_BREAKPOINTS]
                    .IsOnWaitingState = FALSE;
            SetEvent(g_SyncronizationObjectsHandleTable
                         [DEBUGGER_SYNCRONIZATION_OBJECT_LIST_OR_MODIFY_BREAKPOINTS]
                             .EventHandle);

            break;

        default:
            ShowMessages("err, unknown packet action received from the debugger\n");
            break;
        }
    }
    else
    {
        //
        // It's not a HyperDbg packet, it's probably a GDB packet
        //
        ShowMessages("invalid packet received\n");
        DebugBreak();
    }

    //
    // Wait for debug pause command again
    //
    goto StartAgain;

    return TRUE;
}

/**
 * @brief Check if the remote debugger needs to pause the system
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
BOOLEAN
ListeningSerialPortInDebuggee()
{
StartAgain:

    BOOL Status; /* Status */
    char SerialBuffer[MaxSerialPacketSize] = {
        0};                                     /* Buffer to send and receive data */
    DWORD                   EventMask   = 0;    /* Event mask to trigger */
    char                    ReadData    = NULL; /* temperory Character */
    DWORD                   NoBytesRead = 0;    /* Bytes read by ReadFile() */
    UINT32                  Loop        = 0;
    PDEBUGGER_REMOTE_PACKET TheActualPacket =
        (PDEBUGGER_REMOTE_PACKET)SerialBuffer;

    //
    // Setting Receive Mask
    //
    Status = SetCommMask(g_SerialRemoteComPortHandle, EV_RXCHAR);
    if (Status == FALSE)
    {
        ShowMessages("err, in setting CommMask\n");
        return FALSE;
    }

    //
    // Setting WaitComm() Event
    //
    Status = WaitCommEvent(g_SerialRemoteComPortHandle, &EventMask, NULL); /* Wait for the character to be received */

    if (Status == FALSE)
    {
        ShowMessages("err,in setting WaitCommEvent()\n");
        return FALSE;
    }

    //
    // Read data and store in a buffer
    //
    do
    {
        Status = ReadFile(g_SerialRemoteComPortHandle, &ReadData, sizeof(ReadData), &NoBytesRead, NULL);

        //
        // Check to make sure that we don't pass the boundaries
        //
        if (!(MaxSerialPacketSize > Loop))
        {
            //
            // Invalid buffer
            //
            ShowMessages("err, a buffer received in debuggee which exceeds the "
                         "buffer limitation\n");
            goto StartAgain;
        }

        SerialBuffer[Loop] = ReadData;

        if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)SerialBuffer))
        {
            break;
        }

        ++Loop;
    } while (NoBytesRead > 0);

    //
    // Because we used overlapped I/O on the other side, sometimes
    // the debuggee might cancel the read so it returns, if it returns
    // then we should restart reading again
    //
    if (Loop == 1 && SerialBuffer[0] == NULL)
    {
        //
        // Chunk data to cancel non async read
        //
        goto StartAgain;
    }

    //
    // Get actual length of received data
    //
    // ShowMessages("\nNumber of bytes received = %d\n", Loop);
    // for (size_t i = 0; i < Loop; i++) {
    //   ShowMessages("%x ", SerialBuffer[i]);
    // }
    // ShowMessages("\n");

    if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER)
    {
        //
        // Check checksum
        //
        if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
                                  Loop - sizeof(BYTE)) !=
            TheActualPacket->Checksum)
        {
            ShowMessages("err checksum is invalid\n");
            goto StartAgain;
        }

        //
        // Check if the packet type is correct
        //
        if (TheActualPacket->TypeOfThePacket !=
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE)
        {
            //
            // sth wrong happened, the packet is not belonging to use
            // nothing to do, just wait again
            //
            ShowMessages("err, unknown packet received from the debugger\n");
            goto StartAgain;
        }

        //
        // It's a HyperDbg packet
        //
        switch (TheActualPacket->RequestedActionOfThePacket)
        {
        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_PAUSE:
            if (!DebuggerPauseDebuggee())
            {
                ShowMessages("err, debugger tries to pause the debuggee but the "
                             "attempt was unsuccessful\n");
            }
            break;
        case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_DO_NOT_READ_ANY_PACKET:
            //
            // Not read anymore
            //
            return TRUE;
            break;
        default:
            ShowMessages("err, unknown packet action received from the debugger\n");
            break;
        }
    }
    else
    {
        //
        // It's not a HyperDbg packet, it's probably a GDB packet
        //
        DebugBreak();
    }

    //
    // Wait for debug pause command again
    //
    goto StartAgain;

    return TRUE;
}

/**
 * @brief Check if the remote debuggee needs to pause the system
 *
 * @param Param
 * @return BOOLEAN
 */
DWORD WINAPI
ListeningSerialPauseDebuggerThread(PVOID Param)
{
    //
    // Create a listening thead in debugger
    //
    ListeningSerialPortInDebugger();

    return 0;
}

/**
 * @brief Check if the remote debugger needs to pause the system
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
DWORD WINAPI
ListeningSerialPauseDebuggeeThread(PVOID Param)
{
    //
    // Create a listening thead in debuggee
    //
    ListeningSerialPortInDebuggee();

    return 0;
}
