/**
 * @file pe-parser.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Portable Executable parser
 * @details
 * @version 0.1
 * @date 2021-12-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Locates the Rich header signature in a PE file
 *
 * The Rich header is an undocumented Microsoft structure embedded in PE files
 * that contains information about the tools and compilers used during the build process.
 * This function searches for the "Rich" signature string within the DOS stub area.
 *
 * @param DosHeader Pointer to the DOS header structure of the PE file
 * @param Key       Output buffer to store the 4-byte XOR key found after "Rich" signature
 *
 * @note The Rich header is located between the DOS header and PE header
 * @note The XOR key is used to decode the actual Rich header entries
 **/
INT
FindRichHeader(PIMAGE_DOS_HEADER DosHeader, CHAR Key[])
{
    //
    // Get base address for offset calculations
    //
    CHAR * BaseAddr = (CHAR *)DosHeader;

    //
    // Get PE header offset - this defines our search boundary
    //
    DWORD Offset = DosHeader->e_lfanew;

    //
    // Search for "Rich" signature
    // We stop 4 bytes before the PE header to avoid reading beyond bounds
    //
    for (DWORD i = 0; i < Offset - 4; ++i)
    {
        //
        // Check for "Rich" signature (4 ASCII bytes)
        //
        if (BaseAddr[i] == 'R' &&
            BaseAddr[i + 1] == 'i' &&
            BaseAddr[i + 2] == 'c' &&
            BaseAddr[i + 3] == 'h')
        {
            //
            // Extract the 4-byte XOR key that immediately follows "Rich"
            //
            memcpy(Key, BaseAddr + i + 4, 4);

            //
            // Return the offset where "Rich" signature was found
            //
            return i;
        }
    }

    //
    // Rich header signature not found
    //
    return 0;
}

/**
 * @brief Decrypts Rich header data using XOR decryption and initializes header info
 *
 * The Rich header is encrypted using a simple XOR cipher with a 4-byte key.
 * This function decrypts the entire header in-place and populates the
 * PEFILE_RICH_HEADER_INFO structure with metadata about the decrypted data.
 *
 * @param RichHeaderPtr Pointer to the raw Rich header data to be decrypted
 * @param RichHeaderSize Size of the Rich header data in bytes
 * @param Key 4-byte XOR key used for decryption
 * @param PeFileRichHeaderInfo structure containing Rich header metadata and buffer information
 *
 * @note The Rich header uses a repeating 4-byte XOR key for encryption
 * @note After decryption, the header contains 16 bytes of metadata followed by 8-byte entries
 * @note Each entry represents one compilation tool (compiler, linker, assembler, etc.)
 *
 */
VOID
FindRichEntries(CHAR *            RichHeaderPtr,
                INT               RichHeaderSize,
                CHAR              Key[],
                PRICH_HEADER_INFO PeFileRichHeaderInfo)
{
    //
    // Decrypt the entire Rich header using XOR with the 4-byte key
    //
    for (int i = 0; i < RichHeaderSize; i += 4)
    {
        //
        // Apply XOR decryption to each 4-byte block
        //
        for (int x = 0; x < 4; x++)
        {
            RichHeaderPtr[i + x] ^= Key[x];
        }
    }

    //
    // Initialize the Rich header info structure
    //
    PeFileRichHeaderInfo->Size        = RichHeaderSize;
    PeFileRichHeaderInfo->PtrToBuffer = RichHeaderPtr;

    //
    // Calculate number of entries: subtract 16-byte header, divide by 8 bytes per entry
    //
    PeFileRichHeaderInfo->Entries = (RichHeaderSize - 16) / 8;
}

/**
 * @brief Parses decrypted Rich header data into structured entries
 *
 * After decryption, the Rich header contains a series of 8-byte entries, each describing
 * a compilation tool used during the build process. This function extracts and converts
 * the little-endian binary data into structured RICH_ENTRY objects.
 *
 * Rich header format after decryption:
 * - Bytes 0-15: Header metadata (DanS signature + padding)
 * - Bytes 16+: 8-byte entries (prodID:2, buildID:2, useCount:4)
 *
 * @param RichHeaderSize Size of the entire Rich header in bytes
 * @param RichHeaderPtr Pointer to the decrypted Rich header data
 * @param PeFileRichHeader structure containing the parsed Rich header entries
 *
 * @warning Assumes the Rich header has been properly decrypted first
 */
VOID
SetRichEntries(INT RichHeaderSize, CHAR * RichHeaderPtr, PRICH_HEADER PeFileRichHeader)
{
    //
    // Start at offset 16 to skip the header metadata, process 8-byte entries
    //
    for (int i = 16; i < RichHeaderSize; i += 8)
    {
        //
        // Extract Product ID (bytes 2-3 of entry, little-endian)
        //
        WORD ProdID = ((unsigned char)RichHeaderPtr[i + 3] << 8) | (unsigned char)RichHeaderPtr[i + 2];

        //
        // Extract Build ID (bytes 0-1 of entry, little-endian)
        //
        WORD BuildID = ((unsigned char)RichHeaderPtr[i + 1] << 8) | (unsigned char)RichHeaderPtr[i];

        //
        // Extract Use Count (bytes 4-7 of entry, little-endian 32-bit)
        //
        DWORD UseCount = ((unsigned char)RichHeaderPtr[i + 7] << 24) |
                         ((unsigned char)RichHeaderPtr[i + 6] << 16) |
                         ((unsigned char)RichHeaderPtr[i + 5] << 8) |
                         (unsigned char)RichHeaderPtr[i + 4];

        //
        // Store the parsed entry (adjust index: i/8 gives entry number, -2 for header offset)
        //
        PeFileRichHeader->Entries[(i / 8) - 2] = {ProdID, BuildID, UseCount};

        //
        // Add null terminator entry if this is the last entry
        //
        if (i + 8 >= RichHeaderSize)
        {
            PeFileRichHeader->Entries[(i / 8) - 1] = {0x0000, 0x0000, 0x00000000};
        }
    }
}

/**
 * @brief Determines the size of the Rich header by finding the DanS signature
 *
 * The Rich header begins with the "DanS" signature (after decryption) and ends
 * with the "Rich" signature. This function works backwards from the "Rich" signature
 * to find the beginning and calculate the total size.
 *
 * Rich header structure:
 * [DanS signature] [Padding] [Tool Entries] [Rich signature] [XOR Key]
 *
 * @param Key 4-byte XOR key for decryption (extracted from after "Rich" signature)
 * @param Index Offset where "Rich" signature was found
 * @param DataPtr Pointer to the beginning of the PE file data
 *
 * @return int Size of the Rich header in bytes, or 0 if DanS signature not found
 *
 */
int
DecryptRichHeader(CHAR Key[], INT Index, CHAR * DataPtr)
{
    //
    // Copy the XOR key from the 4 bytes immediately following "Rich"
    //
    memcpy(Key, DataPtr + (Index + 4), 4);

    //
    // Start searching backwards from just before the "Rich" signature
    //
    int IndexPointer   = Index - 4;
    int RichHeaderSize = 0;

    //
    // Search backwards for the DanS signature that marks the beginning
    //
    while (true)
    {
        char TmpChar[4];

        //
        // Read 4 bytes and decrypt them with the XOR key
        //
        memcpy(TmpChar, DataPtr + IndexPointer, 4);

        for (int i = 0; i < 4; i++)
        {
            TmpChar[i] ^= Key[i];
        }

        //
        // Move backwards and increment size counter
        //
        IndexPointer -= 4;
        RichHeaderSize += 4;

        //
        // Check for DanS signature (0x44='D', 0x61='a' after decryption)
        // Note: Checking bytes 1,0 due to little-endian storage
        //
        if (TmpChar[1] == 0x61 && TmpChar[0] == 0x44)
        {
            break;
        }
    }

    return RichHeaderSize;
}

/**
 * @brief Show hex dump of sections of PE
 * @param Ptr
 * @param Size
 * @param SecAddress
 *
 * @return VOID
 */

VOID
PeHexDump(CHAR * Ptr, int Size, int SecAddress)
{
    int i = 1, Temp = 0;

    //
    // Buffer to store the character dump displayed at the
    // right side
    //
    wchar_t Buf[18];
    ShowMessages("\n\n%x: |", SecAddress);

    Buf[Temp]      = ' '; // initial space
    Buf[Temp + 16] = ' '; // final space
    Buf[Temp + 17] = 0;   // End of Buf
    Temp++;               // Temp = 1;

    for (; i <= Size; i++, Ptr++, Temp++)
    {
        Buf[Temp] = !iswcntrl((*Ptr) & 0xff) ? (*Ptr) & 0xff : '.';
        ShowMessages("%-3.2x", (*Ptr) & 0xff);

        if (i % 16 == 0)
        {
            //
            // print the character dump to the right
            //
            _putws(Buf);
            if (i + 1 <= Size)
                ShowMessages("%x: ", SecAddress += 16);
            Temp = 0;
        }
        if (i % 4 == 0)
            ShowMessages("| ");
    }
    if (i % 16 != 0)
    {
        Buf[Temp] = 0;
        for (; i % 16 != 0; i++)
            ShowMessages("%-3.2c", ' ');
        _putws(Buf);
    }
}

/**
 * @brief Show information about different sections of PE and the dump of sections
 * @param AddressOfFile
 * @param SectionToShow
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile,
                                const CHAR *  SectionToShow,
                                BOOLEAN       Is32Bit)
{
    RICH_HEADER_INFO        PeFileRichHeaderInfo {0};
    RICH_HEADER             PeFileRichHeader {0};
    BOOLEAN                 Result = FALSE, RichFound = FALSE;
    HANDLE                  MapObjectHandle, FileHandle; // File Mapping Object
    UINT32                  NumberOfSections;            // Number of sections
    LPVOID                  BaseAddr;                    // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER       DosHeader;                   // Pointer to DOS Header
    PIMAGE_NT_HEADERS32     NtHeader32 = NULL;           // Pointer to NT Header 32 bit
    PIMAGE_NT_HEADERS64     NtHeader64 = NULL;           // Pointer to NT Header 64 bit
    IMAGE_FILE_HEADER       Header;                      // Pointer to image file header of NT Header
    IMAGE_OPTIONAL_HEADER32 OpHeader32;                  // Optional Header of PE files present in NT Header structure
    IMAGE_OPTIONAL_HEADER64 OpHeader64;                  // Optional Header of PE files present in NT Header structure
    PIMAGE_SECTION_HEADER   SecHeader;                   // Section Header or Section Table Header

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, could not open the file specified\n");
        return FALSE;
    }

    //
    // Mapping Given EXE file to Memory
    //
    MapObjectHandle = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (MapObjectHandle == NULL)
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    BaseAddr = MapViewOfFile(MapObjectHandle, FILE_MAP_READ, 0, 0, 0);

    if (BaseAddr == NULL)
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    // Get the DOS Header Base
    //
    DosHeader = (PIMAGE_DOS_HEADER)BaseAddr; // 0x04000000

    char Key[4];
    int  RichHeaderOffset = FindRichHeader(DosHeader, Key);

    if (RichHeaderOffset != 0)
    {
        char * DataPtr   = new char[DosHeader->e_lfanew];
        DWORD  BytesRead = 0;

        BOOL result = ReadFile(FileHandle, DataPtr, DosHeader->e_lfanew, &BytesRead, NULL);
        if (!result || BytesRead != DosHeader->e_lfanew)
        {
            ShowMessages("ReadFile failed or incomplete read");
        }
        int RichHeaderSize = DecryptRichHeader(Key, RichHeaderOffset, DataPtr);
        int IndexPointer   = RichHeaderOffset - RichHeaderSize;

        char * richHeaderPtr = new char[RichHeaderSize];
        memcpy(richHeaderPtr, DataPtr + IndexPointer, RichHeaderSize);
        delete[] DataPtr;

        FindRichEntries(richHeaderPtr, RichHeaderSize, Key, &PeFileRichHeaderInfo);
        PeFileRichHeader.Entries = new RICH_HEADER_ENTRY[PeFileRichHeaderInfo.Entries];

        SetRichEntries(RichHeaderSize, richHeaderPtr, &PeFileRichHeader);
        RichFound = TRUE;
    }

    //
    // Check for Valid DOS file
    //
    if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
        //
        // Dump the Dos Header info
        //
        ShowMessages("\nValid Dos Exe File\n------------------\n");
        ShowMessages("\nDumping DOS Header Info....\n---------------------------");
        ShowMessages("\n%-36s%s ",
                     "Magic number : ",
                     DosHeader->e_magic == 0x5a4d ? "MZ" : "-");
        ShowMessages("\n%-36s%#x", "Bytes on last page of file :", DosHeader->e_cblp);
        ShowMessages("\n%-36s%#x", "Pages in file : ", DosHeader->e_cp);
        ShowMessages("\n%-36s%#x", "Relocation : ", DosHeader->e_crlc);
        ShowMessages("\n%-36s%#x",
                     "Size of header in paragraphs : ",
                     DosHeader->e_cparhdr);
        ShowMessages("\n%-36s%#x",
                     "Minimum extra paragraphs needed : ",
                     DosHeader->e_minalloc);
        ShowMessages("\n%-36s%#x",
                     "Maximum extra paragraphs needed : ",
                     DosHeader->e_maxalloc);
        ShowMessages("\n%-36s%#x", "Initial (relative) SS value : ", DosHeader->e_ss);
        ShowMessages("\n%-36s%#x", "Initial SP value : ", DosHeader->e_sp);
        ShowMessages("\n%-36s%#x", "Checksum : ", DosHeader->e_csum);
        ShowMessages("\n%-36s%#x", "Initial IP value : ", DosHeader->e_ip);
        ShowMessages("\n%-36s%#x", "Initial (relative) CS value : ", DosHeader->e_cs);
        ShowMessages("\n%-36s%#x",
                     "File address of relocation table : ",
                     DosHeader->e_lfarlc);
        ShowMessages("\n%-36s%#x", "Overlay number : ", DosHeader->e_ovno);
        ShowMessages("\n%-36s%#x", "OEM identifier : ", DosHeader->e_oemid);
        ShowMessages("\n%-36s%#x",
                     "OEM information(e_oemid specific) :",
                     DosHeader->e_oeminfo);
        ShowMessages("\n%-36s%#x", "RVA address of PE header : ", DosHeader->e_lfanew);
        ShowMessages("\n==============================================================="
                     "================\n");
    }
    else
    {
        ShowMessages("\nGiven File is not a valid DOS file\n");
        Result = FALSE;
        goto Finished;
    }

    if (RichFound)
    {
        ShowMessages("\n===============================================================================\n");
        ShowMessages("                              RICH HEADER                                     \n");
        ShowMessages("===============================================================================\n");
        ShowMessages("Entries: %d\n\n", PeFileRichHeaderInfo.Entries);
        ShowMessages("%-10s %-10s %-10s\n", "Build ID", "Prod ID", "Use Count");
        ShowMessages("---------------------------------------\n");

        for (int i = 0; i < PeFileRichHeaderInfo.Entries; i++)
        {
            ShowMessages("0x%08X 0x%08X %10d\n",
                         PeFileRichHeader.Entries[i].BuildID,
                         PeFileRichHeader.Entries[i].ProdID,
                         PeFileRichHeader.Entries[i].UseCount);
        }

        ShowMessages("==============Rich Header End ==================\n");
    }
    else
    {
        ShowMessages("=========== Rich Header Not Found ===========\n");
    }

    //
    // Offset of NT Header is found at 0x3c location in DOS header specified by
    // e_lfanew
    // Get the Base of NT Header(PE Header) 	= DosHeader + RVA address of PE
    // header
    //
    if (Is32Bit)
    {
        NtHeader32 = (PIMAGE_NT_HEADERS32)((UINT64)(DosHeader) + (DosHeader->e_lfanew));
    }
    else
    {
        NtHeader64 = (PIMAGE_NT_HEADERS64)((UINT64)(DosHeader) + (DosHeader->e_lfanew));
    }

    //
    // Identify for valid PE file
    //
    if (Is32Bit && NtHeader32->Signature == IMAGE_NT_SIGNATURE)
    {
        ShowMessages("\nValid PE32 file \n-------------\n");
    }
    else if (!Is32Bit && NtHeader64->Signature == IMAGE_NT_SIGNATURE)
    {
        ShowMessages("\nValid PE64 file \n-------------\n");
    }
    else
    {
        ShowMessages("err, the specified file is not a valid PE file");
        Result = FALSE;
        goto Finished;
    }

    //
    // Dump NT Header Info....
    //
    ShowMessages("\nDumping COFF/PE Header "
                 "Info....\n--------------------------------");
    ShowMessages("\n%-36s%s", "Signature :", "PE");

    //
    // Get the IMAGE FILE HEADER Structure
    //
    if (Is32Bit)
    {
        Header = NtHeader32->FileHeader;
    }
    else
    {
        Header = NtHeader64->FileHeader;
    }

    //
    // Determine Machine Architecture
    //
    ShowMessages("\n%-36s", "Machine Architecture :");

    //
    // Only few are determined (for remaining refer
    // to the above specification)
    //
    switch (Header.Machine)
    {
    case 0x0:
        ShowMessages("All ");
        break;
    case 0x14d:
        ShowMessages("Intel i860");
        break;
    case 0x14c:
        ShowMessages("Intel i386, i486, i586");
        break;
    case 0x200:
        ShowMessages("Intel Itanium processor");
        break;
    case 0x8664:
        ShowMessages("AMD x64");
        break;
    case 0x162:
        ShowMessages("MIPS R3000");
        break;
    case 0x166:
        ShowMessages("MIPS R4000");
        break;
    case 0x183:
        ShowMessages("DEC Alpha AXP");
        break;
    default:
        ShowMessages("Not Found");
        break;
    }

    //
    // Determine the characteristics of the given file
    //
    ShowMessages("\n%-36s", "Characteristics : ");
    if ((Header.Characteristics & 0x0002) == 0x0002)
        ShowMessages("Executable Image, ");
    if ((Header.Characteristics & 0x0020) == 0x0020)
        ShowMessages("Application can address > 2GB, ");
    if ((Header.Characteristics & 0x1000) == 0x1000)
        ShowMessages("System file (Kernel Mode Driver(I think)), ");
    if ((Header.Characteristics & 0x2000) == 0x2000)
        ShowMessages("Dll file, ");
    if ((Header.Characteristics & 0x4000) == 0x4000)
        ShowMessages("Application runs only in Uniprocessor, ");

    //
    // Determine Time Stamp
    //
    ShowMessages("\n%-36s%s",
                 "Time Stamp :",
                 ctime((const time_t *)&(Header.TimeDateStamp)));

    //
    // Determine number of sections
    //
    ShowMessages("%-36s%d", "No.sections(size) :", Header.NumberOfSections);
    ShowMessages("\n%-36s%d", "No.entries in symbol table :", Header.NumberOfSymbols);
    ShowMessages("\n%-36s%d",
                 "Size of optional header :",
                 Header.SizeOfOptionalHeader);

    ShowMessages("\n\nDumping PE Optional Header "
                 "Info....\n-----------------------------------");

    if (Is32Bit)
    {
        //
        // Info about Optional Header
        //
        OpHeader32 = NtHeader32->OptionalHeader;
        ShowMessages("\n\nInfo of optional Header\n-----------------------");
        ShowMessages("\n%-36s%#x",
                     "Address of Entry Point : ",
                     OpHeader32.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader32.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", OpHeader32.Subsystem == 1 ? "Device Driver(Native windows Process)" : OpHeader32.Subsystem == 2 ? "Windows GUI"
                                                                                                                         : OpHeader32.Subsystem == 3   ? "Windows CLI"
                                                                                                                         : OpHeader32.Subsystem == 3   ? "Windows CLI"
                                                                                                                         : OpHeader32.Subsystem == 9   ? "Windows CE GUI"
                                                                                                                                                       : "Unknown");
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader32.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%d", "Size of code segment(.text) : ", OpHeader32.SizeOfCode);
        ShowMessages("\n%-36s%#x",
                     "Base address of code segment(RVA) :",
                     OpHeader32.BaseOfCode);
        ShowMessages("\n%-36s%d",
                     "Size of Initialized data : ",
                     OpHeader32.SizeOfInitializedData);

        ShowMessages("\n%-36s%#x",
                     "Base address of data segment(RVA) :",
                     OpHeader32.BaseOfData);

        ShowMessages("\n%-36s%#x", "Section Alignment :", OpHeader32.SectionAlignment);
        ShowMessages("\n%-36s%d", "Major Linker Version : ", OpHeader32.MajorLinkerVersion);
        ShowMessages("\n%-36s%d", "Minor Linker Version : ", OpHeader32.MinorLinkerVersion);
    }
    else
    {
        //
        // Info about Optional Header
        //
        OpHeader64 = NtHeader64->OptionalHeader;
        ShowMessages("\n\nInfo of optional Header\n-----------------------");
        ShowMessages("\n%-36s%#x",
                     "Address of Entry Point : ",
                     OpHeader64.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader64.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", OpHeader64.Subsystem == 1 ? "Device Driver(Native windows Process)" : OpHeader64.Subsystem == 2 ? "Windows GUI"
                                                                                                                         : OpHeader64.Subsystem == 3   ? "Windows CLI"
                                                                                                                         : OpHeader64.Subsystem == 3   ? "Windows CLI"
                                                                                                                         : OpHeader64.Subsystem == 9   ? "Windows CE GUI"
                                                                                                                                                       : "Unknown");
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader64.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%d", "Size of code segment(.text) : ", OpHeader64.SizeOfCode);
        ShowMessages("\n%-36s%#x",
                     "Base address of code segment(RVA) :",
                     OpHeader64.BaseOfCode);
        ShowMessages("\n%-36s%d",
                     "Size of Initialized data : ",
                     OpHeader64.SizeOfInitializedData);

        ShowMessages("\n%-36s%#x", "Section Alignment :", OpHeader64.SectionAlignment);
        ShowMessages("\n%-36s%d", "Major Linker Version : ", OpHeader64.MajorLinkerVersion);
        ShowMessages("\n%-36s%d", "Minor Linker Version : ", OpHeader64.MinorLinkerVersion);
    }

    ShowMessages("\n\nDumping Sections Header "
                 "Info....\n--------------------------------");

    //
    // Retrieve a pointer to First Section Header(or Section Table Entry)
    //
    if (Is32Bit)
    {
        SecHeader        = IMAGE_FIRST_SECTION(NtHeader32);
        NumberOfSections = NtHeader32->FileHeader.NumberOfSections;
    }
    else
    {
        SecHeader        = IMAGE_FIRST_SECTION(NtHeader64);
        NumberOfSections = NtHeader64->FileHeader.NumberOfSections;
    }

    for (UINT32 i = 0; i < NumberOfSections; i++, SecHeader++)
    {
        if (Is32Bit)
        {
            ShowMessages("\n\nSection Info (%d of %d)", i + 1, NtHeader32->FileHeader.NumberOfSections);
        }
        else
        {
            ShowMessages("\n\nSection Info (%d of %d)", i + 1, NtHeader64->FileHeader.NumberOfSections);
        }

        ShowMessages("\n---------------------");
        ShowMessages("\n%-36s%s", "Section Header name : ", SecHeader->Name);
        ShowMessages("\n%-36s%#x",
                     "ActualSize of code or data : ",
                     SecHeader->Misc.VirtualSize);
        ShowMessages("\n%-36s%#x", "Virtual Address(RVA) :", SecHeader->VirtualAddress);
        ShowMessages("\n%-36s%#x",
                     "Size of raw data (rounded to FA) : ",
                     SecHeader->SizeOfRawData);
        ShowMessages("\n%-36s%#x",
                     "Pointer to Raw Data : ",
                     SecHeader->PointerToRawData);
        ShowMessages("\n%-36s%#x",
                     "Pointer to Relocations : ",
                     SecHeader->PointerToRelocations);
        ShowMessages("\n%-36s%#x",
                     "Pointer to Line numbers : ",
                     SecHeader->PointerToLinenumbers);
        ShowMessages("\n%-36s%#x",
                     "Number of relocations : ",
                     SecHeader->NumberOfRelocations);
        ShowMessages("\n%-36s%#x",
                     "Number of line numbers : ",
                     SecHeader->NumberOfLinenumbers);
        ShowMessages("\n%-36s%s", "Characteristics : ", "Contains ");
        if ((SecHeader->Characteristics & 0x20) == 0x20)
            ShowMessages("executable code, ");
        if ((SecHeader->Characteristics & 0x40) == 0x40)
            ShowMessages("initialized data, ");
        if ((SecHeader->Characteristics & 0x80) == 0x80)
            ShowMessages("uninitialized data, ");
        if ((SecHeader->Characteristics & 0x200) == 0x200)
            ShowMessages("comments and linker commands, ");
        if ((SecHeader->Characteristics & 0x10000000) == 0x10000000)
            ShowMessages("shareable data(via DLLs), ");
        if ((SecHeader->Characteristics & 0x40000000) == 0x40000000)
            ShowMessages("Readable, ");
        if ((SecHeader->Characteristics & 0x80000000) == 0x80000000)
            ShowMessages("Writable, ");

        //
        // show the hex dump if the user needs it
        //
        if (SectionToShow != NULL)
        {
            if (!_strcmpi(SectionToShow, (const char *)SecHeader->Name))
            {
                if (SecHeader->SizeOfRawData != 0)
                {
                    if (Is32Bit)
                    {
                        PeHexDump((char *)((UINT64)DosHeader + SecHeader->PointerToRawData),
                                  SecHeader->SizeOfRawData,
                                  OpHeader32.ImageBase + SecHeader->VirtualAddress);
                    }
                    else
                    {
                        PeHexDump((char *)((UINT64)DosHeader + SecHeader->PointerToRawData),
                                  SecHeader->SizeOfRawData,
                                  (int)(OpHeader64.ImageBase + SecHeader->VirtualAddress));
                    }
                }
            }
        }
    }

    ShowMessages("\n==============================================================="
                 "================\n");

    //
    // Set result to true
    //
    Result = TRUE;

Finished:
    //
    // Unmap and close the handles
    //
    UnmapViewOfFile(BaseAddr);
    CloseHandle(MapObjectHandle);

    return Result;
}

/**
 * @brief Detect whether PE is a 32-bit PE or 64-bit PE
 * @param AddressOfFile
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit)
{
    BOOLEAN                 Result = FALSE;
    HANDLE                  MapObjectHandle, FileHandle; // File Mapping Object
    LPVOID                  BaseAddr;                    // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER       DosHeader;                   // Pointer to DOS Header
    PIMAGE_NT_HEADERS32     NtHeader32 = NULL;           // Pointer to NT Header 32 bit
    IMAGE_OPTIONAL_HEADER32 OpHeader32;                  // Optional Header of PE files present in NT Header structure
    IMAGE_FILE_HEADER       Header;                      // Pointer to image file header of NT Header

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, unable to read the file (%x)\n", GetLastError());
        return FALSE;
    };

    //
    // Mapping Given EXE file to Memory
    //
    MapObjectHandle =
        CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (MapObjectHandle == NULL)
    {
        CloseHandle(FileHandle);

        ShowMessages("err, unable to create file mappings (%x)\n", GetLastError());
        return FALSE;
    }

    BaseAddr = MapViewOfFile(MapObjectHandle, FILE_MAP_READ, 0, 0, 0);

    if (BaseAddr == NULL)
    {
        CloseHandle(FileHandle);

        ShowMessages("err, unable to create map view of file (%x)\n", GetLastError());
        return FALSE;
    }

    //
    // Get the DOS Header Base
    //
    DosHeader = (PIMAGE_DOS_HEADER)BaseAddr; // 0x04000000

    //
    // Check for Valid DOS file
    //
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        Result = FALSE;

        ShowMessages("err, the selected file is not in a valid PE format\n");
        goto Finished;
    }

    //
    // Offset of NT Header is found at 0x3c location in DOS header specified by
    // e_lfanew
    // Get the Base of NT Header(PE Header) 	= DosHeader + RVA address of PE
    // header
    //

    NtHeader32 = (PIMAGE_NT_HEADERS32)((UINT64)(DosHeader) + (DosHeader->e_lfanew));

    //
    // Identify for valid PE file
    //
    if (NtHeader32->Signature != IMAGE_NT_SIGNATURE)
    {
        Result = FALSE;

        ShowMessages("err, invalid image NT signature\n");
        goto Finished;
    }

    //
    // Info about Optional Header
    //
    OpHeader32 = NtHeader32->OptionalHeader;

    //
    // Get the IMAGE FILE HEADER Structure
    //
    Header = NtHeader32->FileHeader;

    //
    // Only few are determined (for remaining refer
    // to the above specification)
    //
    switch (Header.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        *Is32Bit = TRUE;
        Result   = TRUE;
        goto Finished;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        *Is32Bit = FALSE;
        Result   = TRUE;
        goto Finished;
        break;
    default:
        ShowMessages("err, PE file is not i386 or AMD64; thus, it's not supported "
                     "in HyperDbg\n");
        Result = FALSE;
        goto Finished;
        break;
    }

Finished:
    //
    // Unmap and close the handles
    //
    UnmapViewOfFile(BaseAddr);
    CloseHandle(MapObjectHandle);

    return Result;
}

/**
 * @brief Get the syscall number of a given Nt function
 * @param NtFunctionName
 *
 * @return UINT32
 */
UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName)
{
    HMODULE DllHandle = NULL;

    //
    // Load ntdll.dll to get the address of the Nt function
    //
    DllHandle = LoadLibraryA("ntdll.dll");

    if (!DllHandle)
    {
        ShowMessages("err, failed to load ntdll.dll\n");
        return NULL;
    }

    //
    // Choose any Nt function
    //
    VOID * TargetFunc = GetProcAddress(DllHandle, NtFunctionName);

    if (!TargetFunc)
    {
        //
        // If we failed to get the address of the Nt function,
        // maybe it's from win32u.dll
        //
        DllHandle = LoadLibraryA("win32u.dll");

        if (!DllHandle)
        {
            ShowMessages("err, failed to load win32u.dll\n");
            return NULL;
        }

        TargetFunc = GetProcAddress(DllHandle, NtFunctionName);

        if (!TargetFunc)
        {
            ShowMessages("err, failed to get address of %s\n", NtFunctionName);
            return NULL;
        }
    }

    //
    // By default, we send 30 bytes to the disassembler,
    // since usually the syscall handler is less than 30 bytes,
    // and we want to avoid disassembling another function
    //
    return HyperDbgGetImmediateValueOnEaxForSyscallNumber((unsigned char *)TargetFunc, 30, TRUE);
}
