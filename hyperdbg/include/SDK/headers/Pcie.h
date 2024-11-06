/**
 * @file Pcie.h
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief PCIe-related data structures
 * @details
 * @version 0.10.3
 * @date 2024-10-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		        	  Headers                   //
//////////////////////////////////////////////////

//
// PCIe Base Specification, Rev. 4.0, Version 1.0, Table 7-59: Link Address for Link Type 1
// Bus: 0-255 (8 bit)
// Device: 0-31 (5 bit)
// Function: 0-7 (3 bit)
//
// TODO
// We're limited to sending fixed buffers, so we'll have to choose some reasonable numbers here instead of assuming spec-mandated maximum numbers.
// Ensure the following parameters do not result in exceeding MaxSerialPacketSize. Consider sending multiple packets if necessary.
//
#define DOMAIN_MAX_NUM   2
#define BUS_MAX_NUM      10
#define DEVICE_MAX_NUM   2
#define FUNCTION_MAX_NUM 1

//
// TODO
// We currently limit ourselves to PCI configuration space (i.e. CAM).
//

/**
 * @brief PCI Common Header
 *
 */
typedef struct _PORTABLE_PCI_COMMON_HEADER
{
    UINT16 VendorId;
    UINT16 DeviceId;
    UINT16 Command;
    UINT16 Status;
    UINT8  RevisionId;
    UINT8  ClassCode[3];
    UINT8  CacheLineSize;
    UINT8  PrimaryLatencyTimer;
    UINT8  HeaderType;
    UINT8  Bist;
} PORTABLE_PCI_COMMON_HEADER, *PPORTABLE_PCI_COMMON_HEADER;

/**
 * @brief PCI Device Header
 *
 */
typedef struct _PORTABLE_PCI_DEVICE_HEADER
{
    UINT32 Bar[6];          // Base Address Registers
    UINT32 CardBusCISPtr;   // CardBus CIS Pointer
    UINT16 SubVendorId;     // Subsystem Vendor ID
    UINT16 SubSystemId;     // Subsystem ID
    UINT32 ROMBar;          // Expansion ROM Base Address
    UINT8  CapabilitiesPtr; // Capabilities Pointer
    UINT8  Reserved[3];
    UINT32 Reserved1;
    UINT8  InterruptLine; // Interrupt Line
    UINT8  InterruptPin;  // Interrupt Pin
    UINT8  MinGnt;        // Min_Gnt
    UINT8  MaxLat;        // Max_Lat
} PORTABLE_PCI_DEVICE_HEADER, *PPORTABLE_PCI_DEVICE_HEADER;

/**
 * @brief PCI Configuration Space Header
 *
 */
typedef struct _PORTABLE_PCI_CONFIG_SPACE_HEADER
{
    PORTABLE_PCI_COMMON_HEADER CommonHeader;
    PORTABLE_PCI_DEVICE_HEADER DeviceHeader;
    // TODO: Add Device Private, Capabilities, Enhanced Capabilities
} PORTABLE_PCI_CONFIG_SPACE_HEADER, *PPORTABLE_PCI_CONFIG_SPACE_HEADER;

/**
 * @brief PCI Function Data Structure
 *
 */
typedef struct _PCI_FUNCTION
{
    UINT8 Placeholder;
} PCI_FUNCTION, *PPCI_FUNCTION;

/**
 * @brief PCI Device Data Structure
 *
 */
typedef struct _PCI_DEVICE
{
    PORTABLE_PCI_CONFIG_SPACE_HEADER ConfigSpace[DEVICE_MAX_NUM];
    PCI_FUNCTION                     Function[FUNCTION_MAX_NUM];
} PCI_DEVICE, *PPCI_DEVICE;

/**
 * @brief PCI Bus Data Structure
 *
 */
typedef struct _PCI_BUS
{
    PCI_DEVICE Device[DEVICE_MAX_NUM];
} PCI_BUS, *PPCI_BUS;

/**
 * @brief PCI Domain Data Structure
 *
 */
typedef struct _PCI_DOMAIN
{
    PCI_BUS Bus[BUS_MAX_NUM];
} PCI_DOMAIN, *PPCI_DOMAIN;

/**
 * @brief PCI Tree Data Structure
 *
 */
typedef struct _PCI_TREE
{
    PCI_DOMAIN Domain[DOMAIN_MAX_NUM];
} PCI_TREE, *PPCI_TREE;
