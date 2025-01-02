/**
 * @file pci-id.h
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief PCI ID-related data structures
 * @details
 * @version 0.12
 * @date 2024-12-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#define PCI_ID_AS_STR_LENGTH (sizeof(UINT16) * 2)
#define PCI_NAME_STR_LENGTH  255

typedef struct SubDevice
{
    UINT16             SubVendorId;
    UINT16             SubDeviceId;
    char               SubSystemName[PCI_NAME_STR_LENGTH];
    struct SubDevice * Next;
} SubDevice;

typedef struct Device
{
    UINT16          DeviceId;
    char            DeviceName[PCI_NAME_STR_LENGTH];
    SubDevice *     SubDevices;
    struct Device * Next;
} Device;

typedef struct Vendor
{
    UINT16   VendorId;
    char     VendorName[PCI_NAME_STR_LENGTH];
    Device * Devices;
} Vendor;

//
// PCI ID database courtesy of PCI ID Database (pciutils) project at
// https://pci-ids.ucw.cz/
//
#define PCI_ID_DATABASE_PATH "constants\\pci.ids"

//////////////////////////////////////////////////
//					  Functions                 //
//////////////////////////////////////////////////
Vendor *
GetVendorById(UINT16 VendorId);
void
FreeVendor(Vendor * VendorToFree);
void
FreePciIdDatabase();
Device *
GetDeviceFromVendor(Vendor * VendorToUse, UINT16 DeviceId);
SubDevice *
GetSubDeviceFromDevice(Device * DeviceToUse, UINT16 SubVendorId, UINT16 SubDeviceId);
