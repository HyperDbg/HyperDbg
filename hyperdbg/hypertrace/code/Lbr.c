/**
 * @file Lbr.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Message logging and tracing implementation
 * @details Modified from LIBIHT project (Thomasaon Zhao et al) with Windows style updates.
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#include "pch.h"

//////////////////////////////////////////////////
//             Global Definitions               //
//////////////////////////////////////////////////

ULONGLONG  LbrCapacity = 0;
LIST_ENTRY LbrStateHead;
KSPIN_LOCK LbrStateLock;

//
// Typical Intel LBR capacities based on CPU model
// This is a subset; you can expand this as needed
//
CPU_LBR_MAP CPU_LBR_MAPS[] = {
    {0x5c, 32},
    {0x5f, 32},
    {0x4e, 32},
    {0x5e, 32},
    {0x8e, 32},
    {0x9e, 32},
    {0x55, 32},
    {0x66, 32},
    {0x7a, 32},
    {0x67, 32},
    {0x6a, 32},
    {0x6c, 32},
    {0x7d, 32},
    {0x7e, 32},
    {0x8c, 32},
    {0x8d, 32},
    {0xa5, 32},
    {0xa6, 32},
    {0xa7, 32},
    {0xa8, 32},
    {0x86, 32},
    {0x8a, 32},
    {0x96, 32},
    {0x9c, 32},
    {0x3d, 16},
    {0x47, 16},
    {0x4f, 16},
    {0x56, 16},
    {0x3c, 16},
    {0x45, 16},
    {0x46, 16},
    {0x3f, 16},
    {0x2a, 16},
    {0x2d, 16},
    {0x3a, 16},
    {0x3e, 16},
    {0x1a, 16},
    {0x1e, 16},
    {0x1f, 16},
    {0x2e, 16},
    {0x25, 16},
    {0x2c, 16},
    {0x2f, 16},
    {0x17, 4},
    {0x1d, 4},
    {0x0f, 4},
    {0x37, 8},
    {0x4a, 8},
    {0x4c, 8},
    {0x4d, 8},
    {0x5a, 8},
    {0x5d, 8},
    {0x1c, 8},
    {0x26, 8},
    {0x27, 8},
    {0x35, 8},
    {0x36, 8}};

/**
 * @brief Read LBR MSRs and store the values in the provided LBR_STATE structure
 *
 * @param State
 * @return VOID
 */
VOID
LbrGetLbr(LBR_STATE * State)
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);

    xacquire_lock(LbrStateLock, &OldIrql);
    xrdmsr(MSR_LBR_SELECT, &State->Config.LbrSelect);
    xrdmsr(MSR_LBR_TOS, &State->Data->LbrTos);

    for (i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xrdmsr(MSR_LBR_NHM_FROM + i, &State->Data->Entries[i].From);
        xrdmsr(MSR_LBR_NHM_TO + i, &State->Data->Entries[i].To);
    }
    xrelease_lock(&LbrStateLock, &OldIrql);
}

/**
 * @brief Write LBR MSRs from the provided LBR_STATE structure
 *
 * @param State
 * @return VOID
 */
VOID
LbrPutLbr(LBR_STATE * State)
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xacquire_lock(&LbrStateLock, &OldIrql);
    xwrmsr(MSR_LBR_SELECT, State->Config.LbrSelect);
    xwrmsr(MSR_LBR_TOS, State->Data->LbrTos);

    for (i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, State->Data->Entries[i].From);
        xwrmsr(MSR_LBR_NHM_TO + i, State->Data->Entries[i].To);
    }
    xrelease_lock(&LbrStateLock, &OldIrql);

    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr |= DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);
}

/**
 * @brief Flush LBR MSRs by disabling LBR and clearing all LBR entries
 *
 * @return VOID
 */
VOID
LbrFlushLbr()
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xlock_core(&OldIrql);

    //
    // Disable LBR
    //
    LogInfo("LIBIHT-COM: Flush LBR on cpu core: %d\n", xcoreid());
    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);

    //
    // Flush LBR registers
    //
    xwrmsr(MSR_LBR_SELECT, 0);
    xwrmsr(MSR_LBR_TOS, 0);

    for (i = 0; i < LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }

    xrelease_core(&OldIrql);
}

/**
 * @brief Enable LBR for a specific process and store the configuration in the global LBR state list
 *
 * @param Request
 * @return BOOLEAN
 */
BOOLEAN
LbrEnableLbr(LBR_IOCTL_REQUEST * Request)
{
    LBR_STATE * State;

    State = LbrFindLbrState(Request->LbrConfig.Pid);
    if (State)
    {
        LogInfo("LIBIHT-COM: LBR already enabled for pid %d\n",
                Request->LbrConfig.Pid);
        return FALSE;
    }

    State = LbrCreateLbrState();
    if (State == NULL)
    {
        LogInfo("LIBIHT-COM: Create LBR state failed\n");
        return FALSE;
    }

    //
    // Setup config fields for LBR state
    //
    State->Parent           = NULL;
    State->Config.Pid       = Request->LbrConfig.Pid ? Request->LbrConfig.Pid : xgetcurrent_pid();
    State->Config.LbrSelect = Request->LbrConfig.LbrSelect ? Request->LbrConfig.LbrSelect : LBR_SELECT;
    LbrInsertLbrState(State);

    //
    // If the requesting process is the current process, trace it right away
    //
    if (State->Config.Pid == xgetcurrent_pid())
        LbrPutLbr(State);

    return TRUE;
}

/**
 * @brief Disable LBR for a specific process and remove the corresponding LBR state from the global list
 *
 * @param Request
 * @return BOOLEAN
 */
BOOLEAN
LbrDisableLbr(LBR_IOCTL_REQUEST * Request)
{
    LBR_STATE * State;

    State = LbrFindLbrState(Request->LbrConfig.Pid);
    if (State == NULL)
    {
        LogInfo("LIBIHT-COM: LBR not enabled for pid %d\n",
                Request->LbrConfig.Pid);
        return FALSE;
    }

    if (State->Config.Pid == xgetcurrent_pid())
    {
        LbrGetLbr(State);
    }

    LbrRemoveLbrState(State);

    return TRUE;
}

/**
 * @brief Dump LBR info for a specific process to debug logs and optionally copy the LBR data to user buffer
 *
 * @param Request
 * @return BOOLEAN
 */
BOOLEAN
LbrDumpLbr(LBR_IOCTL_REQUEST * Request)
{
    ULONGLONG   i, BytesLeft;
    LBR_STATE * State;
    LBR_DATA    ReqBuf;
    KIRQL       OldIrql;

    State = LbrFindLbrState(Request->LbrConfig.Pid);

    if (State == NULL)
    {
        LogInfo("LIBIHT-COM: LBR not enabled for pid %d\n",
                Request->LbrConfig.Pid);
        return FALSE;
    }

    //
    // Examine if the current process is the owner of the LBR state
    //
    if (State->Config.Pid == xgetcurrent_pid())
    {
        LogInfo("LIBIHT-COM: Dump LBR for current process\n");

        //
        // Get fresh LBR info
        //
        LbrGetLbr(State);
        LbrPutLbr(State);
    }

    xacquire_lock(LbrStateLock, &OldIrql);

    //
    // Dump the LBR state to debug logs
    //
    LogInfo("PROC_PID:             %d\n", State->Config.Pid);
    LogInfo("MSR_LBR_SELECT:       0x%llx\n", State->Config.LbrSelect);
    LogInfo("MSR_LBR_TOS:          %lld\n", State->Data->LbrTos);

    for (i = 0; i < LbrCapacity; i++)
    {
        LogInfo("MSR_LBR_NHM_FROM[%2d]: 0x%llx\n", (ULONG)i, State->Data->Entries[i].From);
        LogInfo("MSR_LBR_NHM_TO  [%2d]: 0x%llx\n", (ULONG)i, State->Data->Entries[i].To);
    }

    LogInfo("LIBIHT-COM: LBR info for cpuid: %d\n", xcoreid());

    //
    // Dump the LBR data to userspace buffer
    //
    if (Request->Buffer)
    {
        BytesLeft = xcopy_from_user(&ReqBuf, Request->Buffer, sizeof(LBR_DATA));
        if (BytesLeft)
        {
            LogInfo("LIBIHT-COM: Copy LBR data from user failed\n");
            xrelease_lock(LbrStateLock, &OldIrql);
            return FALSE;
        }

        ReqBuf.LbrTos = State->Data->LbrTos;
        if (ReqBuf.Entries)
        {
            BytesLeft = xcopy_to_user(ReqBuf.Entries,
                                      State->Data->Entries,
                                      LbrCapacity * sizeof(LBR_STACK_ENTRY));

            if (BytesLeft)
            {
                LogInfo("LIBIHT-COM: Copy LBR data to user failed\n");
                xrelease_lock(LbrStateLock, &OldIrql);
                return FALSE;
            }
        }

        BytesLeft = xcopy_to_user(Request->Buffer, &ReqBuf, sizeof(LBR_DATA));
        if (BytesLeft)
        {
            LogInfo("LIBIHT-COM: Copy LBR data to user failed\n");
            xrelease_lock(LbrStateLock, &OldIrql);
            return FALSE;
        }
    }

    xrelease_lock(LbrStateLock, &OldIrql);
    return TRUE;
}

/**
 * @brief Update LBR configuration for a specific process and optionally refresh the LBR MSRs if the current process is the owner
 *
 * @param Request
 * @return BOOLEAN
 */
BOOLEAN
LbrConfigLbr(LBR_IOCTL_REQUEST * Request)
{
    LBR_STATE * State;

    State = LbrFindLbrState(Request->LbrConfig.Pid);

    if (State == NULL)
    {
        LogInfo("LIBIHT-COM: LBR not enabled for pid %d\n",
                Request->LbrConfig.Pid);
        return FALSE;
    }

    if (State->Config.Pid == xgetcurrent_pid())
    {
        LbrGetLbr(State);
        State->Config.LbrSelect = Request->LbrConfig.LbrSelect;
        LbrPutLbr(State);
    }
    else
    {
        State->Config.LbrSelect = Request->LbrConfig.LbrSelect;
    }

    return TRUE;
}

/**
 * @brief Create a new LBR_STATE structure with allocated memory for LBR data and entries
 *
 * @return LBR_STATE*
 */
LBR_STATE *
LbrCreateLbrState()
{
    LBR_STATE *       State;
    LBR_DATA *        Data;
    LBR_STACK_ENTRY * Entries;

    State = xmalloc(sizeof(LBR_STATE));
    if (State == NULL)
        return NULL;

    Data = xmalloc(sizeof(LBR_DATA));
    if (Data == NULL)
    {
        xfree(State);
        return NULL;
    }
    SIZE_T TotalEntrySize = (SIZE_T)sizeof(LBR_STACK_ENTRY) * LbrCapacity;
    Entries               = xmalloc(TotalEntrySize);
    if (Entries == NULL)
    {
        xfree(Data);
        xfree(State);
        return NULL;
    }

    xmemset(State, sizeof(LBR_STATE));
    xmemset(Data, sizeof(LBR_DATA));
    xmemset(Entries, TotalEntrySize);

    State->Data   = Data;
    Data->Entries = Entries;

    return State;
}

/**
 * @brief Find the LBR_STATE structure for a specific process ID from the global list
 *
 * @param Pid
 * @return LBR_STATE*
 */
LBR_STATE *
LbrFindLbrState(ULONG Pid)
{
    KIRQL       OldIrql;
    LBR_STATE * RetState = NULL;
    PLIST_ENTRY Link;

    xacquire_lock(&LbrStateLock, &OldIrql);

    //
    // Iterating through LIST_ENTRY correctly
    //
    for (Link = LbrStateHead.Flink; Link != &LbrStateHead; Link = Link->Flink)
    {
        LBR_STATE * Curr = CONTAINING_RECORD(Link, LBR_STATE, List);
        if (Pid != 0 && Curr->Config.Pid == Pid)
        {
            RetState = Curr;
            break;
        }
    }

    xrelease_lock(&LbrStateLock, &OldIrql);
    return RetState;
}

/**
 * @brief Insert a new LBR_STATE structure into the global list with proper locking
 *
 * @param NewState
 * @return VOID
 */
VOID
LbrInsertLbrState(LBR_STATE * NewState)
{
    KIRQL OldIrql;

    if (NewState == NULL)
        return;

    xacquire_lock(LbrStateLock, &OldIrql);
    LogInfo("LIBIHT-COM: Insert LBR state for pid %d\n", NewState->Config.Pid);
    xlist_add(NewState->List, LbrStateHead);
    xrelease_lock(LbrStateLock, &OldIrql);
}

/**
 * @brief Remove an existing LBR_STATE structure from the global list and free its associated memory with proper locking
 *
 * @param OldState
 * @return VOID
 */
VOID
LbrRemoveLbrState(LBR_STATE * OldState)
{
    KIRQL OldIrql;

    if (OldState == NULL)
        return;

    xacquire_lock(LbrStateLock, &OldIrql);
    LogInfo("LIBIHT-COM: Remove LBR state for pid %d\n", OldState->Config.Pid);

    xlist_del(OldState->List);
    xfree(OldState->Data->Entries);
    xfree(OldState->Data);
    xfree(OldState);

    xrelease_lock(LbrStateLock, &OldIrql);
}

/**
 * @brief Free all LBR_STATE structures in the global list and their associated memory with proper locking
 *
 * @return VOID
 */
VOID
LbrFreeLbrStatList()
{
    KIRQL       OldIrql;
    LBR_STATE * CurrState;
    PLIST_ENTRY CurrLink;

    xacquire_lock(&LbrStateLock, &OldIrql);

    CurrLink = LbrStateHead.Flink;
    while (CurrLink != &LbrStateHead)
    {
        CurrState = CONTAINING_RECORD(CurrLink, LBR_STATE, List);
        CurrLink  = CurrLink->Flink; // Get next before deleting

        xlist_del(CurrState->List);
        xfree(CurrState->Data->Entries);
        xfree(CurrState->Data);
        xfree(CurrState);
    }

    xrelease_lock(&LbrStateLock, &OldIrql);
}

/**
 * @brief Handle IOCTL requests for LBR operations by dispatching to the appropriate function based on the command
 *
 * @param Request
 * @return BOOLEAN
 */
BOOLEAN
LbrIoctlHandler(XIOCTL_REQUEST * Request)
{
    BOOLEAN Status = TRUE;

    LogInfo("LIBIHT-COM: LBR ioctl command %d.\n", Request->Cmd);
    switch (Request->Cmd)
    {
    case LIBIHT_IOCTL_ENABLE_LBR:
        Status = LbrEnableLbr(&Request->Body.Lbr);
        break;
    case LIBIHT_IOCTL_DISABLE_LBR:
        Status = LbrDisableLbr(&Request->Body.Lbr);
        break;
    case LIBIHT_IOCTL_DUMP_LBR:
        Status = LbrDumpLbr(&Request->Body.Lbr);
        break;
    case LIBIHT_IOCTL_CONFIG_LBR:
        Status = LbrConfigLbr(&Request->Body.Lbr);
        break;
    default:
        LogInfo("LIBIHT-COM: Invalid LBR ioctl command\n");
        Status = FALSE;
        break;
    }

    return Status;
}

/**
 * @brief Handle context switch events by saving the LBR state of the previous process and restoring the LBR state of the next process if they are being traced
 *
 * @param PrevPid
 * @param NextPid
 * @return VOID
 */
VOID
LbrCswitchHandler(ULONG PrevPid,
                  ULONG NextPid)
{
    LBR_STATE * PrevState;
    LBR_STATE * NextState;

    PrevState = LbrFindLbrState(PrevPid);
    NextState = LbrFindLbrState(NextPid);

    if (PrevState)
    {
        LogInfo("LIBIHT-COM: LBR context switch from pid %d on cpu core %d\n",
                PrevState->Config.Pid,
                xcoreid());
        LbrGetLbr(PrevState);
    }

    if (NextState)
    {
        LogInfo("LIBIHT-COM: LBR context switch to pid %d on cpu core %d\n",
                NextState->Config.Pid,
                xcoreid());
        LbrPutLbr(NextState);
    }
}

/**
 * @brief Handle new process creation events by inheriting the LBR state from the parent process if it is being traced
 *
 * @param ParentPid
 * @param ChildPid
 * @return VOID
 */
VOID
LbrNewprocHandler(
    ULONG ParentPid,
    ULONG ChildPid)
{
    LBR_STATE *ParentState, *ChildState;
    KIRQL      OldIrql;

    ParentState = LbrFindLbrState(ParentPid);
    if (ParentState == NULL)
        return;

    LogInfo("LIBIHT-COM: LBR new child process pid %d, parent pid %d\n",
            ChildPid,
            ParentPid);

    ChildState = LbrCreateLbrState();
    if (ChildState == NULL)
        return;

    xacquire_lock(LbrStateLock, &OldIrql);
    ChildState->Parent           = ParentState;
    ChildState->Config.Pid       = ChildPid;
    ChildState->Config.LbrSelect = ParentState->Config.LbrSelect;
    xmemcpy(ChildState->Data,
            ParentState->Data,
            sizeof(LBR_DATA) + LbrCapacity * sizeof(LBR_STACK_ENTRY));
    xrelease_lock(LbrStateLock, &OldIrql);

    LbrInsertLbrState(ChildState);

    if (ChildPid == xgetcurrent_pid())
        LbrPutLbr(ChildState);
}

/**
 * @brief Check if the current CPU supports LBR by examining the CPU family and model and looking up the corresponding LBR capacity
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrCheck()
{
    ULONG     a, b, c, d;
    ULONG     Family, Model;
    ULONGLONG i;

    xcpuid(1, &a, &b, &c, &d);

    Family = ((a >> 8) & 0xF) + ((a >> 20) & 0xFF);
    Model  = ((a >> 4) & 0xF) | ((a >> 12) & 0xF0);

    for (i = 0; i < sizeof(CPU_LBR_MAPS) / sizeof(CPU_LBR_MAPS[0]); ++i)
    {
        if (Model == CPU_LBR_MAPS[i].Model)
        {
            LbrCapacity = CPU_LBR_MAPS[i].LbrCapacity;
            break;
        }
    }

    if (LbrCapacity == 0)
    {
        return FALSE;
    }

    return TRUE;
}
