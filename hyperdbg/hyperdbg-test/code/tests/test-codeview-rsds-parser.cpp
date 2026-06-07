/**
 * @file test-codeview-rsds-parser.cpp
 * @author jtaw5649
 * @brief Test cases for CodeView RSDS parser helpers
 * @details
 * @version 0.19
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

static constexpr SIZE_T RsdsFixtureSize          = 0x600;
static constexpr LONG   RsdsPeHeaderOffset       = 0x80;
static constexpr DWORD  RsdsSectionRva           = 0x1000;
static constexpr DWORD  RsdsSectionRaw           = 0x200;
static constexpr DWORD  RsdsSectionSize          = 0x300;
static constexpr DWORD  RsdsDebugDirectoryRva    = 0x1100;
static constexpr DWORD  RsdsDebugDirectoryRaw    = 0x300;
static constexpr DWORD  RsdsPayloadRva           = 0x1140;
static constexpr DWORD  RsdsPayloadRaw           = 0x340;
static constexpr DWORD  RsdsLoadedDebugRva       = 0x280;
static constexpr DWORD  RsdsLoadedPayloadRva     = 0x2c0;
static constexpr SIZE_T RsdsHighLoadedSize       = 0x22000;
static constexpr DWORD  RsdsHighLoadedDebugRva   = 0x20000;
static constexpr DWORD  RsdsHighLoadedPayloadRva = 0x20100;
static constexpr DWORD  RsdsBogusRawPointer      = 0xfffff000;

static const GUID RsdsGuid64    = {0x67452301, 0xab89, 0xefcd, {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe}};
static const GUID RsdsGuid32    = {0x01234567, 0x89ab, 0xcdef, {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10}};
static const GUID RsdsGuidMulti = {0xaabbccdd, 0xeeff, 0x1122, {0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa}};

/**
 * @brief Calculates the file offset of the optional header based on the PE header offset
 *
 * @return SIZE_T The file offset of the optional header
 */
static SIZE_T
RsdsOptionalHeaderOffset()
{
    return RsdsPeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
}

/**
 * @brief Calculates the file offset of the first section header based on the optional header size
 *
 * @param OptionalHeaderSize The size of the optional header, obtained from the IMAGE_FILE_HEADER
 *
 * @return SIZE_T The file offset of the first section header
 */
static SIZE_T
RsdsSectionHeaderOffset(SIZE_T OptionalHeaderSize)
{
    return RsdsOptionalHeaderOffset() + OptionalHeaderSize;
}

/**
 * @brief Builds a minimal PE image in the provided buffer with the specified architecture
 *
 * @param Buffer The buffer to write the PE image into. Must be at least RsdsFixtureSize bytes
 * @param Is32Bit Whether to build a 32-bit (true) or 64-bit (false) PE image
 *
 * @return VOID
 */
static VOID
RsdsBuildMinimalPe(BYTE * Buffer, BOOLEAN Is32Bit)
{
    ZeroMemory(Buffer, RsdsFixtureSize);

    IMAGE_DOS_HEADER * DosHeader = (IMAGE_DOS_HEADER *)Buffer;
    DosHeader->e_magic           = IMAGE_DOS_SIGNATURE;
    DosHeader->e_lfanew          = RsdsPeHeaderOffset;

    BYTE * NtHeaders    = Buffer + RsdsPeHeaderOffset;
    *(DWORD *)NtHeaders = IMAGE_NT_SIGNATURE;

    IMAGE_FILE_HEADER * FileHeader   = (IMAGE_FILE_HEADER *)(NtHeaders + sizeof(DWORD));
    FileHeader->Machine              = Is32Bit ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_AMD64;
    FileHeader->NumberOfSections     = 1;
    FileHeader->SizeOfOptionalHeader = Is32Bit ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64);

    BYTE * OptionalHeader = NtHeaders + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    if (Is32Bit)
    {
        IMAGE_OPTIONAL_HEADER32 * OptionalHeader32                                  = (IMAGE_OPTIONAL_HEADER32 *)OptionalHeader;
        OptionalHeader32->Magic                                                     = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        OptionalHeader32->SizeOfHeaders                                             = 0x200;
        OptionalHeader32->SizeOfImage                                               = 0x2000;
        OptionalHeader32->NumberOfRvaAndSizes                                       = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        OptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = RsdsDebugDirectoryRva;
        OptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size           = sizeof(IMAGE_DEBUG_DIRECTORY);
    }
    else
    {
        IMAGE_OPTIONAL_HEADER64 * OptionalHeader64                                  = (IMAGE_OPTIONAL_HEADER64 *)OptionalHeader;
        OptionalHeader64->Magic                                                     = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        OptionalHeader64->SizeOfHeaders                                             = 0x200;
        OptionalHeader64->SizeOfImage                                               = 0x2000;
        OptionalHeader64->NumberOfRvaAndSizes                                       = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        OptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = RsdsDebugDirectoryRva;
        OptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size           = sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    IMAGE_SECTION_HEADER * SectionHeader = (IMAGE_SECTION_HEADER *)(Buffer + RsdsSectionHeaderOffset(FileHeader->SizeOfOptionalHeader));
    CopyMemory(SectionHeader->Name, ".rdata", sizeof(".rdata") - 1);
    SectionHeader->Misc.VirtualSize = RsdsSectionSize;
    SectionHeader->VirtualAddress   = RsdsSectionRva;
    SectionHeader->SizeOfRawData    = RsdsSectionSize;
    SectionHeader->PointerToRawData = RsdsSectionRaw;
}

/**
 * @brief Updates the debug directory entry in the PE image to point to the specified directory
 *
 * @param Buffer The buffer containing the PE image
 * @param DirectoryRva The RVA of the debug directory to set
 * @param DirectorySize The size of the debug directory
 *
 * @return VOID
 */
static VOID
RsdsSetDebugDirectory(BYTE * Buffer, DWORD DirectoryRva, DWORD DirectorySize)
{
    BYTE * NtHeaders      = Buffer + RsdsPeHeaderOffset;
    BYTE * OptionalHeader = NtHeaders + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    WORD   Magic          = *(WORD *)OptionalHeader;

    if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER32 * OptionalHeader32                                  = (IMAGE_OPTIONAL_HEADER32 *)OptionalHeader;
        OptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = DirectoryRva;
        OptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size           = DirectorySize;
    }
    else
    {
        IMAGE_OPTIONAL_HEADER64 * OptionalHeader64                                  = (IMAGE_OPTIONAL_HEADER64 *)OptionalHeader;
        OptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = DirectoryRva;
        OptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size           = DirectorySize;
    }
}

/**
 * @brief Updates the NumberOfRvaAndSizes field in the optional header to the specified value
 *
 * @param Buffer The buffer containing the PE image
 * @param NumberOfRvaAndSizes The value to set for the NumberOfRvaAndSizes field
 *
 * @return VOID
 */
static VOID
RsdsSetNumberOfRvaAndSizes(BYTE * Buffer, DWORD NumberOfRvaAndSizes)
{
    BYTE * NtHeaders      = Buffer + RsdsPeHeaderOffset;
    BYTE * OptionalHeader = NtHeaders + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    WORD   Magic          = *(WORD *)OptionalHeader;

    if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        ((IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->NumberOfRvaAndSizes = NumberOfRvaAndSizes;
    }
    else
    {
        ((IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->NumberOfRvaAndSizes = NumberOfRvaAndSizes;
    }
}

/**
 * @brief Updates the SizeOfImage field in the optional header to the specified value
 *
 * @param Buffer The buffer containing the PE image
 * @param SizeOfImage The value to set for the SizeOfImage field
 *
 * @return VOID
 */
static VOID
RsdsSetSizeOfImage(BYTE * Buffer, DWORD SizeOfImage)
{
    BYTE * NtHeaders      = Buffer + RsdsPeHeaderOffset;
    BYTE * OptionalHeader = NtHeaders + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    WORD   Magic          = *(WORD *)OptionalHeader;

    if (Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        ((IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->SizeOfImage = SizeOfImage;
    }
    else
    {
        ((IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->SizeOfImage = SizeOfImage;
    }
}

/**
 * @brief Writes an RSDS CodeView payload to the specified location in the buffer
 *
 * @param Buffer The buffer to write the payload into
 * @param RawOffset The file offset to write the payload at
 * @param Guid The GUID to include in the payload
 * @param Age The age to include in the payload
 * @param Path The path to include in the payload
 * @param IncludeNul Whether to include a NUL terminator at the end of the path
 *
 * @return VOID
 */
static VOID
RsdsWritePayload(BYTE * Buffer, DWORD RawOffset, const GUID & Guid, DWORD Age, const CHAR * Path, BOOLEAN IncludeNul)
{
    BYTE * Payload = Buffer + RawOffset;
    CopyMemory(Payload, "RSDS", sizeof(DWORD));
    CopyMemory(Payload + sizeof(DWORD), &Guid, sizeof(Guid));
    CopyMemory(Payload + sizeof(DWORD) + sizeof(GUID), &Age, sizeof(Age));

    SIZE_T PathLength = strlen(Path);
    CopyMemory(Payload + sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD), Path, PathLength);
    if (IncludeNul)
    {
        Payload[sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD) + PathLength] = '\0';
    }
}

/**
 * @brief Calculates the size of an RSDS CodeView payload based on the path length and whether to include a NUL terminator
 *
 * @param Path The path to be included in the payload
 * @param IncludeNul Whether to include a NUL terminator at the end of the path
 *
 * @return DWORD The size of the payload in bytes
 */
static DWORD
RsdsPayloadSize(const CHAR * Path, BOOLEAN IncludeNul)
{
    return (DWORD)(sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD) + strlen(Path) + (IncludeNul ? 1 : 0));
}

/**
 * @brief Writes an IMAGE_DEBUG_DIRECTORY entry to the specified location in the buffer
 *
 * @param Buffer The buffer to write the debug entry into
 * @param EntryRawOffset The file offset to write the debug entry at
 * @param Type The Type field of the debug entry
 * @param PayloadRva The AddressOfRawData field of the debug entry
 * @param PayloadRaw The PointerToRawData field of the debug entry
 * @param PayloadSize The SizeOfData field of the debug entry
 *
 * @return VOID
 */
static VOID
RsdsWriteDebugEntry(BYTE * Buffer, DWORD EntryRawOffset, DWORD Type, DWORD PayloadRva, DWORD PayloadRaw, DWORD PayloadSize)
{
    IMAGE_DEBUG_DIRECTORY * DebugEntry = (IMAGE_DEBUG_DIRECTORY *)(Buffer + EntryRawOffset);
    ZeroMemory(DebugEntry, sizeof(*DebugEntry));
    DebugEntry->Type             = Type;
    DebugEntry->SizeOfData       = PayloadSize;
    DebugEntry->AddressOfRawData = PayloadRva;
    DebugEntry->PointerToRawData = PayloadRaw;
}

/**
 * @brief Builds a minimal PE image with a valid RSDS debug entry in the specified buffer
 *
 * @param Buffer The buffer to write the PE image into. Must be at least RsdsFixtureSize bytes
 * @param Guid The GUID to include in the RSDS payload
 * @param Age The age to include in the RSDS payload
 * @param Path The path to include in the RSDS payload
 *
 * @return VOID
 */
static VOID
RsdsWriteValidDebugEntry(BYTE * Buffer, const GUID & Guid, DWORD Age, const CHAR * Path)
{
    DWORD PayloadSize = RsdsPayloadSize(Path, TRUE);
    RsdsWritePayload(Buffer, RsdsPayloadRaw, Guid, Age, Path, TRUE);
    RsdsWriteDebugEntry(Buffer, RsdsDebugDirectoryRaw, IMAGE_DEBUG_TYPE_CODEVIEW, RsdsPayloadRva, RsdsPayloadRaw, PayloadSize);
}

/**
 * @brief Builds a minimal PE image with a valid RSDS debug entry suitable for loaded PE parsing in the specified buffer
 *
 * @param Buffer The buffer to write the PE image into. Must be at least RsdsFixtureSize bytes
 * @param Is32Bit Whether to build a 32-bit (true) or 64-bit (false) PE image
 *
 * @return VOID
 */
static VOID
RsdsBuildLoadedPe(BYTE * Buffer, BOOLEAN Is32Bit)
{
    RsdsBuildMinimalPe(Buffer, Is32Bit);
    RsdsSetDebugDirectory(Buffer, RsdsLoadedDebugRva, sizeof(IMAGE_DEBUG_DIRECTORY));
}

/**
 * @brief Writes a valid RSDS debug entry suitable for loaded PE parsing to the specified buffer
 *
 * @param Buffer The buffer to write the debug entry into
 * @param Guid The GUID to include in the RSDS payload
 * @param Age The age to include in the RSDS payload
 * @param Path The path to include in the RSDS payload
 *
 * @return VOID
 */
static VOID
RsdsWriteValidLoadedDebugEntry(BYTE * Buffer, const GUID & Guid, DWORD Age, const CHAR * Path)
{
    DWORD PayloadSize = RsdsPayloadSize(Path, TRUE);
    RsdsWritePayload(Buffer, RsdsLoadedPayloadRva, Guid, Age, Path, TRUE);
    RsdsWriteDebugEntry(Buffer, RsdsLoadedDebugRva, IMAGE_DEBUG_TYPE_CODEVIEW, RsdsLoadedPayloadRva, RsdsBogusRawPointer, PayloadSize);
}

/**
 * @brief Compares two GUIDs for equality
 *
 * @param Left The first GUID to compare
 * @param Right The second GUID to compare
 *
 * @return BOOLEAN TRUE if the GUIDs are equal, FALSE otherwise
 */
static BOOLEAN
RsdsGuidEquals(const GUID & Left, const GUID & Right)
{
    return memcmp(&Left, &Right, sizeof(Left)) == 0;
}

/**
 * @brief Helper function to test that the RSDS parser successfully extracts the expected information from the provided buffer
 *
 * @param Buffer The buffer containing the PE image to parse
 * @param ExpectedPdb The expected PDB file name to be extracted from the RSDS payload
 * @param ExpectedGuid The expected GUID to be extracted from the RSDS payload
 * @param ExpectedAge The expected age to be extracted from the RSDS payload
 *
 * @return BOOLEAN TRUE if the parser successfully extracted the expected information, FALSE otherwise
 */
static BOOLEAN
RsdsExpectSuccess(const BYTE * Buffer, const CHAR * ExpectedPdb, const GUID & ExpectedGuid, DWORD ExpectedAge)
{
    CHAR  PdbFileName[MAX_PATH] = {0};
    GUID  Guid                  = {0};
    DWORD Age                   = 0;

    return SymExtractCodeViewRsdsInfoFromPeImage(Buffer, RsdsFixtureSize, PdbFileName, sizeof(PdbFileName), &Guid, &Age) &&
           strcmp(PdbFileName, ExpectedPdb) == 0 && RsdsGuidEquals(Guid, ExpectedGuid) && Age == ExpectedAge;
}

/**
 * @brief Helper function to test that the RSDS parser fails to extract information from the provided buffer and leaves output parameters unchanged
 *
 * @param Buffer The buffer containing the PE image to parse
 *
 * @return BOOLEAN TRUE if the parser failed as expected and left output parameters unchanged, FALSE otherwise
 */
static BOOLEAN
RsdsExpectFailure(const BYTE * Buffer)
{
    CHAR  PdbFileName[MAX_PATH] = {'x'};
    GUID  Guid                  = RsdsGuid64;
    GUID  EmptyGuid             = {0};
    DWORD Age                   = 0x12345678;

    return !SymExtractCodeViewRsdsInfoFromPeImage(Buffer, RsdsFixtureSize, PdbFileName, sizeof(PdbFileName), &Guid, &Age) &&
           PdbFileName[0] == '\0' && RsdsGuidEquals(Guid, EmptyGuid) && Age == 0;
}

/**
 * @brief Helper function to test that the RSDS parser successfully extracts the expected information from the provided buffer when parsing as a loaded PE
 *
 * @param Buffer The buffer containing the PE image to parse
 * @param ExpectedPdb The expected PDB file name to be extracted from the RSDS payload
 * @param ExpectedGuid The expected GUID to be extracted from the RSDS payload
 * @param ExpectedAge The expected age to be extracted from the RSDS payload
 *
 * @return BOOLEAN TRUE if the parser successfully extracted the expected information, FALSE otherwise
 */
static BOOLEAN
RsdsExpectLoadedSuccess(const BYTE * Buffer, const CHAR * ExpectedPdb, const GUID & ExpectedGuid, DWORD ExpectedAge)
{
    CHAR  PdbFileName[MAX_PATH] = {0};
    GUID  Guid                  = {0};
    DWORD Age                   = 0;

    return SymExtractCodeViewRsdsInfoFromLoadedPeImage(Buffer, RsdsFixtureSize, PdbFileName, sizeof(PdbFileName), &Guid, &Age) &&
           strcmp(PdbFileName, ExpectedPdb) == 0 && RsdsGuidEquals(Guid, ExpectedGuid) && Age == ExpectedAge;
}

/**
 * @brief Helper function to test that the RSDS parser fails to extract information from the provided buffer when parsing as a loaded PE and leaves output parameters unchanged
 *
 * @param Buffer The buffer containing the PE image to parse
 *
 * @return BOOLEAN TRUE if the parser failed as expected and left output parameters unchanged, FALSE otherwise
 */
static BOOLEAN
RsdsExpectLoadedFailure(const BYTE * Buffer)
{
    CHAR  PdbFileName[MAX_PATH] = {'x'};
    GUID  Guid                  = RsdsGuid64;
    GUID  EmptyGuid             = {0};
    DWORD Age                   = 0x12345678;

    return !SymExtractCodeViewRsdsInfoFromLoadedPeImage(Buffer, RsdsFixtureSize, PdbFileName, sizeof(PdbFileName), &Guid, &Age) &&
           PdbFileName[0] == '\0' && RsdsGuidEquals(Guid, EmptyGuid) && Age == 0;
}

// Context structure for the fake fallback function used in some test cases
typedef struct _RSDS_FAKE_FALLBACK_CONTEXT
{
    INT32   CallCount;
    BOOLEAN Succeed;
} RSDS_FAKE_FALLBACK_CONTEXT, *PRSDS_FAKE_FALLBACK_CONTEXT;

/**
 * @brief A fake fallback function that can be used to test the behavior of the RSDS parser when a fallback is triggered
 *
 * @param Context A pointer to an RSDS_FAKE_FALLBACK_CONTEXT structure that controls the behavior of the fallback
 * @param PdbFile The output buffer to receive the PDB file name (must be at least MAX_PATH bytes)
 * @param PdbFileSize The size of the PdbFile buffer in bytes
 * @param Guid The output parameter to receive the GUID
 * @param Age The output parameter to receive the age
 *
 * @return BOOLEAN TRUE if the fallback succeeded and filled output parameters, FALSE if the fallback failed and left output parameters unchanged
 */
static BOOLEAN
RsdsFakeFallback(PVOID Context, CHAR * PdbFile, SIZE_T PdbFileSize, GUID * Guid, DWORD * Age)
{
    PRSDS_FAKE_FALLBACK_CONTEXT FallbackContext = (PRSDS_FAKE_FALLBACK_CONTEXT)Context;

    FallbackContext->CallCount++;
    if (!FallbackContext->Succeed)
    {
        return FALSE;
    }

    if (strcpy_s(PdbFile, PdbFileSize, "fallback.pdb") != 0)
    {
        return FALSE;
    }

    *Guid = RsdsGuidMulti;
    *Age  = 0x2b;

    return TRUE;
}

/**
 * @brief Runs a series of test cases to validate the behavior of the RSDS parser helper functions
 *
 * @return BOOLEAN TRUE if all test cases passed, FALSE if any test case failed
 */
BOOLEAN
TestCodeViewRsdsParser()
{
    BYTE  Buffer[RsdsFixtureSize] = {0};
    INT32 TestNum                 = 0;

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid64, 7, "C:\\symbols\\valid64.pdb");
    TestNum++;
    if (RsdsExpectSuccess(Buffer, "valid64.pdb", RsdsGuid64, 7))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] valid PE32+ RSDS entry was not parsed\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid64, 7, "C:\\symbols\\unadvertised.pdb");
    RsdsSetNumberOfRvaAndSizes(Buffer, IMAGE_DIRECTORY_ENTRY_DEBUG);
    TestNum++;
    if (RsdsExpectFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] raw parser accepted an unadvertised debug directory\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, TRUE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid32, 9, "C:/symbols/valid32.pdb");
    TestNum++;
    if (RsdsExpectSuccess(Buffer, "valid32.pdb", RsdsGuid32, 9))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] valid PE32 RSDS entry was not parsed\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid64, 7, "C:\\symbols\\valid64.pdb");
    RsdsSetDebugDirectory(Buffer, 0x3000, sizeof(IMAGE_DEBUG_DIRECTORY));
    TestNum++;
    if (RsdsExpectFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] unmapped debug directory parsed successfully\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid64, 7, "C:\\symbols\\invalid.pdb");
    CopyMemory(Buffer + RsdsPayloadRaw, "ABCD", sizeof(DWORD));
    TestNum++;
    if (RsdsExpectFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] unsupported CodeView signature parsed successfully\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWritePayload(Buffer, RsdsPayloadRaw, RsdsGuid64, 7, "C:\\symbols\\missing-nul.pdb", FALSE);
    RsdsWriteDebugEntry(Buffer,
                        RsdsDebugDirectoryRaw,
                        IMAGE_DEBUG_TYPE_CODEVIEW,
                        RsdsPayloadRva,
                        RsdsPayloadRaw,
                        RsdsPayloadSize("C:\\symbols\\missing-nul.pdb", FALSE));
    TestNum++;
    if (RsdsExpectFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] RSDS path without NUL terminator parsed successfully\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsSetDebugDirectory(Buffer, RsdsDebugDirectoryRva, sizeof(IMAGE_DEBUG_DIRECTORY) * 2);
    RsdsWriteDebugEntry(Buffer, RsdsDebugDirectoryRaw, IMAGE_DEBUG_TYPE_CODEVIEW, RsdsPayloadRva, RsdsPayloadRaw, sizeof(DWORD));
    CopyMemory(Buffer + RsdsPayloadRaw, "NB10", sizeof(DWORD));
    RsdsWritePayload(Buffer, 0x390, RsdsGuidMulti, 11, "D:/alt/second-valid.pdb", TRUE);
    RsdsWriteDebugEntry(Buffer,
                        RsdsDebugDirectoryRaw + sizeof(IMAGE_DEBUG_DIRECTORY),
                        IMAGE_DEBUG_TYPE_CODEVIEW,
                        0x1190,
                        0x390,
                        RsdsPayloadSize("D:/alt/second-valid.pdb", TRUE));
    TestNum++;
    if (RsdsExpectSuccess(Buffer, "second-valid.pdb", RsdsGuidMulti, 11))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] parser did not skip invalid first entry and parse second RSDS entry\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid64, 0x21, "C:\\loaded\\loaded64.pdb");
    TestNum++;
    if (RsdsExpectLoadedSuccess(Buffer, "loaded64.pdb", RsdsGuid64, 0x21))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded PE32+ RSDS entry was not parsed\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid64, 0x21, "C:\\loaded\\unadvertised.pdb");
    RsdsSetNumberOfRvaAndSizes(Buffer, IMAGE_DIRECTORY_ENTRY_DEBUG);
    TestNum++;
    if (RsdsExpectLoadedFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser accepted an unadvertised debug directory\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, TRUE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid32, 0x22, "C:/loaded/loaded32.pdb");
    TestNum++;
    if (RsdsExpectLoadedSuccess(Buffer, "loaded32.pdb", RsdsGuid32, 0x22))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded PE32 RSDS entry was not parsed\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid64, 0x23, "C:\\loaded\\bogus-raw-ignored.pdb");
    TestNum++;
    if (RsdsExpectLoadedSuccess(Buffer, "bogus-raw-ignored.pdb", RsdsGuid64, 0x23))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser used bogus PointerToRawData instead of loaded RVA\n");
        return FALSE;
    }

    std::vector<BYTE> HighLoadedBuffer(RsdsHighLoadedSize);
    RsdsBuildLoadedPe(HighLoadedBuffer.data(), FALSE);
    RsdsSetSizeOfImage(HighLoadedBuffer.data(), (DWORD)HighLoadedBuffer.size());
    RsdsSetDebugDirectory(HighLoadedBuffer.data(), RsdsHighLoadedDebugRva, sizeof(IMAGE_DEBUG_DIRECTORY));
    RsdsWritePayload(HighLoadedBuffer.data(), RsdsHighLoadedPayloadRva, RsdsGuid64, 0x28, "C:\\loaded\\high-rva.pdb", TRUE);
    RsdsWriteDebugEntry(HighLoadedBuffer.data(),
                        RsdsHighLoadedDebugRva,
                        IMAGE_DEBUG_TYPE_CODEVIEW,
                        RsdsHighLoadedPayloadRva,
                        RsdsBogusRawPointer,
                        RsdsPayloadSize("C:\\loaded\\high-rva.pdb", TRUE));
    CHAR  HighLoadedPdbFileName[MAX_PATH] = {0};
    GUID  HighLoadedGuid                  = {0};
    DWORD HighLoadedAge                   = 0;
    TestNum++;
    if (SymExtractCodeViewRsdsInfoFromLoadedPeImage(HighLoadedBuffer.data(),
                                                    HighLoadedBuffer.size(),
                                                    HighLoadedPdbFileName,
                                                    sizeof(HighLoadedPdbFileName),
                                                    &HighLoadedGuid,
                                                    &HighLoadedAge) &&
        strcmp(HighLoadedPdbFileName, "high-rva.pdb") == 0 && RsdsGuidEquals(HighLoadedGuid, RsdsGuid64) &&
        HighLoadedAge == 0x28)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser did not parse high-RVA RSDS data\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid64, 0x24, "C:\\loaded\\invalid-dir.pdb");
    RsdsSetDebugDirectory(Buffer, RsdsFixtureSize - sizeof(IMAGE_DEBUG_DIRECTORY) / 2, sizeof(IMAGE_DEBUG_DIRECTORY));
    TestNum++;
    if (RsdsExpectLoadedFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser accepted malformed debug directory bounds\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWriteValidLoadedDebugEntry(Buffer, RsdsGuid64, 0x25, "C:\\loaded\\unsupported.pdb");
    CopyMemory(Buffer + RsdsLoadedPayloadRva, "ABCD", sizeof(DWORD));
    TestNum++;
    if (RsdsExpectLoadedFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser accepted unsupported CodeView signature\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsWritePayload(Buffer, RsdsLoadedPayloadRva, RsdsGuid64, 0x26, "C:\\loaded\\missing-nul.pdb", FALSE);
    RsdsWriteDebugEntry(Buffer,
                        RsdsLoadedDebugRva,
                        IMAGE_DEBUG_TYPE_CODEVIEW,
                        RsdsLoadedPayloadRva,
                        RsdsBogusRawPointer,
                        RsdsPayloadSize("C:\\loaded\\missing-nul.pdb", FALSE));
    TestNum++;
    if (RsdsExpectLoadedFailure(Buffer))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser accepted RSDS path without NUL terminator\n");
        return FALSE;
    }

    RsdsBuildLoadedPe(Buffer, FALSE);
    RsdsSetDebugDirectory(Buffer, RsdsLoadedDebugRva, sizeof(IMAGE_DEBUG_DIRECTORY) * 2);
    RsdsWriteDebugEntry(Buffer, RsdsLoadedDebugRva, IMAGE_DEBUG_TYPE_CODEVIEW, RsdsLoadedPayloadRva, RsdsBogusRawPointer, sizeof(DWORD));
    CopyMemory(Buffer + RsdsLoadedPayloadRva, "NB10", sizeof(DWORD));
    RsdsWritePayload(Buffer, 0x330, RsdsGuidMulti, 0x27, "D:/loaded/second-loaded.pdb", TRUE);
    RsdsWriteDebugEntry(Buffer,
                        RsdsLoadedDebugRva + sizeof(IMAGE_DEBUG_DIRECTORY),
                        IMAGE_DEBUG_TYPE_CODEVIEW,
                        0x330,
                        RsdsBogusRawPointer,
                        RsdsPayloadSize("D:/loaded/second-loaded.pdb", TRUE));
    TestNum++;
    if (RsdsExpectLoadedSuccess(Buffer, "second-loaded.pdb", RsdsGuidMulti, 0x27))
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] loaded parser did not skip invalid first entry and parse second RSDS entry\n");
        return FALSE;
    }

    CHAR       SymbolServerRelativePath[MAX_PATH]                = {0};
    CHAR       GuidAndAgeDetails[MAX_PATH]                       = {0};
    CHAR       SmallGuidAndAgeDetails[MAXIMUM_GUID_AND_AGE_SIZE] = {0};
    const GUID Guid                                              = {0x01234567, 0x89ab, 0xcdef, {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10}};
    TestNum++;
    if (SymFormatPdbIdentity("valid32.pdb",
                             &Guid,
                             0x1a,
                             SymbolServerRelativePath,
                             sizeof(SymbolServerRelativePath),
                             GuidAndAgeDetails,
                             sizeof(GuidAndAgeDetails)) &&
        strcmp(GuidAndAgeDetails, "0123456789abcdeffedcba98765432101a") == 0 &&
        strcmp(SymbolServerRelativePath, "valid32.pdb/0123456789abcdeffedcba98765432101a/valid32.pdb") == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] PDB identity formatting did not match symbol server path and GUID+age expectations\n");
        return FALSE;
    }

    TestNum++;
    if (SymFormatPdbIdentity("valid32.pdb",
                             &Guid,
                             0x1a,
                             NULL,
                             0,
                             SmallGuidAndAgeDetails,
                             sizeof(SmallGuidAndAgeDetails)) &&
        strcmp(SmallGuidAndAgeDetails, "0123456789abcdeffedcba98765432101a") == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] GUID+age identity did not fit the SDK-sized buffer\n");
        return FALSE;
    }

    RsdsBuildMinimalPe(Buffer, FALSE);
    RsdsWriteValidDebugEntry(Buffer, RsdsGuid64, 0x1c, "C:\\preferred\\preferred.pdb");
    RSDS_FAKE_FALLBACK_CONTEXT FallbackContext         = {0, TRUE};
    CHAR                       PreferredPath[MAX_PATH] = {0};
    TestNum++;
    if (SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                  RsdsFixtureSize,
                                                  PreferredPath,
                                                  sizeof(PreferredPath),
                                                  NULL,
                                                  0,
                                                  NULL,
                                                  0,
                                                  RsdsFakeFallback,
                                                  &FallbackContext) &&
        strcmp(PreferredPath, "preferred.pdb/67452301ab89efcd1032547698badcfe1c/preferred.pdb") == 0 &&
        FallbackContext.CallCount == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] PE RSDS identity was not preferred over fallback identity\n");
        return FALSE;
    }

    FallbackContext.CallCount = 0;
    CHAR ZeroSizeOutput       = 'x';
    TestNum++;
    if (!SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                   RsdsFixtureSize,
                                                   &ZeroSizeOutput,
                                                   0,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   RsdsFakeFallback,
                                                   &FallbackContext) &&
        ZeroSizeOutput == 'x' && FallbackContext.CallCount == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] zero-sized output buffer was written or reported success\n");
        return FALSE;
    }

    FallbackContext.CallCount = 0;
    TestNum++;
    if (!SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                   RsdsFixtureSize,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   RsdsFakeFallback,
                                                   &FallbackContext) &&
        FallbackContext.CallCount == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] identity formatting without requested output reported success or used fallback\n");
        return FALSE;
    }

    FallbackContext.CallCount    = 0;
    CHAR SmallPdbPath[4]         = {'x'};
    CHAR SmallFailureGuidAge[64] = {'x'};
    TestNum++;
    if (!SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                   RsdsFixtureSize,
                                                   NULL,
                                                   0,
                                                   SmallPdbPath,
                                                   sizeof(SmallPdbPath),
                                                   SmallFailureGuidAge,
                                                   sizeof(SmallFailureGuidAge),
                                                   RsdsFakeFallback,
                                                   &FallbackContext) &&
        SmallPdbPath[0] == '\0' && SmallFailureGuidAge[0] == '\0' && FallbackContext.CallCount == 0)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] output formatting failure leaked partial identity data\n");
        return FALSE;
    }

    ZeroMemory(Buffer, sizeof(Buffer));
    FallbackContext.CallCount      = 0;
    FallbackContext.Succeed        = TRUE;
    CHAR FallbackPdb[MAX_PATH]     = {0};
    CHAR FallbackGuidAge[MAX_PATH] = {0};
    TestNum++;
    if (SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                  RsdsFixtureSize,
                                                  NULL,
                                                  0,
                                                  FallbackPdb,
                                                  sizeof(FallbackPdb),
                                                  FallbackGuidAge,
                                                  sizeof(FallbackGuidAge),
                                                  RsdsFakeFallback,
                                                  &FallbackContext) &&
        strcmp(FallbackPdb, "fallback.pdb") == 0 &&
        strcmp(FallbackGuidAge, "aabbccddeeff112233445566778899aa2b") == 0 &&
        FallbackContext.CallCount == 1)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] malformed PE bytes did not use fallback identity\n");
        return FALSE;
    }

    FallbackContext.CallCount    = 0;
    FallbackContext.Succeed      = FALSE;
    CHAR FailedPath[MAX_PATH]    = {'x'};
    CHAR FailedPdb[MAX_PATH]     = {'x'};
    CHAR FailedGuidAge[MAX_PATH] = {'x'};
    TestNum++;
    if (!SymFormatPdbIdentityFromPeImageOrFallback(Buffer,
                                                   RsdsFixtureSize,
                                                   FailedPath,
                                                   sizeof(FailedPath),
                                                   FailedPdb,
                                                   sizeof(FailedPdb),
                                                   FailedGuidAge,
                                                   sizeof(FailedGuidAge),
                                                   RsdsFakeFallback,
                                                   &FallbackContext) &&
        FailedPath[0] == '\0' && FailedPdb[0] == '\0' && FailedGuidAge[0] == '\0' && FallbackContext.CallCount == 1)
    {
        printf("[+] Test number %d Passed\n", TestNum);
    }
    else
    {
        printf("[-] Test number %d Failed\n", TestNum);
        printf("[x] fallback failure reported success or left success-looking output\n");
        return FALSE;
    }

    return TRUE;
}
