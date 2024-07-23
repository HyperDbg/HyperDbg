/**
 * @file Windows.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Windows specific headers
 * @details
 * @version 0.10
 * @date 2024-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Definitions	        		//
//////////////////////////////////////////////////

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    PVOID  Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    _Field_size_(NumberOfModules) RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// Linked lists

FORCEINLINE VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

_Check_return_
FORCEINLINE BOOLEAN
IsListEmpty(
    _In_ PLIST_ENTRY ListHead)
{
    return ListHead->Flink == ListHead;
}

FORCEINLINE BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink        = Entry->Flink;
    Blink        = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;

    return Flink == Blink;
}

FORCEINLINE PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry           = ListHead->Flink;
    Flink           = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink    = ListHead;

    return Entry;
}

FORCEINLINE PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry           = ListHead->Blink;
    Blink           = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink    = ListHead;

    return Entry;
}

FORCEINLINE VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;

    Blink           = ListHead->Blink;
    Entry->Flink    = ListHead;
    Entry->Blink    = Blink;
    Blink->Flink    = Entry;
    ListHead->Blink = Entry;
}

FORCEINLINE VOID
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink;

    Flink           = ListHead->Flink;
    Entry->Flink    = Flink;
    Entry->Blink    = ListHead;
    Flink->Blink    = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE VOID
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend)
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    ListHead->Blink->Flink     = ListToAppend;
    ListHead->Blink            = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink        = ListEnd;
}

FORCEINLINE PSINGLE_LIST_ENTRY
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead)
{
    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;

    if (FirstEntry)
        ListHead->Next = FirstEntry->Next;

    return FirstEntry;
}

FORCEINLINE VOID
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ PSINGLE_LIST_ENTRY Entry)
{
    Entry->Next    = ListHead->Next;
    ListHead->Next = Entry;
}

//
// MessageId: STATUS_UNSUCCESSFUL
//
// MessageText:
//
// {Operation Failed}
// The requested operation was unsuccessful.
//
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

typedef enum _SYSTEM_INFORMATION_CLASS2
{
    SystemModuleInformation = 11 // q: RTL_PROCESS_MODULES

} SYSTEM_INFORMATION_CLASS2;
