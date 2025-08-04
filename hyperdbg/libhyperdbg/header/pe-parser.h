/**
 * @file pe-parser.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for Portable Executable parser
 * @details
 * @version 0.1
 * @date 2021-12-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					  Functions                 //
//////////////////////////////////////////////////

BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile, const CHAR * SectionToShow, BOOLEAN Is32Bit);

BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit);

UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName);
int
FindRichHeader(PIMAGE_DOS_HEADER dosHeader, char key[5]);

void 
SetRichEntries(int richHeaderSize, char* richHeaderPtr);

void 
FindRichEntries(char* richHeaderPtr, int richHeaderSize, char key[]);

int 
DecryptRichHeader(char key[], int index, char* dataPtr);


typedef struct __RICH_HEADER_INFO {
    int size;
    char* ptrToBuffer;
    int entries;
} RICH_HEADER_INFO, * PRICH_HEADER_INFO;

typedef struct __RICH_HEADER_ENTRY {
    WORD prodID;
    WORD buildID;
    DWORD useCount;
} RICH_HEADER_ENTRY, * PRICH_HEADER_ENTRY;

typedef struct __RICH_HEADER {
    PRICH_HEADER_ENTRY entries;
} RICH_HEADER, * PRICH_HEADER;

RICH_HEADER_INFO PEFILE_RICH_HEADER_INFO;
RICH_HEADER PEFILE_RICH_HEADER;