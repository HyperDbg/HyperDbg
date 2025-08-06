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
//				  Structures                    //
//////////////////////////////////////////////////

typedef struct _RICH_HEADER_INFO
{
    int    Size;
    char * PtrToBuffer;
    int    Entries;
} RICH_HEADER_INFO, *PRICH_HEADER_INFO;

typedef struct _RICH_HEADER_ENTRY
{
    WORD  ProdID;
    WORD  BuildID;
    DWORD UseCount;
} RICH_HEADER_ENTRY, *PRICH_HEADER_ENTRY;

typedef struct _RICH_HEADER
{
    PRICH_HEADER_ENTRY Entries;
} RICH_HEADER, *PRICH_HEADER;

//////////////////////////////////////////////////
//					  Functions                 //
//////////////////////////////////////////////////

BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile, const CHAR * SectionToShow, BOOLEAN Is32Bit);

BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit);

UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName);

INT
FindRichHeader(PIMAGE_DOS_HEADER DosHeader, CHAR Key[]);

VOID
SetRichEntries(INT RichHeaderSize, CHAR * RichHeaderPtr);

VOID
FindRichEntries(CHAR * RichHeaderPtr, INT RichHeaderSize, CHAR Key[]);

INT
DecryptRichHeader(CHAR Key[], INT Index, CHAR * DataPtr);
