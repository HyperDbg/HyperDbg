/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    msm8x60.c

Abstract:

    This module contains support for the Qualcomm 8960 serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

#define UART_RX_BYTES_TO_RECEIVE 0x2000
#define RX_FIFO_WIDTH            sizeof(UINT32)

//
// Register addresses
//

#define UART_DM_MR1_ADDR             0x00000000
#define UART_DM_MR2_ADDR             0x00000004
#define UART_DM_SR_ADDR              0x00000008
#define UART_DM_CSR_ADDR             0x00000008
#define UART_DM_CR_ADDR              0x00000010
#define UART_DM_ISR_ADDR             0x00000014
#define UART_DM_IMR_ADDR             0x00000014
#define UART_DM_IPR_ADDR             0x00000018
#define UART_DM_TFWR_ADDR            0x0000001c
#define UART_DM_RFWR_ADDR            0x00000020
#define UART_DM_HCR_ADDR             0x00000024
#define UART_DM_DMRX_ADDR            0x00000034
#define UART_DM_IRDA_ADDR            0x00000038
#define UART_DM_RX_TOTAL_SNAP_ADDR   0x00000038
#define UART_DM_DMEN_ADDR            0x0000003c
#define UART_DM_NO_CHARS_FOR_TX_ADDR 0x00000040
#define UART_DM_BADR_ADDR            0x00000044
#define UART_DM_TXFS_ADDR            0x0000004c
#define UART_DM_RXFS_ADDR            0x00000050
#define UART_DM_TF_ADDR              0x00000070
#define UART_DM_RF_ADDR              0x00000070
#define UART_DM_SIM_CFG_ADDR         0x00000080

//
// Register Masks
//

//
// Mode Register 1
//

#define UART_DM_MR1_RFRC 0x80 // Ready-for-receiving Control
#define UART_DM_MR1_CTSC 0x40 // Clear-to-send Control

//
// Mode Register 2
//

#define UART_DM_MR2_LOOPBACK 0x80 // Channel Mode
#define UART_DM_MR2_ERRMODE  0x40 // Error Mode
#define UART_DM_MR2_5BPC     0x00 // 5 Bits per character
#define UART_DM_MR2_6BPC     0x10 // 6 Bits per character
#define UART_DM_MR2_7BPC     0x20 // 7 Bits per character
#define UART_DM_MR2_8BPC     0x30 // 8 Bits per character
#define UART_DM_MR2_05SB     0x00 // 0.5 Stop Bit
#define UART_DM_MR2_1SB      0x04 // 1 Stop Bit
#define UART_DM_MR2_15SB     0x08 // 1.5 Stop Bit
#define UART_DM_MR2_2SB      0x0C // 2 Stop Bits
#define UART_DM_MR2_NOPAR    0x00 // No Parity
#define UART_DM_MR2_OPAR     0x01 // Odd Parity
#define UART_DM_MR2_EPAR     0x02 // Even Parity
#define UART_DM_MR2_SPAR     0x03 // Space Parity

//
// Status Register
//

#define UART_DM_SR_RXRDY_BMSK         0x1
#define UART_DM_SR_TXRDY_BMSK         0x4
#define UART_DM_SR_TXEMT_BMSK         0x8
#define UART_DM_SR_UART_OVERRUN_BMSK  0x10
#define UART_DM_SR_PAR_FRAME_ERR_BMSK 0x20
#define UART_DM_SR_RX_BREAK_BMSK      0x40
#define UART_DM_SR_HUNT_CHAR_BMSK     0x80
#define UART_DM_SR_ERROR_BMSK         (UART_DM_SR_UART_OVERRUN_BMSK | \
                               UART_DM_SR_PAR_FRAME_ERR_BMSK)

//
// Interrupt Status Register
//

#define UART_DM_ISR_RXSTALE_BMSK  0x8
#define UART_DM_ISR_TX_READY_BMSK 0x80

//
// Receive FIFO Status Register
//

#define UART_DM_RXFS_RX_FIFO_STATE_LSB_BMSK 0x7f

//
// Data mover enable/disable register
//

#define UART_DM_DMA_EN_RXTX_DM_DIS 0x00

//
// Command Register
//

#define UART_DM_CR_ENA_RX        0x01   // Enable Receiver
#define UART_DM_CR_DIS_RX        0x02   // Disable Receiver
#define UART_DM_CR_ENA_TX        0x04   // Enable Transmitter
#define UART_DM_CR_DIS_TX        0x08   // Disable Transmitter
#define UART_DM_CR_NULL_CMD      0x0000 // Null Command
#define UART_DM_CR_RESET_RX      0x0010 // Reset Receiver
#define UART_DM_CR_RESET_TX      0x0020 // Reset Transmitter
#define UART_DM_CR_RESET_ERR     0x0030 // Reset Error Status
#define UART_DM_CR_RESET_BRK_INT 0x0040 // Reset Break Change Interrupt
#define UART_DM_CR_STA_BRK       0x0050 // Start Break
#define UART_DM_CR_STO_BRK       0x0060 // Stop Break
#define UART_DM_CR_CLR_DCTS      0x0070 // Clear CTS Change (delta)
#define UART_DM_CR_RESET_STALE   0x0080 // Clears the stale interrupt
#define UART_DM_CR_SAMP_MODE     0x0090 // Sample Data Mode
#define UART_DM_CR_TEST_PARITY   0x00A0 // Test Parity
#define UART_DM_CR_TEST_FRAME    0x00B0 // Test Frame
#define UART_DM_CR_RESET_SAMPLE  0x00C0 // Reset Sample Data Mode
#define UART_DM_CR_SET_RFR       0x00D0 // Set RFR
#define UART_DM_CR_RESET_RFR     0x00E0 // Reset RFR
#define UART_DM_CR_RESET_TX_ERR  0x0800 // Clears TX_ERROR interrupt
#define UART_DM_CR_RESET_TX_DONE 0x0810 // Clears TX_DONE interrupt
#define UART_DM_CR_ENA_CR_PROT   0x0100 // Enable CR protection
#define UART_DM_CR_DIS_CR_PROT   0x0200 // Disable CR protection
#define UART_DM_CR_RESET_TX_RDY  0x0300 // Clears TX_READY interrupt
#define UART_DM_CR_FORCE_STALE   0x0400 // SW force stale
#define UART_DM_CR_ENA_STALE_EVT 0x0500 // Enables Stale Event
#define UART_DM_CR_DIS_STALE_EVT 0x0600 // Disables Stale Event

//
// Interrupt Mask Register
//

#define UART_DM_IMR_TX_DONE   0x200 // TX Done
#define UART_DM_IMR_TX_ERROR  0x100 // TX Error
#define UART_DM_IMR_TX_READY  0x080 // TX Ready
#define UART_DM_IMR_CUR_CTS   0x040 // Current CTS
#define UART_DM_IMR_DELTA_CTS 0x020 // Delta CTS
#define UART_DM_IMR_RXLEV     0x010 // RX Level exceeded
#define UART_DM_IMR_RXSTALE   0x008 // Stale RX character occurred
#define UART_DM_IMR_RXBREAK   0x004 // RX Break occurred
#define UART_DM_IMR_RXHUNT    0x002 // RX Hunt character received
#define UART_DM_IMR_TXLEV     0x001 // TX Level or below met
#define UART_DM_IMR_NONE      0x000 // No interrupt

#define UART_DM_MR1_DEFAULT 0 // No RFR or CTS
#define UART_DM_MR2_DEFAULT (UART_DM_MR2_8BPC | \
                             UART_DM_MR2_1SB |  \
                             UART_DM_MR2_NOPAR)
#define UART_DM_IMR_DEFAULT 0

//
// Interrupt Program Register
//

#define UART_DM_IPR_DEFAULT 0x2

//
// RX FIFO Base Address Register
//

#define UART_DM_BADR_DEFAULT 0x70

//
// For UART IRDA Register
//

#define UART_DM_IRDA_DISABLE 0x00 // Disable UART IRDA transceiver

//
// Channel Command
//

#define UART_DM_CH_CMD_RESET_RECEIVER          0x01
#define UART_DM_CH_CMD_RESET_TRANSMITTER       0x02
#define UART_DM_CH_CMD_RESET_ERROR_STATUS      0x03
#define UART_DM_CH_CMD_RESET_BREAK_CHANGE_IRQ  0x04
#define UART_DM_CH_CMD_START_BREAK             0x05
#define UART_DM_CH_CMD_STOP_BREAK              0x06
#define UART_DM_CH_CMD_RESET_CTS_N             0x07
#define UART_DM_CH_CMD_RESET_STALE_IRQ         0x08
#define UART_DM_CH_CMD_PACKET_MODE             0x09
#define UART_DM_CH_CMD_TEST_PARITY_ON          0x0A
#define UART_DM_CH_CMD_TEST_FRAME_ON           0x0B
#define UART_DM_CH_CMD_MODE_RESET              0x0C
#define UART_DM_CH_CMD_SET_RFR_N               0x0D
#define UART_DM_CH_CMD_RESET_RFR_N             0x0E
#define UART_DM_CH_CMD_UART_RESET_INT          0x0F
#define UART_DM_CH_CMD_RESET_TX_ERROR          0x10
#define UART_DM_CH_CMD_CLEAR_TX_DONE           0x11
#define UART_DM_CH_CMD_RESET_BRK_START_IRQ     0x12
#define UART_DM_CH_CMD_RESET_BRK_END_IRQ       0x13
#define UART_DM_CH_CMD_RESET_PAR_FRAME_ERR_IRQ 0x14

//
// General Command
//

#define UART_DM_GENERAL_CMD_CR_PROTECTION_ENABLE  0x01
#define UART_DM_GENERAL_CMD_CR_PROTECTION_DISABLE 0x02
#define UART_DM_GENERAL_CMD_RESET_TX_READY_IRQ    0x03
#define UART_DM_GENERAL_CMD_SW_FORCE_STALE        0x04
#define UART_DM_GENERAL_CMD_ENABLE_STALE_EVENT    0x05
#define UART_DM_GENERAL_CMD_DISABLE_STALE_EVENT   0x06

// --------------------------------------------------------------------- Macros

#define UART_DM_READ_REG(addr, offset) \
    READ_REGISTER_ULONG((ULONG *)((PUCHAR)addr + offset))

#define UART_DM_WRITE_REG(addr, offset, val) \
    WRITE_REGISTER_ULONG((ULONG *)((PUCHAR)addr + offset), val)

#define UART_DM_CH_CMD(a, v)           \
    UART_DM_WRITE_REG((a),             \
                      UART_DM_CR_ADDR, \
                      ((((v) >> 4) & 0x1) << 11) | (((v)&0xF) << 4))
#define UART_DM_GENERAL_CMD(a, v) \
    UART_DM_WRITE_REG((a), UART_DM_CR_ADDR, ((v)&0x7) << 8)

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
MSM8x60SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

// ------------------------------------------------------------------ Functions

BOOLEAN
MSM8x60InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of an MSM8x60 serial UART.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Supplies a boolean which indicates if the UART is accessed
        through memory-mapped registers or legacy port I/O.

    AccessSize - Supplies an ACPI Generic Access Size value which indicates the
        type of bus access that should be performed when accessing the UART.

    BitWidth - Supplies a number indicating how wide the UART's registers are.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{
    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);

    if (MemoryMapped == FALSE)
    {
        return FALSE;
    }

    if (BitWidth != 32)
    {
        return FALSE;
    }

    Port->Flags = 0;

    UART_DM_WRITE_REG(Port->Address, UART_DM_CR_ADDR, UART_DM_CR_DIS_RX);
    UART_DM_WRITE_REG(Port->Address, UART_DM_CR_ADDR, UART_DM_CR_DIS_TX);
    UART_DM_WRITE_REG(Port->Address, UART_DM_CR_ADDR, UART_DM_CR_RESET_ERR);
    UART_DM_WRITE_REG(Port->Address, UART_DM_CR_ADDR, UART_DM_CR_RESET_RX);
    UART_DM_WRITE_REG(Port->Address, UART_DM_CR_ADDR, UART_DM_CR_RESET_TX);
    UART_DM_WRITE_REG(Port->Address, UART_DM_MR1_ADDR, UART_DM_MR1_DEFAULT);
    UART_DM_WRITE_REG(Port->Address, UART_DM_MR2_ADDR, UART_DM_MR2_DEFAULT);

    MSM8x60SetBaud(Port, Port->BaudRate);

    UART_DM_WRITE_REG(Port->Address, UART_DM_IMR_ADDR, UART_DM_IMR_DEFAULT);
    UART_DM_WRITE_REG(Port->Address, UART_DM_IPR_ADDR, UART_DM_IPR_DEFAULT);
    UART_DM_WRITE_REG(Port->Address, UART_DM_BADR_ADDR, UART_DM_BADR_DEFAULT);
    UART_DM_WRITE_REG(Port->Address, UART_DM_DMEN_ADDR, 0);
    UART_DM_WRITE_REG(Port->Address, UART_DM_DMRX_ADDR, UART_RX_BYTES_TO_RECEIVE);
    UART_DM_CH_CMD(Port->Address, UART_DM_CH_CMD_RESET_STALE_IRQ);
    UART_DM_GENERAL_CMD(Port->Address, UART_DM_GENERAL_CMD_ENABLE_STALE_EVENT);
    UART_DM_WRITE_REG(Port->Address,
                      UART_DM_CR_ADDR,
                      (UART_DM_CR_ENA_RX | UART_DM_CR_ENA_TX));

    return TRUE;
}

BOOLEAN
MSM8x60SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate)

/*++

Routine Description:

    Set the baud rate for the UART hardware and record it in the port object.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Rate - Supplies the desired baud rate in bits per second.

Return Value:

    TRUE if the baud rate was programmed, FALSE if it was not.

--*/

{
    UINT32 DivisorLatch;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Clock rate is 1843200.
    //

    switch (Rate)
    {
    case 75:
        DivisorLatch = 0x00;
        break;
    case 150:
        DivisorLatch = 0x11;
        break;
    case 300:
        DivisorLatch = 0x22;
        break;
    case 600:
        DivisorLatch = 0x33;
        break;
    case 1200:
        DivisorLatch = 0x44;
        break;
    case 2400:
        DivisorLatch = 0x55;
        break;
    case 3600:
        DivisorLatch = 0x66;
        break;
    case 4800:
        DivisorLatch = 0x77;
        break;
    case 7200:
        DivisorLatch = 0x88;
        break;
    case 9600:
        DivisorLatch = 0x99;
        break;
    case 14400:
        DivisorLatch = 0xAA;
        break;
    case 19200:
        DivisorLatch = 0xBB;
        break;
    case 28800:
        DivisorLatch = 0xCC;
        break;
    case 38400:
        DivisorLatch = 0xDD;
        break;
    case 57600:
        DivisorLatch = 0xEE;
        break;
    case 115200:
        DivisorLatch = 0xFF;
        break;
    default:
        DivisorLatch = 0xFF;
        break;
    }

    UART_DM_WRITE_REG(Port->Address, UART_DM_CSR_ADDR, DivisorLatch);
    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
MSM8x60GetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR    Byte)

/*++

Routine Description:

    Fetch a data byte from the UART device and return it.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the address of variable to hold the result.

Return Value:

    UART_STATUS code.

--*/

{
    static UINT32 RxWord;
    static UINT32 Queued = 0;
    static UINT32 Read   = 0;
    static UINT32 Snap   = 0;

    ULONG limitcount;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    if (Queued == 0)
    {
        PUCHAR Address = Port->Address;
        if ((UART_DM_READ_REG(Address, UART_DM_SR_ADDR) &
             UART_DM_SR_ERROR_BMSK) != 0)
        {
            UART_DM_CH_CMD(Address, UART_DM_CH_CMD_RESET_ERROR_STATUS);
        }

        limitcount = 1;
        while (limitcount-- != 0)
        {
            if ((UART_DM_READ_REG(Address, UART_DM_SR_ADDR) &
                 UART_DM_SR_RXRDY_BMSK) == 0)
            {
                continue;
            }

            //
            // Read only if there are more than 4 bytes in the FIFO or RX Stale
            // occured.
            //

            if ((UART_DM_READ_REG(Address, UART_DM_RXFS_ADDR) &
                 UART_DM_RXFS_RX_FIFO_STATE_LSB_BMSK) == 1)
            {
                if ((UART_DM_READ_REG(Address, UART_DM_ISR_ADDR) &
                     UART_DM_ISR_RXSTALE_BMSK) == 0)
                {
                    continue;
                }
            }

            RxWord = UART_DM_READ_REG(Address, UART_DM_RF_ADDR);
            Queued = RX_FIFO_WIDTH;
            Read += RX_FIFO_WIDTH;

            if (Snap == 0)
            {
                if (UART_DM_READ_REG(Address, UART_DM_ISR_ADDR) &
                    UART_DM_ISR_RXSTALE_BMSK)
                {
                    Snap = UART_DM_READ_REG(Address,
                                            UART_DM_RX_TOTAL_SNAP_ADDR);
                }
            }

            if (Snap != 0)
            {
                if (Snap <= Read)
                {
                    if (Snap + RX_FIFO_WIDTH > Read)
                    {
                        Queued = Snap + RX_FIFO_WIDTH - Read;
                    }
                    else
                    {
                        Queued = 0;
                    }

                    Read = 0;
                    Snap = 0;
                    UART_DM_CH_CMD(Address, UART_DM_CH_CMD_RESET_STALE_IRQ);
                    UART_DM_WRITE_REG(Address,
                                      UART_DM_DMRX_ADDR,
                                      UART_RX_BYTES_TO_RECEIVE);

                    UART_DM_GENERAL_CMD(Address,
                                        UART_DM_GENERAL_CMD_ENABLE_STALE_EVENT);
                }
            }

            break;
        }
    }

    if (Queued != 0)
    {
        *Byte  = (int)(RxWord & 0xFF);
        RxWord = RxWord >> 8;
        Queued -= 1;
        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
MSM8x60PutByte(
    _Inout_ PCPPORT Port,
    UCHAR           Byte,
    BOOLEAN         BusyWait)

/*++

Routine Description:

    Write a data byte out to the UART device.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the data to emit.

    BusyWait - Supplies a flag to control whether this routine will busy
        wait (spin) for the UART hardware to be ready to transmit.

Return Value:

    UART_STATUS code.

--*/

{
    PUCHAR Address;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    Address = Port->Address;

    if ((UART_DM_READ_REG(Address, UART_DM_SR_ADDR) &
         UART_DM_SR_TXEMT_BMSK) == 0)
    {
        if (BusyWait != FALSE)
        {
            while ((UART_DM_READ_REG(Address, UART_DM_ISR_ADDR) &
                    UART_DM_ISR_TX_READY_BMSK) == 0)
                ;
        }
        else if ((UART_DM_READ_REG(Address, UART_DM_ISR_ADDR) &
                  UART_DM_ISR_TX_READY_BMSK) == 0)
        {
            return UartNotReady;
        }
    }

    UART_DM_WRITE_REG(Address, UART_DM_NO_CHARS_FOR_TX_ADDR, 1);
    UART_DM_GENERAL_CMD(Address, UART_DM_GENERAL_CMD_RESET_TX_READY_IRQ);
    UART_DM_WRITE_REG(Address, UART_DM_TF_ADDR, (UINT32)Byte);
    return UartSuccess;
}

BOOLEAN
MSM8x60RxReady(
    _Inout_ PCPPORT Port)

/*++

Routine Description:

    This routine determines if there is data pending in the UART.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

Return Value:

    TRUE if data is available, FALSE otherwise.

--*/

{
    UINT32 Sr;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    Sr = UART_DM_READ_REG(Port->Address, UART_DM_SR_ADDR);
    if (CHECK_FLAG(Sr, UART_DM_SR_RXRDY_BMSK))
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER MSM8x60HardwareDriver = {
    MSM8x60InitializePort,
    MSM8x60SetBaud,
    MSM8x60GetByte,
    MSM8x60PutByte,
    MSM8x60RxReady};
