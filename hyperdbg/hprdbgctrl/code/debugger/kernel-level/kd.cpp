/**
 * @file kd.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief routines to kernel debugging
 * @details
 * @version 0.1
 * @date 2020-12-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern PMODULE_SYMBOL_DETAIL g_SymbolTable;
extern UINT32                g_SymbolTableSize;
extern UINT32                g_SymbolTableCurrentIndex;
extern HANDLE                g_SerialListeningThreadHandle;
extern HANDLE                g_SerialRemoteComPortHandle;
extern HANDLE                g_DebuggeeStopCommandEventHandle;
extern DEBUGGER_SYNCRONIZATION_EVENTS_STATE
                                            g_KernelSyncronizationObjectsHandleTable[DEBUGGER_MAXIMUM_SYNCRONIZATION_KERNEL_DEBUGGER_OBJECTS];
extern BYTE                                 g_CurrentRunningInstruction[MAXIMUM_INSTR_SIZE];
extern BOOLEAN                              g_IsConnectedToHyperDbgLocally;
extern OVERLAPPED                           g_OverlappedIoStructureForReadDebugger;
extern OVERLAPPED                           g_OverlappedIoStructureForWriteDebugger;
extern DEBUGGER_EVENT_AND_ACTION_REG_BUFFER g_DebuggeeResultOfRegisteringEvent;
extern DEBUGGER_EVENT_AND_ACTION_REG_BUFFER
               g_DebuggeeResultOfAddingActionsToEvent;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;
extern BOOLEAN g_IsDebuggerConntectedToNamedPipe;
extern BOOLEAN g_IsDebuggeeRunning;
extern BOOLEAN g_IsDebuggerModulesLoaded;
extern BOOLEAN g_SerialConnectionAlreadyClosed;
extern BOOLEAN g_IgnoreNewLoggingMessages;
extern BOOLEAN g_SharedEventStatus;
extern BOOLEAN g_IsRunningInstruction32Bit;
extern BOOLEAN g_IgnorePauseRequests;
extern BYTE    g_EndOfBufferCheckSerial[4];
extern ULONG   g_CurrentRemoteCore;

/**
 * @brief compares the buffer with a string
 *
 * @param CurrentLoopIndex Number of previously read bytes
 * @param Buffer
 * @return BOOLEAN
 */
BOOLEAN
KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE * Buffer)
{
    UINT32 ActualBufferLength;

    ActualBufferLength = *CurrentLoopIndex;

    //
    // End of buffer is 4 character long
    //
    if (*CurrentLoopIndex <= 3)
    {
        return FALSE;
    }

    if (Buffer[ActualBufferLength] == SERIAL_END_OF_BUFFER_CHAR_4 &&
        Buffer[ActualBufferLength - 1] == SERIAL_END_OF_BUFFER_CHAR_3 &&
        Buffer[ActualBufferLength - 2] == SERIAL_END_OF_BUFFER_CHAR_2 &&
        Buffer[ActualBufferLength - 3] == SERIAL_END_OF_BUFFER_CHAR_1)
    {
        //
        // Clear the end character
        //
        Buffer[ActualBufferLength - 3] = NULL;
        Buffer[ActualBufferLength - 2] = NULL;
        Buffer[ActualBufferLength - 1] = NULL;
        Buffer[ActualBufferLength]     = NULL;

        //
        // Set the new length
        //
        *CurrentLoopIndex = ActualBufferLength - 3;

        return TRUE;
    }
    return FALSE;
}

/**
 * @brief compares the buffer with a string
 *
 * @param Buffer
 * @param CompareBuffer
 * @return BOOLEAN
 */
BOOLEAN
KdCompareBufferWithString(CHAR * Buffer, const CHAR * CompareBuffer)
{
    int Result;

    Result = strcmp(Buffer, CompareBuffer);

    if (Result == 0)
        return TRUE;
    else
        return FALSE;
}

/**
 * @brief calculate the checksum of recived buffer from debugger
 *
 * @param Buffer
 * @param LengthReceived
 * @return BYTE
 */
BYTE
KdComputeDataChecksum(PVOID Buffer, UINT32 Length)
{
    BYTE CalculatedCheckSum = 0;
    BYTE Temp               = 0;
    while (Length--)
    {
        Temp               = *(BYTE *)Buffer;
        CalculatedCheckSum = CalculatedCheckSum + Temp;
        Buffer             = (PVOID)((UINT64)Buffer + 1);
    }
    return CalculatedCheckSum;
}

/**
 * @brief Interpret the packets from debuggee in the case of paused
 *
 * @return VOID
 */
VOID
KdInterpretPausedDebuggee()
{
    //
    // Wait for handshake to complete or in other words
    // get the receive packet
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PAUSED_DEBUGGEE_DETAILS);
}

/**
 * @brief Sends a continue or 'g' command packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendContinuePacketToDebuggee()
{
    //
    // No core
    //
    g_CurrentRemoteCore = DEBUGGER_DEBUGGEE_IS_RUNNING_NO_CORE;

    //
    // Send 'g' as continue packet
    //
    if (!KdCommandPacketToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CONTINUE))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Sends a change core or '~ x' command packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendSwitchCorePacketToDebuggee(UINT32 NewCore)
{
    DEBUGGEE_CHANGE_CORE_PACKET CoreChangePacket = {0};

    CoreChangePacket.NewCore = NewCore;

    if (CoreChangePacket.NewCore == g_CurrentRemoteCore)
    {
        //
        // We show an error message here when there is no core change (because the
        // target core and current operating core is the same)
        //
        ShowMessages("the current operating core is %x (not changed)\n",
                     CoreChangePacket.NewCore);

        return FALSE;
    }

    //
    // Send '~' as switch packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_CORE,
            (CHAR *)&CoreChangePacket,
            sizeof(DEBUGGEE_CHANGE_CORE_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of core change received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_CORE_SWITCHING_RESULT);

    return TRUE;
}

/**
 * @brief Sends a query or request to enable/disable/clear for event
 * @details if IsQueryState is TRUE then TypeOfAction is ignored
 * @param Tag
 * @param TypeOfAction
 * @param IsEnabled If it's a query state then this argument can be used
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendEventQueryAndModifyPacketToDebuggee(
    UINT64                      Tag,
    DEBUGGER_MODIFY_EVENTS_TYPE TypeOfAction,
    BOOLEAN *                   IsEnabled)
{
    DEBUGGER_MODIFY_EVENTS ModifyAndQueryEventPacket = {0};

    g_SharedEventStatus = FALSE;

    //
    // Fill the structure of packet
    //
    ModifyAndQueryEventPacket.Tag          = Tag;
    ModifyAndQueryEventPacket.TypeOfAction = TypeOfAction;

    //
    // Send modify and query event packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_AND_MODIFY_EVENT,
            (CHAR *)&ModifyAndQueryEventPacket,
            sizeof(DEBUGGER_MODIFY_EVENTS)))
    {
        return FALSE;
    }

    //
    // Wait until the result of query and modify event is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_MODIFY_AND_QUERY_EVENT);

    if (TypeOfAction == DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
    {
        //
        // We should read the results to set IsEnabled variable
        //
        *IsEnabled = g_SharedEventStatus;
    }

    return TRUE;
}

/**
 * @brief Send a flush request to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendFlushPacketToDebuggee()
{
    DEBUGGER_FLUSH_LOGGING_BUFFERS FlushPacket = {0};

    //
    // Send 'flush' command as flush packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_FLUSH_BUFFERS,
            (CHAR *)&FlushPacket,
            sizeof(DEBUGGER_FLUSH_LOGGING_BUFFERS)))
    {
        return FALSE;
    }

    //
    // Wait until the result of flushing received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_FLUSH_RESULT);

    return TRUE;
}

/**
 * @brief Send a callstack request to the debuggee
 * @param BaseAddress
 * @param Size
 * @param DisplayMethod
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendCallStackPacketToDebuggee(UINT64                            BaseAddress,
                                UINT32                            Size,
                                DEBUGGER_CALLSTACK_DISPLAY_METHOD DisplayMethod,
                                BOOLEAN                           Is32Bit)
{
    UINT32                      FrameCount;
    UINT32                      CallstackRequestSize = 0;
    PDEBUGGER_CALLSTACK_REQUEST CallstackPacket      = NULL;

    if (Size == 0)
    {
        return FALSE;
    }

    if (Is32Bit)
    {
        FrameCount = Size / sizeof(UINT32);
    }
    else
    {
        FrameCount = Size / sizeof(UINT64);
    }

    CallstackRequestSize = sizeof(DEBUGGER_CALLSTACK_REQUEST) + (sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME) * FrameCount);

    //
    // Allocate buffer for request
    //
    CallstackPacket = (PDEBUGGER_CALLSTACK_REQUEST)malloc(CallstackRequestSize);

    if (CallstackPacket == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(CallstackPacket, CallstackRequestSize);

    //
    // Set the details
    //
    CallstackPacket->BaseAddress   = BaseAddress;
    CallstackPacket->Is32Bit       = Is32Bit;
    CallstackPacket->Size          = Size;
    CallstackPacket->BufferSize    = CallstackRequestSize;
    CallstackPacket->FrameCount    = FrameCount;
    CallstackPacket->DisplayMethod = DisplayMethod;

    //
    // Send 'k' command as callstack request packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CALLSTACK,
            (CHAR *)CallstackPacket,
            CallstackRequestSize))
    {
        free(CallstackPacket);
        return FALSE;
    }

    //
    // Wait until the result of callstack is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_CALLSTACK_RESULT);

    free(CallstackPacket);
    return TRUE;
}

/**
 * @brief Send a test query request to the debuggee
 *
 * @param Option
 * @return BOOLEAN
 */
BOOLEAN
KdSendTestQueryPacketToDebuggee(UINT32 RequestIndex)
{
    DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER TestQueryPacket = {0};

    TestQueryPacket.RequestIndex = RequestIndex;

    //
    // Send 'test query' command as query packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_TEST_QUERY,
            (CHAR *)&TestQueryPacket,
            sizeof(DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER)))
    {
        return FALSE;
    }

    //
    // Wait until the result of test query is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_TEST_QUERY);

    return TRUE;
}

/**
 * @brief Send symbol reload packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendSymbolReloadPacketToDebuggee(UINT32 ProcessId)
{
    DEBUGGEE_SYMBOL_REQUEST_PACKET SymbolRequest = {0};

    SymbolRequest.ProcessId = ProcessId;

    //
    // Send '.sym reload' as symbol reload packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_RELOAD,
            (CHAR *)&SymbolRequest,
            sizeof(DEBUGGEE_SYMBOL_REQUEST_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until all of the symbol packets are received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SYMBOL_RELOAD);

    return TRUE;
}

/**
 * @brief Send a Read register packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendReadRegisterPacketToDebuggee(PDEBUGGEE_REGISTER_READ_DESCRIPTION RegDes)
{
    //
    // Send r command as read register packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_REGISTERS,
            (CHAR *)RegDes,
            sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION)))
    {
        return FALSE;
    }

    //
    // Wait until the result of read registers received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_READ_REGISTERS);

    return TRUE;
}

/**
 * @brief Send a Read memory packet to the debuggee
 * @param ReadMem
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendReadMemoryPacketToDebuggee(PDEBUGGER_READ_MEMORY ReadMem)
{
    PDEBUGGER_READ_MEMORY ActualBufferToSend = NULL;
    UINT                  Size               = 0;

    Size               = ReadMem->Size * sizeof(unsigned char) + sizeof(DEBUGGER_READ_MEMORY);
    ActualBufferToSend = (PDEBUGGER_READ_MEMORY)malloc(Size);

    if (ActualBufferToSend == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(ActualBufferToSend, Size);

    memcpy(ActualBufferToSend, ReadMem, sizeof(DEBUGGER_READ_MEMORY));

    //
    // Send u-d command as read memory packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_MEMORY,
            (CHAR *)ActualBufferToSend,
            Size))
    {
        free(ActualBufferToSend);
        return FALSE;
    }

    //
    // Wait until the result of read registers received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_READ_MEMORY);

    free(ActualBufferToSend);
    return TRUE;
}

/**
 * @brief Send an Edit memory packet to the debuggee
 * @param EditMem
 * @param Size
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendEditMemoryPacketToDebuggee(PDEBUGGER_EDIT_MEMORY EditMem, UINT32 Size)
{
    //
    // Send d command as read memory packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_EDIT_MEMORY,
            (CHAR *)EditMem,
            Size))
    {
        return FALSE;
    }

    //
    // Wait until the result of read registers received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_EDIT_MEMORY);

    return TRUE;
}

/**
 * @brief Send a register event request to the debuggee
 * @details as this command uses one global variable to transfer the buffers
 * so should not be called simultaneously
 * @param Event
 * @param EventBufferLength
 *
 * @return PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
 */
PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendRegisterEventPacketToDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                                    UINT32                         EventBufferLength)
{
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET Header;
    UINT32                                              Len;

    Len = EventBufferLength +
          sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET);

    Header = (PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)malloc(Len);

    if (Header == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(Header, Len);

    //
    // Set length in header
    //
    Header->Length = EventBufferLength;

    //
    // Move buffer
    //
    memcpy((PVOID)((UINT64)Header +
                   sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)),
           (PVOID)Event,
           EventBufferLength);

    RtlZeroMemory(&g_DebuggeeResultOfRegisteringEvent,
                  sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

    //
    // Send register event packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_REGISTER_EVENT,
            (CHAR *)Header,
            Len))
    {
        free(Header);
        return NULL;
    }

    //
    // Wait until the result of registering received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_REGISTER_EVENT);

    free(Header);

    return &g_DebuggeeResultOfRegisteringEvent;
}

/**
 * @brief Send an add action to event request to the debuggee
 * @details as this command uses one global variable to transfer the buffers
 * so should not be called simultaneously
 * @param GeneralAction
 * @param GeneralActionLength
 *
 * @return PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
 */
PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER
KdSendAddActionToEventPacketToDebuggee(PDEBUGGER_GENERAL_ACTION GeneralAction,
                                       UINT32                   GeneralActionLength)
{
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET Header;
    UINT32                                              Len;

    Len = GeneralActionLength +
          sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET);

    Header = (PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)malloc(Len);

    if (Header == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(Header, Len);

    //
    // Set length in header
    //
    Header->Length = GeneralActionLength;

    //
    // Move buffer
    //
    memcpy((PVOID)((UINT64)Header +
                   sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)),
           (PVOID)GeneralAction,
           GeneralActionLength);

    RtlZeroMemory(&g_DebuggeeResultOfAddingActionsToEvent,
                  sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER));

    //
    // Send add action to event packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_ADD_ACTION_TO_EVENT,
            (CHAR *)Header,
            Len))
    {
        free(Header);
        return NULL;
    }

    //
    // Wait until the result of adding action to event received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_ADD_ACTION_TO_EVENT);

    free(Header);

    return &g_DebuggeeResultOfAddingActionsToEvent;
}

/**
 * @brief Sends a change process or show process details packet to the debuggee
 * @param ActionType
 * @param NewPid
 * @param NewProcess
 * @param SetChangeByClockInterrupt
 * @param SymDetailsForProcessList
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendSwitchProcessPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE ActionType,
                                    UINT32                                   NewPid,
                                    UINT64                                   NewProcess,
                                    BOOLEAN                                  SetChangeByClockInterrupt,
                                    PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS    SymDetailsForProcessList)
{
    DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET ProcessChangePacket = {0};

    ProcessChangePacket.ActionType        = ActionType;
    ProcessChangePacket.ProcessId         = NewPid;
    ProcessChangePacket.Process           = NewProcess;
    ProcessChangePacket.IsSwitchByClkIntr = SetChangeByClockInterrupt;

    //
    // Check if the command really needs these information or not
    // it's because some of the command don't need symbol offset informations
    //
    if (SymDetailsForProcessList != NULL)
    {
        memcpy(&ProcessChangePacket.ProcessListSymDetails, SymDetailsForProcessList, sizeof(DEBUGGEE_PROCESS_LIST_NEEDED_DETAILS));
    }

    //
    // Send '.process' as switch packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_PROCESS,
            (CHAR *)&ProcessChangePacket,
            sizeof(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of process change received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PROCESS_SWITCHING_RESULT);

    return TRUE;
}

/**
 * @brief Sends a change thread or show threads detail packet to the debuggee
 * @param ActionType
 * @param NewTid
 * @param NewThread
 * @param CheckByClockInterrupt
 * @param SymDetailsForThreadList
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendSwitchThreadPacketToDebuggee(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_TYPE ActionType,
                                   UINT32                                  NewTid,
                                   UINT64                                  NewThread,
                                   BOOLEAN                                 CheckByClockInterrupt,
                                   PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS    SymDetailsForThreadList)
{
    DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET ThreadChangePacket = {0};

    ThreadChangePacket.ActionType            = ActionType;
    ThreadChangePacket.ThreadId              = NewTid;
    ThreadChangePacket.Thread                = NewThread;
    ThreadChangePacket.CheckByClockInterrupt = CheckByClockInterrupt;

    //
    // Check if the command really needs these information or not
    // it's because some of the command don't need symbol offset informations
    //
    if (SymDetailsForThreadList != NULL)
    {
        memcpy(&ThreadChangePacket.ThreadListSymDetails, SymDetailsForThreadList, sizeof(DEBUGGEE_THREAD_LIST_NEEDED_DETAILS));
    }

    //
    // Send '.thread' as switch packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_THREAD,
            (CHAR *)&ThreadChangePacket,
            sizeof(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of thread change received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_THREAD_SWITCHING_RESULT);

    return TRUE;
}

/**
 * @brief Sends a PTE or '!pte' command packet to the debuggee
 * @param PtePacket
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendPtePacketToDebuggee(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PtePacket)
{
    //
    // Send 'bp' as a breakpoint packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_QUERY_PTE,
            (CHAR *)PtePacket,
            sizeof(DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS)))
    {
        return FALSE;
    }

    //
    // Wait until the result of PTE packet is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PTE_RESULT);

    return TRUE;
}

/**
 * @brief Sends VA2PA and PA2VA packest, or '!va2pa' and '!pa2va' commands packet to the debuggee
 * @param Va2paAndPa2vaPacket
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendVa2paAndPa2vaPacketToDebuggee(PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS Va2paAndPa2vaPacket)
{
    //
    // Send '!va2pa' or '!pa2va' as a query packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_PA2VA_AND_VA2PA,
            (CHAR *)Va2paAndPa2vaPacket,
            sizeof(DEBUGGER_VA2PA_AND_PA2VA_COMMANDS)))
    {
        return FALSE;
    }

    //
    // Wait until the result of VA2PA or PA2VA packet is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_VA2PA_AND_PA2VA_RESULT);

    return TRUE;
}

/**
 * @brief Sends a breakpoint set or 'bp' command packet to the debuggee
 * @param BpPacket
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendBpPacketToDebuggee(PDEBUGGEE_BP_PACKET BpPacket)
{
    //
    // Send 'bp' as a breakpoint packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_BP,
            (CHAR *)BpPacket,
            sizeof(DEBUGGEE_BP_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of putting breakpoint is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_BP);

    return TRUE;
}

/**
 * @brief Sends a breakpoint list or modification packet to the debuggee
 * @param ListOrModifyPacket
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendListOrModifyPacketToDebuggee(
    PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET ListOrModifyPacket)
{
    //
    // Send list or modify breakpoint packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_LIST_OR_MODIFY_BREAKPOINTS,
            (CHAR *)ListOrModifyPacket,
            sizeof(DEBUGGEE_BP_LIST_OR_MODIFY_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of listing or modifying breakpoints is received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_LIST_OR_MODIFY_BREAKPOINTS);

    return TRUE;
}

/**
 * @brief Sends a script packet to the debuggee
 * @param BufferAddress
 * @param BufferLength
 * @param Pointer
 * @param IsFormat
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendScriptPacketToDebuggee(UINT64 BufferAddress, UINT32 BufferLength, UINT32 Pointer, BOOLEAN IsFormat)
{
    PDEBUGGEE_SCRIPT_PACKET ScriptPacket;
    UINT32                  SizeOfStruct = 0;

    SizeOfStruct = sizeof(DEBUGGEE_SCRIPT_PACKET) + BufferLength;

    ScriptPacket = (DEBUGGEE_SCRIPT_PACKET *)malloc(SizeOfStruct);

    RtlZeroMemory(ScriptPacket, SizeOfStruct);

    //
    // Fill the script packet buffer
    //
    ScriptPacket->ScriptBufferSize    = BufferLength;
    ScriptPacket->ScriptBufferPointer = Pointer;
    ScriptPacket->IsFormat            = IsFormat;

    //
    // Move the buffer at the bottom of the script packet
    //
    memcpy((PVOID)((UINT64)ScriptPacket + sizeof(DEBUGGEE_SCRIPT_PACKET)),
           (PVOID)BufferAddress,
           BufferLength);

    //
    // Send script packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_RUN_SCRIPT,
            (CHAR *)ScriptPacket,
            SizeOfStruct))
    {
        free(ScriptPacket);
        return FALSE;
    }

    if (IsFormat)
    {
        //
        // Wait for formats results too
        //
        DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SCRIPT_FORMATS_RESULT);
    }

    //
    // Wait until the result of script engine received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SCRIPT_RUNNING_RESULT);

    free(ScriptPacket);
    return TRUE;
}

/**
 * @brief Sends user input packet to the debuggee
 * @param Sendbuf
 * @param Len
 * @param IgnoreBreakingAgain
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendUserInputPacketToDebuggee(const char * Sendbuf, int Len, BOOLEAN IgnoreBreakingAgain)
{
    PDEBUGGEE_USER_INPUT_PACKET UserInputPacket;
    UINT32                      SizeOfStruct = 0;

    SizeOfStruct = sizeof(DEBUGGEE_USER_INPUT_PACKET) + Len;

    UserInputPacket = (DEBUGGEE_USER_INPUT_PACKET *)malloc(SizeOfStruct);

    RtlZeroMemory(UserInputPacket, SizeOfStruct);

    //
    // Fill the script packet buffer descriptors
    //
    UserInputPacket->CommandLen           = Len;
    UserInputPacket->IgnoreFinishedSignal = IgnoreBreakingAgain;

    //
    // Move the user input buffer at the bottom of the structure packet
    //
    memcpy((PVOID)((UINT64)UserInputPacket + sizeof(DEBUGGEE_USER_INPUT_PACKET)),
           (PVOID)Sendbuf,
           Len);

    //
    // Send user-input packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_USER_INPUT_BUFFER,
            (CHAR *)UserInputPacket,
            SizeOfStruct))
    {
        free(UserInputPacket);
        return FALSE;
    }

    //
    // Wait until the result of user-input received
    //
    if (!IgnoreBreakingAgain)
    {
        DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_DEBUGGEE_FINISHED_COMMAND_EXECUTION);
    }

    free(UserInputPacket);

    return TRUE;
}

/**
 * @brief Sends seach query request packet to the debuggee
 * @param SearchRequestBuffer
 * @param SearchRequestBufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendSearchRequestPacketToDebuggee(UINT64 * SearchRequestBuffer, UINT32 SearchRequestBufferSize)
{
    //
    // Send search request packet
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SEARCH_QUERY,
            (CHAR *)SearchRequestBuffer,
            SearchRequestBufferSize))
    {
        return FALSE;
    }

    //
    // Wait until the result of search request received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SEARCH_QUERY_RESULT);

    return TRUE;
}

/**
 * @brief Sends p (step out) and t (step in) packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendStepPacketToDebuggee(DEBUGGER_REMOTE_STEPPING_REQUEST StepRequestType)
{
    DEBUGGEE_STEP_PACKET StepPacket = {0};
    UINT32               CallInstructionSize;

    //
    // Set the type of step packet
    //
    StepPacket.StepType = StepRequestType;

    //
    // Check if it's a step-over
    //
    if (StepRequestType == DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER)
    {
        //
        // We should check whether the current instruction is a 'call'
        // instruction or not, if yes we have to compute the length of call
        //
        if (HyperDbgCheckWhetherTheCurrentInstructionIsCall(
                g_CurrentRunningInstruction,
                MAXIMUM_INSTR_SIZE,
                g_IsRunningInstruction32Bit ? FALSE : TRUE, // equals to !g_IsRunningInstruction32Bit
                &CallInstructionSize))
        {
            //
            // It's a call in step-over
            //
            StepPacket.IsCurrentInstructionACall = TRUE;
            StepPacket.CallLength                = CallInstructionSize;
        }
    }

    //
    // Send step packet to the serial
    //
    if (!KdCommandPacketAndBufferToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_STEP,
            (CHAR *)&StepPacket,
            sizeof(DEBUGGEE_STEP_PACKET)))
    {
        return FALSE;
    }

    //
    // Wait until the result of user-input received
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_IS_DEBUGGER_RUNNING);

    return TRUE;
}

/**
 * @brief Sends a PAUSE packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendPausePacketToDebuggee()
{
    //
    // Send pause packet to debuggee
    //
    if (!KdCommandPacketToDebuggee(
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE,
            DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_PAUSE))
    {
        return FALSE;
    }

    //
    // Handle the paused state (show rip, instructions, etc.)
    //
    KdInterpretPausedDebuggee();

    return TRUE;
}

/**
 * @brief Get Windows name, version and build to send to debuggger
 *
 * @param BufferToSave
 *
 * @return BOOLEAN
 */
BOOLEAN
KdGetWindowVersion(CHAR * BufferToSave)
{
    HKeyHolder currentVersion;
    DWORD      valueType;
    CHAR       bufferResult[MAXIMUM_CHARACTER_FOR_OS_NAME]         = {0};
    BYTE       bufferProductName[MAXIMUM_CHARACTER_FOR_OS_NAME]    = {0};
    BYTE       bufferCurrentBuild[MAXIMUM_CHARACTER_FOR_OS_NAME]   = {0};
    BYTE       bufferDisplayVersion[MAXIMUM_CHARACTER_FOR_OS_NAME] = {0};
    DWORD      bufferSize                                          = MAXIMUM_CHARACTER_FOR_OS_NAME;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)",
                      0,
                      KEY_QUERY_VALUE,
                      &currentVersion) != ERROR_SUCCESS)
    {
        // return FALSE;
    }

    if (RegQueryValueExA(currentVersion, "ProductName", nullptr, &valueType, bufferProductName, &bufferSize) != ERROR_SUCCESS)
    {
        // return FALSE;
    }

    if (valueType != REG_SZ)
    {
        //  return FALSE;
    }

    bufferSize = MAXIMUM_CHARACTER_FOR_OS_NAME;

    if (RegQueryValueExA(currentVersion, "DisplayVersion", nullptr, &valueType, bufferDisplayVersion, &bufferSize) != ERROR_SUCCESS)
    {
        // return FALSE;
    }

    if (valueType != REG_SZ)
    {
        //  return FALSE;
    }
    bufferSize = MAXIMUM_CHARACTER_FOR_OS_NAME;

    if (RegQueryValueExA(currentVersion, "CurrentBuild", nullptr, &valueType, bufferCurrentBuild, &bufferSize) != ERROR_SUCCESS)
    {
        //  return FALSE;
    }

    if (valueType != REG_SZ)
    {
        // return FALSE;
    }

    sprintf_s(bufferResult, "%s %s %s (OS Build %s)", bufferProductName, IsWindowsServer() ? "- Server" : "- Client", bufferDisplayVersion, bufferCurrentBuild);

    memcpy(BufferToSave, bufferResult, MAXIMUM_CHARACTER_FOR_OS_NAME);

    return TRUE;
}

/**
 * @brief Receive packet from the debugger
 *
 * @param BufferToSave
 * @param LengthReceived
 *
 * @return BOOLEAN
 */
BOOLEAN
KdReceivePacketFromDebuggee(CHAR *   BufferToSave,
                            UINT32 * LengthReceived)
{
    BOOL   Status;             /* Status */
    char   ReadData    = NULL; /* temperory Character */
    DWORD  NoBytesRead = 0;    /* Bytes read by ReadFile() */
    UINT32 Loop        = 0;

    //
    // Read data and store in a buffer
    //
    do
    {
        if (g_IsSerialConnectedToRemoteDebugger)
        {
            //
            // It's a debuggee
            //
            Status = ReadFile(g_SerialRemoteComPortHandle, &ReadData, sizeof(ReadData), &NoBytesRead, NULL);
        }
        else
        {
            //
            // It's a debugger
            //

            //
            // Try to read one byte in overlapped I/O (in debugger)
            //
            if (!ReadFile(g_SerialRemoteComPortHandle, &ReadData, sizeof(ReadData), NULL, &g_OverlappedIoStructureForReadDebugger))
            {
                DWORD e = GetLastError();

                if (e != ERROR_IO_PENDING)
                {
                    return FALSE;
                }
            }

            //
            // Wait till one packet becomes available
            //
            WaitForSingleObject(g_OverlappedIoStructureForReadDebugger.hEvent,
                                INFINITE);

            //
            // Get the result
            //
            GetOverlappedResult(g_SerialRemoteComPortHandle,
                                &g_OverlappedIoStructureForReadDebugger,
                                &NoBytesRead,
                                FALSE);

            //
            // Reset event for next try
            //
            ResetEvent(g_OverlappedIoStructureForReadDebugger.hEvent);
        }

        //
        // We already now that the maximum packet size is MaxSerialPacketSize
        // Check to make sure that we don't pass the boundaries
        //
        if (!(MaxSerialPacketSize > Loop))
        {
            //
            // Invalid buffer
            //
            ShowMessages("err, a buffer received in which exceeds the "
                         "buffer limitation\n");
            return FALSE;
        }

        BufferToSave[Loop] = ReadData;

        if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)BufferToSave))
        {
            break;
        }

        Loop++;

    } while (NoBytesRead > 0);

    //
    // Set the length
    //
    *LengthReceived = Loop;

    return TRUE;
}

/**
 * @brief Sends a special packet to the debuggee
 *
 * @param Buffer
 * @param Length
 * @return BOOLEAN
 */
BOOLEAN
KdSendPacketToDebuggee(const CHAR * Buffer, UINT32 Length, BOOLEAN SendEndOfBuffer)
{
    BOOL  Status;
    DWORD BytesWritten  = 0;
    DWORD LastErrorCode = 0;

    //
    // Start getting debuggee messages again
    //
    g_IgnoreNewLoggingMessages = FALSE;

    //
    // Double check if buffer not pass the boundary
    //
    if (Length + SERIAL_END_OF_BUFFER_CHARS_COUNT > MaxSerialPacketSize)
    {
        ShowMessages("err, buffer is above the maximum buffer size that can be sent to debuggee (%d > %d)",
                     Length + SERIAL_END_OF_BUFFER_CHARS_COUNT,
                     MaxSerialPacketSize);
        return FALSE;
    }

    //
    // Check if the remote code's handle found or not
    //
    if (g_SerialRemoteComPortHandle == NULL)
    {
        ShowMessages("err, handle to remote debuggee's com port is not found\n");
        return FALSE;
    }

    if (g_IsSerialConnectedToRemoteDebugger)
    {
        //
        // It's for a debuggee
        //
        Status = WriteFile(g_SerialRemoteComPortHandle, // Handle to the Serialport
                           Buffer,                      // Data to be written to the port
                           Length,                      // No of bytes to write into the port
                           &BytesWritten,               // No of bytes written to the port
                           NULL);

        if (Status == FALSE)
        {
            ShowMessages("err, fail to write to com port or named pipe (error %x).\n",
                         GetLastError());
            return FALSE;
        }

        //
        // Check if message delivered successfully
        //
        if (BytesWritten != Length)
        {
            return FALSE;
        }
    }
    else
    {
        //
        // It's a debugger
        //

        if (WriteFile(g_SerialRemoteComPortHandle, Buffer, Length, NULL, &g_OverlappedIoStructureForWriteDebugger))
        {
            //
            // Write Completed
            //
            goto Out;
        }

        LastErrorCode = GetLastError();
        if (LastErrorCode != ERROR_IO_PENDING)
        {
            //
            // Error
            //
            // ShowMessages("err, on sending serial packets (%x)", LastErrorCode);
            return FALSE;
        }

        //
        // Wait until write completed
        //
        if (WaitForSingleObject(g_OverlappedIoStructureForWriteDebugger.hEvent,
                                INFINITE) != WAIT_OBJECT_0)
        {
            // ShowMessages("err, on sending serial packets (signal error)");
            return FALSE;
        }

        //
        // Reset event
        //
        ResetEvent(g_OverlappedIoStructureForWriteDebugger.hEvent);
    }

Out:
    if (SendEndOfBuffer)
    {
        //
        // Send End of Buffer Packet
        //
        if (!KdSendPacketToDebuggee((const CHAR *)g_EndOfBufferCheckSerial,
                                    sizeof(g_EndOfBufferCheckSerial),
                                    FALSE))
        {
            return FALSE;
        }
    }

    //
    // All the bytes are sent
    //
    return TRUE;
}

/**
 * @brief Sends a HyperDbg packet to the debuggee
 *
 * @param RequestedAction
 * @return BOOLEAN
 */
BOOLEAN
KdCommandPacketToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};

    //
    // There is no check for boundary here as it's fixed to
    // sizeof(DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION) + sizeof(DEBUGGER_REMOTE_PACKET)
    //

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKET;
    Packet.TypeOfThePacket = PacketType;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = RequestedAction;

    //
    // calculate checksum of the packet
    //
    Packet.Checksum =
        KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                              sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    if (!KdSendPacketToDebuggee((const CHAR *)&Packet,
                                sizeof(DEBUGGER_REMOTE_PACKET),
                                TRUE))
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Sends a HyperDbg packet + a buffer to the debuggee
 *
 * @param RequestedAction
 * @param Buffer
 * @param BufferLength
 * @return BOOLEAN
 */
BOOLEAN
KdCommandPacketAndBufferToDebuggee(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction,
    CHAR *                                  Buffer,
    UINT32                                  BufferLength)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};

    //
    // Check if buffer not pass the boundary
    //
    if (sizeof(DEBUGGER_REMOTE_PACKET) + BufferLength + SERIAL_END_OF_BUFFER_CHARS_COUNT >
        MaxSerialPacketSize)
    {
        ShowMessages("err, buffer is above the maximum buffer size that can be sent to debuggee (%d > %d)",
                     sizeof(DEBUGGER_REMOTE_PACKET) + BufferLength + SERIAL_END_OF_BUFFER_CHARS_COUNT,
                     MaxSerialPacketSize);

        return FALSE;
    }

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKET;
    Packet.TypeOfThePacket = PacketType;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = RequestedAction;

    //
    // calculate checksum of the packet
    //
    Packet.Checksum =
        KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                              sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    Packet.Checksum += KdComputeDataChecksum((PVOID)Buffer, BufferLength);

    //
    // Send the first buffer (without ending buffer indication)
    //
    if (!KdSendPacketToDebuggee((const CHAR *)&Packet,
                                sizeof(DEBUGGER_REMOTE_PACKET),
                                FALSE))
    {
        return FALSE;
    }

    //
    // Send the second buffer (with ending buffer indication)
    //
    if (!KdSendPacketToDebuggee((const CHAR *)Buffer, BufferLength, TRUE))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief check if the debuggee needs to be paused
 *
 * @return VOID
 */
VOID
KdBreakControlCheckAndPauseDebugger()
{
    //
    // Check if debuggee is running, otherwise the user
    // pressed ctrl+c multiple times
    //
    if (g_IsDebuggeeRunning)
    {
        //
        // Send the pause request to the remote computer
        //
        if (!KdSendPausePacketToDebuggee())
        {
            ShowMessages("err, unable to pause the debuggee\n");
        }

        //
        // Signal the event
        //
        DbgReceivedKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_IS_DEBUGGER_RUNNING);
    }
}

/**
 * @brief Set the status of the debuggee to wait for the pause
 *
 * @return VOID
 */
VOID
KdSetStatusAndWaitForPause()
{
    //
    // Set the debuggee to show that it's  running
    //
    g_IsDebuggeeRunning = TRUE;

    //
    // Halt the UI
    //
    KdTheRemoteSystemIsRunning();
}

/**
 * @brief check if the debuggee needs to be continued
 *
 * @return VOID
 */
VOID
KdBreakControlCheckAndContinueDebugger()
{
    //
    // Check if debuggee is paused or not
    //
    if (!g_IsDebuggeeRunning)
    {
        //
        // Send the continue request to the remote computer
        //
        if (KdSendContinuePacketToDebuggee())
        {
            //
            // Set the debuggee to show that it's running
            //
            KdSetStatusAndWaitForPause();
        }
        else
        {
            ShowMessages("err, unable to continue the debuggee\n");
        }
    }
}

/**
 * @brief wait for a event to be triggered and if the debuggee
 * is running it just halts the system
 *
 * @return VOID
 */
VOID
KdTheRemoteSystemIsRunning()
{
    //
    // Indicate that the debuggee is running
    //
    ShowMessages("debuggee is running...\n");

    //
    // Wait until the users press CTRL+C
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_IS_DEBUGGER_RUNNING);
}

/**
 * @brief Prepare serial to connect to the remote server
 * @details wait to connect to debuggee (this is debugger)
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
BOOLEAN
KdPrepareSerialConnectionToRemoteSystem(HANDLE  SerialHandle,
                                        BOOLEAN IsNamedPipe)
{
StartAgain:

    BOOL  Status;        /* Status */
    DWORD EventMask = 0; /* Event mask to trigger */

    //
    // Show an indication to connect the debugger
    //
    ShowMessages("waiting for debuggee to connect...\n");

    if (!IsNamedPipe)
    {
        //
        // Setting Receive Mask
        //
        Status = SetCommMask(SerialHandle, EV_RXCHAR);
        if (Status == FALSE)
        {
            //
            // Can be ignored
            //
            // ShowMessages("err, in setting CommMask\n");
            // return FALSE;
        }

        //
        // Setting WaitComm() Event
        //
        Status = WaitCommEvent(SerialHandle, &EventMask, NULL); /* Wait for the character to be received */

        if (Status == FALSE)
        {
            //
            // Can be ignored
            //
            // ShowMessages("err, in setting WaitCommEvent\n");
            // return FALSE;
        }
    }

    //
    // Initialize the handle table
    //
    for (size_t i = 0; i < DEBUGGER_MAXIMUM_SYNCRONIZATION_KERNEL_DEBUGGER_OBJECTS; i++)
    {
        g_KernelSyncronizationObjectsHandleTable[i].IsOnWaitingState = FALSE;
        g_KernelSyncronizationObjectsHandleTable[i].EventHandle =
            CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    //
    // the debuggee is not already closed the connection
    //
    g_SerialConnectionAlreadyClosed = FALSE;

    //
    // Create the listening thread in debugger
    //

    g_SerialListeningThreadHandle =
        CreateThread(NULL, 0, ListeningSerialPauseDebuggerThread, NULL, 0, NULL);

    //
    // Wait for the 'Start' packet on the listener side
    //
    DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_STARTED_PACKET_RECEIVED);

    //
    // Check to make sure the connection not close before starting a session
    //
    if (!g_SerialConnectionAlreadyClosed)
    {
        //
        // Ignore handling breaks
        //
        g_IgnorePauseRequests = TRUE;

        //
        // The next step is getting symbol details
        //
        ShowMessages("getting symbol details...\n");

        //
        // Wait to receive symbol details
        //
        DbgWaitForKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SYMBOL_RELOAD);

        //
        // Connected to the debuggee
        //
        g_IsSerialConnectedToRemoteDebuggee = TRUE;

        //
        // And debuggee is running
        //
        g_IsDebuggeeRunning = TRUE;

        //
        // Save the handler
        //
        g_SerialRemoteComPortHandle = SerialHandle;

        //
        // Is serial handle for a named pipe
        //
        g_IsDebuggerConntectedToNamedPipe = IsNamedPipe;

        //
        // Now, the user can press ctrl+c to pause the debuggee
        //
        ShowMessages("press CTRL+C to pause the debuggee\n");

        //
        // Process CTRL+C breaks again
        //
        g_IgnorePauseRequests = FALSE;

        //
        // Wait for event on this thread
        //
        KdTheRemoteSystemIsRunning();
    }

    return TRUE;
}

/**
 * @brief Prepare and initialize COM port
 *
 * @param PortName
 * @param Baudrate
 * @param Port
 * @param IsPreparing
 * @param IsNamedPipe
 * @return BOOLEAN
 */
BOOLEAN
KdPrepareAndConnectDebugPort(const char * PortName, DWORD Baudrate, UINT32 Port, BOOLEAN IsPreparing, BOOLEAN IsNamedPipe)
{
    HANDLE                     Comm;               /* Handle to the Serial port */
    BOOL                       Status;             /* Status */
    DCB                        SerialParams = {0}; /* Initializing DCB structure */
    COMMTIMEOUTS               Timeouts     = {0}; /* Initializing timeouts structure */
    char                       PortNo[20]   = {0}; /* contain friendly name */
    BOOLEAN                    StatusIoctl;
    ULONG                      ReturnedLength;
    PDEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest;

    //
    // Check if the debugger or debuggee is already active
    //
    if (IsConnectedToAnyInstanceOfDebuggerOrDebuggee())
    {
        return FALSE;
    }

    if (IsPreparing && IsNamedPipe)
    {
        ShowMessages("err, cannot used named pipe for debuggee");
        return FALSE;
    }

    if (!IsNamedPipe)
    {
        //
        // It's a serial
        //

        //
        // Append name to make a Windows understandable format
        //
        sprintf_s(PortNo, 20, "\\\\.\\%s", PortName);

        //
        // Open the serial com port (if it's the debugger (not debuggee)) then
        // open with Overlapped I/O
        //

        if (IsPreparing)
        {
            //
            // It's debuggee (Non-overlapped I/O)
            //
            Comm = CreateFile(PortNo,                       // Friendly name
                              GENERIC_READ | GENERIC_WRITE, // Read/Write Access
                              0,                            // No Sharing, ports cant be shared
                              NULL,                         // No Security
                              OPEN_EXISTING,                // Open existing port only
                              0,                            // Non Overlapped I/O
                              NULL);                        // Null for Comm Devices
        }
        else
        {
            //
            // It's debugger (Overlapped I/O)
            //
            Comm = CreateFile(PortNo,                       // Friendly name
                              GENERIC_READ | GENERIC_WRITE, // Read/Write Access
                              0,                            // No Sharing, ports cant be shared
                              NULL,                         // No Security
                              OPEN_EXISTING,                // Open existing port only
                              FILE_FLAG_OVERLAPPED,         // Overlapped I/O
                              NULL);                        // Null for Comm Devices

            //
            // Create event for overlapped I/O (Read)
            //
            g_OverlappedIoStructureForReadDebugger.hEvent =
                CreateEvent(NULL, TRUE, FALSE, NULL);

            //
            // Create event for overlapped I/O (Write)
            //
            g_OverlappedIoStructureForWriteDebugger.hEvent =
                CreateEvent(NULL, TRUE, FALSE, NULL);
        }

        if (Comm == INVALID_HANDLE_VALUE)
        {
            ShowMessages("err, port can't be opened\n");
            return FALSE;
        }

        //
        // Purge the serial port
        //
        PurgeComm(Comm,
                  PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

        //
        // Setting the Parameters for the SerialPort
        //
        SerialParams.DCBlength = sizeof(SerialParams);

        //
        // retreives the current settings
        //
        Status = GetCommState(Comm, &SerialParams);

        if (Status == FALSE)
        {
            CloseHandle(Comm);
            ShowMessages("err, to Get the COM state\n");
            return FALSE;
        }

        SerialParams.BaudRate =
            Baudrate;                       // BaudRate = 9600 (Based on user selection)
        SerialParams.ByteSize = 8;          // ByteSize = 8
        SerialParams.StopBits = ONESTOPBIT; // StopBits = 1
        SerialParams.Parity   = NOPARITY;   // Parity = None
        Status                = SetCommState(Comm, &SerialParams);

        if (Status == FALSE)
        {
            CloseHandle(Comm);
            ShowMessages("err, to Setting DCB Structure\n");
            return FALSE;
        }

        //
        // Setting Timeouts (not use it anymore as we use special signature for
        // ending buffer)
        // (no need anymore as we use end buffer detection mechanism)
        //

        /*
    Timeouts.ReadIntervalTimeout = 50;
    Timeouts.ReadTotalTimeoutConstant = 50;
    Timeouts.ReadTotalTimeoutMultiplier = 10;
    Timeouts.WriteTotalTimeoutConstant = 50;
    Timeouts.WriteTotalTimeoutMultiplier = 10;

    if (SetCommTimeouts(Comm, &Timeouts) == FALSE) {
      ShowMessages("err, to Setting Time outs (%x).\n", GetLastError());
      return FALSE;
    }
    */
    }
    else
    {
        //
        // It's a namedpipe
        //
        Comm = NamedPipeClientCreatePipeOverlappedIo(PortName);

        if (!Comm)
        {
            //
            // Unable to create handle
            //
            ShowMessages("is virtual machine running?\n");
            return FALSE;
        }
    }

    if (IsPreparing)
    {
        //
        // It's a debuggee request
        //

        //
        // First, connect to local machine and we load the VMM module as it's a
        // module that is responsible for working on debugger
        //
        g_IsConnectedToHyperDbgLocally = TRUE;

        //
        // Load the VMM
        //
        if (!CommandLoadVmmModule())
        {
            CloseHandle(Comm);
            ShowMessages("failed to install or load the driver\n");
            return FALSE;
        }

        //
        // Check if driver is loaded or not, in the case
        // of connecting to a remote machine as debuggee
        //
        if (!g_DeviceHandle)
        {
            CloseHandle(Comm);
            AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);
        }

        //
        // Allocate buffer
        //
        DebuggeeRequest =
            (PDEBUGGER_PREPARE_DEBUGGEE)malloc(SIZEOF_DEBUGGER_PREPARE_DEBUGGEE);

        if (DebuggeeRequest == NULL)
        {
            CloseHandle(Comm);
            ShowMessages("err, unable to allocate memory for request packet");
            return FALSE;
        }

        RtlZeroMemory(DebuggeeRequest, SIZEOF_DEBUGGER_PREPARE_DEBUGGEE);

        //
        // Prepare the details structure
        //
        DebuggeeRequest->PortAddress = Port;
        DebuggeeRequest->Baudrate    = Baudrate;

        //
        // Get base address of ntoskrnl
        //
        DebuggeeRequest->NtoskrnlBaseAddress = DebuggerGetNtoskrnlBase();

        //
        // Set the debuggee name, version, and build number
        //
        if (!KdGetWindowVersion(DebuggeeRequest->OsName))
        {
            //
            // It's not an error if it returned null
            //
            // return FALSE;
        }

        //
        // Send the request to the kernel
        //
        StatusIoctl =
            DeviceIoControl(g_DeviceHandle,                   // Handle to device
                            IOCTL_PREPARE_DEBUGGEE,           // IO Control code
                            DebuggeeRequest,                  // Input Buffer to driver.
                            SIZEOF_DEBUGGER_PREPARE_DEBUGGEE, // Input buffer
                                                              // length
                            DebuggeeRequest,                  // Output Buffer from driver.
                            SIZEOF_DEBUGGER_PREPARE_DEBUGGEE, // Length of output
                                                              // buffer in bytes.
                            &ReturnedLength,                  // Bytes placed in buffer.
                            NULL                              // synchronous call
            );

        if (!StatusIoctl)
        {
            CloseHandle(Comm);
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            //
            // Free the buffer
            //
            free(DebuggeeRequest);

            return FALSE;
        }

        if (DebuggeeRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
        {
            //
            // Ignore handling CTRL+C breaks
            //
            g_IgnorePauseRequests = TRUE;

            //
            // initialize and load symbols (pdb) and send the details to the debugger
            //
            ShowMessages("synchronizing modules' symbol details\n");

            //
            // Do not pause debugger after finish
            //
            KdReloadSymbolsInDebuggee(FALSE, GetCurrentProcessId());
        }
        else
        {
            CloseHandle(Comm);
            ShowErrorMessage(DebuggeeRequest->Result);

            //
            // Free the buffer
            //
            free(DebuggeeRequest);

            return FALSE;
        }

        //
        // show the message that the operation was successful,
        // we show it here because if we set the next variables
        // to true, then it will be sent to the debugger instead
        // of showing the message in debuggee
        //
        ShowMessages("the operation was successful\n");

        //
        // Serial connection is not already closed
        //
        g_SerialConnectionAlreadyClosed = FALSE;

        //
        // Indicate that we connected to the debugger
        //
        g_IsSerialConnectedToRemoteDebugger = TRUE;

        //
        // Set handle to serial device
        //
        g_SerialRemoteComPortHandle = Comm;

        //
        // Free the buffer
        //
        free(DebuggeeRequest);

        //
        // Wait here so the user can't give new commands
        // Create an event (manually. no signal)
        //
        g_DebuggeeStopCommandEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);

        //
        // Create a thread to listen for pauses from the remote debugger
        //

        g_SerialListeningThreadHandle = CreateThread(
            NULL,
            0,
            ListeningSerialPauseDebuggeeThread,
            NULL,
            0,
            NULL);

        //
        // Test should be removed
        //
        // HyperDbgInterpreter("!syscall script { print(@rax); }");
        // HyperDbgInterpreter("!epthook fffff801`639b1030 script { print(@rax);
        // }"); HyperDbgInterpreter("!msrwrite script { print(@rax); }");
        //

        //
        // Now we should wait on this state until the user closes the connection to
        // debuggee from debugger
        //
        WaitForSingleObject(g_DebuggeeStopCommandEventHandle, INFINITE);

        //
        // Close the event's handle
        //
        CloseHandle(g_DebuggeeStopCommandEventHandle);
        g_DebuggeeStopCommandEventHandle = NULL;

        //
        // Handle CTRL+C breaks again
        //
        g_IgnorePauseRequests = FALSE;

        //
        // Finish it here
        //
        return TRUE;
    }
    else
    {
        //
        // Save the handler
        //
        g_SerialRemoteComPortHandle = Comm;

        //
        // If we are here, then it's a debugger (not debuggee)
        // let's prepare the debuggee
        //
        KdPrepareSerialConnectionToRemoteSystem(Comm, IsNamedPipe);
    }

    //
    // everything was done
    //
    return TRUE;
}

/**
 * @brief Send general buffer from debuggee to debugger
 * @param RequestedAction
 * @param Buffer
 * @param Length
 * @param PauseDebuggeeWhenSent
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendGeneralBuffersFromDebuggeeToDebugger(
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedAction,
    PVOID                                   Buffer,
    UINT32                                  BufferLength,
    BOOLEAN                                 PauseDebuggeeWhenSent)
{
    BOOL  Status;
    ULONG ReturnedLength;
    PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER
    GeneralPacketFromDebuggeeToDebuggerRequest;
    UINT32 Length;

    //
    // Compute the length of the packet (add to header )
    //
    Length = sizeof(DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER) +
             BufferLength;

    //
    // Allocate the target buffer
    //
    GeneralPacketFromDebuggeeToDebuggerRequest =
        (PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER)malloc(Length);

    if (GeneralPacketFromDebuggeeToDebuggerRequest == NULL)
    {
        //
        // err, allocating buffer
        //
        return FALSE;
    }

    RtlZeroMemory(GeneralPacketFromDebuggeeToDebuggerRequest, Length);

    //
    // Fill the header structure
    //
    GeneralPacketFromDebuggeeToDebuggerRequest->RequestedAction = RequestedAction;
    GeneralPacketFromDebuggeeToDebuggerRequest->LengthOfBuffer  = BufferLength;
    GeneralPacketFromDebuggeeToDebuggerRequest->PauseDebuggeeWhenSent =
        PauseDebuggeeWhenSent;

    //
    // Move the memory to the bottom of the structure
    //
    memcpy(
        (PVOID)((UINT64)GeneralPacketFromDebuggeeToDebuggerRequest +
                sizeof(DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER)),
        (PVOID)Buffer,
        BufferLength);

    //
    // Send Ioctl to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                                // Handle to device
        IOCTL_SEND_GENERAL_BUFFER_FROM_DEBUGGEE_TO_DEBUGGER,           // IO Control code
        GeneralPacketFromDebuggeeToDebuggerRequest,                    // Input Buffer to driver.
        Length,                                                        // Input buffer
                                                                       // length
        GeneralPacketFromDebuggeeToDebuggerRequest,                    // Output Buffer from driver.
        SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER, // Length
                                                                       // of
                                                                       // output
                                                                       // buffer
                                                                       // in
                                                                       // bytes.
        &ReturnedLength,                                               // Bytes placed in buffer.
        NULL                                                           // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(GeneralPacketFromDebuggeeToDebuggerRequest);
        return FALSE;
    }

    if (GeneralPacketFromDebuggeeToDebuggerRequest->KernelResult !=
        DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        ShowErrorMessage(GeneralPacketFromDebuggeeToDebuggerRequest->KernelResult);

        //
        // Free the buffer
        //
        free(GeneralPacketFromDebuggeeToDebuggerRequest);
        return FALSE;
    }

    free(GeneralPacketFromDebuggeeToDebuggerRequest);
    return TRUE;
}

/**
 * @brief Send the packets of reloading symbols to build a new
 * symbol table to the debugger and send the finished packet to
 * the debugger
 * @param PauseDebuggee
 * @param UserProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
KdReloadSymbolsInDebuggee(BOOLEAN PauseDebuggee, UINT32 UserProcessId)
{
    DEBUGGEE_SYMBOL_UPDATE_RESULT SymReload = {0};

    //
    // In kernel debugger, if the process id is not specified
    // we choose the HyperDbg's process to load its modules
    //
    if (UserProcessId == NULL)
    {
        UserProcessId = GetCurrentProcessId();
    }

    //
    // Request debuggee to send new symbol packets
    //
    SymbolPrepareDebuggerWithSymbolInfo(UserProcessId);

    //
    // Set the status
    //
    SymReload.KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

    //
    // Send the finished request packet
    //
    return KdSendGeneralBuffersFromDebuggeeToDebugger(
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SYMBOL_FINISHED,
        &SymReload,
        sizeof(DEBUGGEE_SYMBOL_UPDATE_RESULT),
        PauseDebuggee);
}

/**
 * @brief Send close packet to the debuggee and debugger
 * @details This function close the connection in both debuggee and debugger
 * both of the debuggee and debugger can use this function
 *
 * @return BOOLEAN
 */
BOOLEAN
KdCloseConnection()
{
    //
    // The process of closing connection already started
    //
    if (g_SerialConnectionAlreadyClosed)
    {
        return TRUE;
    }
    else
    {
        g_SerialConnectionAlreadyClosed = TRUE;
    }

    //
    // Unload the VMM driver if it's debuggee
    //
    if (g_IsSerialConnectedToRemoteDebugger)
    {
        if (g_IsConnectedToHyperDbgLocally && g_IsDebuggerModulesLoaded)
        {
            HyperDbgUnloadVmm();
        }
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send close & unload packet to debuggee (vmx-root)
        //
        if (KdCommandPacketToDebuggee(
                DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT,
                DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CLOSE_AND_UNLOAD_DEBUGGEE))
        {
            //
            // It's a debugger, we should send the close packet to debuggee
            //
            ShowMessages("unloading debugger vmm module on debuggee...\n");

            //
            // Send another packet so the user-mode is not waiting for new packet
            //
            !KdCommandPacketToDebuggee(
                DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE,
                DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_DO_NOT_READ_ANY_PACKET);
        }
    }
    else
    {
        //
        // If we're here, probably the connection started but the debuggee is closed
        // without sending the start packet; thus, the debugger has no idea about
        // connection but we should uninitialize everything
        //
        ShowMessages("err, start packet not received but the target machine closed the "
                     "connection\n");

        //
        // Not waiting for start packet
        //
        DbgReceivedKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_STARTED_PACKET_RECEIVED);
    }

    //
    // Close handle and uninitialize everything
    //
    KdUninitializeConnection();

    return TRUE;
}

/**
 * @brief Register an event in the debuggee
 * @param EventRegBuffer
 * @param Length
 *
 * @return BOOLEAN
 */
BOOLEAN
KdRegisterEventInDebuggee(PDEBUGGER_GENERAL_EVENT_DETAIL EventRegBuffer,
                          UINT32                         Length)
{
    BOOL                                 Status;
    ULONG                                ReturnedLength;
    DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Send IOCTL
    //
    Status =
        DeviceIoControl(g_DeviceHandle,                // Handle to device
                        IOCTL_DEBUGGER_REGISTER_EVENT, // IO Control code
                        EventRegBuffer,
                        Length                                        // Input Buffer to driver.
                        ,                                             // Input buffer length
                        &ReturnedBuffer,                              // Output Buffer from driver.
                        sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                      // of
                                                                      // output
                                                                      // buffer
                                                                      // in
                                                                      // bytes.
                        &ReturnedLength,                              // Bytes placed in buffer.
                        NULL                                          // synchronous call
        );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Now that we registered the event (with or without error),
    // we should send the results back to the debugger
    //
    return KdSendGeneralBuffersFromDebuggeeToDebugger(
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT,
        &ReturnedBuffer,
        sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER),
        TRUE);
}

/**
 * @brief Add action to an event in the debuggee
 * @param ActionAddingBuffer
 * @param Length
 *
 * @return BOOLEAN
 */
BOOLEAN
KdAddActionToEventInDebuggee(PDEBUGGER_GENERAL_ACTION ActionAddingBuffer,
                             UINT32                   Length)
{
    BOOL                                 Status;
    ULONG                                ReturnedLength;
    DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    Status =
        DeviceIoControl(g_DeviceHandle,                               // Handle to device
                        IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT,           // IO Control code
                        ActionAddingBuffer,                           // Input Buffer to driver.
                        Length,                                       // Input buffer length
                        &ReturnedBuffer,                              // Output Buffer from driver.
                        sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                      // of
                                                                      // output
                                                                      // buffer
                                                                      // in
                                                                      // bytes.
                        &ReturnedLength,                              // Bytes placed in buffer.
                        NULL                                          // synchronous call
        );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Now that we added the action to the event, we should send the
    // results to the debugger
    //
    return KdSendGeneralBuffersFromDebuggeeToDebugger(
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT,
        &ReturnedBuffer,
        sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER),
        TRUE);
}

/**
 * @brief Modify event ioctl in the debuggee
 * @param ModifyEvent
 *
 * @return BOOLEAN
 */
BOOLEAN
KdSendModifyEventInDebuggee(PDEBUGGER_MODIFY_EVENTS ModifyEvent)
{
    BOOLEAN Status;
    ULONG   ReturnedLength;

    //
    // This mechanism is only used for clear event,
    // it is because we apply enable/disable in kernel
    // directly
    //

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Send the request to the kernel
    //

    Status = DeviceIoControl(g_DeviceHandle,                // Handle to device
                             IOCTL_DEBUGGER_MODIFY_EVENTS,  // IO Control code
                             ModifyEvent,                   // Input Buffer to driver.
                             SIZEOF_DEBUGGER_MODIFY_EVENTS, // Input buffer length
                             ModifyEvent,                   // Output Buffer from driver.
                             SIZEOF_DEBUGGER_MODIFY_EVENTS, // Length of output
                                                            // buffer in bytes.
                             &ReturnedLength,               // Bytes placed in buffer.
                             NULL                           // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Send the buffer back to debugger
    //
    return KdSendGeneralBuffersFromDebuggeeToDebugger(
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT,
        ModifyEvent,
        sizeof(DEBUGGER_MODIFY_EVENTS),
        TRUE);
}

/**
 * @brief Handle user-input in debuggee
 * @param Descriptor
 * @return VOID
 */
VOID
KdHandleUserInputInDebuggee(DEBUGGEE_USER_INPUT_PACKET * Descriptor)
{
    BOOL                                            Status;
    ULONG                                           ReturnedLength;
    CHAR *                                          Input;
    DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL FinishExecutionRequest = {0};

    Input = (CHAR *)Descriptor + sizeof(DEBUGGEE_USER_INPUT_PACKET);

    //
    // Run the command
    //
    HyperDbgInterpreter(Input);

    //
    // Check if it needs to send a signal to indicate that the execution of
    // command finished
    //

    if (!Descriptor->IgnoreFinishedSignal)
    {
        //
        // By the way, we don't need to send an input buffer
        // to the kernel, but let's keep it like this, if we
        // want to pass some other aguments to the kernel in
        // the future
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                         // Handle to device
            IOCTL_SEND_SIGNAL_EXECUTION_IN_DEBUGGEE_FINISHED,       // IO Control code
            &FinishExecutionRequest,                                // Input Buffer to driver.
            SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL, // Input buffer
                                                                    // length
            &FinishExecutionRequest,                                // Output Buffer from driver.
            SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL, // Length of
                                                                    // output buffer
                                                                    // in bytes.
            &ReturnedLength,                                        // Bytes placed in buffer.
            NULL                                                    // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return;
        }
    }
}

/**
 * @brief Send result of user-mode ShowMessages to debuggee
 * @param Input
 * @param Length
 * @return VOID
 */
VOID
KdSendUsermodePrints(CHAR * Input, UINT32 Length)
{
    BOOL                                         Status;
    ULONG                                        ReturnedLength;
    PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER UsermodeMessageRequest;
    UINT32                                       SizeToSend;

    SizeToSend = sizeof(DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER) + Length;

    UsermodeMessageRequest =
        (DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER *)malloc(SizeToSend);

    RtlZeroMemory(UsermodeMessageRequest, SizeToSend);

    //
    // Set the length
    //
    UsermodeMessageRequest->Length = Length;

    //
    // Move the user message buffer at the bottom of the structure packet
    //
    memcpy((PVOID)((UINT64)UsermodeMessageRequest +
                   sizeof(DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER)),
           (PVOID)Input,
           Length);

    Status = DeviceIoControl(
        g_DeviceHandle,                           // Handle to device
        IOCTL_SEND_USERMODE_MESSAGES_TO_DEBUGGER, // IO Control code
        UsermodeMessageRequest,                   // Input Buffer to driver.
        SizeToSend,                               // Input buffer
                                                  // length
        UsermodeMessageRequest,                   // Output Buffer from driver.
        SizeToSend,                               // Length of
                                                  // output buffer
                                                  // in bytes.
        &ReturnedLength,                          // Bytes placed in buffer.
        NULL                                      // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(UsermodeMessageRequest);
        return;
    }

    free(UsermodeMessageRequest);
}

/**
 * @brief Send each packet of debugging information (PDB) to the debugger
 * @param SymbolDetailPacket
 * @param CurrentSymbolInfoIndex
 * @param TotalSymbols
 *
 * @return VOID
 */
VOID
KdSendSymbolDetailPacket(PMODULE_SYMBOL_DETAIL SymbolDetailPacket, UINT32 CurrentSymbolInfoIndex, UINT32 TotalSymbols)
{
    BOOLEAN                       Result;
    ULONG                         ReturnedLength;
    PDEBUGGER_UPDATE_SYMBOL_TABLE UsermodeSymDetailRequest;

    UsermodeSymDetailRequest =
        (DEBUGGER_UPDATE_SYMBOL_TABLE *)malloc(sizeof(DEBUGGER_UPDATE_SYMBOL_TABLE));

    RtlZeroMemory(UsermodeSymDetailRequest, sizeof(DEBUGGER_UPDATE_SYMBOL_TABLE));

    //
    // Set other parameters for the symbol details
    //
    UsermodeSymDetailRequest->CurrentSymbolIndex = CurrentSymbolInfoIndex;
    UsermodeSymDetailRequest->TotalSymbols       = TotalSymbols;

    //
    // Move the user message buffer at the bottom of the structure packet
    //
    memcpy((PVOID)((UINT64)&UsermodeSymDetailRequest->SymbolDetailPacket),
           (PVOID)SymbolDetailPacket,
           sizeof(MODULE_SYMBOL_DETAIL));

    //
    // Send the symbol update buffer to the debugger
    //
    Result = KdSendGeneralBuffersFromDebuggeeToDebugger(
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_UPDATE_SYMBOL_INFO,
        UsermodeSymDetailRequest,
        sizeof(DEBUGGER_UPDATE_SYMBOL_TABLE),
        FALSE);

    if (!Result)
    {
        ShowMessages("err, sending symbol packets failed in debuggee");
        free(UsermodeSymDetailRequest);
        return;
    }

    free(UsermodeSymDetailRequest);
}

/**
 * @brief Uninitialize everything in both debuggee and debugger
 * @details This function close the connection in both debuggee and debugger
 * both of the debuggee and debugger can use this function
 *
 * @return VOID
 */
VOID
KdUninitializeConnection()
{
    //
    // Close the handles of the listening threads
    //
    if (g_SerialListeningThreadHandle != NULL)
    {
        CloseHandle(g_SerialListeningThreadHandle);
        g_SerialListeningThreadHandle = NULL;
    }

    if (g_OverlappedIoStructureForReadDebugger.hEvent != NULL)
    {
        CloseHandle(g_OverlappedIoStructureForReadDebugger.hEvent);
    }

    if (g_OverlappedIoStructureForWriteDebugger.hEvent != NULL)
    {
        CloseHandle(g_OverlappedIoStructureForWriteDebugger.hEvent);
    }

    if (g_DebuggeeStopCommandEventHandle != NULL)
    {
        //
        // Signal the debuggee to get new commands
        //
        SetEvent(g_DebuggeeStopCommandEventHandle);
    }

    //
    // Unpause the debugger to get commands
    //
    DbgReceivedKernelResponse(DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_IS_DEBUGGER_RUNNING);

    //
    // Close synchronization objects
    //
    for (size_t i = 0; i < DEBUGGER_MAXIMUM_SYNCRONIZATION_KERNEL_DEBUGGER_OBJECTS; i++)
    {
        if (g_KernelSyncronizationObjectsHandleTable[i].EventHandle != NULL)
        {
            if (g_KernelSyncronizationObjectsHandleTable[i].IsOnWaitingState)
            {
                DbgReceivedKernelResponse(i);
            }

            CloseHandle(g_KernelSyncronizationObjectsHandleTable[i].EventHandle);
            g_KernelSyncronizationObjectsHandleTable[i].EventHandle = NULL;
        }
    }

    //
    // Unallocate symbol data
    //
    SymbolDeleteSymTable();

    //
    // No current core
    //
    g_CurrentRemoteCore = DEBUGGER_DEBUGGEE_IS_RUNNING_NO_CORE;

    //
    // If connected to debugger
    //
    g_IsSerialConnectedToRemoteDebugger = FALSE;

    //
    // No longer connected to the debuggee
    //
    g_IsSerialConnectedToRemoteDebuggee = FALSE;

    //
    // And debuggee is not running
    //
    g_IsDebuggeeRunning = FALSE;

    //
    // Clear the handler
    //
    if (g_SerialRemoteComPortHandle != NULL)
    {
        CloseHandle(g_SerialRemoteComPortHandle);
        g_SerialRemoteComPortHandle = NULL;
    }

    //
    // Start getting debuggee messages on next try
    //
    g_IgnoreNewLoggingMessages = FALSE;

    //
    // Is serial handle for a named pipe
    //
    g_IsDebuggerConntectedToNamedPipe = FALSE;
}
