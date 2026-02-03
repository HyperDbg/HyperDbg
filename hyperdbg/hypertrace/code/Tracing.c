/**
 * @file Tracing.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Message logging and tracing implementation
 * @details Modified from LIBIHT project (Thomasaon Zhao et al) with Windows style updates.
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#include "pch.h"
#include "Tracing.h"

//////////////////////////////////////////////////
//             Global Definitions               //
//////////////////////////////////////////////////

ULONGLONG  lbr_capacity = 0;
LIST_ENTRY lbr_state_head;
KSPIN_LOCK lbr_state_lock;

// Typical Intel LBR capacities based on CPU model
// This is a subset; you can expand this as needed
struct cpu_lbr_map cpu_lbr_maps[] = {
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

// Note: MAX_IRQL_LEN is removed in favor of native KIRQL.

void
XWriteMsr(
    ULONG     Msr,
    ULONGLONG Val)
{
    __writemsr(Msr, Val);
}

void
LbrGetLbr(struct lbr_state * State)
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);

    xacquire_lock(lbr_state_lock, &OldIrql);
    xrdmsr(MSR_LBR_SELECT, &State->config.lbr_select);
    xrdmsr(MSR_LBR_TOS, &State->data->lbr_tos);

    for (i = 0; i < (ULONG)lbr_capacity; i++)
    {
        xrdmsr(MSR_LBR_NHM_FROM + i, &State->data->entries[i].from);
        xrdmsr(MSR_LBR_NHM_TO + i, &State->data->entries[i].to);
    }
    xrelease_lock(&lbr_state_lock, &OldIrql);
}

void
LbrPutLbr(struct lbr_state * State)
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xacquire_lock(&lbr_state_lock, &OldIrql);
    xwrmsr(MSR_LBR_SELECT, State->config.lbr_select);
    xwrmsr(MSR_LBR_TOS, State->data->lbr_tos);

    for (i = 0; i < (ULONG)lbr_capacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, State->data->entries[i].from);
        xwrmsr(MSR_LBR_NHM_TO + i, State->data->entries[i].to);
    }
    xrelease_lock(&lbr_state_lock, &OldIrql);

    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr |= DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);
}

void
LbrFlushLbr(
    VOID)
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xlock_core(&OldIrql);

    // Disable LBR
    xprintdbg("LIBIHT-COM: Flush LBR on cpu core: %d\n", xcoreid());
    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);

    // Flush LBR registers
    xwrmsr(MSR_LBR_SELECT, 0);
    xwrmsr(MSR_LBR_TOS, 0);

    for (i = 0; i < lbr_capacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }

    xrelease_core(&OldIrql);
}

NTSTATUS
LbrEnableLbr(
    struct lbr_ioctl_request * Request)
{
    struct lbr_state * State;

    State = LbrFindLbrState(Request->lbr_config.pid);
    if (State)
    {
        xprintdbg("LIBIHT-COM: LBR already enabled for pid %d\n",
                  Request->lbr_config.pid);
        return STATUS_ALREADY_REGISTERED;
    }

    State = LbrCreateLbrState();
    if (State == NULL)
    {
        xprintdbg("LIBIHT-COM: Create LBR state failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Setup config fields for LBR state
    State->parent            = NULL;
    State->config.pid        = Request->lbr_config.pid ? Request->lbr_config.pid : xgetcurrent_pid();
    State->config.lbr_select = Request->lbr_config.lbr_select ? Request->lbr_config.lbr_select : LBR_SELECT;
    LbrInsertLbrState(State);

    // If the requesting process is the current process, trace it right away
    if (State->config.pid == xgetcurrent_pid())
        LbrPutLbr(State);

    return STATUS_SUCCESS;
}

NTSTATUS
LbrDisableLbr(
    struct lbr_ioctl_request * Request)
{
    struct lbr_state * State;

    State = LbrFindLbrState(Request->lbr_config.pid);
    if (State == NULL)
    {
        xprintdbg("LIBIHT-COM: LBR not enabled for pid %d\n",
                  Request->lbr_config.pid);
        return STATUS_NOT_FOUND;
    }

    if (State->config.pid == xgetcurrent_pid())
        LbrGetLbr(State);

    LbrRemoveLbrState(State);
    return STATUS_SUCCESS;
}

NTSTATUS
LbrDumpLbr(
    struct lbr_ioctl_request * Request)
{
    ULONGLONG          i, BytesLeft;
    struct lbr_state * State;
    struct lbr_data    ReqBuf;
    KIRQL              OldIrql;

    State = LbrFindLbrState(Request->lbr_config.pid);
    if (State == NULL)
    {
        xprintdbg("LIBIHT-COM: LBR not enabled for pid %d\n",
                  Request->lbr_config.pid);
        return STATUS_NOT_FOUND;
    }

    // Examine if the current process is the owner of the LBR state
    if (State->config.pid == xgetcurrent_pid())
    {
        xprintdbg("LIBIHT-COM: Dump LBR for current process\n");
        // Get fresh LBR info
        LbrGetLbr(State);
        LbrPutLbr(State);
    }

    xacquire_lock(lbr_state_lock, &OldIrql);

    // Dump the LBR state to debug logs
    xprintdbg("PROC_PID:             %d\n", State->config.pid);
    xprintdbg("MSR_LBR_SELECT:       0x%llx\n", State->config.lbr_select);
    xprintdbg("MSR_LBR_TOS:          %lld\n", State->data->lbr_tos);

    for (i = 0; i < lbr_capacity; i++)
    {
        xprintdbg("MSR_LBR_NHM_FROM[%2d]: 0x%llx\n", (ULONG)i, State->data->entries[i].from);
        xprintdbg("MSR_LBR_NHM_TO  [%2d]: 0x%llx\n", (ULONG)i, State->data->entries[i].to);
    }

    xprintdbg("LIBIHT-COM: LBR info for cpuid: %d\n", xcoreid());

    // Dump the LBR data to userspace buffer
    if (Request->buffer)
    {
        BytesLeft = xcopy_from_user(&ReqBuf, Request->buffer, sizeof(struct lbr_data));
        if (BytesLeft)
        {
            xprintdbg("LIBIHT-COM: Copy LBR data from user failed\n");
            xrelease_lock(lbr_state_lock, &OldIrql);
            return STATUS_UNSUCCESSFUL;
        }

        ReqBuf.lbr_tos = State->data->lbr_tos;
        if (ReqBuf.entries)
        {
            BytesLeft = xcopy_to_user(ReqBuf.entries,
                                      State->data->entries,
                                      lbr_capacity * sizeof(struct lbr_stack_entry));

            if (BytesLeft)
            {
                xprintdbg("LIBIHT-COM: Copy LBR data to user failed\n");
                xrelease_lock(lbr_state_lock, &OldIrql);
                return STATUS_UNSUCCESSFUL;
            }
        }

        BytesLeft = xcopy_to_user(Request->buffer, &ReqBuf, sizeof(struct lbr_data));
        if (BytesLeft)
        {
            xprintdbg("LIBIHT-COM: Copy LBR data to user failed\n");
            xrelease_lock(lbr_state_lock, &OldIrql);
            return STATUS_UNSUCCESSFUL;
        }
    }

    xrelease_lock(lbr_state_lock, &OldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
LbrConfigLbr(
    struct lbr_ioctl_request * Request)
{
    struct lbr_state * State;

    State = LbrFindLbrState(Request->lbr_config.pid);
    if (State == NULL)
    {
        xprintdbg("LIBIHT-COM: LBR not enabled for pid %d\n",
                  Request->lbr_config.pid);
        return STATUS_NOT_FOUND;
    }

    if (State->config.pid == xgetcurrent_pid())
    {
        LbrGetLbr(State);
        State->config.lbr_select = Request->lbr_config.lbr_select;
        LbrPutLbr(State);
    }
    else
    {
        State->config.lbr_select = Request->lbr_config.lbr_select;
    }

    return STATUS_SUCCESS;
}

struct lbr_state *
LbrCreateLbrState(
    VOID)
{
    struct lbr_state *       State;
    struct lbr_data *        Data;
    struct lbr_stack_entry * Entries;

    State = xmalloc(sizeof(struct lbr_state));
    if (State == NULL)
        return NULL;

    Data = xmalloc(sizeof(struct lbr_data));
    if (Data == NULL)
    {
        xfree(State);
        return NULL;
    }
    SIZE_T TotalEntrySize = (SIZE_T)sizeof(struct lbr_stack_entry) * lbr_capacity;
    Entries               = xmalloc(TotalEntrySize);
    if (Entries == NULL)
    {
        xfree(Data);
        xfree(State);
        return NULL;
    }

    xmemset(State, sizeof(struct lbr_state));
    xmemset(Data, sizeof(struct lbr_data));
    xmemset(Entries, TotalEntrySize);

    State->data   = Data;
    Data->entries = Entries;

    return State;
}

struct lbr_state *
LbrFindLbrState(ULONG Pid)
{
    KIRQL              OldIrql;
    struct lbr_state * RetState = NULL;
    PLIST_ENTRY        Link;

    xacquire_lock(&lbr_state_lock, &OldIrql);

    // Iterating through LIST_ENTRY correctly
    for (Link = lbr_state_head.Flink; Link != &lbr_state_head; Link = Link->Flink)
    {
        struct lbr_state * Curr = CONTAINING_RECORD(Link, struct lbr_state, list);
        if (Pid != 0 && Curr->config.pid == Pid)
        {
            RetState = Curr;
            break;
        }
    }

    xrelease_lock(&lbr_state_lock, &OldIrql);
    return RetState;
}

void
LbrInsertLbrState(
    struct lbr_state * NewState)
{
    KIRQL OldIrql;

    if (NewState == NULL)
        return;

    xacquire_lock(lbr_state_lock, &OldIrql);
    xprintdbg("LIBIHT-COM: Insert LBR state for pid %d\n", NewState->config.pid);
    xlist_add(NewState->list, lbr_state_head);
    xrelease_lock(lbr_state_lock, &OldIrql);
}

void
LbrRemoveLbrState(
    struct lbr_state * OldState)
{
    KIRQL OldIrql;

    if (OldState == NULL)
        return;

    xacquire_lock(lbr_state_lock, &OldIrql);
    xprintdbg("LIBIHT-COM: Remove LBR state for pid %d\n", OldState->config.pid);

    xlist_del(OldState->list);
    xfree(OldState->data->entries);
    xfree(OldState->data);
    xfree(OldState);

    xrelease_lock(lbr_state_lock, &OldIrql);
}

void
LbrFreeLbrStatList(VOID)
{
    KIRQL              OldIrql;
    struct lbr_state * CurrState;
    PLIST_ENTRY        CurrLink;

    xacquire_lock(&lbr_state_lock, &OldIrql);

    CurrLink = lbr_state_head.Flink;
    while (CurrLink != &lbr_state_head)
    {
        CurrState = CONTAINING_RECORD(CurrLink, struct lbr_state, list);
        CurrLink  = CurrLink->Flink; // Get next before deleting

        xlist_del(CurrState->list);
        xfree(CurrState->data->entries);
        xfree(CurrState->data);
        xfree(CurrState);
    }

    xrelease_lock(&lbr_state_lock, &OldIrql);
}

NTSTATUS
LbrIoctlHandler(
    struct xioctl_request * Request)
{
    NTSTATUS Status = STATUS_SUCCESS;

    xprintdbg("LIBIHT-COM: LBR ioctl command %d.\n", Request->cmd);
    switch (Request->cmd)
    {
    case LIBIHT_IOCTL_ENABLE_LBR:
        Status = LbrEnableLbr(&Request->body.lbr);
        break;
    case LIBIHT_IOCTL_DISABLE_LBR:
        Status = LbrDisableLbr(&Request->body.lbr);
        break;
    case LIBIHT_IOCTL_DUMP_LBR:
        Status = LbrDumpLbr(&Request->body.lbr);
        break;
    case LIBIHT_IOCTL_CONFIG_LBR:
        Status = LbrConfigLbr(&Request->body.lbr);
        break;
    default:
        xprintdbg("LIBIHT-COM: Invalid LBR ioctl command\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}

void
LbrCswitchHandler(
    ULONG PrevPid,
    ULONG NextPid)
{
    struct lbr_state *PrevState, *NextState;

    PrevState = LbrFindLbrState(PrevPid);
    NextState = LbrFindLbrState(NextPid);

    if (PrevState)
    {
        xprintdbg("LIBIHT-COM: LBR context switch from pid %d on cpu core %d\n",
                  PrevState->config.pid,
                  xcoreid());
        LbrGetLbr(PrevState);
    }

    if (NextState)
    {
        xprintdbg("LIBIHT-COM: LBR context switch to pid %d on cpu core %d\n",
                  NextState->config.pid,
                  xcoreid());
        LbrPutLbr(NextState);
    }
}

void
LbrNewprocHandler(
    ULONG ParentPid,
    ULONG ChildPid)
{
    struct lbr_state *ParentState, *ChildState;
    KIRQL             OldIrql;

    ParentState = LbrFindLbrState(ParentPid);
    if (ParentState == NULL)
        return;

    xprintdbg("LIBIHT-COM: LBR new child process pid %d, parent pid %d\n",
              ChildPid,
              ParentPid);

    ChildState = LbrCreateLbrState();
    if (ChildState == NULL)
        return;

    xacquire_lock(lbr_state_lock, &OldIrql);
    ChildState->parent            = ParentState;
    ChildState->config.pid        = ChildPid;
    ChildState->config.lbr_select = ParentState->config.lbr_select;
    xmemcpy(ChildState->data,
            ParentState->data,
            sizeof(struct lbr_data) + lbr_capacity * sizeof(struct lbr_stack_entry));
    xrelease_lock(lbr_state_lock, &OldIrql);

    LbrInsertLbrState(ChildState);

    if (ChildPid == xgetcurrent_pid())
        LbrPutLbr(ChildState);
}

NTSTATUS
LbrCheck(VOID)
{
    ULONG     a, b, c, d;
    ULONG     Family, Model;
    ULONGLONG i;

    xcpuid(1, &a, &b, &c, &d);

    Family = ((a >> 8) & 0xF) + ((a >> 20) & 0xFF);
    Model  = ((a >> 4) & 0xF) | ((a >> 12) & 0xF0);

    for (i = 0; i < sizeof(cpu_lbr_maps) / sizeof(cpu_lbr_maps[0]); ++i)
    {
        if (Model == cpu_lbr_maps[i].model)
        {
            lbr_capacity = cpu_lbr_maps[i].lbr_capacity;
            break;
        }
    }

    if (lbr_capacity == 0)
        return STATUS_NOT_SUPPORTED;
    return STATUS_SUCCESS;
}
