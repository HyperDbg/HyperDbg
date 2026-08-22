/**
 * @file pci-id.cpp
 * @author Bj�rn Ruytenberg (bjorn@bjornweb.nl)
 * @brief Provides runtime access to PCI ID database
 * @details
 * @version 0.12
 * @date 2024-12-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

static CHAR * PciIdDatabaseBuffer = NULL;

/**
 * @brief Trims whitespaces in passed string
 *
 * @param Str
 * @param MaxLen
 * @return CHAR*
 */
CHAR *
TrimWhitespace(CHAR * Str, UINT8 MaxLen)
{
    CHAR * End;
    while (*Str == ' ')
        Str++; // Trim leading space
    if (*Str == '\0')
        return Str;
    End = Str + PlatformStrnlen(Str, MaxLen) - 1;
    while (End > Str && (*End == ' ' || *End == '\n' || *End == '\r'))
        End--;
    *(End + 1) = '\0';
    return Str;
}

/**
 * @brief Converts passed string to lowercase
 *
 * @param Str
 * @return CHAR*
 */
CHAR *
ToLower(CHAR * Str)
{
    UINT8  StrLength   = (UINT8)PlatformStrnlen(Str, PCI_ID_AS_STR_LENGTH);
    CHAR * CurrentChar = Str;

    while (CurrentChar < Str + StrLength)
    {
        *CurrentChar = tolower(*CurrentChar);
        CurrentChar++;
    }
    return Str;
}

/**
 * @brief Read line from string. Treats SrcBuffer as a stream (similar to fgets and friends), i.e. updates SrcBuffer by number of characters read.
 *
 * @param DestBuffer
 * @param CharLimit
 * @param SrcBuffer
 * @return CHAR*
 */
CHAR *
ReadLine(CHAR * DestBuffer, UINT64 CharLimit, CHAR ** SrcBuffer)
{
    CHAR * Line = strchr(*SrcBuffer, '\n');
    if (!Line)
    {
        return NULL;
    }
    else
    {
        //
        // The copy length is clamped to the destination, otherwise a line longer than
        // CharLimit makes strncpy_s() invoke the invalid parameter handler
        //
        SIZE_T LineLength = (SIZE_T)(Line - *SrcBuffer);

        if (LineLength > CharLimit - 1)
        {
            LineLength = (SIZE_T)(CharLimit - 1);
        }

        PlatformStrNCpy(DestBuffer, (SIZE_T)CharLimit, *SrcBuffer, LineLength);
        *SrcBuffer += (Line - *SrcBuffer + 1);
        return *SrcBuffer;
    }
}

/**
 * @brief Get Vendor by PCI ID, encoded in ASCII. Do not call directly - use GetVendorById() instead.
 *
 * @param Filename
 * @param VendorId
 * @return Vendor *
 */
Vendor *
GetVendorByIdStr(const CHAR * Filename, const CHAR * VendorId)
{
    Vendor *    MatchedVendor = NULL;
    BOOLEAN     FoundVendorId = FALSE;
    Device *    LastDevice    = NULL;
    SubDevice * LastSubDevice = NULL;
    CHAR *      PciIdDbBufPtr = NULL;
    CHAR        Line[1024]    = {'\0'};

    if (!PciIdDatabaseBuffer)
    {
        FILE * f      = fopen(Filename, "rb");
        SIZE_T Length = 0;

        if (f == NULL)
        {
            ShowMessages("err, cannot open file '%s' (error 0x%x)\n", Filename, errno);
            return NULL;
        }

        fseek(f, 0, SEEK_END);

        LONG FileSize = ftell(f);

        if (FileSize < 0)
        {
            ShowMessages("err, cannot determine the size of file '%s' (error: 0x%x)\n", Filename, errno);
            fclose(f);
            return NULL;
        }

        Length = (SIZE_T)FileSize;

        //
        // One extra byte is allocated for the null terminator, as the buffer is later
        // walked with strchr() by ReadLine() and would otherwise be read past its end
        //
        PciIdDatabaseBuffer = (CHAR *)malloc(Length + 1);
        if (!PciIdDatabaseBuffer)
        {
            fclose(f);
            return NULL;
        }

        fseek(f, 0, SEEK_SET);

        SIZE_T BytesRead = fread(PciIdDatabaseBuffer, 1, Length, f);
        fclose(f);

        PciIdDatabaseBuffer[BytesRead] = '\0';
    }

    PciIdDbBufPtr = PciIdDatabaseBuffer;

    while (ReadLine(Line, sizeof(Line), &PciIdDbBufPtr) != NULL)
    {
        CHAR FormatStr[24];

        // Skip comments and empty lines
        if (Line[0] == '#' || Line[0] == '\0')
        {
            continue;
        }

        // Find vendor
        // We assume PCI ID database comprises unique entries only, i.e. we return the first matching entry
        if (Line[0] != '\t' && FoundVendorId == FALSE)
        {
            CHAR VendorBuf[PCI_ID_AS_STR_LENGTH + 1], VendorNameBuf[PCI_NAME_STR_LENGTH + 1];

            snprintf(FormatStr, sizeof(FormatStr), "%%4s %%%d[^\n]", PCI_NAME_STR_LENGTH); // FormatStr = "%4s %PCI_NAME_STR_LENGTH[^\n]"
            if (sscanf(Line, FormatStr, VendorBuf, VendorNameBuf) == 2)
            {
                //
                // VendorId is a pointer, so sizeof() on it yielded the pointer size
                // rather than the length of a PCI vendor id
                //
                if (strncmp(VendorBuf, VendorId, PCI_ID_AS_STR_LENGTH) == 0)
                {
                    //
                    // calloc() so that the Devices list head starts out empty: it is
                    // only assigned once a device line is parsed, and FreeVendor()
                    // would otherwise walk an uninitialized pointer for a vendor that
                    // has no devices listed
                    //
                    MatchedVendor = (Vendor *)calloc(1, sizeof(Vendor));
                    if (!MatchedVendor)
                    {
                        return NULL;
                    }

                    INT Result = sscanf(VendorBuf, "%hx", &(MatchedVendor->VendorId));
                    if (Result != 1)
                    {
                        FreeVendor(MatchedVendor);
                        return NULL;
                    }
                    PlatformStrNCpy(MatchedVendor->VendorName, sizeof(MatchedVendor->VendorName), TrimWhitespace(VendorNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
                    FoundVendorId = TRUE;
                }
            }
        }
        // Get all devices for vendor
        else if (Line[0] == '\t' && Line[1] != '\t' && FoundVendorId == TRUE)
        {
            CHAR DeviceBuf[PCI_ID_AS_STR_LENGTH + 1], DeviceNameBuf[PCI_NAME_STR_LENGTH + 1];

            snprintf(FormatStr, sizeof(FormatStr), "%%4s %%%d[^\n]", PCI_NAME_STR_LENGTH); // FormatStr = "%4s %PCI_NAME_STR_LENGTH[^\n]"
            if (sscanf(Line + 1, FormatStr, DeviceBuf, DeviceNameBuf) == 2)
            {
                Device * NewDevice = (Device *)malloc(sizeof(Device));
                if (!NewDevice)
                {
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                int Result = sscanf(DeviceBuf, "%hx", &(NewDevice->DeviceId));
                if (Result != 1)
                {
                    //
                    // NewDevice is not linked into the vendor's list yet, so it has to
                    // be released separately from FreeVendor()
                    //
                    free(NewDevice);
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                PlatformStrNCpy(NewDevice->DeviceName, sizeof(NewDevice->DeviceName), TrimWhitespace(DeviceNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
                NewDevice->SubDevices = NULL;
                NewDevice->Next       = NULL;

                if (LastDevice)
                {
                    LastDevice->Next = NewDevice;
                }
                else
                {
                    MatchedVendor->Devices = NewDevice; // First device
                }
                LastDevice    = NewDevice;
                LastSubDevice = NULL;
            }
        }
        // Get all subdevices for device
        else if (Line[0] == '\t' && Line[1] == '\t' && FoundVendorId == TRUE && LastDevice)
        {
            CHAR SubVendorBuf[PCI_ID_AS_STR_LENGTH + 1], SubDeviceBuf[PCI_ID_AS_STR_LENGTH + 1], SubsystemNameBuf[PCI_NAME_STR_LENGTH + 1];

            snprintf(FormatStr, sizeof(FormatStr), "%%4s %%4s %%%d[^\n]", PCI_NAME_STR_LENGTH); // FormatStr = "%4s %4s %PCI_NAME_STR_LENGTH[^\n]"
            if (sscanf(Line + 2, FormatStr, SubVendorBuf, SubDeviceBuf, SubsystemNameBuf) == 3)
            {
                SubDevice * NewSubDevice = (SubDevice *)malloc(sizeof(SubDevice));
                if (!NewSubDevice)
                {
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                int Result = sscanf(SubVendorBuf, "%hx", &NewSubDevice->SubVendorId);
                if (Result != 1)
                {
                    //
                    // NewSubDevice is not linked into the device's list yet, so it has
                    // to be released separately from FreeVendor()
                    //
                    free(NewSubDevice);
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                Result = sscanf(SubDeviceBuf, "%hx", &NewSubDevice->SubDeviceId);
                if (Result != 1)
                {
                    free(NewSubDevice);
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                PlatformStrNCpy(NewSubDevice->SubSystemName, sizeof(NewSubDevice->SubSystemName), TrimWhitespace(SubsystemNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
                NewSubDevice->Next = NULL;

                if (LastSubDevice)
                {
                    LastSubDevice->Next = NewSubDevice;
                }
                else
                {
                    LastDevice->SubDevices = NewSubDevice; // First subdevice
                }
                LastSubDevice = NewSubDevice;
            }
        }
        else if (Line[0] != '\t' && FoundVendorId == TRUE) // We hit the next vendor entry, so we're done parsing
        {
            break;
        }
    }

    return MatchedVendor;
}

/**
 * @brief Frees Vendor and all of its members
 *
 * @param VendorToFree
 * @return VOID
 */
VOID
FreeVendor(Vendor * VendorToFree)
{
    if (VendorToFree == NULL)
        return;

    Device * CurrentDevice = VendorToFree->Devices;
    while (CurrentDevice)
    {
        SubDevice * CurrentSubDevice = CurrentDevice->SubDevices;

        while (CurrentSubDevice)
        {
            SubDevice * NextSubDevice = CurrentSubDevice->Next;
            free(CurrentSubDevice);
            CurrentSubDevice = NextSubDevice;
        }

        Device * NextDevice = CurrentDevice->Next;
        free(CurrentDevice);
        CurrentDevice = NextDevice;
    }

    //
    // The Vendor itself is allocated by GetVendorByIdStr() and was previously never
    // released, leaking one Vendor per call for every PCI device that got enumerated
    //
    VendorToFree->Devices = NULL;
    free(VendorToFree);
}

/**
 * @brief Frees PciIdDatabaseBuffer
 * @return VOID
 */
VOID
FreePciIdDatabase()
{
    if (PciIdDatabaseBuffer != NULL)
    {
        free(PciIdDatabaseBuffer);
        PciIdDatabaseBuffer = NULL;
    }
}

/**
 * @brief Returns Vendor entry, including corresponding devices and subdevices
 * @details Use FreeVendor() on returned Vendor pointer after usage. First call will initialize database - call FreeDatabase() once done querying.
 *
 * @param VendorId
 * @return Vendor
 */
Vendor *
GetVendorById(UINT16 VendorId)
{
#ifdef _WIN32
    CHAR    VendorIdAsStr[5];
    CHAR    ExecutablePath[MAX_PATH];
    HMODULE hModule = GetModuleHandle(NULL);

    snprintf(VendorIdAsStr, sizeof(VendorIdAsStr), "%04X", VendorId);

    DWORD PathLength = GetModuleFileName(hModule, ExecutablePath, sizeof(ExecutablePath));

    //
    // A zero length means the call failed; a length equal to the buffer size means the
    // path was truncated and, on older Windows versions, left without a null terminator
    //
    if (PathLength == 0 || PathLength >= sizeof(ExecutablePath))
    {
        return NULL;
    }

    // Extract executable name
    CHAR * ExecutableName = strrchr(ExecutablePath, '\\');
    if (ExecutableName != NULL)
    {
        ExecutableName++;
    }
    else
    {
        ExecutableName = ExecutablePath;
    }

    // Swap executable name for PCI_ID_DATABASE_PATH
    //
    // The database path can be longer than the executable name it replaces, so the
    // room left in ExecutablePath is checked before overwriting the tail
    //
    SIZE_T RemainingSpace = sizeof(ExecutablePath) - (SIZE_T)(ExecutableName - ExecutablePath);

    if (RemainingSpace < sizeof(PCI_ID_DATABASE_PATH))
    {
        return NULL;
    }

    memcpy(ExecutableName, PCI_ID_DATABASE_PATH, sizeof(PCI_ID_DATABASE_PATH));

    return GetVendorByIdStr(ExecutablePath, ToLower(VendorIdAsStr));
#else
    CHAR VendorIdAsStr[5];
    CHAR DatabasePath[MAX_PATH];

    snprintf(VendorIdAsStr, sizeof(VendorIdAsStr), "%04X", VendorId);

    //
    // The database ships next to the executable, which is what
    // SetupPathForFileName resolves; it also normalizes the separators of
    // PCI_ID_DATABASE_PATH ("constants\\pci.ids") on the way. The existence
    // check is left to GetVendorByIdStr, which reports the missing database
    //
    if (!SetupPathForFileName(PCI_ID_DATABASE_PATH, DatabasePath, sizeof(DatabasePath), FALSE))
    {
        return NULL;
    }

    return GetVendorByIdStr(DatabasePath, ToLower(VendorIdAsStr));
#endif
}

/**
 * @brief Returns Device entry corresponding to DeviceId
 *
 * @param VendorToUse
 * @param DeviceId
 * @return Device
 */
Device *
GetDeviceFromVendor(Vendor * VendorToUse, UINT16 DeviceId)
{
    Device * CurrentDevice = NULL;
    if (!VendorToUse)
    {
        return NULL;
    }

    CurrentDevice = VendorToUse->Devices;
    while (CurrentDevice != NULL)
    {
        if (CurrentDevice->DeviceId == DeviceId)
        {
            return CurrentDevice;
        }
        CurrentDevice = CurrentDevice->Next;
    }
    return NULL;
}

/**
 * @brief Returns SubDevice entry corresponding to SubVendorId and DeviceId
 *
 * @param DeviceToUse
 * @param SubVendorId
 * @param SubDeviceId
 * @return SubDevice
 */
SubDevice *
GetSubDeviceFromDevice(Device * DeviceToUse, UINT16 SubVendorId, UINT16 SubDeviceId)
{
    SubDevice * CurrentSubDevice = NULL;
    if (!DeviceToUse)
    {
        return NULL;
    }

    CurrentSubDevice = DeviceToUse->SubDevices;
    while (CurrentSubDevice != NULL)
    {
        if (CurrentSubDevice->SubVendorId == SubVendorId && CurrentSubDevice->SubDeviceId == SubDeviceId)
        {
            return CurrentSubDevice;
        }
        CurrentSubDevice = CurrentSubDevice->Next;
    }
    return NULL;
}
