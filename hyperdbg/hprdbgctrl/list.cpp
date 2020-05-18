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

void InitializeListHead(PLIST_ENTRY ListHead) {
  ListHead->Flink = ListHead->Blink = ListHead;
}

void InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry) {
  PLIST_ENTRY Flink;

  Flink = ListHead->Flink;
  Entry->Flink = Flink;
  Entry->Blink = ListHead;
  Flink->Blink = Entry;
  ListHead->Flink = Entry;
}
