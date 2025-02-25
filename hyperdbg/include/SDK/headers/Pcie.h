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
#pragma pack(1)

//////////////////////////////////////////////////
//		        	  Headers                   //
//////////////////////////////////////////////////

//
// TODO
// - Add support for domains beyond 0000
// - Add ECAM support
//
// PCIe Base Specification, Rev. 4.0, Version 1.0, Table 7-59: Link Address for Link Type 1
// Bus: 0-255 (8 bit)
// Device: 0-31 (5 bit)
// Function: 0-7 (3 bit)
//
#define BUS_BIT_WIDTH      8
#define DEVICE_BIT_WIDTH   5
#define FUNCTION_BIT_WIDTH 3

//
// NOTE
// On real-world systems, the number of endpoints will likely not exceed 255.
// If increasing DEV_MAX_NUM is necessary, ensure PCI_DEV_MINIMAL[DEV_MAX_NUM] does not result in exceeding MaxSerialPacketSize.
//
#define DOMAIN_MAX_NUM          0
#define BUS_MAX_NUM             255
#define DEVICE_MAX_NUM          32
#define FUNCTION_MAX_NUM        8
#define DEV_MAX_NUM             255
#define CAM_CONFIG_SPACE_LENGTH 255

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
typedef union _PORTABLE_PCI_DEVICE_HEADER
{
    struct _PORTABLE_PCI_EP_HEADER
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
    } ConfigSpaceEp;

    struct _PORTABLE_PCI_BRIDGE_HEADER
    {
        UINT32 Bar[2];
        UINT8  PrimaryBusNumber;
        UINT8  SecondaryBusNumber;
        UINT8  SubordinateBusNumber;
        UINT8  SecondaryLatencyTimer;
        UINT8  IoBase;
        UINT8  IoLimit;
        UINT16 SecondaryStatus;
        UINT16 MemoryBase;
        UINT16 MemoryLimit;
        UINT16 PrefetchableMemoryBase;
        UINT16 PrefetchableMemoryLimit;
        UINT32 PrefetchableBaseUpper32b;
        UINT32 PrefetchableLimitUpper32b;
        UINT16 IoBaseUpper16b;
        UINT16 IoLimitUpper16b;
        UINT8  CapabilityPtr;
        UINT8  Reserved[3];
        UINT32 ROMBar;
        UINT8  InterruptLine;
        UINT8  InterruptPin;
        UINT16 BridgeControl;
    } ConfigSpacePtpBridge;

    // TODO: Add support for HeaderType 0x2 (PCI-to-CardBus bridge)
} PORTABLE_PCI_DEVICE_HEADER, *PPORTABLE_PCI_DEVICE_HEADER;

/**
 * @brief PCI Configuration Space Minimal Header for !pcitree
 *
 */
typedef struct _PORTABLE_PCI_CONFIG_SPACE_HEADER_MINIMAL
{
    //
    // Common header
    //
    UINT16 VendorId;
    UINT16 DeviceId;
    UINT8  ClassCode[3];
} PORTABLE_PCI_CONFIG_SPACE_HEADER_MINIMAL, *PPORTABLE_PCI_CONFIG_SPACE_HEADER_MINIMAL;

/**
 * @brief PCI Device Minimal Data Structure for !pcitree
 *
 */
typedef struct _PCI_DEV_MINIMAL
{
    UINT8                                    Bus : BUS_BIT_WIDTH;
    UINT8                                    Device : DEVICE_BIT_WIDTH;
    UINT8                                    Function : FUNCTION_BIT_WIDTH;
    PORTABLE_PCI_CONFIG_SPACE_HEADER_MINIMAL ConfigSpace;
} PCI_DEV_MINIMAL, *PPCI_DEV_MINIMAL;

/**
 * @brief PCI Device MMIO BAR Metadata
 *
 */
typedef struct _PCI_DEV_MMIOBAR_INFO
{
    BOOL   Is64Bit;
    BOOL   IsEnabled;
    UINT64 BarOffsetEnd;
    UINT64 BarSize;
} PCI_DEV_MMIOBAR_INFO, *PPCI_DEV_MMIOBAR_INFO;

/**
 * @brief PCI Configuration Space Header
 *
 */
typedef struct _PORTABLE_PCI_CONFIG_SPACE_HEADER
{
    PORTABLE_PCI_COMMON_HEADER CommonHeader;
    PORTABLE_PCI_DEVICE_HEADER DeviceHeader;
} PORTABLE_PCI_CONFIG_SPACE_HEADER, *PPORTABLE_PCI_CONFIG_SPACE_HEADER;

/**
 * @brief PCI Device Data Structure
 *
 */
typedef struct _PCI_DEV
{
    UINT8                            Bus : BUS_BIT_WIDTH;
    UINT8                            Device : DEVICE_BIT_WIDTH;
    UINT8                            Function : FUNCTION_BIT_WIDTH;
    PORTABLE_PCI_CONFIG_SPACE_HEADER ConfigSpace;
    BYTE                             ConfigSpaceAdditional[CAM_CONFIG_SPACE_LENGTH - sizeof(PORTABLE_PCI_CONFIG_SPACE_HEADER)];
    PCI_DEV_MMIOBAR_INFO             MmioBarInfo[6];
} PCI_DEV, *PPCI_DEV;

#pragma pack()
