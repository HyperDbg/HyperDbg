/**
 * @file list.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The list working functions headers
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#define CONTAINING_RECORD(address, type, field) \
    ((type *)((char *)(address) - (unsigned long)(&((type *)0)->field)))

//////////////////////////////////////////
//				Functions 		     	//
//////////////////////////////////////////

void
InitializeListHead(PLIST_ENTRY ListHead);
void
InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
BOOLEAN
RemoveEntryList(PLIST_ENTRY Entry);
