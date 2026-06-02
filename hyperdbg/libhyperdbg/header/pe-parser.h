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
//			  	  Structures                    //
//////////////////////////////////////////////////

typedef struct _RICH_HEADER_INFO
{
    int    Size;
    CHAR * PtrToBuffer;
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

typedef struct _PE_RAW_SECTION_RANGE
{
    ULONGLONG                    Start;
    ULONGLONG                    End;
    const IMAGE_SECTION_HEADER * Section;
} PE_RAW_SECTION_RANGE, *PPE_RAW_SECTION_RANGE;

//////////////////////////////////////////////////
//					  Functions                 //
//////////////////////////////////////////////////

BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile,
                                const CHAR *  SectionToShow,
                                BOOLEAN       Is32Bit);

BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit);

UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName);
