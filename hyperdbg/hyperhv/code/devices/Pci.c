/**
 * @file Pci.c
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief Routines for interacting with PCI(e) fabric
 * @details
 * @version 0.10.3
 * @date 2024-11-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Read from PCI configuration space (CAM) at given offset
 * @param Bus
 * @param Device
 * @param Function
 * @param Offset
 * @param Width Number of bytes to return. Supported data types: BYTE (1), WORD (2), DWORD (4), QWORD (8).
 *
 * @return QWORD
 */
QWORD
PciReadCam(WORD Bus, WORD Device, WORD Function, BYTE Offset, UINT8 Width)
{
    WORD Target = 0;

    //
    // Restrict Offset to CAM address space; restrict BDF to our internal limits
    //
    if (Offset >= sizeof(PORTABLE_PCI_COMMON_HEADER) + sizeof(PORTABLE_PCI_DEVICE_HEADER) ||
        Bus >= BUS_MAX_NUM ||
        Device >= DEVICE_MAX_NUM ||
        Function >= FUNCTION_MAX_NUM)
    {
        return MAXDWORD64;
    }

    Target = Function + ((Device & 0x1F) << 3) + ((Bus & 0xFF) << 8);
    _outpd(CFGADR, (DWORD)(Target << 8) | 0x80000000UL | ((DWORD)Offset & ~3));

    switch (Width)
    {
    case sizeof(BYTE):
        return (BYTE)_inp(CFGDAT + (Offset & 0x3));
        break;
    case sizeof(WORD):
        return (WORD)_inpw(CFGDAT + (Offset & 0x2));
        break;
    case sizeof(DWORD):
        return _inpd(CFGDAT);
        break;
    case sizeof(QWORD):
        return (PciReadCam(Bus, Device, Function, 4, sizeof(DWORD)) << 32) | PciReadCam(Bus, Device, Function, 0, sizeof(DWORD));
        break;
    default:
        return MAXDWORD64;
    }
}
