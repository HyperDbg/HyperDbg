/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    spimax311.c

Abstract:

    This module contains support for the Maxim Integrated Products MAX311xE
    SPI UART hardware.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"

// ---------------------------------------------------------------- Definitions

//
// SPI Control bits.
//

#define CRTLR0_SLV_OE      (1 << 10) // 1 = Slave TXD disabled
#define CTRLR0_TMOD_TX_RX  0
#define CTRLR0_TMOD_TX     (1 << 8)
#define CTRLR0_TMOD_RX     (2 << 8)
#define CTRLR0_TMOD_EEPROM (3 << 8)
#define CTRLR0_SCPOL       (1 << 7) // Serial Clock Polarity (inactive state)
#define CTRLR0_SCPH        (1 << 6) // Serial Clock Phase (1=Start, 0=Middle) \
                                    // of data bit
#define CTRLR0_FRF_MOTOROLA_SPI                      0
#define CTRLR0_FRF_TEXAS_INSTRUMENTS_SSP             (1 << 4)
#define CTRLR0_FRF_NATIONAL_SEMICONDUCTORS_MICROWIRE (2 << 4)
#define CTRLR0_DFS                                   15 // Data frame size mask in bits

#define SSIENR_SSI_EN 1 // 1=Enabled

//
// SPI baud rate.
//

#define BAUDR_MAX_RATE 2
#define BAUDR_OFF      0

//
// Define the SPI status register.
//

#define SR_DCOL (1 << 6) // 1 = Transmit data collision error
#define SR_TXE  (1 << 5) // 1 = Transmission error
#define SR_RFF  (1 << 4) // 1 = Receive FIFO is full
#define SR_RFNE (1 << 3) // 1 = Receive FIFO is not empty
#define SR_TFE  (1 << 2) // 1 = Transmit FIFO is empty
#define SR_TFNF (1 << 1) // 1 = Transmit FIFO is not full
#define SR_BUSY (1 << 0) // 1 = SPI unit is transferring data

//
// UART commands. The first two bits sent to the UART determine whether the UART
// is reading or writing, and whether the next byte is data or configuration.
//

#define MAX311XE_WRITE_CONFIG (3 << 14)
#define MAX311XE_READ_CONFIG  (1 << 14)
#define MAX311XE_WRITE_DATA   (2 << 14)
#define MAX311XE_READ_DATA    0

//
// Configuration register bits.
//

#define MAX311XE_WC_FEN_BAR (1 << 13) //  1 = FIFO Disable
#define MAX311XE_WC_SHDNI   (1 << 12) //  1 = Enter shutdown
#define MAX311XE_WC_TM_BAR  (1 << 11) //  1 = Transmit buffer empty \
                                      //      interrupt enabled
#define MAX311XE_WC_RM_BAR (1 << 10)  //  1 = Data available interrupt \
                                      //      Enable
#define MAX311XE_WC_PM_BAR (1 << 9)   //  1 = Parity bit high received \
                                      //      interrupt enabled
#define MAX311XE_WC_RAM_BAR (1 << 8)  //  1 = Receiver-activity (shutdown  \
                                      //      mode)/Framming-error (normal \
                                      //      operation) interrupt enabled
#define MAX311XE_WC_IRDA    (1 << 7)  //  1 = IrDA mode is enabled
#define MAX311XE_WC_ST      (1 << 6)  //  1 = Transmit two stop bits
#define MAX311XE_WC_PE      (1 << 5)  //  1 = Parity enabled
#define MAX311XE_WC_L       (1 << 4)  //  1 = 7 bits, 0 = 8 Bits
#define MAX311XE_WC_DIV_1   0         //  Clock divider for baud-rate
#define MAX311XE_WC_DIV_2   1
#define MAX311XE_WC_DIV_4   2
#define MAX311XE_WC_DIV_8   3
#define MAX311XE_WC_DIV_16  4
#define MAX311XE_WC_DIV_32  5
#define MAX311XE_WC_DIV_64  6
#define MAX311XE_WC_DIV_128 7
#define MAX311XE_WC_DIV_3   8
#define MAX311XE_WC_DIV_6   9
#define MAX311XE_WC_DIV_12  10
#define MAX311XE_WC_DIV_24  11
#define MAX311XE_WC_DIV_48  12
#define MAX311XE_WC_DIV_96  13
#define MAX311XE_WC_DIV_192 14
#define MAX311XE_WC_DIV_384 15

//
// Read configuration register bits.
//

#define MAX311XE_RC_R       (1 << 15) //  1 = Receive data available
#define MAX311XE_RC_T       (1 << 14) //  1 = Transmit buffer empty
#define MAX311XE_RC_FEN_BAR (1 << 13) //  1 = FIFO Disable
#define MAX311XE_RC_SHDNI   (1 << 12) //  1 = Enter shutdown
#define MAX311XE_RC_TM_BAR  (1 << 11) //  1 = Transmit buffer empty \
                                      //      interrupt enabled
#define MAX311XE_RC_RM_BAR (1 << 10)  //  1 = Data available interrupt \
                                      //      enable
#define MAX311XE_RC_PM_BAR (1 << 9)   //  1 = Parity bit high received \
                                      //      interrupt enabled
#define MAX311XE_RC_RAM_BAR (1 << 8)  //  1 = Receiver Activity (shutdown    \
                                      //      mode) or Framing Error (normal \
                                      //      operation) interrupt enabled
#define MAX311XE_RC_IRDA    (1 << 7)  //  1 = IrDA mode is enabled
#define MAX311XE_RC_ST      (1 << 6)  //  1 = Transmit two stop bits
#define MAX311XE_RC_PE      (1 << 5)  //  1 = Parity enabled
#define MAX311XE_RC_L       (1 << 4)  //  1 = 7 bits, 0 = 8 Bits
#define MAX311XE_RC_DIV_1   0         //  Clock divider for baud-rate
#define MAX311XE_RC_DIV_2   1
#define MAX311XE_RC_DIV_4   2
#define MAX311XE_RC_DIV_8   3
#define MAX311XE_RC_DIV_16  4
#define MAX311XE_RC_DIV_32  5
#define MAX311XE_RC_DIV_64  6
#define MAX311XE_RC_DIV_128 7
#define MAX311XE_RC_DIV_3   8
#define MAX311XE_RC_DIV_6   9
#define MAX311XE_RC_DIV_12  10
#define MAX311XE_RC_DIV_24  11
#define MAX311XE_RC_DIV_48  12
#define MAX311XE_RC_DIV_96  13
#define MAX311XE_RC_DIV_192 14
#define MAX311XE_RC_DIV_384 15

//
// Write data register bits.
//

#define MAX311XE_WD_TE_BAR  (1 << 10) //  1 = Disable transmit
#define MAX311XE_WD_RTS_BAR (1 << 9)  //  1 = RTS active
#define MAX311XE_WD_PT      (1 << 8)  //  1 = Parity bit to transmit
#define MAX311XE_WD_DATA    0xFF      //  Transmit data byte

//
// Read data register bits.
//

#define MAX311XE_RD_R     (1 << 15) //  1 = Receive data available
#define MAX311XE_RD_T     (1 << 14) //  1 = Transmit buffer is empty
#define MAX311XE_RD_RA_FE (1 << 10) //  1 = Receive Activity (UART     \
                                    //      Shutdown) or Framing-Error \
                                    //      (normal operation)
#define MAX311XE_RD_CTS  (1 << 9)   //  1 = CTS active
#define MAX311XE_RD_PR   (1 << 8)   //  Received parity bit
#define MAX311XE_RD_DATA 0xFF       //  Received data byte

//
// Define the device select bits.
//

#define SELECTOR_LED       1
#define SELECTOR_UART      2
#define SELECTOR_2MB_FLASH 4

#define RECEIVE_BUFFER_SIZE 1024

// ------------------------------------------------------------ Data structures

//
// Declare the SPI device registers.
//

typedef struct _DW_APB_SSI_REGISTERS
{
    ULONG Ctrlr0;           //  00  Control Register 0
    ULONG Ctrlr1;           //  04  Control Register 1
    ULONG Ssienr;           //  08  SSI Enable Register
    ULONG Mwcr;             //  0c  Microwire Control Register
    ULONG Ser;              //  10  Slave Enable Register
    ULONG Baudr;            //  14  Baud Rate Select
    ULONG Txftlr;           //  18  Transmit FIFO Threshold Level
    ULONG Rxftlr;           //  1c  Receive FIFO Threshold Level
    ULONG Txflr;            //  20  Transmit FIFO Level Register
    ULONG Rxflr;            //  24  Receive FIFO Level Register
    ULONG Sr;               //  28  Status Register
    ULONG Imr;              //  2c  Interrupt Mask Register
    ULONG Isr;              //  30  Interrupt Status Register
    ULONG Risr;             //  34  Raw Interrupt Status Register
    ULONG Txoicr;           //  38  Transmit FIFO Overflow Interrupt Clear
                            //      Register
    ULONG Rxoicr;           //  3c  Receive FIFO Overflow Interrupt Clear
                            //      Register
    ULONG Rxuicr;           //  40  Receive FIFO Underflow Interrupt Clear
                            //      Register
    ULONG Msticr;           //  44  Multi-Master Interrupt Clear Register
    ULONG Icr;              //  48  Interrupt Clear Register
    ULONG Dmacr;            //  4c  DMA Control Register
    ULONG Dmatdlr;          //  50  DMA Transmit Data Level
    ULONG Dmardlr;          //  54  DMA Receive Data Level
    ULONG Idr;              //  58  Identification Register
    ULONG Ssi_comp_version; //  5c  CoreKit Version ID Register
    ULONG Dr;               //  60  Data Register
} DW_APB_SSI_REGISTERS, *PDW_APB_SSI_REGISTERS;

//
// Internal data for this driver.
//

typedef struct _SERIAL_PORT_MAX311XE
{
    ULONG  RxBufferFill;
    ULONG  RxBufferDrain;
    UINT16 SpiBaudRate;
    UINT16 RxBuffer[RECEIVE_BUFFER_SIZE];
} SERIAL_PORT_MAX311XE, *PSERIAL_PORT_MAX311XE;

// -------------------------------------------------------------------- Globals

static SERIAL_PORT_MAX311XE Max311;

// ----------------------------------------------- Internal Function Prototypes

BOOLEAN
SpiMax311SetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

// --------------------------------------------------------- Internal Functions

VOID
SpiInit(
    _Inout_ PDW_APB_SSI_REGISTERS Spi,
    UINT16                        ControlRegister0,
    UINT16                        ControlRegister1,
    UINT16                        BaudRateRegister)

/*++

Routine Description:

    This routine initializes the SPI controller. It must be called with
    interrupts disabled.

Arguments:

    Spi - Supplies the base address of the SPI controller

    ControlRegister0 - Supplies the value to write into CTRLR0

    ControlRegister1 - Supplies the value to write into CTRLR1

    BaudRateRegister - Supplies the value to write into BAUDR

Return Value:

    None.

--*/

{
    UINT16 Isr;
    UINT16 Risr;
    ULONG  SpiStatus;

    //
    // Wait until the SPI is idle.
    //

    do
    {
        SpiStatus = READ_REGISTER_ULONG(&(Spi->Sr));
        SpiStatus &= SR_BUSY | SR_TFE;
    } while (SpiStatus != SR_TFE);

    //
    // Remove any data not read during the previous transaction.
    //

    SpiStatus = READ_REGISTER_ULONG(&(Spi->Sr)) & SR_RFNE;
    while (SpiStatus != 0)
    {
        //
        // Discard the previous transaction's data.
        //

        READ_REGISTER_ULONG(&(Spi->Dr));
        SpiStatus = READ_REGISTER_ULONG(&(Spi->Sr)) & SR_RFNE;
    }

    //
    // Deselect the previous chip by writing 0 into the Slave Enable Register.
    //

    WRITE_REGISTER_ULONG(&(Spi->Ser), 0);

    //
    // Disable the SPI controller by writing 0 into the Enable register.
    //

    WRITE_REGISTER_ULONG(&(Spi->Ssienr), 0);

    //
    // Initialize the SPI controller. Control Register 0 receives the type and
    // bits per single transfer. Control Register 1 receives the number of bytes
    // to receive. Baudr receives the baud rate. Note that the controller must
    // be disabled when programming these registers.
    //

    WRITE_REGISTER_ULONG(&(Spi->Ctrlr0), ControlRegister0);
    WRITE_REGISTER_ULONG(&(Spi->Ctrlr1), ControlRegister1);
    WRITE_REGISTER_ULONG(&(Spi->Baudr), BaudRateRegister);

    //
    // Clear any pending interrupts in the SPI controller.
    //

    do
    {
        Risr = (UINT16)READ_REGISTER_ULONG(&(Spi->Risr));
        Isr  = (UINT16)READ_REGISTER_ULONG(&(Spi->Isr));
    } while ((Risr != 0) || (Isr != 0));

    //
    // Enable the SPI controller by writing 1 into the Enable Register.
    //

    WRITE_REGISTER_ULONG(&(Spi->Ssienr), 1);
}

UINT16
SpiSend16(
    _In_ PCPPORT Port,
    UINT16       Value)

/*++

Routine Description:

    Write a byte out to the SPI device and receive data back in the process.

Arguments:

    Port - Pointer to the CPPORT object that contains the base address of
        the SPI device.

    Value - The data value to write to the SPI device.

    Return Value:

        Data read from the SPI device.

--*/

{
    UINT32                Data;
    PDW_APB_SSI_REGISTERS Spi;
    ULONG                 SpiStatus;

    //
    // Locate the SPI controller.
    //

    Spi = (PDW_APB_SSI_REGISTERS)Port->Address;

    //
    // Initialize the SPI controller. The value for Control Register 0 indicates
    // to both send and receive data, and to send 16 bits per transfer. The
    // baud rate register is for the SPI controller, not the UART.
    //

    SpiInit(Spi,
            CTRLR0_TMOD_TX_RX | (16 - 1),
            0,
            Max311.SpiBaudRate);

    //
    // Select the UART device on the SPI bus.
    //

    WRITE_REGISTER_ULONG(&(Spi->Ser), SELECTOR_UART);

    //
    // Write the value to the UART.
    //

    WRITE_REGISTER_ULONG(&(Spi->Dr), Value);

    //
    // Wait until the SPI operation is complete.
    //

    do
    {
        SpiStatus = READ_REGISTER_ULONG(&(Spi->Sr));
        SpiStatus &= SR_BUSY | SR_TFE | SR_RFNE;

    } while (SpiStatus != (SR_TFE | SR_RFNE));

    //
    // Read the data value that came in as a result of the send.
    //

    Data = READ_REGISTER_ULONG(&(Spi->Dr));
    return (UINT16)Data;
}

VOID
SpiMax311BufferRxData(
    UINT16 Value)

/*++

Routine Description:

    Place a receive character into the circular receive buffer.

Arguments:

    Value - Value to buffer.

Return Value:

    None.

--*/

{
    ULONG   NextHead;
    PUINT16 RxBufferHead;

    //
    // A receive character is available. Buffer this character for a later
    // receive operation.
    //

    RxBufferHead = Max311.RxBuffer + Max311.RxBufferFill;
    NextHead     = (Max311.RxBufferFill + 1) % RECEIVE_BUFFER_SIZE;
    if (NextHead != Max311.RxBufferDrain)
    {
        *RxBufferHead       = Value;
        Max311.RxBufferFill = NextHead;
    }
}

BOOLEAN
SpiMax311TxEmpty(
    _In_ PCPPORT Port)

/*++

Routine Description:

    This routine determines if the transmit buffer is empty.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

Return Value:

    TRUE if the transmit buffer is empty, FALSE if it has data.

--*/

{
    UINT16 Value;

    //
    // Buffer any receive data.
    //

    while (TRUE)
    {
        //
        // Get the transmit buffer status from the UART.
        //

        Value = SpiSend16(Port, MAX311XE_READ_DATA);

        //
        // Exit the loop if no receive data.
        //

        if (CHECK_FLAG(Value, MAX311XE_RD_R) == 0)
        {
            break;
        }

        //
        // Buffer any received data.
        //

        SpiMax311BufferRxData(Value);
    }

    //
    // Determine if there is space in the transmit buffer.
    //

    if (CHECK_FLAG(Value, MAX311XE_RD_T) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

// ------------------------------------------------------------------ Functions

BOOLEAN
SpiMax311InitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth)

/*++

Routine Description:

    This routine performs the initialization of a MAX311xE serial UART.

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
    PDW_APB_SSI_REGISTERS Spi;

    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(MemoryMapped);
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

    Port->Flags = 0;

    //
    // Discard any previously buffered data.
    //

    Max311.RxBufferDrain = 0;
    Max311.RxBufferFill  = 0;

    //
    // Store the current SPI Baud Rate register value. It is assumed that the
    // firmware set this up to work correctly with the debugging device.
    //

    Spi                = (PDW_APB_SSI_REGISTERS)Port->Address;
    Max311.SpiBaudRate = (UINT16)Spi->Baudr;

    //
    // Initialize the hardware.
    //

    SpiMax311SetBaud(Port, Port->BaudRate);
    return TRUE;
}

BOOLEAN
SpiMax311SetBaud(
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
    UINT16 ConfigValue;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Select the baud rate
    //

    switch (Rate)
    {
    case 230400:
        ConfigValue = MAX311XE_WC_DIV_1;
        break;

    default:
        __fallthrough;

    case 115200:
        ConfigValue = MAX311XE_WC_DIV_2;
        break;

    case 76800:
        ConfigValue = MAX311XE_WC_DIV_3;
        break;

    case 57600:
        ConfigValue = MAX311XE_WC_DIV_4;
        break;

    case 38400:
        ConfigValue = MAX311XE_WC_DIV_6;
        break;

    case 28800:
        ConfigValue = MAX311XE_WC_DIV_8;
        break;

    case 19200:
        ConfigValue = MAX311XE_WC_DIV_12;
        break;

    case 14400:
        ConfigValue = MAX311XE_WC_DIV_16;
        break;

    case 9600:
        ConfigValue = MAX311XE_WC_DIV_24;
        break;

    case 7200:
        ConfigValue = MAX311XE_WC_DIV_32;
        break;

    case 4800:
        ConfigValue = MAX311XE_WC_DIV_48;
        break;

    case 3600:
        ConfigValue = MAX311XE_WC_DIV_64;
        break;

    case 2400:
        ConfigValue = MAX311XE_WC_DIV_96;
        break;

    case 1800:
        ConfigValue = MAX311XE_WC_DIV_128;
        break;

    case 1200:
        ConfigValue = MAX311XE_WC_DIV_192;
        break;

    case 600:
        ConfigValue = MAX311XE_WC_DIV_384;
        break;
    }

    //
    // Configure the UART for 8 data bits, no parity, 1 stop bit (8N1).
    //

    ConfigValue |= MAX311XE_WRITE_CONFIG;
    SpiSend16(Port, ConfigValue);
    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
SpiMax311GetByte(
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
    PUINT16 RxBufferTail;
    UINT16  Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Empty the receive buffer first.
    //

    if (Max311.RxBufferDrain != Max311.RxBufferFill)
    {
        //
        // Get the next value from the receive buffer, and move the buffer
        // pointer.
        //

        RxBufferTail         = Max311.RxBuffer + Max311.RxBufferDrain;
        Value                = *RxBufferTail;
        *Byte                = (UCHAR)Value;
        Max311.RxBufferDrain = (Max311.RxBufferDrain + 1) % RECEIVE_BUFFER_SIZE;

        //
        // Return unsuccessfully if any errors are indicated.
        //

        if (CHECK_FLAG(Value, MAX311XE_RD_RA_FE) != FALSE)
        {
            return UartError;
        }

        return UartSuccess;
    }

    //
    // The receive buffer is empty, so go to the physical device.
    // Read the byte from the UART.
    //

    Value = SpiSend16(Port, MAX311XE_READ_DATA);
    if (CHECK_FLAG(Value, MAX311XE_RD_R) != 0)
    {
        //
        // Return unsuccessfully if any errors are indicated.
        //

        *Byte = (UCHAR)Value;
        if (CHECK_FLAG(Value, MAX311XE_RD_RA_FE) != FALSE)
        {
            return UartError;
        }

        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
SpiMax311PutByte(
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
    UINT16 Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return UartNotReady;
    }

    //
    // Wait for the port to be not busy.
    //

    if (BusyWait != FALSE)
    {
        while (!SpiMax311TxEmpty(Port))
            ;
    }
    else if (!SpiMax311TxEmpty(Port))
    {
        return UartNotReady;
    }

    //
    // The transmitter buffer is now clear, send the character.
    //

    Value = SpiSend16(Port, (UINT16)Byte | MAX311XE_WRITE_DATA);

    //
    // Buffer any receive data.
    //

    while (TRUE)
    {
        //
        // Exit the loop if no receive data came through.
        //

        if (CHECK_FLAG(Value, MAX311XE_RD_R) == 0)
        {
            break;
        }

        //
        // Buffer any receive data.
        //

        SpiMax311BufferRxData(Value);

        //
        // Get the receive buffer status from the UART.
        //

        Value = SpiSend16(Port, MAX311XE_READ_DATA);
    }

    return UartSuccess;
}

BOOLEAN
SpiMax311RxReady(
    _In_ PCPPORT Port)

/*++

Routine Description:

    This routine determines if there is data pending in the receive buffer.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

Return Value:

    TRUE if data is available, FALSE otherwise.

--*/

{
    UINT16 Value;

    if ((Port == NULL) || (Port->Address == NULL))
    {
        return FALSE;
    }

    //
    // Buffer any receive data.
    //

    while (TRUE)
    {
        //
        // Get the receive buffer status from the UART.
        //

        Value = SpiSend16(Port, MAX311XE_READ_DATA);

        //
        // Exit the loop if no receive data came through.
        //

        if (CHECK_FLAG(Value, MAX311XE_RD_R) == 0)
        {
            break;
        }

        //
        // Buffer any received data.
        //

        SpiMax311BufferRxData(Value);
    }

    //
    // Determine if there is a received character waiting in the buffer.
    //

    if (Max311.RxBufferFill != Max311.RxBufferDrain)
    {
        return TRUE;
    }

    return FALSE;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER SpiMax311HardwareDriver = {
    SpiMax311InitializePort,
    SpiMax311SetBaud,
    SpiMax311GetByte,
    SpiMax311PutByte,
    SpiMax311RxReady};
