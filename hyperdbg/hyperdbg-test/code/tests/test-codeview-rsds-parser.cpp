/**
 * @file test-codeview-rsds-parser.cpp
 * @author jtaw5649
 * @brief Test cases for CodeView RSDS parser helpers
 * @details
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include "header/codeview-rsds.h"
#include "header/pdb-identity.h"

static constexpr SIZE_T RsdsFixtureSize       = 0x600;
static constexpr LONG   RsdsPeHeaderOffset    = 0x80;
static constexpr DWORD  RsdsSectionRva        = 0x1000;
static constexpr DWORD  RsdsSectionRaw        = 0x200;
static constexpr DWORD  RsdsSectionSize       = 0x300;
static constexpr DWORD  RsdsDebugDirectoryRva = 0x1100;
static constexpr DWORD  RsdsDebugDirectoryRaw = 0x300;
static constexpr DWORD  RsdsPayloadRva        = 0x1140;
static constexpr DWORD  RsdsPayloadRaw        = 0x340;

static const GUID RsdsGuid64    = {0x67452301, 0xab89, 0xefcd, {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe}};
static const GUID RsdsGuid32    = {0x01234567, 0x89ab, 0xcdef, {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10}};
static const GUID RsdsGuidMulti = {0xaabbccdd, 0xeeff, 0x1122, {0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa}};

static SIZE_T
RsdsOptionalHeaderOffset()
{
    return RsdsPeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
}

static SIZE_T
RsdsSectionHeaderOffset(SIZE_T OptionalHeaderSize)
{
    return RsdsOptionalHeaderOffset() + OptionalHeaderSize;
}

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

static DWORD
RsdsPayloadSize(const CHAR * Path, BOOLEAN IncludeNul)
{
    return (DWORD)(sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD) + strlen(Path) + (IncludeNul ? 1 : 0));
}

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

static VOID
RsdsWriteValidDebugEntry(BYTE * Buffer, const GUID & Guid, DWORD Age, const CHAR * Path)
{
    DWORD PayloadSize = RsdsPayloadSize(Path, TRUE);
    RsdsWritePayload(Buffer, RsdsPayloadRaw, Guid, Age, Path, TRUE);
    RsdsWriteDebugEntry(Buffer, RsdsDebugDirectoryRaw, IMAGE_DEBUG_TYPE_CODEVIEW, RsdsPayloadRva, RsdsPayloadRaw, PayloadSize);
}

static BOOLEAN
RsdsGuidEquals(const GUID & Left, const GUID & Right)
{
    return memcmp(&Left, &Right, sizeof(Left)) == 0;
}

static BOOLEAN
RsdsExpectSuccess(const BYTE * Buffer, const CHAR * ExpectedPdb, const GUID & ExpectedGuid, DWORD ExpectedAge)
{
    CHAR  PdbFileName[MAX_PATH] = {0};
    GUID  Guid                  = {0};
    DWORD Age                   = 0;

    return SymExtractCodeViewRsdsInfoFromPeImage(Buffer, RsdsFixtureSize, PdbFileName, sizeof(PdbFileName), &Guid, &Age) &&
           strcmp(PdbFileName, ExpectedPdb) == 0 && RsdsGuidEquals(Guid, ExpectedGuid) && Age == ExpectedAge;
}

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

    CHAR       SymbolServerRelativePath[MAX_PATH] = {0};
    CHAR       GuidAndAgeDetails[MAX_PATH]        = {0};
    const GUID Guid                               = {0x01234567, 0x89ab, 0xcdef, {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10}};
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

    return TRUE;
}
