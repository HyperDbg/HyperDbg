/**
 * @file nt-list.h
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief Cross-platform NT-style intrusive doubly-linked list helpers + CONTAINING_RECORD
 * @details The shared debugger code uses the NT LIST_ENTRY API (InitializeListHead,
 *          InsertHeadList, RemoveEntryList, CONTAINING_RECORD, ...). On Windows these
 *          come from <windows.h> (or, under USE_NATIVE_SDK_HEADERS, from the in-tree
 *          platform/user/header/Windows.h). Platforms whose system headers don't ship
 *          them (Linux) get them here, so the shared code compiles unchanged.
 *
 *          These operate on the exact LIST_ENTRY {Flink, Blink} layout defined in
 *          SDK/headers/DataTypes.h, which is part of the kernel<->user IOCTL ABI --
 *          that's why we mirror the NT list rather than reaching for sys/queue.h
 *          (different layout + a colliding LIST_ENTRY name).
 *
 *          The body is compiled only where the OS headers don't already provide these
 *          (i.e. not on Windows), so including this unconditionally is safe.
 *
 * @version 0.1
 * @date 2026-06-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef _WIN32

#    include <stddef.h> // offsetof

//
// Given the address of a LIST_ENTRY field embedded in a struct, recover the
// address of the containing struct.
//
#    ifndef CONTAINING_RECORD
#        define CONTAINING_RECORD(address, type, field) \
            ((type *)((char *)(address) - offsetof(type, field)))
#    endif

//
// Initialize a list head to the empty (points-to-self) state.
//
static inline void
InitializeListHead(PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

//
// TRUE if the list has no entries.
//
static inline BOOLEAN
IsListEmpty(const LIST_ENTRY * ListHead)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

//
// Unlink an entry. Returns TRUE if the list is now empty.
//
static inline BOOLEAN
RemoveEntryList(PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink = Entry->Flink;
    PLIST_ENTRY Blink = Entry->Blink;

    Blink->Flink = Flink;
    Flink->Blink = Blink;

    return (BOOLEAN)(Flink == Blink);
}

//
// Unlink and return the first entry.
//
static inline PLIST_ENTRY
RemoveHeadList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Entry = ListHead->Flink;
    PLIST_ENTRY Flink = Entry->Flink;

    ListHead->Flink = Flink;
    Flink->Blink    = ListHead;

    return Entry;
}

//
// Unlink and return the last entry.
//
static inline PLIST_ENTRY
RemoveTailList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Entry = ListHead->Blink;
    PLIST_ENTRY Blink = Entry->Blink;

    ListHead->Blink = Blink;
    Blink->Flink    = ListHead;

    return Entry;
}

//
// Insert an entry at the tail of the list.
//
static inline void
InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink = ListHead->Blink;

    Entry->Flink    = ListHead;
    Entry->Blink    = Blink;
    Blink->Flink    = Entry;
    ListHead->Blink = Entry;
}

//
// Insert an entry at the head of the list.
//
static inline void
InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink = ListHead->Flink;

    Entry->Flink    = Flink;
    Entry->Blink    = ListHead;
    Flink->Blink    = Entry;
    ListHead->Flink = Entry;
}

//
// Splice the entries of ListToAppend onto the tail of ListHead.
//
static inline void
AppendTailList(PLIST_ENTRY ListHead, PLIST_ENTRY ListToAppend)
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    ListHead->Blink->Flink     = ListToAppend;
    ListHead->Blink            = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink        = ListEnd;
}

#endif // !_WIN32
