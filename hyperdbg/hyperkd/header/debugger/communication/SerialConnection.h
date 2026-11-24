/**
 * @file SerialConnection.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for serial port connection from debuggee to debugger
 * @details
 * @version 0.1
 * @date 2020-12-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			  External Functions				//
//////////////////////////////////////////////////

VOID
KdHyperDbgTest(UINT16 Byte);

VOID
KdHyperDbgPrepareDebuggeeConnectionPort(UINT32 PortAddress, UINT32 Baudrate);

VOID
KdHyperDbgSendByte(UCHAR Byte, BOOLEAN BusyWait);

BOOLEAN
KdHyperDbgRecvByte(PUCHAR RecvByte);

BOOLEAN
KdHyperDbgDbcIsActive();

BOOLEAN
KdHyperDbgDbcIsReady();

VOID
KdHyperDbgDbcSetLongWait(BOOLEAN Allow);

BOOLEAN
KdHyperDbgDbcSetSendWait(BOOLEAN Allow);

VOID
KdHyperDbgDbcSetControlPriority(BOOLEAN Enable);

BOOLEAN
KdHyperDbgDbcCanSendPacket(UINT64 Length);

BOOLEAN
KdHyperDbgDbcWaitForSend(UINT32 Milliseconds);

BOOLEAN
KdHyperDbgDbcWaitForSendEx(UINT32 Milliseconds, BOOLEAN IgnoreLoss);

VOID
KdHyperDbgDbcClearSendLoss();

UINT32
KdHyperDbgDbcGetSendLossCount();

//////////////////////////////////////////////////
//			  USB VMCALL Structures				//
//////////////////////////////////////////////////

//
// Operations performed under DebuggerResponseLock in vmx-root for USB setup and reload
//
typedef enum _SERIAL_CONNECTION_DBC_OP
{
    SERIAL_CONNECTION_DBC_OP_CLEAR_SEND_LOSS = 0,
    SERIAL_CONNECTION_DBC_OP_POLL_SEND,
    SERIAL_CONNECTION_DBC_OP_SET_CONTROL_PRIORITY,
    SERIAL_CONNECTION_DBC_OP_SET_LONG_WAIT,
    SERIAL_CONNECTION_DBC_OP_BEGIN_SEND_AND_WAIT,
    SERIAL_CONNECTION_DBC_OP_TRY_SEND_SYMBOL,
    SERIAL_CONNECTION_DBC_OP_TRY_SEND_CONFIRMED,
} SERIAL_CONNECTION_DBC_OP;

//
// Parameters for one USB operation executed through a vmcall
//
typedef struct _SERIAL_CONNECTION_DBC_OPERATION
{
    SERIAL_CONNECTION_DBC_OP                Type;
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response;
    CHAR *                                  Buffer;
    UINT32                                  Length;
    UINT32                                  LossCount;
    BOOLEAN                                 Flag;
    BOOLEAN                                 IsFinishPacket;
    BOOLEAN                                 PriorityHeld;
    BOOLEAN                                 Result;
    BOOLEAN                                 Lost;
    BOOLEAN                                 Abort;
} SERIAL_CONNECTION_DBC_OPERATION, *PSERIAL_CONNECTION_DBC_OPERATION;

//////////////////////////////////////////////////
//					 Functions					//
//////////////////////////////////////////////////

VOID
SerialConnectionTest();

NTSTATUS
SerialConnectionPrepare(PDEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest);

BOOLEAN
SerialConnectionCheckPort(UINT32 SerialPort);

BOOLEAN
SerialConnectionCheckBaudrate(DWORD Baudrate);

BOOLEAN
SerialConnectionSend(CHAR * Buffer, UINT32 Length);

BOOLEAN
SerialConnectionRecvBuffer(CHAR *   BufferToSave,
                           UINT32 * LengthReceived);

BOOLEAN
SerialConnectionSendTwoBuffers(CHAR * Buffer1, UINT32 Length1, CHAR * Buffer2, UINT32 Length2);

BOOLEAN
SerialConnectionSendThreeBuffers(CHAR * Buffer1,
                                 UINT32 Length1,
                                 CHAR * Buffer2,
                                 UINT32 Length2,
                                 CHAR * Buffer3,
                                 UINT32 Length3);

NTSTATUS
SerialConnectionPrepareDbc(PDEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest);

NTSTATUS
SerialConnectionDbcHandshake(PDEBUGGEE_DBC_HANDSHAKE_PACKET DbcHandshakeRequest);

NTSTATUS
SerialConnectionDbcReceive(PDEBUGGEE_DBC_RECEIVE_PACKET DbcReceiveRequest);

VOID
SerialConnectionDbcReceiveFromVmxRoot(PDEBUGGEE_DBC_RECEIVE_PACKET DbcReceiveRequest);

VOID
SerialConnectionDbcOperationFromVmxRoot(PSERIAL_CONNECTION_DBC_OPERATION Operation);

VOID
SerialConnectionDbcSetLongWait(BOOLEAN Allow);

BOOLEAN
SerialConnectionDbcWaitForHost(UINT32 TimeoutMs);

BOOLEAN
SerialConnectionDbcSendSymbolPacket(CHAR * Buffer, UINT32 Length);

BOOLEAN
SerialConnectionDbcSendAndWaitForHost(DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
                                      CHAR *                                  Buffer,
                                      UINT32                                  Length);

VOID
SerialConnectionDbcEndSetup();

UINT32
SerialConnectionDbcSendSetupBuffer(PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest);

//////////////////////////////////////////////////
//					 Constants					//
//////////////////////////////////////////////////

//
// Baud rates at which the communication device operates
//
#define CBR_110    110
#define CBR_300    300
#define CBR_600    600
#define CBR_1200   1200
#define CBR_2400   2400
#define CBR_4800   4800
#define CBR_9600   9600
#define CBR_14400  14400
#define CBR_19200  19200
#define CBR_38400  38400
#define CBR_56000  56000
#define CBR_57600  57600
#define CBR_115200 115200
#define CBR_128000 128000
#define CBR_256000 256000

//
// Serial ports
//
#define COM1_PORT 0x03F8
#define COM2_PORT 0x02F8
#define COM3_PORT 0x03E8
#define COM4_PORT 0x02E8

//
// The xHCI Debug Capability (DbC) is used instead of a serial port
//
#define SERIAL_PORT_DBC 0xDBC0

//
// Time limit for the host to take each USB setup packet
//
#define SERIAL_CONNECTION_DBC_SETUP_TIMEOUT_MS 30000

//
// Length of each sleep between USB setup or reload retries in milliseconds
//
#define SERIAL_CONNECTION_DBC_POLL_MS 1

//
// Short vmcall poll window in microseconds before sleeping on a full ring or busy host
//
#define SERIAL_CONNECTION_DBC_BUSY_POLL_US 1000

//
// Maximum number of vmcall polls in one short poll burst
//
#define SERIAL_CONNECTION_DBC_BUSY_POLL_MAX_ITERS 1024

//
// Number of CPU pause instructions between short vmcall polls
//
#define SERIAL_CONNECTION_DBC_BUSY_POLL_PAUSES 32
