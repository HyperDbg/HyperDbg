/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    kdcom.h

Abstract:

    Contains constants used when interacting with 16550-based serial ports.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-1990

Revision History:

    John Vert (jvert) 19-Jul-1991
        Moved into HAL

--*/

#pragma once

#define COM1_PORT 0x03F8
#define COM2_PORT 0x02F8
#define COM3_PORT 0x03E8
#define COM4_PORT 0x02E8

#define COM_DAT 0x00
#define COM_IEN 0x01 // interrupt enable register
#define COM_FCR 0x02 // fifo control register
#define COM_LCR 0x03 // line control register
#define COM_MCR 0x04 // modem control register
#define COM_LSR 0x05 // line status register
#define COM_MSR 0x06 // modem status register
#define COM_SCR 0x07 // scratch register
#define COM_DLL 0x00 // divisor latch least sig
#define COM_DLM 0x01 // divisor latch most sig

#define COM_BI 0x10 // Break detect
#define COM_FE 0x08 // Framing error
#define COM_PE 0x04 // Parity error
#define COM_OE 0x02 // Overrun error

#define LC_DLAB 0x80 // LCR divisor latch access bit

#define CLOCK_RATE 115200 // Hardware base clock frequency

#define MC_DTRRTS   0x03 // Control bits to assert DTR and RTS
#define MS_DSRCTSCD 0xB0 // Status bits for DSR, CTS and CD
#define MS_CD       0x80 // MSR bit to indicate carrier detect

#define FC_ENABLE         0x01 // FCR control bit to enable the FIFO
#define FC_CLEAR_RECEIVE  0x02 // FCR control bit to clear receive FIFO
#define FC_CLEAR_TRANSMIT 0x04 // FCR control bit to clear transmit FIFO

#define COM_OUTRDY 0x20 // LSR bit to indicate transmitter is empty
#define COM_DATRDY 0x01 // LSR bit to indicate data is available

#define BD_150    150
#define BD_300    300
#define BD_600    600
#define BD_1200   1200
#define BD_2400   2400
#define BD_4800   4800
#define BD_9600   9600
#define BD_14400  14400
#define BD_19200  19200
#define BD_56000  56000
#define BD_57600  57600
#define BD_115200 115200

//
// This bit controls the loopback testing mode of the device.  Basically
// the outputs are connected to the inputs (and vice versa).
//

#define SERIAL_MCR_LOOP 0x10

//
// This bit is used for general purpose output.
//

#define SERIAL_MCR_OUT1 0x04

//
// This bit contains the (complemented) state of the clear to send
// (CTS) line.
//

#define SERIAL_MSR_CTS 0x10

//
// This bit contains the (complemented) state of the data set ready
// (DSR) line.
//

#define SERIAL_MSR_DSR 0x20

//
// This bit contains the (complemented) state of the ring indicator
// (RI) line.
//

#define SERIAL_MSR_RI 0x40

//
// This bit contains the (complemented) state of the data carrier detect
// (DCD) line.
//

#define SERIAL_MSR_DCD 0x80

#define SERIAL_LSR_NOT_PRESENT 0xff
