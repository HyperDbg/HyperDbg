/**
 * @file pci-id.cpp
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief Provides runtime access to PCI ID database
 * @details
 * @version 0.12
 * @date 2024-12-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

static char * PciIdDatabaseBuffer = NULL;

/**
 * @brief Trims whitespaces in passed string
 *
 * @param Str
 * @param MaxLen
 * @return char *
 */
char *
TrimWhitespace(char * Str, UINT8 MaxLen)
{
    char * End;
    while (*Str == ' ')
        Str++; // Trim leading space
    if (*Str == '\0')
        return Str;
    End = Str + strnlen_s(Str, MaxLen) - 1;
    while (End > Str && (*End == ' ' || *End == '\n' || *End == '\r'))
        End--;
    *(End + 1) = '\0';
    return Str;
}

/**
 * @brief Converts passed string to lowercase
 *
 * @param Str
 * @return char *
 */
char *
ToLower(char * Str)
{
    UINT8  StrLength   = (UINT8)strnlen_s(Str, PCI_ID_AS_STR_LENGTH);
    char * CurrentChar = Str;

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
 * @return char *
 */
char *
ReadLine(char * DestBuffer, UINT64 CharLimit, char ** SrcBuffer)
{
    char * Line = strchr(*SrcBuffer, '\n');
    if (!Line)
    {
        return NULL;
    }
    else
    {
        strncpy_s(DestBuffer, CharLimit, *SrcBuffer, (Line - *SrcBuffer));
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
GetVendorByIdStr(const char * Filename, const char * VendorId)
{
    Vendor *    MatchedVendor = NULL;
    BOOLEAN     FoundVendorId = FALSE;
    Device *    LastDevice    = NULL;
    SubDevice * LastSubDevice = NULL;
    char *      PciIdDbBufPtr = NULL;
    char        Line[1024]    = {'\0'};

    if (!PciIdDatabaseBuffer)
    {
        FILE * f      = fopen(Filename, "rb");
        size_t Length = 0;

        if (f == NULL)
        {
            ShowMessages("Error: Cannot open file '%s': error %d\n", Filename, errno);
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        Length = ftell(f);

        PciIdDatabaseBuffer = (char *)malloc(Length);
        if (!PciIdDatabaseBuffer)
        {
            return NULL;
        }

        fseek(f, 0, SEEK_SET);
        fread(PciIdDatabaseBuffer, 1, Length, f);
        fclose(f);
    }

    PciIdDbBufPtr = PciIdDatabaseBuffer;

    while (ReadLine(Line, sizeof(Line), &PciIdDbBufPtr) != NULL)
    {
        char FormatStr[24];

        // Skip comments and empty lines
        if (Line[0] == '#' || Line[0] == '\0')
        {
            continue;
        }

        // Find vendor
        // We assume PCI ID database comprises unique entries only, i.e. we return the first matching entry
        if (Line[0] != '\t' && FoundVendorId == FALSE)
        {
            char VendorBuf[PCI_ID_AS_STR_LENGTH + 1], VendorNameBuf[PCI_NAME_STR_LENGTH + 1];

            snprintf(FormatStr, sizeof(FormatStr), "%%4s %%%d[^\n]", PCI_NAME_STR_LENGTH); // FormatStr = "%4s %PCI_NAME_STR_LENGTH[^\n]"
            if (sscanf(Line, FormatStr, VendorBuf, VendorNameBuf) == 2)
            {
                if (strncmp(VendorBuf, VendorId, sizeof(VendorId)) == 0)
                {
                    MatchedVendor = (Vendor *)malloc(sizeof(Vendor));
                    if (!MatchedVendor)
                    {
                        return NULL;
                    }

                    int result = sscanf(VendorBuf, "%hx", &(MatchedVendor->VendorId));
                    if (result != 1)
                    {
                        return NULL;
                    }
                    strncpy_s(MatchedVendor->VendorName, sizeof(MatchedVendor->VendorName), TrimWhitespace(VendorNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
                    FoundVendorId = TRUE;
                }
            }
        }
        // Get all devices for vendor
        else if (Line[0] == '\t' && Line[1] != '\t' && FoundVendorId == TRUE)
        {
            char DeviceBuf[PCI_ID_AS_STR_LENGTH + 1], DeviceNameBuf[PCI_NAME_STR_LENGTH + 1];

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
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                strncpy_s(NewDevice->DeviceName, sizeof(NewDevice->DeviceName), TrimWhitespace(DeviceNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
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
            char SubVendorBuf[PCI_ID_AS_STR_LENGTH + 1], SubDeviceBuf[PCI_ID_AS_STR_LENGTH + 1], SubsystemNameBuf[PCI_NAME_STR_LENGTH + 1];

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
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                Result = sscanf(SubDeviceBuf, "%hx", &NewSubDevice->SubDeviceId);
                if (Result != 1)
                {
                    FreeVendor(MatchedVendor);
                    return NULL;
                }

                strncpy_s(NewSubDevice->SubSystemName, sizeof(NewSubDevice->SubSystemName), TrimWhitespace(SubsystemNameBuf, PCI_NAME_STR_LENGTH), _TRUNCATE);
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
 * @return void
 */
void
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
            SubDevice * nextSubDevice = CurrentSubDevice->Next;
            free(CurrentSubDevice);
            CurrentSubDevice = nextSubDevice;
        }

        Device * NextDevice = CurrentDevice->Next;
        free(CurrentDevice);
        CurrentDevice = NextDevice;
    }
}

/**
 * @brief Frees PciIdDatabaseBuffer
 * @return void
 */
void
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
    char    VendorIdAsStr[5];
    char    ExecutablePath[MAX_PATH];
    HMODULE hModule = GetModuleHandle(NULL);

    snprintf(VendorIdAsStr, sizeof(VendorIdAsStr), "%04X", VendorId);
    GetModuleFileName(hModule, ExecutablePath, sizeof(ExecutablePath));

    // Extract executable name
    char * ExecutableName = strrchr(ExecutablePath, '\\');
    if (ExecutableName != NULL)
    {
        ExecutableName++;
    }
    else
    {
        ExecutableName = ExecutablePath;
    }

    // Swap executable name for PCI_ID_DATABASE_PATH
    strncpy(ExecutableName, PCI_ID_DATABASE_PATH, sizeof(PCI_ID_DATABASE_PATH));

    return GetVendorByIdStr(ExecutablePath, ToLower(VendorIdAsStr));
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
