/**
 * @file list.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The list working functions headers
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief List initializer
 * 
 * @param ListHead 
 */
void
InitializeListHead(PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

/**
 * @brief insert entry to the top of the list
 * 
 * @param ListHead 
 * @param Entry 
 */
void
InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink;

    Flink           = ListHead->Flink;
    Entry->Flink    = Flink;
    Entry->Blink    = ListHead;
    Flink->Blink    = Entry;
    ListHead->Flink = Entry;
}

/**
 * @brief remove the entry from the list
 * 
 * @param Entry 
 * @return BOOLEAN 
 */
BOOLEAN
RemoveEntryList(PLIST_ENTRY Entry)
{
    PLIST_ENTRY PrevEntry;
    PLIST_ENTRY NextEntry;

    NextEntry = Entry->Flink;
    PrevEntry = Entry->Blink;
    if ((NextEntry->Blink != Entry) || (PrevEntry->Flink != Entry))
    {
        //
        // Error
        //
        _CrtDbgBreak();
    }

    PrevEntry->Flink = NextEntry;
    NextEntry->Blink = PrevEntry;
    return (BOOLEAN)(PrevEntry == NextEntry);
}
