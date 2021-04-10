/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    hardware.c

Abstract:

    This file defines the global UART hardware driver table.

--*/

#include "common.h"

// --------------------------------------------------------------------- Macros

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// -------------------------------------------------------------------- Globals

//
// The array order here is the serial subtype as specified
// in the Microsoft Debug Port Table 2 (DBG2) specification.
//

PUART_HARDWARE_DRIVER UartHardwareDrivers[] = {

#if defined(_X86_) || defined(_AMD64_)

    &Legacy16550HardwareDriver, // 0x0
    &Uart16550HardwareDriver,   // 0x1
    &SpiMax311HardwareDriver,   // 0x2
    NULL,
    NULL,
    NULL,
    NULL, // 0x3-0x6
    NULL, // 0x7 = UEFI debug protocol
    NULL,
    NULL,
    NULL,                // 0x8-0xA
    &UsifHardwareDriver, // 0xB
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,                  // 0xC-0x11
    &MM16550HardwareDriver // 0x12

#elif defined(_ARM_) || defined(_ARM64_)

    NULL,                     // 0x0
    &Uart16550HardwareDriver, // 0x1
    NULL,                     // 0x2
    &PL011HardwareDriver,     // 0x3
    &MSM8x60HardwareDriver,   // 0x4
    &NvidiaHardwareDriver,    // 0x5
    &OmapHardwareDriver,      // 0x6
    NULL,                     // 0x7 = UEFI debug protocol
    &Apm88xxxxHardwareDriver, // 0x8
    &MSM8974HardwareDriver,   // 0x9
    &Sam5250HardwareDriver,   // 0xA
    NULL,                     // 0xB
    &MX6HardwareDriver,       // 0xC
    &SBSA32HardwareDriver,    // 0xD
    &SBSAHardwareDriver,      // 0xE
    NULL,                     // 0xF = ARM DCC
    &Bcm2835HardwareDriver,   // 0x10
    &SDM845HardwareDriver,    // 0x11
    &MM16550HardwareDriver    // 0x12

#else

#    error "Unknown Processor Architecture"

#endif

};

C_ASSERT(ARRAY_SIZE(UartHardwareDrivers) == 19);

ULONG UartHardwareDriverCount = ARRAY_SIZE(UartHardwareDrivers);
