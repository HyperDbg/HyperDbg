/**
 * @file pe-image-reader.h
 * @author jtaw5649
 * @brief Bounded in-memory Portable Executable reader
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/*
typedef struct _IMAGE_DOS_HEADER
{                    // DOS .EXE header
    WORD e_magic;    // Magic number
    WORD e_cblp;     // Bytes on last page of file
    WORD e_cp;       // Pages in file
    WORD e_crlc;     // Relocations
    WORD e_cparhdr;  // Size of header in paragraphs
    WORD e_minalloc; // Minimum extra paragraphs needed
    WORD e_maxalloc; // Maximum extra paragraphs needed
    WORD e_ss;       // Initial (relative) SS value
    WORD e_sp;       // Initial SP value
    WORD e_csum;     // Checksum
    WORD e_ip;       // Initial IP value
    WORD e_cs;       // Initial (relative) CS value
    WORD e_lfarlc;   // File address of relocation table
    WORD e_ovno;     // Overlay number
    WORD e_res[4];   // Reserved words
    WORD e_oemid;    // OEM identifier (for e_oeminfo)
    WORD e_oeminfo;  // OEM information; e_oemid specific
    WORD e_res2[10]; // Reserved words
    LONG e_lfanew;   // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER
{
    WORD  Machine;
    WORD  NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD  SizeOfOptionalHeader;
    WORD  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER
{
    BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
    union
    {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD  NumberOfRelocations;
    WORD  NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

*/

typedef struct _PE_IMAGE_READER
{
    const BYTE *                 ImageBase;
    SIZE_T                       ImageSize;
    const IMAGE_DOS_HEADER *     DosHeader;
    const BYTE *                 NtHeaders;
    const IMAGE_FILE_HEADER *    FileHeader;
    const IMAGE_SECTION_HEADER * SectionHeaders;
    WORD                         OptionalHeaderMagic;
    BOOLEAN                      Is32Bit;
} PE_IMAGE_READER, *PPE_IMAGE_READER;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
PeImageReaderInitialize(const BYTE * ImageBase, SIZE_T ImageSize, PPE_IMAGE_READER Reader);

BOOLEAN
PeImageReaderIs32Bit(PPE_IMAGE_READER Reader);

BOOLEAN
PeImageReaderGetPointerAtOffset(PPE_IMAGE_READER Reader, SIZE_T Offset, SIZE_T Length, const BYTE ** Pointer);

BOOLEAN
PeImageReaderGetSectionName(const IMAGE_SECTION_HEADER * SectionHeader, CHAR * NameBuffer, SIZE_T NameBufferSize);

BOOLEAN
PeImageReaderRvaToFileOffset(PPE_IMAGE_READER Reader, DWORD Rva, DWORD Length, PSIZE_T FileOffset);
