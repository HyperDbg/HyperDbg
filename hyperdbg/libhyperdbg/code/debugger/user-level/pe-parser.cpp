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

#include "header/pe-image-reader.h"

static const char *
PeGetSubsystemName(WORD Subsystem)
{
    switch (Subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE:
        return "Device Driver(Native windows Process)";
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        return "Windows GUI";
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        return "Windows CLI";
    case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
        return "Windows CE GUI";
    default:
        return "Unknown";
    }
}

static VOID
PeShowDllCharacteristics(WORD DllCharacteristics)
{
    BOOLEAN AnyFlag = FALSE;

    struct DLL_CHARACTERISTIC_NAME
    {
        WORD         Flag;
        const char * Name;
    };

    static const DLL_CHARACTERISTIC_NAME CommonDllCharacteristics[] = {
        {IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA, "High Entropy VA"},
        {IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE, "Dynamic Base"},
        {IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY, "Force Integrity"},
        {IMAGE_DLLCHARACTERISTICS_NX_COMPAT, "NX Compatible"},
        {IMAGE_DLLCHARACTERISTICS_NO_ISOLATION, "No Isolation"},
        {IMAGE_DLLCHARACTERISTICS_NO_SEH, "No SEH"},
        {IMAGE_DLLCHARACTERISTICS_NO_BIND, "No Bind"},
        {IMAGE_DLLCHARACTERISTICS_APPCONTAINER, "AppContainer"},
        {IMAGE_DLLCHARACTERISTICS_WDM_DRIVER, "WDM Driver"},
        {IMAGE_DLLCHARACTERISTICS_GUARD_CF, "Guard CF"},
        {IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE, "Terminal Server Aware"},
    };

    for (UINT32 i = 0; i < RTL_NUMBER_OF(CommonDllCharacteristics); i++)
    {
        if ((DllCharacteristics & CommonDllCharacteristics[i].Flag) == CommonDllCharacteristics[i].Flag)
        {
            ShowMessages("%s%s", AnyFlag ? ", " : "", CommonDllCharacteristics[i].Name);
            AnyFlag = TRUE;
        }
    }

    if (!AnyFlag)
    {
        ShowMessages("None");
    }
}

static VOID
PeShowEntrypointFileOffset(PPE_IMAGE_READER Reader, DWORD AddressOfEntryPoint)
{
    SIZE_T FileOffset = 0;

    if (PeImageReaderRvaToFileOffset(Reader, AddressOfEntryPoint, 1, &FileOffset))
    {
        ShowMessages("\n%-36s%#llx", "Entrypoint file offset :", (UINT64)FileOffset);
    }
    else
    {
        ShowMessages("\n%-36s%s", "Entrypoint file offset :", "not mapped");
    }
}

static const char *
PeGetDataDirectoryName(UINT32 Index)
{
    static const char * DirectoryNames[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] = {
        "Export Table",
        "Import Table",
        "Resource Table",
        "Exception Table",
        "Certificate Table",
        "Base Relocation Table",
        "Debug",
        "Architecture",
        "Global Ptr",
        "TLS Table",
        "Load Config Table",
        "Bound Import",
        "Import Address Table",
        "Delay Import Descriptor",
        "CLR Runtime Header",
        "Reserved",
    };

    if (Index < RTL_NUMBER_OF(DirectoryNames))
    {
        return DirectoryNames[Index];
    }

    return "Unknown";
}

static BOOLEAN
PeAddDword(DWORD Left, DWORD Right, DWORD * Result)
{
    if (Result == NULL || Right > MAXDWORD - Left)
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

static BOOLEAN
PeAddUlonglong(ULONGLONG Left, ULONGLONG Right, ULONGLONG * Result)
{
    if (Result == NULL || Right > (~((ULONGLONG)0) - Left))
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

typedef struct _PE_RAW_SECTION_RANGE
{
    ULONGLONG                    Start;
    ULONGLONG                    End;
    const IMAGE_SECTION_HEADER * Section;
} PE_RAW_SECTION_RANGE, *PPE_RAW_SECTION_RANGE;

static INT
PeCompareRawSectionRange(const VOID * Left, const VOID * Right)
{
    const PE_RAW_SECTION_RANGE * LeftRange  = (const PE_RAW_SECTION_RANGE *)Left;
    const PE_RAW_SECTION_RANGE * RightRange = (const PE_RAW_SECTION_RANGE *)Right;

    if (LeftRange->Start < RightRange->Start)
    {
        return -1;
    }

    if (LeftRange->Start > RightRange->Start)
    {
        return 1;
    }

    if (LeftRange->End < RightRange->End)
    {
        return -1;
    }

    if (LeftRange->End > RightRange->End)
    {
        return 1;
    }

    return 0;
}

static BOOLEAN
PeRvaContainsRange(DWORD RangeRva, DWORD RangeSize, DWORD Rva, DWORD Size)
{
    DWORD RangeEnd = 0;
    DWORD RvaEnd   = 0;

    if (!PeAddDword(RangeRva, RangeSize, &RangeEnd) || !PeAddDword(Rva, Size, &RvaEnd))
    {
        return FALSE;
    }

    return Rva >= RangeRva && RvaEnd <= RangeEnd;
}

static BOOLEAN
PeGetPointerAtRva(PPE_IMAGE_READER Reader, DWORD Rva, DWORD Length, const BYTE ** Pointer)
{
    SIZE_T FileOffset = 0;

    if (!PeImageReaderRvaToFileOffset(Reader, Rva, Length, &FileOffset))
    {
        return FALSE;
    }

    return PeImageReaderGetPointerAtOffset(Reader, FileOffset, Length, Pointer);
}

typedef enum _PE_ASCII_STRING_STATUS
{
    PeAsciiStringInvalid,
    PeAsciiStringOk,
    PeAsciiStringTruncated,
} PE_ASCII_STRING_STATUS;

static PE_ASCII_STRING_STATUS
PeReadAsciiStringAtRva(PPE_IMAGE_READER Reader, DWORD Rva, DWORD MaxLength, CHAR * Buffer, SIZE_T BufferSize)
{
    if (Buffer == NULL || BufferSize == 0 || MaxLength == 0)
    {
        return PeAsciiStringInvalid;
    }

    Buffer[0] = '\0';

    SIZE_T OutputIndex = 0;
    for (DWORD Index = 0; Index < MaxLength; Index++)
    {
        DWORD        CharacterRva = 0;
        const BYTE * Character    = NULL;

        if (!PeAddDword(Rva, Index, &CharacterRva) || !PeGetPointerAtRva(Reader, CharacterRva, 1, &Character))
        {
            return PeAsciiStringInvalid;
        }

        if (*Character == '\0')
        {
            return PeAsciiStringOk;
        }

        if (OutputIndex + 1 < BufferSize)
        {
            Buffer[OutputIndex++] = (*Character >= 0x20 && *Character <= 0x7e) ? (CHAR)*Character : '.';
            Buffer[OutputIndex]   = '\0';
        }
    }

    return PeAsciiStringTruncated;
}

static const char *
PeAsciiStatusName(PE_ASCII_STRING_STATUS Status)
{
    switch (Status)
    {
    case PeAsciiStringInvalid:
        return "invalid mapping";
    case PeAsciiStringTruncated:
        return "truncated or unterminated";
    default:
        return "readable";
    }
}

static BOOLEAN
PeVaToRva(ULONGLONG Va, ULONGLONG ImageBase, DWORD * Rva)
{
    ULONGLONG Difference = 0;

    if (Rva == NULL || Va < ImageBase)
    {
        return FALSE;
    }

    Difference = Va - ImageBase;
    if (Difference > MAXDWORD)
    {
        return FALSE;
    }

    *Rva = (DWORD)Difference;
    return TRUE;
}

static DWORD
PeReadDwordFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    DWORD Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

static ULONGLONG
PeReadQwordFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    ULONGLONG Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

static ULONGLONG
PeReadLoadConfigPointer(const BYTE * Buffer, SIZE_T Offset, BOOLEAN Is32Bit)
{
    return Is32Bit ? PeReadDwordFromBuffer(Buffer, Offset) : PeReadQwordFromBuffer(Buffer, Offset);
}

static PE_ASCII_STRING_STATUS
PeReadAsciiStringFromBuffer(const BYTE * StringPointer, DWORD MaxLength, CHAR * Buffer, SIZE_T BufferSize)
{
    if (StringPointer == NULL || Buffer == NULL || BufferSize == 0 || MaxLength == 0)
    {
        return PeAsciiStringInvalid;
    }

    Buffer[0] = '\0';

    SIZE_T OutputIndex = 0;
    for (DWORD Index = 0; Index < MaxLength; Index++)
    {
        if (StringPointer[Index] == '\0')
        {
            return PeAsciiStringOk;
        }

        if (OutputIndex + 1 < BufferSize)
        {
            Buffer[OutputIndex++] = (StringPointer[Index] >= 0x20 && StringPointer[Index] <= 0x7e) ? (CHAR)StringPointer[Index] : '.';
            Buffer[OutputIndex]   = '\0';
        }
    }

    return PeAsciiStringTruncated;
}

static BOOLEAN
PeReadWordAtRva(PPE_IMAGE_READER Reader, DWORD Rva, WORD * Value)
{
    const BYTE * Pointer = NULL;

    if (Value == NULL || !PeGetPointerAtRva(Reader, Rva, sizeof(WORD), &Pointer))
    {
        return FALSE;
    }

    CopyMemory(Value, Pointer, sizeof(*Value));
    return TRUE;
}

static BOOLEAN
PeReadThunkAtRva(PPE_IMAGE_READER Reader, DWORD Rva, BOOLEAN Is32Bit, ULONGLONG * Value)
{
    const BYTE * Pointer = NULL;
    DWORD        Size    = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    if (Value == NULL || !PeGetPointerAtRva(Reader, Rva, Size, &Pointer))
    {
        return FALSE;
    }

    if (Is32Bit)
    {
        DWORD Value32 = 0;
        CopyMemory(&Value32, Pointer, sizeof(Value32));
        *Value = Value32;
    }
    else
    {
        CopyMemory(Value, Pointer, sizeof(*Value));
    }

    return TRUE;
}

static VOID
PeShowTls(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * TlsDirectory, BOOLEAN Is32Bit, ULONGLONG ImageBase)
{
    const DWORD MaxCallbacks = 0x200;
    DWORD       TlsSize      = Is32Bit ? sizeof(IMAGE_TLS_DIRECTORY32) : sizeof(IMAGE_TLS_DIRECTORY64);

    ShowMessages("\n\nTLS\n---");

    if (TlsDirectory->VirtualAddress == 0 || TlsDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "empty");
        return;
    }

    if (!PeRvaContainsRange(TlsDirectory->VirtualAddress, TlsDirectory->Size, TlsDirectory->VirtualAddress, TlsSize))
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "invalid bounds");
        return;
    }

    const BYTE * TlsPointer = NULL;
    if (!PeGetPointerAtRva(Reader, TlsDirectory->VirtualAddress, TlsSize, &TlsPointer))
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "not mapped");
        return;
    }

    ULONGLONG StartRawDataVa     = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 0) : PeReadQwordFromBuffer(TlsPointer, 0);
    ULONGLONG EndRawDataVa       = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 4) : PeReadQwordFromBuffer(TlsPointer, 8);
    ULONGLONG AddressOfIndexVa   = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 8) : PeReadQwordFromBuffer(TlsPointer, 16);
    ULONGLONG AddressCallbacksVa = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 12) : PeReadQwordFromBuffer(TlsPointer, 24);
    DWORD     SizeOfZeroFill     = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 16) : PeReadDwordFromBuffer(TlsPointer, 32);
    DWORD     Characteristics    = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 20) : PeReadDwordFromBuffer(TlsPointer, 36);

    ShowMessages("\n%-36s%#llx", "Start address of raw data VA :", StartRawDataVa);
    ShowMessages("\n%-36s%#llx", "End address of raw data VA :", EndRawDataVa);
    ShowMessages("\n%-36s%#llx", "Address of index VA :", AddressOfIndexVa);
    ShowMessages("\n%-36s%#llx", "Address of callbacks VA :", AddressCallbacksVa);
    ShowMessages("\n%-36s%#x", "Size of zero fill :", SizeOfZeroFill);
    ShowMessages("\n%-36s%#x", "Characteristics :", Characteristics);

    DWORD CallbacksRva = 0;
    if (AddressCallbacksVa == 0)
    {
        ShowMessages("\n%-36s%s", "TLS callbacks :", "empty");
        return;
    }

    if (!PeVaToRva(AddressCallbacksVa, ImageBase, &CallbacksRva))
    {
        ShowMessages("\n%-36s%#llx", "Warning, invalid callbacks VA :", AddressCallbacksVa);
        return;
    }

    DWORD EntrySize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);
    for (DWORD CallbackIndex = 0; CallbackIndex < MaxCallbacks; CallbackIndex++)
    {
        DWORD EntryRva = 0;
        if (!PeAddDword(CallbacksRva, CallbackIndex * EntrySize, &EntryRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "TLS callback RVA overflow");
            return;
        }

        ULONGLONG CallbackVa = 0;
        if (!PeReadThunkAtRva(Reader, EntryRva, Is32Bit, &CallbackVa))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid callback entry RVA :", EntryRva);
            return;
        }

        if (CallbackVa == 0)
        {
            return;
        }

        DWORD  CallbackRva = 0;
        SIZE_T FileOffset  = 0;
        if (PeVaToRva(CallbackVa, ImageBase, &CallbackRva))
        {
            if (PeImageReaderRvaToFileOffset(Reader, CallbackRva, 1, &FileOffset))
            {
                ShowMessages("\n    [%u] VA %#llx RVA %#x file offset %#llx", CallbackIndex, CallbackVa, CallbackRva, (UINT64)FileOffset);
            }
            else
            {
                ShowMessages("\n    [%u] VA %#llx RVA %#x file offset %s", CallbackIndex, CallbackVa, CallbackRva, "not mapped");
            }
        }
        else
        {
            ShowMessages("\n    [%u] VA %#llx RVA %s", CallbackIndex, CallbackVa, "invalid");
        }
    }

    ShowMessages("\n%-36s%#x", "Warning, TLS callback cap reached :", MaxCallbacks);
}

static VOID
PeShowDebugCodeView(PPE_IMAGE_READER Reader, const IMAGE_DEBUG_DIRECTORY * Entry)
{
    const DWORD MaxPdbPathLength = 0x400;

    if (Entry->Type != IMAGE_DEBUG_TYPE_CODEVIEW || Entry->SizeOfData < sizeof(DWORD))
    {
        return;
    }

    const BYTE * Payload = NULL;
    if ((Entry->PointerToRawData == 0 || !PeImageReaderGetPointerAtOffset(Reader, Entry->PointerToRawData, Entry->SizeOfData, &Payload)) &&
        (Entry->AddressOfRawData == 0 || !PeGetPointerAtRva(Reader, Entry->AddressOfRawData, Entry->SizeOfData, &Payload)))
    {
        ShowMessages("\n%-36s%s", "    CodeView payload :", "invalid bounds");
        return;
    }

    if (memcmp(Payload, "RSDS", sizeof(DWORD)) == 0)
    {
        if (Entry->SizeOfData < 24)
        {
            ShowMessages("\n%-36s%s", "    CodeView RSDS :", "truncated");
            return;
        }

        DWORD        Age                           = PeReadDwordFromBuffer(Payload, 20);
        DWORD        PathOffset                    = 24;
        DWORD        PathSize                      = Entry->SizeOfData - 24;
        DWORD        PathLimit                     = PathSize < MaxPdbPathLength ? PathSize : MaxPdbPathLength;
        CHAR         PdbPath[MaxPdbPathLength + 1] = {0};
        const BYTE * Guid                          = Payload + 4;

        PE_ASCII_STRING_STATUS PathStatus = PeReadAsciiStringFromBuffer(Payload + PathOffset, PathLimit, PdbPath, sizeof(PdbPath));
        ShowMessages("\n%-36s%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     "    CodeView RSDS GUID :",
                     Guid[3],
                     Guid[2],
                     Guid[1],
                     Guid[0],
                     Guid[5],
                     Guid[4],
                     Guid[7],
                     Guid[6],
                     Guid[8],
                     Guid[9],
                     Guid[10],
                     Guid[11],
                     Guid[12],
                     Guid[13],
                     Guid[14],
                     Guid[15]);
        ShowMessages("\n%-36s%u", "    CodeView age :", Age);
        ShowMessages("\n%-36s%s", "    CodeView PDB path :", PathStatus == PeAsciiStringOk ? PdbPath : PeAsciiStatusName(PathStatus));
    }
    else if (memcmp(Payload, "NB10", sizeof(DWORD)) == 0)
    {
        if (Entry->SizeOfData < 16)
        {
            ShowMessages("\n%-36s%s", "    CodeView NB10 :", "truncated");
            return;
        }

        DWORD Offset                        = PeReadDwordFromBuffer(Payload, 4);
        DWORD Timestamp                     = PeReadDwordFromBuffer(Payload, 8);
        DWORD Age                           = PeReadDwordFromBuffer(Payload, 12);
        DWORD PathOffset                    = 16;
        DWORD PathSize                      = Entry->SizeOfData - 16;
        DWORD PathLimit                     = PathSize < MaxPdbPathLength ? PathSize : MaxPdbPathLength;
        CHAR  PdbPath[MaxPdbPathLength + 1] = {0};

        PE_ASCII_STRING_STATUS PathStatus = PeReadAsciiStringFromBuffer(Payload + PathOffset, PathLimit, PdbPath, sizeof(PdbPath));
        ShowMessages("\n%-36s%#x", "    CodeView NB10 offset :", Offset);
        ShowMessages("\n%-36s%#x", "    CodeView NB10 signature :", Timestamp);
        ShowMessages("\n%-36s%u", "    CodeView age :", Age);
        ShowMessages("\n%-36s%s", "    CodeView PDB path :", PathStatus == PeAsciiStringOk ? PdbPath : PeAsciiStatusName(PathStatus));
    }
}

static VOID
PeShowDebug(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * DebugDirectory)
{
    const DWORD MaxDebugEntries = 0x1000;

    ShowMessages("\n\nDebug\n-----");

    if (DebugDirectory->VirtualAddress == 0 || DebugDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Debug directory :", "empty");
        return;
    }

    DWORD EntryCount = DebugDirectory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (EntryCount == 0)
    {
        ShowMessages("\n%-36s%s", "Warning :", "debug directory is smaller than one descriptor");
        return;
    }

    BOOLEAN EntryCapped = FALSE;
    if (EntryCount > MaxDebugEntries)
    {
        EntryCount  = MaxDebugEntries;
        EntryCapped = TRUE;
    }

    for (DWORD EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++)
    {
        DWORD EntryRva = 0;
        if (!PeAddDword(DebugDirectory->VirtualAddress, EntryIndex * (DWORD)sizeof(IMAGE_DEBUG_DIRECTORY), &EntryRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "debug directory RVA overflow");
            break;
        }

        const IMAGE_DEBUG_DIRECTORY * Entry = NULL;
        if (!PeGetPointerAtRva(Reader, EntryRva, sizeof(IMAGE_DEBUG_DIRECTORY), (const BYTE **)&Entry))
        {
            ShowMessages("\n%-36s%s", "Warning :", "debug directory entry is not mapped");
            break;
        }

        ShowMessages("\n[%u] characteristics %#x time date stamp %#x major %u minor %u type %u size %#x address %#x raw %#x",
                     EntryIndex,
                     Entry->Characteristics,
                     Entry->TimeDateStamp,
                     Entry->MajorVersion,
                     Entry->MinorVersion,
                     Entry->Type,
                     Entry->SizeOfData,
                     Entry->AddressOfRawData,
                     Entry->PointerToRawData);

        if (Entry->SizeOfData != 0 && Entry->PointerToRawData != 0)
        {
            const BYTE * Payload = NULL;
            ShowMessages("\n%-36s%s", "    Payload bounds :", PeImageReaderGetPointerAtOffset(Reader, Entry->PointerToRawData, Entry->SizeOfData, &Payload) ? "valid" : "invalid");
        }

        PeShowDebugCodeView(Reader, Entry);
    }

    if (EntryCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, debug entry cap reached :", MaxDebugEntries);
    }
}

static BOOLEAN
PeLoadConfigHasField(DWORD AvailableSize, SIZE_T Offset, SIZE_T FieldSize)
{
    return Offset <= AvailableSize && FieldSize <= AvailableSize - Offset;
}

static VOID
PeShowLoadConfigDword(const BYTE * Config, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label)
{
    if (PeLoadConfigHasField(AvailableSize, Offset, sizeof(DWORD)))
    {
        ShowMessages("\n%-36s%#x", Label, PeReadDwordFromBuffer(Config, Offset));
    }
}

static VOID
PeShowLoadConfigPointer(const BYTE * Config, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label, BOOLEAN Is32Bit)
{
    SIZE_T FieldSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    if (PeLoadConfigHasField(AvailableSize, Offset, FieldSize))
    {
        ShowMessages("\n%-36s%#llx", Label, PeReadLoadConfigPointer(Config, Offset, Is32Bit));
    }
}

static VOID
PeShowLoadConfig(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * LoadConfigDirectory, BOOLEAN Is32Bit)
{
    ShowMessages("\n\nLoad config\n-----------");

    if (LoadConfigDirectory->VirtualAddress == 0 || LoadConfigDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "empty");
        return;
    }

    if (LoadConfigDirectory->Size < sizeof(DWORD))
    {
        ShowMessages("\n%-36s%s", "Warning :", "load config size is too small");
        return;
    }

    const BYTE * Config = NULL;
    if (!PeGetPointerAtRva(Reader, LoadConfigDirectory->VirtualAddress, sizeof(DWORD), &Config))
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "not mapped");
        return;
    }

    DWORD ConfigSize    = PeReadDwordFromBuffer(Config, 0);
    DWORD AvailableSize = LoadConfigDirectory->Size < ConfigSize ? LoadConfigDirectory->Size : ConfigSize;

    ShowMessages("\n%-36s%#x", "Size :", ConfigSize);
    if (AvailableSize < sizeof(DWORD))
    {
        ShowMessages("\n%-36s%s", "Warning :", "load config size is too small");
        return;
    }

    if (!PeGetPointerAtRva(Reader, LoadConfigDirectory->VirtualAddress, AvailableSize, &Config))
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "invalid bounds");
        return;
    }

    PeShowLoadConfigDword(Config, AvailableSize, 4, "Time date stamp :");
    if (PeLoadConfigHasField(AvailableSize, 8, sizeof(WORD)))
    {
        WORD MajorVersion = 0;
        CopyMemory(&MajorVersion, Config + 8, sizeof(MajorVersion));
        ShowMessages("\n%-36s%u", "Major version :", MajorVersion);
    }
    if (PeLoadConfigHasField(AvailableSize, 10, sizeof(WORD)))
    {
        WORD MinorVersion = 0;
        CopyMemory(&MinorVersion, Config + 10, sizeof(MinorVersion));
        ShowMessages("\n%-36s%u", "Minor version :", MinorVersion);
    }
    PeShowLoadConfigDword(Config, AvailableSize, 12, "Global flags clear :");
    PeShowLoadConfigDword(Config, AvailableSize, 16, "Global flags set :");
    PeShowLoadConfigDword(Config, AvailableSize, 20, "Critical section timeout :");
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 44 : 72, "Process heap flags :");
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 60 : 88, "Security cookie :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 64 : 96, "SE handler table :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 68 : 104, "SE handler count :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 72 : 112, "Guard CF check pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 76 : 120, "Guard CF dispatch pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 80 : 128, "Guard CF function table :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 84 : 136, "Guard CF function count :", Is32Bit);
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 88 : 144, "Guard flags :");
}

static VOID
PeShowDataDirectories(PPE_IMAGE_READER             Reader,
                      const IMAGE_DATA_DIRECTORY * Directories,
                      DWORD                        NumberOfRvaAndSizes)
{
    ShowMessages("\n\nData directories\n----------------");
    ShowMessages("\n%-36s%u", "Number of RVA and sizes :", NumberOfRvaAndSizes);
    if (NumberOfRvaAndSizes > IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
    {
        ShowMessages("\n%-36s%s", "Warning :", "number of RVA and sizes exceeds PE directory table");
    }

    for (UINT32 Index = 0; Index < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; Index++)
    {
        const char * Name         = PeGetDataDirectoryName(Index);
        const char * AddressLabel = Index == IMAGE_DIRECTORY_ENTRY_SECURITY ? "file offset" : "RVA";

        if (Index >= NumberOfRvaAndSizes)
        {
            ShowMessages("\n[%2u] %-27s %s", Index, Name, "not declared");
            continue;
        }

        const IMAGE_DATA_DIRECTORY * Directory = &Directories[Index];
        if (Directory->VirtualAddress == 0 || Directory->Size == 0)
        {
            ShowMessages("\n[%2u] %-27s %s %#x, size %#x, %s",
                         Index,
                         Name,
                         AddressLabel,
                         Directory->VirtualAddress,
                         Directory->Size,
                         "empty");
            continue;
        }

        const BYTE * Pointer    = NULL;
        SIZE_T       FileOffset = 0;

        if (Index == IMAGE_DIRECTORY_ENTRY_SECURITY)
        {
            BOOLEAN HasBounds = PeImageReaderGetPointerAtOffset(Reader,
                                                                Directory->VirtualAddress,
                                                                Directory->Size,
                                                                &Pointer);

            ShowMessages("\n[%2u] %-27s file offset %#x, size %#x, %s",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         HasBounds ? "valid bounds" : "invalid bounds");
            if (HasBounds)
            {
                ShowMessages(", mapped file offset %#llx", (UINT64)Directory->VirtualAddress);
            }

            continue;
        }

        if (PeImageReaderRvaToFileOffset(Reader, Directory->VirtualAddress, Directory->Size, &FileOffset))
        {
            ShowMessages("\n[%2u] %-27s RVA %#x, size %#x, mapped, mapped file offset %#llx",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         (UINT64)FileOffset);
        }
        else
        {
            ShowMessages("\n[%2u] %-27s RVA %#x, size %#x, %s",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         "not mapped");
        }
    }
}

static VOID
PeShowOverlayAndWarnings(PPE_IMAGE_READER             Reader,
                         WORD                         Machine,
                         WORD                         OptionalHeaderMagic,
                         DWORD                        AddressOfEntryPoint,
                         DWORD                        SizeOfImage,
                         DWORD                        SizeOfHeaders,
                         const IMAGE_DATA_DIRECTORY * Directories,
                         DWORD                        NumberOfRvaAndSizes)
{
    ULONGLONG OverlayOffset     = SizeOfHeaders < Reader->ImageSize ? SizeOfHeaders : Reader->ImageSize;
    BOOLEAN   OverlayUnreliable = FALSE;

    ShowMessages("\n\nOverlay and warnings\n--------------------");

    if ((ULONGLONG)SizeOfHeaders > (ULONGLONG)Reader->ImageSize)
    {
        ShowMessages("\n%-36s%s", "Warning :", "size of headers is larger than file size");
    }

    if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC && Machine == IMAGE_FILE_MACHINE_I386)
    {
        ShowMessages("\n%-36s%s", "Warning :", "PE32+ optional header with i386 machine type");
    }
    else if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
             (Machine == IMAGE_FILE_MACHINE_AMD64 || Machine == IMAGE_FILE_MACHINE_IA64))
    {
        ShowMessages("\n%-36s%s", "Warning :", "PE32 optional header with 64-bit machine type");
    }

    for (UINT32 i = 0; i < Reader->FileHeader->NumberOfSections; i++)
    {
        CHAR                         SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        const IMAGE_SECTION_HEADER * Section     = &Reader->SectionHeaders[i];
        DWORD                        VirtualSpan = Section->Misc.VirtualSize > Section->SizeOfRawData ? Section->Misc.VirtualSize : Section->SizeOfRawData;
        DWORD                        VirtualEnd  = 0;

        PeImageReaderGetSectionName(Section, SectionName, sizeof(SectionName));

        if (VirtualSpan != 0)
        {
            if (!PeAddDword(Section->VirtualAddress, VirtualSpan, &VirtualEnd))
            {
                ShowMessages("\n%-36ssection '%s' RVA range overflows", "Warning :", SectionName);
            }
            else if (SizeOfImage != 0 && VirtualEnd > SizeOfImage)
            {
                ShowMessages("\n%-36ssection '%s' RVA range exceeds SizeOfImage", "Warning :", SectionName);
            }
        }

        if (Section->SizeOfRawData != 0)
        {
            ULONGLONG RawStart = Section->PointerToRawData;
            ULONGLONG RawEnd   = 0;

            if (!PeAddUlonglong(RawStart, Section->SizeOfRawData, &RawEnd))
            {
                ShowMessages("\n%-36ssection '%s' raw data range overflows", "Warning :", SectionName);
                OverlayUnreliable = TRUE;
            }
            else if (RawStart > (ULONGLONG)Reader->ImageSize || RawEnd > (ULONGLONG)Reader->ImageSize)
            {
                ShowMessages("\n%-36ssection '%s' raw data is outside file", "Warning :", SectionName);
                OverlayUnreliable = TRUE;
            }
            else if (RawEnd > OverlayOffset)
            {
                OverlayOffset = RawEnd;
            }
        }
    }

    if (Reader->FileHeader->NumberOfSections > 1)
    {
        PE_RAW_SECTION_RANGE * Ranges = (PE_RAW_SECTION_RANGE *)malloc(sizeof(PE_RAW_SECTION_RANGE) * Reader->FileHeader->NumberOfSections);
        UINT32                 Count  = 0;

        if (Ranges == NULL)
        {
            ShowMessages("\n%-36s%s", "Warning :", "raw section overlap check skipped, allocation failed");
        }
        else
        {
            for (UINT32 i = 0; i < Reader->FileHeader->NumberOfSections; i++)
            {
                const IMAGE_SECTION_HEADER * Section = &Reader->SectionHeaders[i];
                ULONGLONG                    End;

                if (Section->SizeOfRawData == 0 ||
                    !PeAddUlonglong(Section->PointerToRawData, Section->SizeOfRawData, &End) ||
                    Section->PointerToRawData > Reader->ImageSize || End > Reader->ImageSize)
                {
                    continue;
                }

                Ranges[Count].Start   = Section->PointerToRawData;
                Ranges[Count].End     = End;
                Ranges[Count].Section = Section;
                Count++;
            }

            if (Count > 1)
            {
                UINT32 MaxEndIndex = 0;

                qsort(Ranges, Count, sizeof(Ranges[0]), PeCompareRawSectionRange);

                for (UINT32 i = 1; i < Count; i++)
                {
                    if (Ranges[i].Start < Ranges[MaxEndIndex].End)
                    {
                        CHAR LeftName[IMAGE_SIZEOF_SHORT_NAME + 1];
                        CHAR RightName[IMAGE_SIZEOF_SHORT_NAME + 1];

                        PeImageReaderGetSectionName(Ranges[MaxEndIndex].Section, LeftName, sizeof(LeftName));
                        PeImageReaderGetSectionName(Ranges[i].Section, RightName, sizeof(RightName));
                        ShowMessages("\n%-36sraw data overlap detected between '%s' and '%s'; additional overlaps may be omitted",
                                     "Warning :",
                                     LeftName,
                                     RightName);
                    }

                    if (Ranges[i].End > Ranges[MaxEndIndex].End)
                    {
                        MaxEndIndex = i;
                    }
                }
            }

            free(Ranges);
        }
    }

    if (AddressOfEntryPoint != 0)
    {
        BOOLEAN                      EntrypointFound   = FALSE;
        const IMAGE_SECTION_HEADER * EntrypointSection = NULL;

        for (UINT32 i = 0; i < Reader->FileHeader->NumberOfSections; i++)
        {
            const IMAGE_SECTION_HEADER * Section     = &Reader->SectionHeaders[i];
            DWORD                        VirtualSpan = Section->Misc.VirtualSize > Section->SizeOfRawData ? Section->Misc.VirtualSize : Section->SizeOfRawData;
            DWORD                        VirtualEnd  = 0;

            if (VirtualSpan == 0 || !PeAddDword(Section->VirtualAddress, VirtualSpan, &VirtualEnd))
            {
                continue;
            }

            if (AddressOfEntryPoint >= Section->VirtualAddress && AddressOfEntryPoint < VirtualEnd)
            {
                EntrypointFound   = TRUE;
                EntrypointSection = Section;
                break;
            }
        }

        if (!EntrypointFound)
        {
            ShowMessages("\n%-36s%s", "Warning :", "entrypoint is outside all sections");
        }
        else if ((EntrypointSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
        {
            CHAR SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

            PeImageReaderGetSectionName(EntrypointSection, SectionName, sizeof(SectionName));
            ShowMessages("\n%-36sentrypoint section '%s' is not executable", "Warning :", SectionName);
        }
    }

    if (OverlayUnreliable)
    {
        ShowMessages("\n%-36s%s", "Overlay :", "not computed because section raw data is invalid");
    }
    else if ((ULONGLONG)Reader->ImageSize > OverlayOffset)
    {
        ULONGLONG OverlaySize = (ULONGLONG)Reader->ImageSize - OverlayOffset;

        ShowMessages("\n%-36s%#llx", "Overlay offset :", OverlayOffset);
        ShowMessages("\n%-36s%#llx", "Overlay size :", OverlaySize);

        if (Directories != NULL && NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY)
        {
            const IMAGE_DATA_DIRECTORY * SecurityDirectory = &Directories[IMAGE_DIRECTORY_ENTRY_SECURITY];

            if (SecurityDirectory->VirtualAddress != 0 && SecurityDirectory->Size != 0 &&
                (ULONGLONG)SecurityDirectory->VirtualAddress >= OverlayOffset)
            {
                ShowMessages("\n%-36s%s", "Info :", "overlay includes certificate data; not necessarily suspicious");
            }
        }
    }
    else
    {
        ShowMessages("\n%-36s%s", "Overlay :", "none");
    }
}

static VOID
PeShowImports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ImportDirectory, BOOLEAN Is32Bit)
{
    const DWORD MaxImportDescriptors = 0x1000;
    const DWORD MaxTotalImports      = 0x2000;
    const DWORD MaxDllNameLength     = 0x200;
    const DWORD MaxImportNameLength  = 0x200;

    ShowMessages("\n\nImports\n-------");

    if (ImportDirectory->VirtualAddress == 0 || ImportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Import directory :", "empty");
        return;
    }

    const BYTE * DirectoryPointer = NULL;
    if (!PeGetPointerAtRva(Reader, ImportDirectory->VirtualAddress, sizeof(IMAGE_IMPORT_DESCRIPTOR), &DirectoryPointer))
    {
        ShowMessages("\n%-36s%s", "Import directory :", "not mapped");
        return;
    }

    DWORD DescriptorCount = ImportDirectory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    if (DescriptorCount == 0)
    {
        ShowMessages("\n%-36s%s", "Warning :", "import directory is smaller than one descriptor");
        return;
    }

    BOOLEAN DescriptorCapped = FALSE;
    if (DescriptorCount > MaxImportDescriptors)
    {
        DescriptorCount  = MaxImportDescriptors;
        DescriptorCapped = TRUE;
    }

    DWORD   TotalImports       = 0;
    BOOLEAN FoundTerminator    = FALSE;
    BOOLEAN TotalImportsCapped = FALSE;

    for (DWORD DescriptorIndex = 0; DescriptorIndex < DescriptorCount; DescriptorIndex++)
    {
        DWORD DescriptorRva = 0;

        if (!PeAddDword(ImportDirectory->VirtualAddress,
                        DescriptorIndex * (DWORD)sizeof(IMAGE_IMPORT_DESCRIPTOR),
                        &DescriptorRva))
        {
            break;
        }

        const IMAGE_IMPORT_DESCRIPTOR * Descriptor = NULL;
        if (!PeGetPointerAtRva(Reader,
                               DescriptorRva,
                               sizeof(IMAGE_IMPORT_DESCRIPTOR),
                               (const BYTE **)&Descriptor))
        {
            ShowMessages("\n%-36s%s", "Warning :", "import descriptor is not mapped");
            break;
        }

        if (Descriptor->OriginalFirstThunk == 0 && Descriptor->TimeDateStamp == 0 && Descriptor->ForwarderChain == 0 &&
            Descriptor->Name == 0 && Descriptor->FirstThunk == 0)
        {
            FoundTerminator = TRUE;
            break;
        }

        CHAR                   DllName[MaxDllNameLength + 1] = {0};
        PE_ASCII_STRING_STATUS DllNameStatus                 = PeReadAsciiStringAtRva(Reader,
                                                                                      Descriptor->Name,
                                                                                      MaxDllNameLength,
                                                                                      DllName,
                                                                                      sizeof(DllName));
        if (DllNameStatus != PeAsciiStringOk)
        {
            ShowMessages("\n[%u] DLL name %s", DescriptorIndex, PeAsciiStatusName(DllNameStatus));
            ShowMessages("\n%-36s%s", "Warning :", "skipping imports for descriptor with unreadable DLL name");
            continue;
        }

        ShowMessages("\n[%u] DLL name %s", DescriptorIndex, DllName);

        DWORD LookupThunkRva = Descriptor->OriginalFirstThunk != 0 ? Descriptor->OriginalFirstThunk : Descriptor->FirstThunk;
        if (LookupThunkRva == 0 || Descriptor->FirstThunk == 0)
        {
            ShowMessages("\n%-36s%s", "Warning :", "import thunk array is empty");
            continue;
        }

        DWORD ThunkSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);
        for (DWORD ThunkIndex = 0; TotalImports < MaxTotalImports; ThunkIndex++)
        {
            DWORD LookupEntryRva = 0;
            DWORD IatEntryRva    = 0;
            if (!PeAddDword(LookupThunkRva, ThunkIndex * ThunkSize, &LookupEntryRva) ||
                !PeAddDword(Descriptor->FirstThunk, ThunkIndex * ThunkSize, &IatEntryRva))
            {
                ShowMessages("\n%-36s%s", "Warning :", "import thunk RVA overflow");
                break;
            }

            ULONGLONG ThunkValue = 0;
            if (!PeReadThunkAtRva(Reader, LookupEntryRva, Is32Bit, &ThunkValue))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid thunk RVA :", LookupEntryRva);
                break;
            }

            if (ThunkValue == 0)
            {
                break;
            }

            ULONGLONG OrdinalFlag = Is32Bit ? IMAGE_ORDINAL_FLAG32 : IMAGE_ORDINAL_FLAG64;
            if ((ThunkValue & OrdinalFlag) != 0)
            {
                ShowMessages("\n    [%u] ordinal %u thunk RVA %#x", ThunkIndex, (UINT32)(ThunkValue & 0xffff), IatEntryRva);
                TotalImports++;
                continue;
            }

            if (ThunkValue > MAXDWORD)
            {
                ShowMessages("\n%-36s%#llx", "Warning, invalid import name RVA :", (UINT64)ThunkValue);
                TotalImports++;
                continue;
            }

            DWORD NameRva = (DWORD)ThunkValue;
            WORD  Hint    = 0;
            if (!PeReadWordAtRva(Reader, NameRva, &Hint))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid import hint RVA :", NameRva);
                TotalImports++;
                continue;
            }

            DWORD ImportNameRva = 0;
            if (!PeAddDword(NameRva, sizeof(WORD), &ImportNameRva))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid import name RVA :", NameRva);
                TotalImports++;
                continue;
            }

            CHAR                   ImportName[MaxImportNameLength + 1] = {0};
            PE_ASCII_STRING_STATUS ImportNameStatus                    = PeReadAsciiStringAtRva(Reader,
                                                                                                ImportNameRva,
                                                                                                MaxImportNameLength,
                                                                                                ImportName,
                                                                                                sizeof(ImportName));
            if (ImportNameStatus != PeAsciiStringOk)
            {
                ShowMessages("\n%-36s%#x, %s", "Warning, invalid import name RVA :", ImportNameRva, PeAsciiStatusName(ImportNameStatus));
                TotalImports++;
                continue;
            }

            ShowMessages("\n    [%u] hint %#x name %s thunk RVA %#x", ThunkIndex, Hint, ImportName, IatEntryRva);
            TotalImports++;
        }

        if (TotalImports >= MaxTotalImports)
        {
            TotalImportsCapped = TRUE;
            break;
        }
    }

    if (!FoundTerminator)
    {
        ShowMessages("\n%-36s%s", "Warning :", "import descriptor terminator not found before bound or cap");
    }

    if (DescriptorCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, descriptor cap reached :", MaxImportDescriptors);
    }

    if (TotalImportsCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, import cap reached :", MaxTotalImports);
    }
}

static BOOLEAN
PeValidateExportTable(PPE_IMAGE_READER Reader, DWORD TableRva, DWORD Count, DWORD EntrySize, const BYTE ** Pointer)
{
    DWORD TableSize = 0;

    if (Count == 0)
    {
        *Pointer = NULL;
        return TRUE;
    }

    if (EntrySize != 0 && Count > MAXDWORD / EntrySize)
    {
        return FALSE;
    }

    TableSize = Count * EntrySize;
    return PeGetPointerAtRva(Reader, TableRva, TableSize, Pointer);
}

static VOID
PeShowExportEntry(PPE_IMAGE_READER             Reader,
                  const IMAGE_DATA_DIRECTORY * ExportDirectory,
                  DWORD                        Index,
                  DWORD                        Ordinal,
                  const CHAR *                 Name,
                  DWORD                        FunctionRva)
{
    const DWORD MaxForwarderLength = 0x1000;

    if (PeRvaContainsRange(ExportDirectory->VirtualAddress, ExportDirectory->Size, FunctionRva, 1))
    {
        DWORD ForwarderRemaining = ExportDirectory->Size - (FunctionRva - ExportDirectory->VirtualAddress);
        DWORD ForwarderMaxLength = ForwarderRemaining < MaxForwarderLength ? ForwarderRemaining : MaxForwarderLength;

        CHAR                   Forwarder[MaxForwarderLength + 1] = {0};
        PE_ASCII_STRING_STATUS ForwarderStatus                   = PeReadAsciiStringAtRva(Reader,
                                                                                          FunctionRva,
                                                                                          ForwarderMaxLength,
                                                                                          Forwarder,
                                                                                          sizeof(Forwarder));
        if (ForwarderStatus == PeAsciiStringOk)
        {
            ShowMessages("\n[%u] ordinal %u name %s forwarder %s", Index, Ordinal, Name, Forwarder);
        }
        else
        {
            ShowMessages("\n[%u] ordinal %u name %s forwarder %s", Index, Ordinal, Name, PeAsciiStatusName(ForwarderStatus));
        }

        return;
    }

    ShowMessages("\n[%u] ordinal %u name %s RVA %#x", Index, Ordinal, Name, FunctionRva);
}

static VOID
PeShowExports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ExportDirectory)
{
    const DWORD MaxExports          = 0x2000;
    const DWORD MaxDllNameLength    = 0x200;
    const DWORD MaxExportNameLength = 0x200;

    ShowMessages("\n\nExports\n-------");

    if (ExportDirectory->VirtualAddress == 0 || ExportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Export directory :", "empty");
        return;
    }

    if (!PeRvaContainsRange(ExportDirectory->VirtualAddress,
                            ExportDirectory->Size,
                            ExportDirectory->VirtualAddress,
                            sizeof(IMAGE_EXPORT_DIRECTORY)))
    {
        ShowMessages("\n%-36s%s", "Export directory :", "invalid bounds");
        return;
    }

    const IMAGE_EXPORT_DIRECTORY * Directory = NULL;
    if (!PeGetPointerAtRva(Reader,
                           ExportDirectory->VirtualAddress,
                           sizeof(IMAGE_EXPORT_DIRECTORY),
                           (const BYTE **)&Directory))
    {
        ShowMessages("\n%-36s%s", "Export directory :", "not mapped");
        return;
    }

    CHAR                   DllName[MaxDllNameLength + 1] = {0};
    PE_ASCII_STRING_STATUS DllNameStatus                 = PeReadAsciiStringAtRva(Reader,
                                                                                  Directory->Name,
                                                                                  MaxDllNameLength,
                                                                                  DllName,
                                                                                  sizeof(DllName));
    ShowMessages("\n%-36s%s", "DLL name :", DllNameStatus == PeAsciiStringOk ? DllName : PeAsciiStatusName(DllNameStatus));
    ShowMessages("\n%-36s%u", "Ordinal base :", Directory->Base);
    ShowMessages("\n%-36s%u", "Address table count :", Directory->NumberOfFunctions);
    ShowMessages("\n%-36s%u", "Name pointer count :", Directory->NumberOfNames);

    const BYTE * AddressTablePointer     = NULL;
    const BYTE * NamePointerTablePointer = NULL;
    const BYTE * OrdinalTablePointer     = NULL;

    BOOLEAN AddressTableValid     = PeValidateExportTable(Reader,
                                                          Directory->AddressOfFunctions,
                                                          Directory->NumberOfFunctions,
                                                          sizeof(DWORD),
                                                          &AddressTablePointer);
    BOOLEAN NamePointerTableValid = PeValidateExportTable(Reader,
                                                          Directory->AddressOfNames,
                                                          Directory->NumberOfNames,
                                                          sizeof(DWORD),
                                                          &NamePointerTablePointer);
    BOOLEAN OrdinalTableValid     = PeValidateExportTable(Reader,
                                                          Directory->AddressOfNameOrdinals,
                                                          Directory->NumberOfNames,
                                                          sizeof(WORD),
                                                          &OrdinalTablePointer);

    if (!AddressTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export address table is not mapped");
        return;
    }

    if (!NamePointerTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export name pointer table is not mapped");
    }

    if (!OrdinalTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export ordinal table is not mapped");
    }

    DWORD   NamedCount  = Directory->NumberOfNames;
    BOOLEAN NamedCapped = FALSE;
    if (NamedCount > MaxExports)
    {
        NamedCount  = MaxExports;
        NamedCapped = TRUE;
    }

    DWORD   FunctionCount  = Directory->NumberOfFunctions;
    BOOLEAN FunctionCapped = FALSE;
    if (FunctionCount > MaxExports)
    {
        FunctionCount  = MaxExports;
        FunctionCapped = TRUE;
    }

    BYTE NamedAddressIndexes[MaxExports] = {0};

    if (NamePointerTableValid && OrdinalTableValid)
    {
        for (DWORD NameIndex = 0; NameIndex < NamedCount; NameIndex++)
        {
            DWORD NameRva      = 0;
            WORD  AddressIndex = 0;

            CopyMemory(&NameRva, NamePointerTablePointer + (NameIndex * sizeof(DWORD)), sizeof(NameRva));
            CopyMemory(&AddressIndex, OrdinalTablePointer + (NameIndex * sizeof(WORD)), sizeof(AddressIndex));

            if (AddressIndex >= Directory->NumberOfFunctions)
            {
                ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
                continue;
            }

            DWORD FunctionRva = 0;
            CopyMemory(&FunctionRva, AddressTablePointer + (AddressIndex * sizeof(DWORD)), sizeof(FunctionRva));

            CHAR                   ExportName[MaxExportNameLength + 1] = {0};
            PE_ASCII_STRING_STATUS ExportNameStatus                    = PeReadAsciiStringAtRva(Reader,
                                                                                                NameRva,
                                                                                                MaxExportNameLength,
                                                                                                ExportName,
                                                                                                sizeof(ExportName));
            if (ExportNameStatus != PeAsciiStringOk)
            {
                ShowMessages("\n%-36s%#x, %s", "Warning, invalid export name RVA :", NameRva, PeAsciiStatusName(ExportNameStatus));
                continue;
            }

            DWORD DisplayOrdinal = 0;
            if (!PeAddDword(Directory->Base, AddressIndex, &DisplayOrdinal))
            {
                ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
                continue;
            }

            if (AddressIndex < FunctionCount)
            {
                NamedAddressIndexes[AddressIndex] = TRUE;
            }

            PeShowExportEntry(Reader,
                              ExportDirectory,
                              NameIndex,
                              DisplayOrdinal,
                              ExportName,
                              FunctionRva);
        }
    }

    for (DWORD AddressIndex = 0; AddressIndex < FunctionCount; AddressIndex++)
    {
        if (NamedAddressIndexes[AddressIndex])
        {
            continue;
        }

        DWORD FunctionRva = 0;
        CopyMemory(&FunctionRva, AddressTablePointer + (AddressIndex * sizeof(DWORD)), sizeof(FunctionRva));

        DWORD DisplayOrdinal = 0;
        if (!PeAddDword(Directory->Base, AddressIndex, &DisplayOrdinal))
        {
            ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
            continue;
        }

        PeShowExportEntry(Reader,
                          ExportDirectory,
                          AddressIndex,
                          DisplayOrdinal,
                          "<ordinal-only>",
                          FunctionRva);
    }

    if (NamedCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, named export cap reached :", MaxExports);
    }

    if (FunctionCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, export cap reached :", MaxExports);
    }
}

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
FindRichHeader(PIMAGE_DOS_HEADER DosHeader, DWORD FileSize, CHAR Key[])
{
    //
    // Get base address for offset calculations
    //
    CHAR * BaseAddr = (CHAR *)DosHeader;

    //
    // Get PE header offset - this defines our search boundary
    //
    DWORD Offset = DosHeader->e_lfanew;

    if (FileSize < sizeof(IMAGE_DOS_HEADER) ||
        DosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        Offset < sizeof(IMAGE_DOS_HEADER) ||
        Offset > FileSize ||
        Offset < 8)
    {
        return 0;
    }

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
        WORD ProdID = ((UCHAR)RichHeaderPtr[i + 3] << 8) | (UCHAR)RichHeaderPtr[i + 2];

        //
        // Extract Build ID (bytes 0-1 of entry, little-endian)
        //
        WORD BuildID = ((UCHAR)RichHeaderPtr[i + 1] << 8) | (UCHAR)RichHeaderPtr[i];

        //
        // Extract Use Count (bytes 4-7 of entry, little-endian 32-bit)
        //
        DWORD UseCount = ((UCHAR)RichHeaderPtr[i + 7] << 24) |
                         ((UCHAR)RichHeaderPtr[i + 6] << 16) |
                         ((UCHAR)RichHeaderPtr[i + 5] << 8) |
                         (UCHAR)RichHeaderPtr[i + 4];

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
 * @return INT Size of the Rich header in bytes, or 0 if DanS signature not found
 *
 */
INT
DecryptRichHeader(CHAR Key[], INT Index, CHAR * DataPtr)
{
    //
    // Copy the XOR key from the 4 bytes immediately following "Rich"
    //
    memcpy(Key, DataPtr + (Index + 4), 4);

    //
    // Start searching backwards from just before the "Rich" signature
    //
    INT IndexPointer   = Index - 4;
    INT RichHeaderSize = 0;

    //
    // Search backwards for the DanS signature that marks the beginning
    //
    while (IndexPointer >= 0)
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
            return RichHeaderSize;
        }
    }

    return 0;
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
PeHexDump(CHAR * Ptr, INT Size, INT SecAddress)
{
    INT i = 1, Temp = 0;

    //
    // Buffer to store the character dump displayed at the
    // right side
    //
    WCHAR Buf[18];
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
    RICH_HEADER_INFO             PeFileRichHeaderInfo {0};
    RICH_HEADER                  PeFileRichHeader {0};
    BOOLEAN                      Result = FALSE, RichFound = FALSE, SectionFound = FALSE;
    HANDLE                       MapObjectHandle = NULL, FileHandle = INVALID_HANDLE_VALUE; // File Mapping Object
    UINT32                       NumberOfSections;                                          // Number of sections
    LPVOID                       BaseAddr = NULL;                                           // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER            DosHeader;                                                 // Pointer to DOS Header
    PIMAGE_NT_HEADERS32          NtHeader32 = NULL;                                         // Pointer to NT Header 32 bit
    PIMAGE_NT_HEADERS64          NtHeader64 = NULL;                                         // Pointer to NT Header 64 bit
    IMAGE_FILE_HEADER            Header;                                                    // Pointer to image file header of NT Header
    IMAGE_OPTIONAL_HEADER32      OpHeader32;                                                // Optional Header of PE files present in NT Header structure
    IMAGE_OPTIONAL_HEADER64      OpHeader64;                                                // Optional Header of PE files present in NT Header structure
    const IMAGE_SECTION_HEADER * SecHeader;                                                 // Section Header or Section Table Header
    PE_IMAGE_READER              Reader;
    IMAGE_DATA_DIRECTORY         EmptyDirectory = {0};
    LARGE_INTEGER                FileSize;
    CHAR                         Key[4];
    INT                          RichHeaderOffset;
    time_t                       TimeDateStamp;

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, could not open the file specified\n");
        return FALSE;
    }

    if (!GetFileSizeEx(FileHandle, &FileSize) || FileSize.QuadPart < (LONGLONG)sizeof(IMAGE_DOS_HEADER))
    {
        CloseHandle(FileHandle);
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
        CloseHandle(MapObjectHandle);
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    // Get the DOS Header Base
    //
    DosHeader = (PIMAGE_DOS_HEADER)BaseAddr; // 0x04000000

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        DosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        (LONGLONG)DosHeader->e_lfanew > FileSize.QuadPart ||
        (LONGLONG)DosHeader->e_lfanew + (LONGLONG)(Is32Bit ? sizeof(IMAGE_NT_HEADERS32) : sizeof(IMAGE_NT_HEADERS64)) > FileSize.QuadPart)
    {
        ShowMessages("\nGiven File is not a valid PE file\n");
        Result = FALSE;
        goto Finished;
    }

    if (!PeImageReaderInitialize((const BYTE *)BaseAddr, (SIZE_T)FileSize.QuadPart, &Reader))
    {
        ShowMessages("\nGiven File is not a valid PE file\n");
        Result = FALSE;
        goto Finished;
    }

    RichHeaderOffset = FindRichHeader(DosHeader, (DWORD)FileSize.QuadPart, Key);

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

        if (RichHeaderSize < 16 || ((RichHeaderSize - 16) % 8) != 0 || (RichHeaderSize - 16) / 8 == 0 || IndexPointer < 0)
        {
            delete[] DataPtr;
            goto SkipRichHeader;
        }

        char * richHeaderPtr = new char[RichHeaderSize];
        memcpy(richHeaderPtr, DataPtr + IndexPointer, RichHeaderSize);
        delete[] DataPtr;

        FindRichEntries(richHeaderPtr, RichHeaderSize, Key, &PeFileRichHeaderInfo);
        PeFileRichHeader.Entries = new RICH_HEADER_ENTRY[PeFileRichHeaderInfo.Entries + 1];

        SetRichEntries(RichHeaderSize, richHeaderPtr, &PeFileRichHeader);
        RichFound = TRUE;
    }

SkipRichHeader:

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
    TimeDateStamp = Header.TimeDateStamp;
    ShowMessages("\n%-36s%s",
                 "Time Stamp :",
                 ctime(&TimeDateStamp));

    //
    // Determine number of sections
    //
    ShowMessages("%-36s%d", "No.sections(size) :", Header.NumberOfSections);
    ShowMessages("\n%-36s%d", "No.entries in symbol table :", Header.NumberOfSymbols);
    ShowMessages("\n%-36s%d",
                 "Size of optional header :",
                 Header.SizeOfOptionalHeader);
    ShowMessages("\n%-36s%#x", "Raw machine value :", Header.Machine);
    ShowMessages("\n%-36s%#x", "Pointer to symbol table :", Header.PointerToSymbolTable);
    ShowMessages("\n%-36s%#x", "Raw characteristics value :", Header.Characteristics);

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
        ShowMessages("\n%-36s%#x", "Raw optional header magic :", OpHeader32.Magic);
        PeShowEntrypointFileOffset(&Reader, OpHeader32.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader32.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", PeGetSubsystemName(OpHeader32.Subsystem));
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader32.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%#x", "File Alignment :", OpHeader32.FileAlignment);
        ShowMessages("\n%-36s%#x", "Size of Image :", OpHeader32.SizeOfImage);
        ShowMessages("\n%-36s%#x", "Size of Headers :", OpHeader32.SizeOfHeaders);
        ShowMessages("\n%-36s%#x", "Raw DLL characteristics value :", OpHeader32.DllCharacteristics);
        ShowMessages("\n%-36s", "DLL characteristics flags :");
        PeShowDllCharacteristics(OpHeader32.DllCharacteristics);
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
        PeShowDataDirectories(&Reader, OpHeader32.DataDirectory, OpHeader32.NumberOfRvaAndSizes);
        PeShowTls(&Reader,
                  OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] : &EmptyDirectory,
                  TRUE,
                  OpHeader32.ImageBase);
        PeShowDebug(&Reader,
                    OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DEBUG ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] : &EmptyDirectory);
        PeShowLoadConfig(&Reader,
                         OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] : &EmptyDirectory,
                         TRUE);
        PeShowImports(&Reader,
                      OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] : &EmptyDirectory,
                      TRUE);
        PeShowExports(&Reader,
                      OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : &EmptyDirectory);
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
        ShowMessages("\n%-36s%#x", "Raw optional header magic :", OpHeader64.Magic);
        PeShowEntrypointFileOffset(&Reader, OpHeader64.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader64.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", PeGetSubsystemName(OpHeader64.Subsystem));
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader64.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%#x", "File Alignment :", OpHeader64.FileAlignment);
        ShowMessages("\n%-36s%#x", "Size of Image :", OpHeader64.SizeOfImage);
        ShowMessages("\n%-36s%#x", "Size of Headers :", OpHeader64.SizeOfHeaders);
        ShowMessages("\n%-36s%#x", "Raw DLL characteristics value :", OpHeader64.DllCharacteristics);
        ShowMessages("\n%-36s", "DLL characteristics flags :");
        PeShowDllCharacteristics(OpHeader64.DllCharacteristics);
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
        PeShowDataDirectories(&Reader, OpHeader64.DataDirectory, OpHeader64.NumberOfRvaAndSizes);
        PeShowTls(&Reader,
                  OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] : &EmptyDirectory,
                  FALSE,
                  OpHeader64.ImageBase);
        PeShowDebug(&Reader,
                    OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DEBUG ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] : &EmptyDirectory);
        PeShowLoadConfig(&Reader,
                         OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] : &EmptyDirectory,
                         FALSE);
        PeShowImports(&Reader,
                      OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] : &EmptyDirectory,
                      FALSE);
        PeShowExports(&Reader,
                      OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : &EmptyDirectory);
    }

    ShowMessages("\n\nDumping Sections Header "
                 "Info....\n--------------------------------");

    //
    // Retrieve a pointer to First Section Header(or Section Table Entry)
    //
    SecHeader        = Reader.SectionHeaders;
    NumberOfSections = Reader.FileHeader->NumberOfSections;

    for (UINT32 i = 0; i < NumberOfSections; i++, SecHeader++)
    {
        CHAR         SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        const BYTE * Pointer       = NULL;
        const CHAR * RawDataBounds = "empty";

        PeImageReaderGetSectionName(SecHeader, SectionName, sizeof(SectionName));

        if (SecHeader->SizeOfRawData != 0)
        {
            RawDataBounds = PeImageReaderGetPointerAtOffset(&Reader,
                                                            SecHeader->PointerToRawData,
                                                            SecHeader->SizeOfRawData,
                                                            &Pointer)
                                ? "valid"
                                : "invalid";
        }

        ShowMessages("\n\nSection Info (%d of %d)", i + 1, NumberOfSections);

        ShowMessages("\n---------------------");
        ShowMessages("\n%-36s%s", "Section Header name : ", SectionName);
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
        ShowMessages("\n%-36s%s", "Raw data bounds :", RawDataBounds);
        if (SecHeader->SizeOfRawData != 0 && SecHeader->Misc.VirtualSize > SecHeader->SizeOfRawData)
        {
            ShowMessages("\n%-36s%s", "Warning :", "virtual size is larger than raw data");
        }
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
            if (!_strcmpi(SectionToShow, SectionName))
            {
                SectionFound = TRUE;

                if (SecHeader->SizeOfRawData != 0)
                {
                    if (Pointer == NULL)
                    {
                        ShowMessages("\nerr, invalid section raw data\n");
                        continue;
                    }

                    if (Is32Bit)
                    {
                        PeHexDump((char *)Pointer,
                                  SecHeader->SizeOfRawData,
                                  OpHeader32.ImageBase + SecHeader->VirtualAddress);
                    }
                    else
                    {
                        PeHexDump((char *)Pointer,
                                  SecHeader->SizeOfRawData,
                                  (int)(OpHeader64.ImageBase + SecHeader->VirtualAddress));
                    }
                }
            }
        }
    }

    PeShowOverlayAndWarnings(&Reader,
                             Header.Machine,
                             Is32Bit ? OpHeader32.Magic : OpHeader64.Magic,
                             Is32Bit ? OpHeader32.AddressOfEntryPoint : OpHeader64.AddressOfEntryPoint,
                             Is32Bit ? OpHeader32.SizeOfImage : OpHeader64.SizeOfImage,
                             Is32Bit ? OpHeader32.SizeOfHeaders : OpHeader64.SizeOfHeaders,
                             Is32Bit ? OpHeader32.DataDirectory : OpHeader64.DataDirectory,
                             Is32Bit ? OpHeader32.NumberOfRvaAndSizes : OpHeader64.NumberOfRvaAndSizes);

    if (SectionToShow != NULL && !SectionFound)
    {
        ShowMessages("\nerr, section '%s' was not found\n", SectionToShow);
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
    if (PeFileRichHeaderInfo.PtrToBuffer != NULL)
    {
        delete[] PeFileRichHeaderInfo.PtrToBuffer;
    }

    if (PeFileRichHeader.Entries != NULL)
    {
        delete[] PeFileRichHeader.Entries;
    }

    if (BaseAddr != NULL)
    {
        UnmapViewOfFile(BaseAddr);
    }

    if (MapObjectHandle != NULL)
    {
        CloseHandle(MapObjectHandle);
    }

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FileHandle);
    }

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
    BOOLEAN             Result = FALSE;
    HANDLE              MapObjectHandle, FileHandle; // File Mapping Object
    LPVOID              BaseAddr;                    // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER   DosHeader;                   // Pointer to DOS Header
    PIMAGE_NT_HEADERS32 NtHeader32 = NULL;           // Pointer to NT Header 32 bit
    PE_IMAGE_READER     Reader;
    LARGE_INTEGER       FileSize;

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, unable to read the file (%x)\n", GetLastError());
        return FALSE;
    };

    if (!GetFileSizeEx(FileHandle, &FileSize) || FileSize.QuadPart < (LONGLONG)sizeof(IMAGE_DOS_HEADER))
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

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
        CloseHandle(MapObjectHandle);
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

    if (DosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        (LONGLONG)DosHeader->e_lfanew + (LONGLONG)sizeof(IMAGE_NT_HEADERS32) > FileSize.QuadPart)
    {
        Result = FALSE;

        ShowMessages("err, invalid image NT header offset\n");
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

    if (!PeImageReaderInitialize((const BYTE *)BaseAddr, (SIZE_T)FileSize.QuadPart, &Reader))
    {
        Result = FALSE;

        ShowMessages("err, the selected file is not in a valid PE format\n");
        goto Finished;
    }

    //
    // Only few are determined (for remaining refer
    // to the above specification)
    //
    switch (Reader.FileHeader->Machine)
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
    CloseHandle(FileHandle);

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
    return HyperDbgGetImmediateValueOnEaxForSyscallNumber((UCHAR *)TargetFunc, 30, TRUE);
}
