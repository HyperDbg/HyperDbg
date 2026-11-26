/**
 * @file dbc.c
 * @author lava1879 (magicv4@proton.me)
 * @brief xHCI Debug Capability (DbC) transport for the debuggee
 * @details Call DbcPrepare at PASSIVE_LEVEL. The send and receive paths
 * reuse the mapped registers and allocated memory in vmx-root.
 *
 * @version 0.1
 * @date 2026-11-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <ntddk.h>
#include <intrin.h>
#include "dbc.h"

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

//
// USB identity for the vendor-defined protocol
//
#define DBC_VID      0x0525
#define DBC_PID      0x127A
#define DBC_PROTOCOL 0x00
#define DBC_REVISION 0x0000

//
// Register offsets from the DbC base
//
#define DBC_REG_DCDB     0x04
#define DBC_REG_DCERSTSZ 0x08
#define DBC_REG_DCERSTBA 0x10
#define DBC_REG_DCERDP   0x18
#define DBC_REG_DCCTRL   0x20
#define DBC_REG_DCPORTSC 0x28
#define DBC_REG_DCCP     0x30
#define DBC_REG_DCDDI1   0x38
#define DBC_REG_DCDDI2   0x3C

//
// Masks for reserved bits that writes must preserve
//
#define DBC_DCERSTSZ_RSVDP 0xFFFF0000u
#define DBC_DCERSTBA_RSVDP 0x0000000Fu
#define DBC_DCERDP_RSVDP   0x00000008u
#define DBC_DCCTRL_RSVDP   0x0000FFE0u
#define DBC_DCCP_RSVDP     0x0000000Fu

//
// Control and port status bits
//
#define DBC_DCCTRL_DCR       0x00000001u
#define DBC_DCCTRL_HOT       0x00000004u
#define DBC_DCCTRL_HIT       0x00000008u
#define DBC_DCCTRL_DRC       0x00000010u
#define DBC_DCCTRL_DCE       0x80000000u
#define DBC_DCPORTSC_CCS     0x00000001u
#define DBC_DCPORTSC_PED     0x00000002u
#define DBC_DCPORTSC_PR      0x00000010u
#define DBC_DCPORTSC_CSC     0x00020000u
#define DBC_DCPORTSC_PRC     0x00200000u
#define DBC_DCPORTSC_PLC     0x00400000u
#define DBC_DCPORTSC_CEC     0x00800000u
#define DBC_DCPORTSC_CHANGES (DBC_DCPORTSC_CSC | DBC_DCPORTSC_PRC | DBC_DCPORTSC_PLC | DBC_DCPORTSC_CEC)

//
// Doorbell targets
//
#define DBC_DOORBELL_OUT 0
#define DBC_DOORBELL_IN  1

//
// Endpoint state values
//
#define DBC_EP_STATE_MASK     0x00000007u
#define DBC_EP_STATE_DISABLED 0u
#define DBC_EP_STATE_HALTED   2u
#define DBC_EP_STATE_STOPPED  3u

//
// Transfer Request Block types, completion codes, and length mask
//
#define DBC_TRB_NORMAL         1
#define DBC_TRB_LINK           6
#define DBC_TRB_NO_OP          8
#define DBC_TRB_TRANSFER_EVENT 32
#define DBC_TRB_HOST_CONTROLLER_EVENT 37
#define DBC_CC_SUCCESS         1
#define DBC_CC_BABBLE_ERROR    3
#define DBC_CC_USB_TRANS_ERROR 4
#define DBC_CC_TRB_ERROR       5
#define DBC_CC_STALL_ERROR     6
#define DBC_CC_SHORT_PACKET    13
#define DBC_CC_EVENT_RING_FULL 21
#define DBC_TRB_LENGTH_MASK    0x1FFFFu

//
// Ring and packet sizes
//
#define DBC_RING_TRBS  256
#define DBC_EVENT_TRBS 256
//
// Limit the number of events processed in one poll to bound vmx-root time
//
#define DBC_MAX_EVENTS_PER_POLL (DBC_EVENT_TRBS * 4)
#define DBC_MAX_PACKET 1024

//
// 128 OUT and one IN bound normal completions, not port or error events
//
#define DBC_OUT_MAX_PENDING 128

//
// The USB memory block stays within one 64 KB boundary
//
#define DBC_OFFSET_CONTEXT 0x0000 // Info and endpoint contexts
#define DBC_OFFSET_STRINGS 0x0100 // USB strings
#define DBC_OFFSET_ERST    0x0200 // Event ring segment table
//
// Event, OUT, and IN rings each occupy one 4 KB page (256 TRBs)
//
#define DBC_OFFSET_EVENT   0x1000
#define DBC_OFFSET_OUT     0x2000
#define DBC_OFFSET_IN      0x3000
//
// One receive packet
//
#define DBC_OFFSET_RX      0x4000
//
// The 44 KB USB send ring starts at this offset
//
#define DBC_OFFSET_TX      0x5000
#define DBC_BLOCK_SIZE     0x10000
#define DBC_TX_BUFFER_SIZE (DBC_BLOCK_SIZE - DBC_OFFSET_TX)
//
// Stream large USB packets in smaller parts
//
#define DBC_TX_STREAM_SIZE (DBC_TX_BUFFER_SIZE / 4)
//
// Maximum number of bytes in one OUT TRB, also the page boundary for splits
//
#define DBC_TX_TRB_MAX     0x1000

//
// Offsets within the USB string area
//
#define DBC_STRING_LANGID       0x00
#define DBC_STRING_MANUFACTURER 0x08
#define DBC_STRING_PRODUCT      0x20
#define DBC_STRING_SERIAL       0x60

//
// PCI and controller discovery constants
//
#define DBC_PCI_MAX_BUSES        256
#define DBC_PCI_MAX_DEVICES      32
#define DBC_PCI_MAX_FUNCTIONS    8
#define DBC_PCI_COMMAND_MSE_BME  0x06u
#define DBC_PCI_CLASS_XHCI       0x0C0330u
#define DBC_XHCI_HCCPARAMS1      0x10u
#define DBC_XHCI_USBCMD_OFFSET   0x00u
#define DBC_XHCI_USBCMD_HCRST    0x00000002u
#define DBC_XHCI_USBSTS_OFFSET   0x04u
#define DBC_XHCI_USBSTS_CNR      0x00000800u
#define DBC_XHCI_CAPABILITY_ID   10u
#define DBC_MAX_EXT_CAPABILITIES 256u
#define DBC_REGISTER_BLOCK_SIZE  0x40u

//
// Timeouts in milliseconds
//
#define DBC_CONNECT_TIMEOUT_MS   3000
#define DBC_CONFIGURE_TIMEOUT_MS 10000
#define DBC_SEND_TIMEOUT_MS      1000
#define DBC_DISABLE_TIMEOUT_MS   100
#define DBC_MAXIMUM_SPINS        50000000
#define DBC_WATCHDOG_TIMEOUT_MS  5000
#define DBC_WATCHDOG_MAXIMUM_MS  60000
#define DBC_CONTROLLER_READY_TIMEOUT_MS 1000

//
// Default send wait in microseconds while the debuggee runs
//
#define DBC_RUNNING_WAIT_US 50000

//
// Default limits for packets that wait for a TRB while OUT TRBs are pending
//
#define DBC_COALESCE_PENDING 2
#define DBC_COALESCE_BYTES   DBC_TX_TRB_MAX

//
// Values for time conversion and register checks
//
#define DBC_US_PER_MS              1000ULL
#define DBC_INVALID_REGISTER_VALUE MAXUINT32

//
// Match the end-of-buffer marker in SerialConnection.h
//
#define DBC_END_OF_BUFFER_CHAR_1 0x00
#define DBC_END_OF_BUFFER_CHAR_2 0x80
#define DBC_END_OF_BUFFER_CHAR_3 0xEE
#define DBC_END_OF_BUFFER_CHAR_4 0xFF
#define DBC_END_OF_BUFFER_MARKER                \
    (((UINT32)DBC_END_OF_BUFFER_CHAR_1 << 24) | \
     ((UINT32)DBC_END_OF_BUFFER_CHAR_2 << 16) | \
     ((UINT32)DBC_END_OF_BUFFER_CHAR_3 << 8) |  \
     (UINT32)DBC_END_OF_BUFFER_CHAR_4)

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief DbC state
 *
 */
typedef struct _DBC_STATE
{
    BOOLEAN Active;     // USB selected
    BOOLEAN Ready;      // Host configured the device during setup
    BOOLEAN Configured; // Last polled configuration state
    BOOLEAN Ac64;       // Supports 64-bit addresses
    BOOLEAN InPosted;   // Controller owns an IN transfer
    //
    // The host took no OUT data during the long wait
    //
    BOOLEAN TxStuck;
    BOOLEAN OutHalted;
    BOOLEAN InHalted;

    PUCHAR Mmio; // Mapped register pages
    SIZE_T MmioSize;
    PUCHAR Dbc; // Register base within the mapping
    PUCHAR Block;
    UINT64 BlockPa;

    UINT32 Bus;
    UINT32 Device;
    UINT32 Function;

    UINT32 OutEnqueue;
    UINT32 OutCycle;
    UINT32 InEnqueue;
    UINT32 InCycle;
    UINT32 EventDequeue;
    UINT32 EventCycle;
    UINT32 OutPending;
    UINT64 OutRetainedTrb;

    //
    // Send ring in the TX area, bytes from TxTail to TxHead are in use
    //

    //
    // Offset of the next byte to write
    //
    UINT32  TxHead;
    //
    // Offset of the oldest byte that the controller can read
    //
    UINT32  TxTail;
    //
    // Number of bytes in use in the send ring
    //
    UINT32  TxUsed;
    //
    // Bytes without a TRB, including complete staged packets
    //
    UINT32  TxUncommitted;
    //
    // Number of TRBs reserved for bytes without a TRB
    //
    UINT32  TxUncommittedTrbs;
    //
    // Number of bytes in the last reserved TRB
    //
    UINT32  TxChunkBytes;
    //
    // Time that the current packet spent waiting for space
    //
    UINT32  TxPacketWaitUs;
    //
    // Part of the current packet is queued
    //
    BOOLEAN TxPacketStarted;
    //
    // Drop bytes until the end marker
    //
    BOOLEAN TxDropping;
    //
    // A short wait ended without progress
    //
    BOOLEAN TxNoProgress;
    //
    // A long-wait packet or queued data was lost
    //
    BOOLEAN TxLoss;
    //
    // Use long waits during a halt or USB setup
    //
    BOOLEAN LongWait;
    UINT64  LastOutProgressTsc;

    UINT32 TxWindow;
    UINT32 TxWindowBytes;
    UINT32 RxLength;
    UINT32 RxPosition;

    UINT64 TscPerMs;
    UINT64 WatchdogStartTsc; // Host connected but did not configure DbC since then
    UINT32 WatchdogTimeoutMs;

    //
    // Counters for the kernel debugger
    //
    UINT64 TxBytes;
    UINT64 RxBytes;
    UINT32 TxDroppedBytes;
    UINT32 TxTimeouts;
    UINT32 Reprograms;
    UINT32 RingResets;
    UINT32 ErrorEvents;
    UINT32 LastErrorCode;
    UINT32 OutStallEvents;
    UINT32 InStallEvents;
    UINT32 OtherErrorEvents;
    UINT32 UnknownTrbEvents;
    UINT32 LastErrorResidual;
    UINT64 LastErrorTrb;
    UINT32 ConfigurationTimeoutResets; // DbC turned off because the host did not configure it in time

    //
    // Send ring tuning and counters
    //

    //
    // Maximum wait for space while the debuggee runs
    //
    UINT32 TxRunningWaitUs;
    //
    // Number of complete packets accepted into the send ring
    //
    UINT32 TxPacketsSent;
    //
    // Number of whole packets dropped
    //
    UINT32 TxPacketsDropped;
    //
    // Number of packets cut after a failure
    //
    UINT32 TxPacketsCut;
    //
    // Number of waits for space in the send ring
    //
    UINT32 TxRingFullWaits;
    //
    // Longest wait for space in microseconds
    //
    UINT32 TxLongestWaitUs;
    //
    // Number of bytes that the controller owns
    //
    UINT32 TxBytesInFlight;
    //
    // Number of events read from the event ring
    //
    UINT32 EventsProcessed;
    //
    // Number of OUT TRBs queued
    //
    UINT32 TxTrbsQueued;
    //
    // Queue staged packets below this pending OUT TRB count
    //
    UINT32 TxCoalescePending;
    //
    // Queue staged packets when they reach this byte count
    //
    UINT32 TxCoalesceBytes;
    //
    // Number of bytes in complete packets without a TRB
    //
    UINT32 TxStaged;
    //
    // Number of complete packets without a TRB
    //
    UINT32 TxStagedPackets;
    //
    // Number of protected send or delivery wait scopes
    //
    UINT32 TxControlWaiters;
    //
    // Number of whole event packets dropped for control priority
    //
    UINT32 TxPriorityDrops;
    //
    // Number of delivery loss records since the USB pass started
    //
    UINT32 TxLossCount;
    //
    // Number of Event Ring Full reports
    //
    UINT32 EventRingFullEvents;
    //
    // Last event address published to DCERDP
    //
    UINT64 EventDequeuePublished;
    //
    // First controller page kept for runtime readiness checks
    //
    PUCHAR ControllerMmio;
    //
    // Mapped USBSTS register
    //
    volatile ULONG * UsbSts;
    //
    // Number of OUT TRBs ending at a page split within a commit
    //
    UINT32 TxPageSplitTrbs;
    //
    // Number of event polls that reached the per-poll event limit
    //
    UINT32 EventPollCapHits;
    //
    // Mapped USBCMD register
    //
    volatile ULONG * UsbCmd;

} DBC_STATE, *PDBC_STATE;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

DBC_STATE g_DbcState = {.TxRunningWaitUs   = DBC_RUNNING_WAIT_US,
                        .TxCoalescePending = DBC_COALESCE_PENDING,
                        .TxCoalesceBytes   = DBC_COALESCE_BYTES};

//
// Send ring bytes that each OUT TRB slot frees when it completes
//
static UINT32 g_DbcOutTrbBytes[DBC_RING_TRBS];

static const WCHAR g_DbcStringLangId[]       = {0x0409, 0};
static const WCHAR g_DbcStringManufacturer[] = L"Intel";
static const WCHAR g_DbcStringProduct[]      = L"USB 3.0 Debug Cable";
static const WCHAR g_DbcStringSerial[]       = L"1";

//////////////////////////////////////////////////
//			Registers and Memory				//
//////////////////////////////////////////////////

/**
 * @brief Check CNR before accessing DbC registers
 * @details xHCI 1.2 section 7.6.4.1 guards DbC access while CNR is set.
 * Section 5.4.2 defines the USBSTS.CNR access restriction.
 *
 * @return BOOLEAN TRUE if the controller is ready
 */
static BOOLEAN
DbcControllerReady()
{
    volatile ULONG * UsbSts = g_DbcState.UsbSts;
    UINT32           Status;

    if (UsbSts == NULL)
    {
        return FALSE;
    }

    Status = READ_REGISTER_ULONG(UsbSts);

    return Status != DBC_INVALID_REGISTER_VALUE && !(Status & DBC_XHCI_USBSTS_CNR);
}

/**
 * @brief Check HCRST before programming DbC registers
 * @details xHCI 1.2 section 5.4.1 guards register access while USBCMD.HCRST is set.
 *
 * @return BOOLEAN TRUE if no host controller reset is active
 */
static BOOLEAN
DbcControllerResetIdle()
{
    volatile ULONG * UsbCmd = g_DbcState.UsbCmd;
    UINT32           Command;

    if (UsbCmd == NULL)
    {
        return FALSE;
    }

    Command = READ_REGISTER_ULONG(UsbCmd);

    return Command != DBC_INVALID_REGISTER_VALUE && !(Command & DBC_XHCI_USBCMD_HCRST);
}

/**
 * @brief Record an interrupted protected send or data already queued for USB
 * @details Call only from serialized DbC register operations.
 *
 * @return VOID
 */
static VOID
DbcRecordUnavailableSend()
{
    if (g_DbcState.TxBytesInFlight != 0 || g_DbcState.TxStaged != 0 || g_DbcState.TxPacketStarted ||
        (g_DbcState.LongWait && g_DbcState.TxUsed != 0))
    {
        g_DbcState.TxLoss = TRUE;
        g_DbcState.TxLossCount++;
    }
}

/**
 * @brief Read a 32-bit DbC register
 *
 * @param Offset Byte offset from the DbC capability base
 * @return UINT32
 */
static UINT32
DbcRead32(UINT32 Offset)
{
    UINT32 Value;

    if (!DbcControllerReady())
    {
        DbcRecordUnavailableSend();
        return DBC_INVALID_REGISTER_VALUE;
    }

    Value = READ_REGISTER_ULONG((volatile ULONG *)(g_DbcState.Dbc + Offset));
    if (Value == DBC_INVALID_REGISTER_VALUE)
    {
        DbcRecordUnavailableSend();
    }

    return Value;
}

/**
 * @brief Write a 32-bit DbC register
 *
 * @param Offset Byte offset from the DbC capability base
 * @param Value Value to write
 * @return BOOLEAN TRUE if the register was written
 */
static BOOLEAN
DbcWrite32(UINT32 Offset, UINT32 Value)
{
    if (!DbcControllerReady())
    {
        DbcRecordUnavailableSend();
        return FALSE;
    }

    WRITE_REGISTER_ULONG((volatile ULONG *)(g_DbcState.Dbc + Offset), Value);

    return TRUE;
}

/**
 * @brief Write a DbC address and preserve reserved bits
 * @details Write the high 32 bits only if the controller supports 64-bit addresses.
 *
 * @param Offset Byte offset from the DbC capability base
 * @param Value Physical address value to write
 * @param PreserveMask Reserved bits to preserve in the low 32 bits
 * @return BOOLEAN TRUE if the address was written
 */
static BOOLEAN
DbcWrite64(UINT32 Offset, UINT64 Value, UINT32 PreserveMask)
{
    UINT32 Current = DbcRead32(Offset);
    UINT32 Low;

    if (Current == DBC_INVALID_REGISTER_VALUE)
    {
        return FALSE;
    }

    Low = ((UINT32)Value & ~PreserveMask) | (Current & PreserveMask);
    if (!DbcWrite32(Offset, Low))
    {
        return FALSE;
    }

    if (g_DbcState.Ac64)
    {
        return DbcWrite32(Offset + 4, (UINT32)(Value >> 32));
    }

    return TRUE;
}

/**
 * @brief Compute the physical address of an offset in the contiguous block
 *
 * @param Offset Byte offset inside the block
 * @return UINT64
 */
static UINT64
DbcPhysical(UINT32 Offset)
{
    return g_DbcState.BlockPa + Offset;
}

/**
 * @brief Get a 32-bit volatile pointer into the contiguous block
 *
 * @param Offset Byte offset inside the block
 * @return volatile UINT32 *
 */
static volatile UINT32 *
DbcBlock32(UINT32 Offset)
{
    return (volatile UINT32 *)(g_DbcState.Block + Offset);
}

/**
 * @brief Get an 8-bit volatile pointer into the contiguous block
 *
 * @param Offset Byte offset inside the block
 * @return volatile UCHAR *
 */
static volatile UCHAR *
DbcBlock8(UINT32 Offset)
{
    return (volatile UCHAR *)(g_DbcState.Block + Offset);
}

/**
 * @brief Fill a 32-bit aligned slice of the contiguous block with zeros
 *
 * @param Offset Byte offset inside the block
 * @param Size Size in bytes, a multiple of four
 * @return VOID
 */
static VOID
DbcZeroBlock(UINT32 Offset, UINT32 Size)
{
    volatile UINT32 * Pointer = DbcBlock32(Offset);

    for (UINT32 i = 0; i < Size / sizeof(UINT32); i++)
    {
        Pointer[i] = 0;
    }
}

/**
 * @brief Check for a timeout in vmx-root
 * @details Limit the number of polls in case the timestamp counter stops.
 *
 * @param StartTsc Initial timestamp counter value
 * @param Milliseconds Timeout in milliseconds
 * @param Spins Iteration counter
 * @return BOOLEAN
 */
static BOOLEAN
DbcTimeoutExpired(UINT64 StartTsc, UINT32 Milliseconds, UINT64 * Spins)
{
    (*Spins)++;

    if (*Spins > DBC_MAXIMUM_SPINS)
    {
        return TRUE;
    }

    return (__rdtsc() - StartTsc) > (g_DbcState.TscPerMs * Milliseconds);
}

/**
 * @brief Compute the microseconds since a timestamp counter value
 * @details Returns zero if this core reads an earlier timestamp.
 *
 * @param StartTsc Initial timestamp counter value
 * @return UINT64
 */
static UINT64
DbcElapsedUs(UINT64 StartTsc)
{
    INT64 Ticks = (INT64)(__rdtsc() - StartTsc);

    if (Ticks <= 0 || g_DbcState.TscPerMs == 0)
    {
        return 0;
    }

    return ((UINT64)Ticks * DBC_US_PER_MS) / g_DbcState.TscPerMs;
}

/**
 * @brief Check for a timeout in microseconds in vmx-root
 * @details Limit the number of polls in case the timestamp counter stops.
 *
 * @param StartTsc Initial timestamp counter value
 * @param Microseconds Timeout in microseconds
 * @param Spins Iteration counter
 * @return BOOLEAN
 */
static BOOLEAN
DbcTimeoutExpiredUs(UINT64 StartTsc, UINT32 Microseconds, UINT64 * Spins)
{
    (*Spins)++;

    if (*Spins > DBC_MAXIMUM_SPINS)
    {
        return TRUE;
    }

    return DbcElapsedUs(StartTsc) >= Microseconds;
}

//////////////////////////////////////////////////
//					  Rings						//
//////////////////////////////////////////////////

/**
 * @brief Zero a transfer ring and write its trailing Link TRB
 *
 * @param RingOffset Byte offset of the transfer ring in the block
 * @return VOID
 */
static VOID
DbcInitializeTransferRing(UINT32 RingOffset)
{
    volatile UINT32 * Link   = DbcBlock32(RingOffset + (DBC_RING_TRBS - 1) * 16);
    UINT64            RingPa = DbcPhysical(RingOffset);

    DbcZeroBlock(RingOffset, DBC_RING_TRBS * 16);

    //
    // Link back to the start and toggle the cycle on wrap
    //
    Link[0] = (UINT32)RingPa;
    Link[1] = (UINT32)(RingPa >> 32);
    Link[3] = (DBC_TRB_LINK << 10) | 0x2;
}

/**
 * @brief Initialize both endpoint contexts
 *
 * @param Control Validated DCCTRL register value carrying the burst size
 * @return VOID
 */
static VOID
DbcInitializeEndpointContexts(UINT32 Control)
{
    volatile UINT32 * Context = DbcBlock32(DBC_OFFSET_CONTEXT);
    UINT32            EpInfo2;

    EpInfo2 = (((Control >> 16) & 0xFF) << 8) | ((UINT32)DBC_MAX_PACKET << 16);

    //
    // Bulk OUT at offset 64 with dequeue cycle state 1
    //
    Context[16 + 0] = 0;
    Context[16 + 1] = EpInfo2 | (2 << 3);
    Context[16 + 2] = (UINT32)DbcPhysical(DBC_OFFSET_OUT) | 1;
    Context[16 + 3] = (UINT32)(DbcPhysical(DBC_OFFSET_OUT) >> 32);
    Context[16 + 4] = 0x1000;

    //
    // Bulk IN at offset 128 with dequeue cycle state 1
    //
    Context[32 + 0] = 0;
    Context[32 + 1] = EpInfo2 | (6 << 3);
    Context[32 + 2] = (UINT32)DbcPhysical(DBC_OFFSET_IN) | 1;
    Context[32 + 3] = (UINT32)(DbcPhysical(DBC_OFFSET_IN) >> 32);
    Context[32 + 4] = 0x1000;
}

/**
 * @brief Queue one Normal TRB on a transfer ring
 *
 * @param RingOffset Byte offset of the transfer ring in the block
 * @param Enqueue Enqueue index to update
 * @param Cycle Producer cycle state to update
 * @param DataPa Physical address of the payload buffer
 * @param Length Transfer length in bytes
 * @return VOID
 */
static VOID
DbcQueueNormalTrb(UINT32 RingOffset, UINT32 * Enqueue, UINT32 * Cycle, UINT64 DataPa, UINT32 Length)
{
    volatile UINT32 * Trb  = DbcBlock32(RingOffset + *Enqueue * 16);
    volatile UINT32 * Link = DbcBlock32(RingOffset + (DBC_RING_TRBS - 1) * 16);
    BOOLEAN           Wrap = (*Enqueue == DBC_RING_TRBS - 2);

    Trb[0] = (UINT32)DataPa;
    Trb[1] = (UINT32)(DataPa >> 32);
    Trb[2] = Length & DBC_TRB_LENGTH_MASK;

    if (Wrap)
    {
        //
        // Hand the Link TRB to the controller before exposing the last Normal TRB
        //
        Link[3] = (Link[3] & ~1u) | (*Cycle & 1);
    }

    KeMemoryBarrier();

    //
    // Write the cycle bit last to hand the TRB to the controller
    //
    Trb[3] = (DBC_TRB_NORMAL << 10) | 0x20 | 0x4 | (*Cycle & 1);

    if (Wrap)
    {
        *Enqueue = 0;
        *Cycle ^= 1;
    }
    else
    {
        (*Enqueue)++;
    }
}

/**
 * @brief Ring the DbC doorbell for the given endpoint target
 *
 * @param Target IN or OUT doorbell target
 * @return VOID
 */
static VOID
DbcRingDoorbell(UINT32 Target)
{
    KeMemoryBarrier();
    DbcWrite32(DBC_REG_DCDB, Target << 8);
}

/**
 * @brief Check whether a physical address points into a transfer ring
 *
 * @param RingOffset Byte offset of the transfer ring in the block
 * @param TrbPa Physical address from a transfer event
 * @return BOOLEAN
 */
static BOOLEAN
DbcIsTrbInRing(UINT32 RingOffset, UINT64 TrbPa)
{
    return TrbPa >= DbcPhysical(RingOffset) && TrbPa < DbcPhysical(RingOffset) + DBC_RING_TRBS * 16;
}

//
// USB send ring
//

/**
 * @brief Free the send ring bytes of the oldest pending OUT TRBs
 * @details The controller completes the TRBs of a ring in order.
 *
 * @param Count Number of TRBs that the controller no longer owns
 * @param FirstLost TRUE if the controller skipped the first TRB after an error
 * @return VOID
 */
static VOID
DbcReleaseOutTrbs(UINT32 Count, BOOLEAN FirstLost)
{
    UINT32 NormalTrbs = DBC_RING_TRBS - 1;
    UINT32 Index;
    UINT32 Bytes;

    if (Count > g_DbcState.OutPending)
    {
        Count = g_DbcState.OutPending;
    }

    if (Count == 0)
    {
        return;
    }

    Index = (g_DbcState.OutEnqueue + NormalTrbs - g_DbcState.OutPending) % NormalTrbs;

    for (UINT32 TrbIndex = 0; TrbIndex < Count; TrbIndex++)
    {
        Bytes                   = g_DbcOutTrbBytes[Index];
        g_DbcOutTrbBytes[Index] = 0;

        if (TrbIndex == 0 && FirstLost && Bytes != 0)
        {
            //
            // The host did not get all bytes of this TRB
            //
            g_DbcState.TxDroppedBytes += Bytes;
            g_DbcState.TxPacketsCut++;
            g_DbcState.TxLoss = TRUE;
            g_DbcState.TxLossCount++;
        }

        if (Bytes > g_DbcState.TxBytesInFlight || Bytes > g_DbcState.TxUsed)
        {
            Bytes = min(g_DbcState.TxBytesInFlight, g_DbcState.TxUsed);
        }

        g_DbcState.TxTail = (g_DbcState.TxTail + Bytes) % DBC_TX_BUFFER_SIZE;
        g_DbcState.TxUsed -= Bytes;
        g_DbcState.TxBytesInFlight -= Bytes;
        g_DbcState.OutPending--;

        Index = (Index + 1) % NormalTrbs;
    }

    g_DbcState.LastOutProgressTsc = __rdtsc();
    g_DbcState.TxNoProgress       = FALSE;

    if (g_DbcState.OutPending == 0)
    {
        g_DbcState.TxStuck = FALSE;
    }
}

/**
 * @brief Limit an OUT TRB to one 4 KB page and the send ring end
 *
 * @param Position Offset within the send ring
 * @param Remaining Number of bytes left to describe
 * @return UINT32 Number of bytes for the next TRB
 */
static UINT32
DbcTxChunkLength(UINT32 Position, UINT32 Remaining)
{
    UINT32 PageRemaining = DBC_TX_TRB_MAX - (UINT32)(DbcPhysical(DBC_OFFSET_TX + Position) & (DBC_TX_TRB_MAX - 1));

    return min(min(Remaining, PageRemaining), DBC_TX_BUFFER_SIZE - Position);
}

/**
 * @brief Count the TRBs that the uncommitted bytes need
 * @details Reserve staged bytes separately because a poll can queue them alone.
 *
 * @return VOID
 */
static VOID
DbcTxCountUncommittedTrbs()
{
    UINT32 Position  = (g_DbcState.TxHead + DBC_TX_BUFFER_SIZE - g_DbcState.TxUncommitted) % DBC_TX_BUFFER_SIZE;
    UINT32 Remaining = g_DbcState.TxUncommitted;
    UINT32 Staged    = g_DbcState.TxStaged;
    UINT32 Chunk     = 0;
    UINT32 Trbs      = 0;

    while (Remaining != 0)
    {
        Chunk = DbcTxChunkLength(Position, Remaining);

        //
        // Reserve a separate TRB for a packet after the staged bytes
        //
        if (Staged != 0)
        {
            Chunk = min(Chunk, Staged);
        }

        Trbs++;
        Position = (Position + Chunk) % DBC_TX_BUFFER_SIZE;
        Remaining -= Chunk;
        Staged -= min(Staged, Chunk);
    }

    g_DbcState.TxUncommittedTrbs = Trbs;
    g_DbcState.TxChunkBytes      = Chunk;
}

/**
 * @brief Drop the rest of the current packet
 * @details Bytes of the current packet without a TRB are removed. Queued bytes
 * stay with the controller and complete packets that wait for a TRB stay.
 *
 * @return VOID
 */
static VOID
DbcTxAbortPacket()
{
    UINT32 PacketBytes = g_DbcState.TxUncommitted - g_DbcState.TxStaged;

    g_DbcState.TxHead = (g_DbcState.TxHead + DBC_TX_BUFFER_SIZE - PacketBytes) % DBC_TX_BUFFER_SIZE;
    g_DbcState.TxUsed -= PacketBytes;
    g_DbcState.TxDroppedBytes += PacketBytes;
    g_DbcState.TxUncommitted = g_DbcState.TxStaged;

    if (g_DbcState.TxPacketStarted)
    {
        g_DbcState.TxPacketsCut++;
    }
    else
    {
        g_DbcState.TxPacketsDropped++;
    }

    //
    // Drops before a packet starts do not fail delivery waits unless long waits are enabled
    //
    if (g_DbcState.TxPacketStarted || g_DbcState.LongWait)
    {
        g_DbcState.TxLoss = TRUE;
        g_DbcState.TxLossCount++;
    }

    g_DbcState.TxPacketStarted = FALSE;
    g_DbcState.TxPacketWaitUs  = 0;
    g_DbcState.TxDropping      = TRUE;

    DbcTxCountUncommittedTrbs();
}

/**
 * @brief Reset the send ring after the OUT transfer ring is set up again
 * @details Keeps the bytes of a packet that has no TRB yet. Complete packets
 * that wait for a TRB are dropped, the new host does not expect them.
 *
 * @return VOID
 */
static VOID
DbcTxResetRing()
{
    if (g_DbcState.TxBytesInFlight != 0)
    {
        //
        // Queued bytes are lost with the old ring
        //
        g_DbcState.TxDroppedBytes += g_DbcState.TxBytesInFlight;
        g_DbcState.TxLoss = TRUE;
        g_DbcState.TxLossCount++;
    }

    if (g_DbcState.TxStaged != 0)
    {
        g_DbcState.TxDroppedBytes += g_DbcState.TxStaged;
        g_DbcState.TxPacketsDropped += g_DbcState.TxStagedPackets;
        g_DbcState.TxUncommitted -= g_DbcState.TxStaged;
        g_DbcState.TxStaged        = 0;
        g_DbcState.TxStagedPackets = 0;
        g_DbcState.TxLoss          = TRUE;
        g_DbcState.TxLossCount++;
    }

    if (g_DbcState.TxPacketStarted)
    {
        DbcTxAbortPacket();
    }

    g_DbcState.TxTail          = (g_DbcState.TxHead + DBC_TX_BUFFER_SIZE - g_DbcState.TxUncommitted) % DBC_TX_BUFFER_SIZE;
    g_DbcState.TxUsed          = g_DbcState.TxUncommitted;
    g_DbcState.TxBytesInFlight = 0;
    g_DbcState.TxStuck         = FALSE;
    g_DbcState.TxNoProgress    = FALSE;
    g_DbcState.OutPending      = 0;
    g_DbcState.OutRetainedTrb  = 0;

    RtlZeroMemory(g_DbcOutTrbBytes, sizeof(g_DbcOutTrbBytes));
    DbcTxCountUncommittedTrbs();
}

/**
 * @brief Check whether the next byte needs another reserved OUT TRB
 * @details Reserve staged bytes and the current packet separately.
 *
 * @return BOOLEAN
 */
static BOOLEAN
DbcTxNeedsNewTrb()
{
    return g_DbcState.TxUncommitted == g_DbcState.TxStaged ||
           g_DbcState.TxChunkBytes >= DBC_TX_TRB_MAX ||
           (DbcPhysical(DBC_OFFSET_TX + g_DbcState.TxHead) & (DBC_TX_TRB_MAX - 1)) == 0;
}

/**
 * @brief Check whether the send ring has space for the next byte
 *
 * @return BOOLEAN
 */
static BOOLEAN
DbcTxFits()
{
    if (g_DbcState.TxUsed >= DBC_TX_BUFFER_SIZE)
    {
        return FALSE;
    }

    if (DbcTxNeedsNewTrb() && g_DbcState.OutPending + g_DbcState.TxUncommittedTrbs >= DBC_OUT_MAX_PENDING)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Queue the oldest uncommitted bytes on the OUT ring
 * @details Each TRB stays within one 4 KB page and does not wrap the send ring.
 *
 * @param Bytes Number of bytes to queue, all staged bytes or all uncommitted bytes
 * @return VOID
 */
static VOID
DbcTxCommitBytes(UINT32 Bytes)
{
    UINT32 Position  = (g_DbcState.TxHead + DBC_TX_BUFFER_SIZE - g_DbcState.TxUncommitted) % DBC_TX_BUFFER_SIZE;
    UINT32 Remaining = Bytes;
    UINT32 Chunk;

    if (Bytes == 0 || Bytes > g_DbcState.TxUncommitted)
    {
        return;
    }

    if (g_DbcState.OutPending == 0)
    {
        //
        // Time the progress check from the first queued TRB
        //
        g_DbcState.LastOutProgressTsc = __rdtsc();
    }

    while (Remaining != 0)
    {
        Chunk = DbcTxChunkLength(Position, Remaining);

        g_DbcOutTrbBytes[g_DbcState.OutEnqueue] = Chunk;
        DbcQueueNormalTrb(DBC_OFFSET_OUT, &g_DbcState.OutEnqueue, &g_DbcState.OutCycle, DbcPhysical(DBC_OFFSET_TX + Position), Chunk);
        g_DbcState.OutPending++;
        g_DbcState.TxTrbsQueued++;
        if (Chunk < Remaining)
        {
            g_DbcState.TxPageSplitTrbs++;
        }

        Position = (Position + Chunk) % DBC_TX_BUFFER_SIZE;
        Remaining -= Chunk;
    }

    g_DbcState.TxBytesInFlight += Bytes;
    g_DbcState.TxBytes += Bytes;
    g_DbcState.TxUncommitted -= Bytes;

    if (Bytes >= g_DbcState.TxStaged)
    {
        g_DbcState.TxStaged        = 0;
        g_DbcState.TxStagedPackets = 0;
    }
    else
    {
        g_DbcState.TxStaged -= Bytes;
    }

    DbcTxCountUncommittedTrbs();

    //
    // Ring the doorbell after the halt clears (xHCI 1.2 section 4.10.2.1)
    //
    if (g_DbcState.Configured && !g_DbcState.OutHalted)
    {
        DbcRingDoorbell(DBC_DOORBELL_OUT);
    }
}

/**
 * @brief Queue every uncommitted byte, which starts the current packet
 *
 * @return VOID
 */
static VOID
DbcTxCommitAll()
{
    BOOLEAN HasPacketBytes = g_DbcState.TxUncommitted != g_DbcState.TxStaged;

    DbcTxCommitBytes(g_DbcState.TxUncommitted);

    if (HasPacketBytes)
    {
        g_DbcState.TxPacketStarted = TRUE;
    }
}

/**
 * @brief Queue the complete packets that wait for a TRB
 * @details Packets wait only while OUT TRBs are pending, so that one TRB
 * carries many packets. Call only after a poll found DbC configured.
 *
 * @param Force TRUE to queue them now
 * @return VOID
 */
static VOID
DbcTxFlushStaged(BOOLEAN Force)
{
    Force = Force || g_DbcState.LongWait;

    if (g_DbcState.TxStaged == 0)
    {
        return;
    }

    if (!Force && g_DbcState.OutPending != 0 && g_DbcState.OutPending >= g_DbcState.TxCoalescePending &&
        g_DbcState.TxStaged < g_DbcState.TxCoalesceBytes)
    {
        return;
    }

    DbcTxCommitBytes(g_DbcState.TxStaged);
}

/**
 * @brief Write one USB string descriptor into the contiguous block
 *
 * @param Offset Byte offset inside the block
 * @param String Null-terminated UTF-16 string
 * @return UCHAR Total byte length of the descriptor
 */
static UCHAR
DbcWriteStringDescriptor(UINT32 Offset, const WCHAR * String)
{
    volatile UCHAR * Descriptor = DbcBlock8(Offset);
    UINT32           Length     = 0;

    while (String[Length] != 0)
    {
        Descriptor[2 + Length * 2] = (UCHAR)(String[Length] & 0xFF);
        Descriptor[3 + Length * 2] = (UCHAR)(String[Length] >> 8);
        Length++;
    }

    Descriptor[0] = (UCHAR)(2 + Length * 2);
    Descriptor[1] = 3;

    return Descriptor[0];
}

/**
 * @brief Initialize DbC for setup or after a controller reset
 * @details Reuses the allocated memory after a reset in vmx-root.
 *
 * @return VOID
 */
static VOID
DbcProgram()
{
    volatile UINT32 * Context = DbcBlock32(DBC_OFFSET_CONTEXT);
    volatile UINT32 * Erst    = DbcBlock32(DBC_OFFSET_ERST);
    UINT32            Length0;
    UINT32            Length1;
    UINT32            Length2;
    UINT32            Length3;
    UINT32            Control;

    //
    // Clear software ring state first so any early exit leaves no stale active state
    //
    DbcTxResetRing();

    g_DbcState.OutEnqueue            = 0;
    g_DbcState.OutCycle              = 1;
    g_DbcState.InEnqueue             = 0;
    g_DbcState.InCycle               = 1;
    g_DbcState.EventDequeue          = 0;
    g_DbcState.EventCycle            = 1;
    g_DbcState.InPosted              = FALSE;
    g_DbcState.OutHalted             = FALSE;
    g_DbcState.InHalted              = FALSE;
    g_DbcState.Configured            = FALSE;
    g_DbcState.RxLength              = 0;
    g_DbcState.RxPosition            = 0;
    g_DbcState.WatchdogStartTsc      = 0;
    g_DbcState.EventDequeuePublished = 0;

    //
    // Validate controller readiness and DCCTRL before clearing contexts and rings
    //
    if (!DbcControllerResetIdle() || !DbcControllerReady())
    {
        DbcRecordUnavailableSend();
        return;
    }

    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == DBC_INVALID_REGISTER_VALUE)
    {
        return;
    }

    DbcZeroBlock(DBC_OFFSET_STRINGS, 0x100);
    Length0 = DbcWriteStringDescriptor(DBC_OFFSET_STRINGS + DBC_STRING_LANGID, g_DbcStringLangId);
    Length1 = DbcWriteStringDescriptor(DBC_OFFSET_STRINGS + DBC_STRING_MANUFACTURER, g_DbcStringManufacturer);
    Length2 = DbcWriteStringDescriptor(DBC_OFFSET_STRINGS + DBC_STRING_PRODUCT, g_DbcStringProduct);
    Length3 = DbcWriteStringDescriptor(DBC_OFFSET_STRINGS + DBC_STRING_SERIAL, g_DbcStringSerial);

    //
    // Set the string addresses and lengths in the DbC info context
    //
    DbcZeroBlock(DBC_OFFSET_CONTEXT, 192);
    Context[0] = (UINT32)DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_LANGID);
    Context[1] = (UINT32)(DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_LANGID) >> 32);
    Context[2] = (UINT32)DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_MANUFACTURER);
    Context[3] = (UINT32)(DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_MANUFACTURER) >> 32);
    Context[4] = (UINT32)DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_PRODUCT);
    Context[5] = (UINT32)(DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_PRODUCT) >> 32);
    Context[6] = (UINT32)DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_SERIAL);
    Context[7] = (UINT32)(DbcPhysical(DBC_OFFSET_STRINGS + DBC_STRING_SERIAL) >> 32);
    Context[8] = Length0 | (Length1 << 8) | (Length2 << 16) | (Length3 << 24);

    DbcInitializeTransferRing(DBC_OFFSET_OUT);
    DbcInitializeTransferRing(DBC_OFFSET_IN);
    DbcInitializeEndpointContexts(Control);

    //
    // Use one segment for the event ring
    //
    DbcZeroBlock(DBC_OFFSET_EVENT, DBC_EVENT_TRBS * 16);
    Erst[0] = (UINT32)DbcPhysical(DBC_OFFSET_EVENT);
    Erst[1] = (UINT32)(DbcPhysical(DBC_OFFSET_EVENT) >> 32);
    Erst[2] = DBC_EVENT_TRBS;
    Erst[3] = 0;

    KeMemoryBarrier();

    //
    // Set the event ring dequeue pointer before the segment table address
    //
    Control = DbcRead32(DBC_REG_DCERSTSZ);
    if (Control == DBC_INVALID_REGISTER_VALUE ||
        !DbcWrite32(DBC_REG_DCERSTSZ, (Control & DBC_DCERSTSZ_RSVDP) | 1u))
    {
        return;
    }

    g_DbcState.EventDequeuePublished = 0;
    if (!DbcWrite64(DBC_REG_DCERDP, DbcPhysical(DBC_OFFSET_EVENT), DBC_DCERDP_RSVDP))
    {
        return;
    }
    g_DbcState.EventDequeuePublished = DbcPhysical(DBC_OFFSET_EVENT);

    if (!DbcWrite64(DBC_REG_DCERSTBA, DbcPhysical(DBC_OFFSET_ERST), DBC_DCERSTBA_RSVDP) ||
        !DbcWrite64(DBC_REG_DCCP, DbcPhysical(DBC_OFFSET_CONTEXT), DBC_DCCP_RSVDP) ||
        !DbcWrite32(DBC_REG_DCDDI1, ((UINT32)DBC_VID << 16) | DBC_PROTOCOL) ||
        !DbcWrite32(DBC_REG_DCDDI2, ((UINT32)DBC_REVISION << 16) | DBC_PID))
    {
        return;
    }
    KeMemoryBarrier();

    //
    // Clear the run change bit so the doorbell works
    //
    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control != DBC_INVALID_REGISTER_VALUE)
    {
        DbcWrite32(DBC_REG_DCCTRL, (Control & DBC_DCCTRL_RSVDP) | DBC_DCCTRL_DRC | DBC_DCCTRL_DCE);
    }
}

/**
 * @brief Reset both transfer rings after the debug host disconnects
 * @details Update the dequeue pointers while both endpoints are disabled.
 *
 * @return VOID
 */
static VOID
DbcResetTransferRings()
{
    volatile UINT32 * Context = DbcBlock32(DBC_OFFSET_CONTEXT);

    DbcInitializeTransferRing(DBC_OFFSET_OUT);
    DbcInitializeTransferRing(DBC_OFFSET_IN);

    Context[16 + 2] = (UINT32)DbcPhysical(DBC_OFFSET_OUT) | 1;
    Context[16 + 3] = (UINT32)(DbcPhysical(DBC_OFFSET_OUT) >> 32);
    Context[32 + 2] = (UINT32)DbcPhysical(DBC_OFFSET_IN) | 1;
    Context[32 + 3] = (UINT32)(DbcPhysical(DBC_OFFSET_IN) >> 32);

    DbcTxResetRing();

    g_DbcState.OutEnqueue = 0;
    g_DbcState.OutCycle   = 1;
    g_DbcState.InEnqueue  = 0;
    g_DbcState.InCycle    = 1;
    g_DbcState.InPosted   = FALSE;
    g_DbcState.OutHalted  = FALSE;
    g_DbcState.InHalted   = FALSE;
    g_DbcState.RingResets++;
    KeMemoryBarrier();
}

/**
 * @brief Match a transfer event to the oldest pending TRB on its ring
 *
 * @param RingOffset Byte offset of the transfer ring in the block
 * @param Enqueue Next Normal TRB index
 * @param Pending Number of pending Normal TRBs
 * @param TrbPa Physical address from the transfer event
 * @return BOOLEAN TRUE when the event matches the oldest pending TRB
 */
static BOOLEAN
DbcIsPendingTrb(UINT32 RingOffset, UINT32 Enqueue, UINT32 Pending, UINT64 TrbPa)
{
    UINT32 NormalTrbs = DBC_RING_TRBS - 1;
    UINT32 Index      = (Enqueue + NormalTrbs - Pending) % NormalTrbs;

    return Pending != 0 && TrbPa == DbcPhysical(RingOffset + Index * 16);
}

/**
 * @brief Retire a retained OUT transfer after hardware moves past it
 *
 * @param TrbPa Physical address from a completion that refers to a transfer
 * @return VOID
 */
static VOID
DbcRetireOutTransfersBefore(UINT64 TrbPa)
{
    UINT64 RingPa     = DbcPhysical(DBC_OFFSET_OUT);
    UINT64 Offset     = TrbPa - RingPa;
    UINT32 NormalTrbs = DBC_RING_TRBS - 1;
    UINT32 Retained;
    UINT32 Distance;

    if (g_DbcState.OutRetainedTrb == 0 || (Offset & 0xF) != 0 || Offset >= NormalTrbs * 16)
    {
        return;
    }

    Retained = (UINT32)((g_DbcState.OutRetainedTrb - RingPa) / 16);
    Distance = ((UINT32)(Offset / 16) + NormalTrbs - Retained) % NormalTrbs;

    if (Distance != 0 && Distance < g_DbcState.OutPending)
    {
        //
        // The controller processes each ring in order
        //
        DbcReleaseOutTrbs(Distance, TRUE);
        g_DbcState.OutRetainedTrb = 0;
    }
}

/**
 * @brief Check whether software can reuse a failed transfer buffer
 *
 * @param RingOffset Byte offset of the transfer ring in the block
 * @param TrbPa Physical address of the pending TRB
 * @param Residual Number of bytes left in the transfer
 * @param Stall TRUE if the event reports a Stall Error
 * @param Halted Records a halt for doorbell recovery
 * @return BOOLEAN TRUE when hardware no longer needs the transfer buffer
 */
static BOOLEAN
DbcFinishFailedTrb(UINT32 RingOffset, UINT64 TrbPa, UINT32 Residual, BOOLEAN Stall, BOOLEAN * Halted)
{
    UINT32            Control;
    BOOLEAN           Out     = (RingOffset == DBC_OFFSET_OUT);
    volatile UINT32 * Context;
    volatile UINT32 * Trb;
    UINT32            EpState;
    UINT32            Cycle;
    UINT64            Dequeue;
    BOOLEAN           RingStopped;

    //
    // A USB Transaction Error ends the transfer on disconnect
    //
    if (!g_DbcState.Configured)
    {
        return !Stall;
    }

    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE))
    {
        return FALSE;
    }

    if (!(Control & DBC_DCCTRL_DCR) || (Control & DBC_DCCTRL_DRC))
    {
        return !Stall;
    }

    Context     = DbcBlock32(DBC_OFFSET_CONTEXT + (Out ? 64 : 128));
    EpState     = Context[0] & DBC_EP_STATE_MASK;
    RingStopped = (Control & (Out ? DBC_DCCTRL_HOT : DBC_DCCTRL_HIT)) != 0 ||
                  EpState == DBC_EP_STATE_HALTED || EpState == DBC_EP_STATE_STOPPED;

    if (!RingStopped && !Stall)
    {
        return TRUE;
    }

    //
    // The next doorbell resumes a halted ring at this dequeue pointer
    //
    Dequeue = (Context[2] | ((UINT64)Context[3] << 32)) & ~0xFULL;

    if (!RingStopped)
    {
        //
        // A stall alone does not release the pending TRB
        //
        return FALSE;
    }

    *Halted = TRUE;
    if (Dequeue != TrbPa)
    {
        return TRUE;
    }

    Trb = DbcBlock32((UINT32)(TrbPa - g_DbcState.BlockPa));
    if (Residual >= (Trb[2] & DBC_TRB_LENGTH_MASK))
    {
        return FALSE;
    }

    //
    // A reset can reclaim the ring while software reads the dequeue pointer
    //
    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE))
    {
        return FALSE;
    }

    if (!(Control & DBC_DCCTRL_DCR) || (Control & DBC_DCCTRL_DRC))
    {
        return !Stall;
    }

    EpState     = Context[0] & DBC_EP_STATE_MASK;
    RingStopped = (Control & (Out ? DBC_DCCTRL_HOT : DBC_DCCTRL_HIT)) != 0 ||
                  EpState == DBC_EP_STATE_HALTED || EpState == DBC_EP_STATE_STOPPED;
    if (!RingStopped)
    {
        return FALSE;
    }

    //
    // Keep the cycle bit and skip the transfer on restart
    //
    Cycle  = Trb[3] & 1u;
    Trb[0] = 0;
    Trb[1] = 0;
    Trb[2] = 0;
    KeMemoryBarrier();
    Trb[3] = (DBC_TRB_NO_OP << 10) | Cycle;

    return TRUE;
}

/**
 * @brief Publish an evaluated event address without consecutive duplicate writes
 * @details xHCI 1.2 section 4.9.4 defines the evaluated address rule.
 * Section 7.6.8.3.3 defines DESI and the reserved bit.
 *
 * @param Address Physical address of the last evaluated event
 * @return VOID
 */
static VOID
DbcPublishEventDequeue(UINT64 Address)
{
    if (Address != g_DbcState.EventDequeuePublished)
    {
        if (DbcWrite64(DBC_REG_DCERDP, Address, DBC_DCERDP_RSVDP))
        {
            g_DbcState.EventDequeuePublished = Address;
        }
        else
        {
            g_DbcState.EventDequeuePublished = 0;
        }
    }
}

/**
 * @brief Drain pending DbC events in vmx-root
 * @details Keep received bytes in the buffer until the caller reads them.
 *
 * @return BOOLEAN TRUE if an event indicates a controller state change or the poll limit is reached
 */
static BOOLEAN
DbcProcessEvents()
{
    UINT32  Processed     = 0;
    UINT32  Unflushed     = 0;
    UINT64  LastEvaluated = 0;
    BOOLEAN StateChanged  = FALSE;

    while (Processed < DBC_MAX_EVENTS_PER_POLL)
    {
        volatile UINT32 * Event = DbcBlock32(DBC_OFFSET_EVENT + g_DbcState.EventDequeue * 16);
        UINT32            Dw3   = Event[3];
        UINT32            Type;
        UINT32            CompletionCode;
        UINT32            Residual;
        UINT64            TrbPa;
        BOOLEAN           InSuccess;
        BOOLEAN           OutSuccess;
        BOOLEAN           Success;
        BOOLEAN           Stall;
        BOOLEAN           InRing;
        BOOLEAN           OutRing;

        LastEvaluated = DbcPhysical(DBC_OFFSET_EVENT) + (UINT64)g_DbcState.EventDequeue * 16;

        if ((Dw3 & 1) != g_DbcState.EventCycle)
        {
            break;
        }

        KeMemoryBarrier();

        Type           = (Dw3 >> 10) & 0x3F;
        TrbPa          = Event[0] | ((UINT64)Event[1] << 32);
        CompletionCode = Event[2] >> 24;
        Residual       = Event[2] & 0xFFFFFF;

        if (CompletionCode == DBC_CC_EVENT_RING_FULL &&
            (Type == DBC_TRB_HOST_CONTROLLER_EVENT || Type == DBC_TRB_TRANSFER_EVENT))
        {
            g_DbcState.EventRingFullEvents++;
        }

        if (Type != DBC_TRB_TRANSFER_EVENT)
        {
            StateChanged = TRUE;
        }
        else
        {
            InRing     = DbcIsTrbInRing(DBC_OFFSET_IN, TrbPa);
            OutRing    = DbcIsTrbInRing(DBC_OFFSET_OUT, TrbPa);
            InSuccess  = (CompletionCode == DBC_CC_SUCCESS || CompletionCode == DBC_CC_SHORT_PACKET);
            OutSuccess = (CompletionCode == DBC_CC_SUCCESS && Residual == 0);
            Success    = OutRing ? OutSuccess : InSuccess;
            Stall      = (CompletionCode == DBC_CC_STALL_ERROR);

            if (!InRing && !OutRing)
            {
                g_DbcState.UnknownTrbEvents++;
            }

            if (!Success)
            {
                StateChanged = TRUE;
                g_DbcState.ErrorEvents++;
                g_DbcState.LastErrorCode     = CompletionCode;
                g_DbcState.LastErrorTrb      = TrbPa;
                g_DbcState.LastErrorResidual = Residual;

                if (Stall && InRing)
                {
                    g_DbcState.InStallEvents++;
                }
                else if (Stall && OutRing)
                {
                    g_DbcState.OutStallEvents++;
                }
                else
                {
                    g_DbcState.OtherErrorEvents++;
                }
            }

            if (InRing)
            {
                if (Stall)
                {
                    g_DbcState.InHalted = TRUE;
                }

                if (DbcIsPendingTrb(DBC_OFFSET_IN, g_DbcState.InEnqueue, g_DbcState.InPosted ? 1 : 0, TrbPa) &&
                    (Success || DbcFinishFailedTrb(DBC_OFFSET_IN, TrbPa, Residual, Stall, &g_DbcState.InHalted)))
                {
                    if ((Success || Stall) && Residual < DBC_MAX_PACKET)
                    {
                        g_DbcState.RxLength   = DBC_MAX_PACKET - Residual;
                        g_DbcState.RxPosition = 0;
                        g_DbcState.RxBytes += g_DbcState.RxLength;
                    }

                    g_DbcState.InPosted = FALSE;
                }
            }
            else if (OutRing)
            {
                if (Stall)
                {
                    g_DbcState.OutHalted = TRUE;
                }

                //
                // A stall alone does not show that hardware advanced
                //
                if ((Dw3 & 0x4) == 0 &&
                    (Success || CompletionCode == DBC_CC_BABBLE_ERROR ||
                     CompletionCode == DBC_CC_USB_TRANS_ERROR || CompletionCode == DBC_CC_TRB_ERROR))
                {
                    DbcRetireOutTransfersBefore(TrbPa);
                }

                if (DbcIsPendingTrb(DBC_OFFSET_OUT, g_DbcState.OutEnqueue, g_DbcState.OutPending, TrbPa))
                {
                    if (Success || DbcFinishFailedTrb(DBC_OFFSET_OUT, TrbPa, Residual, Stall, &g_DbcState.OutHalted))
                    {
                        DbcReleaseOutTrbs(1, !Success);
                        g_DbcState.OutRetainedTrb = 0;
                    }
                    else
                    {
                        g_DbcState.OutRetainedTrb = TrbPa;
                    }
                }
            }
        }

        g_DbcState.EventsProcessed++;
        g_DbcState.EventDequeue++;

        if (g_DbcState.EventDequeue == DBC_EVENT_TRBS)
        {
            g_DbcState.EventDequeue = 0;
            g_DbcState.EventCycle ^= 1;
        }

        Processed++;
        Unflushed++;

        if (Unflushed == DBC_EVENT_TRBS / 2)
        {
            //
            // Publish the event just evaluated, not the unevaluated next entry
            //
            DbcPublishEventDequeue(LastEvaluated);
            Unflushed = 0;
        }
    }

    DbcPublishEventDequeue(LastEvaluated);

    if (Processed == DBC_MAX_EVENTS_PER_POLL)
    {
        g_DbcState.EventPollCapHits++;
        StateChanged = TRUE;
    }

    return StateChanged;
}

/**
 * @brief Queue an IN transfer when the receive buffer is empty
 *
 * @return VOID
 */
static VOID
DbcPostInTransfer()
{
    if (g_DbcState.InPosted || !g_DbcState.Configured || g_DbcState.InHalted || g_DbcState.RxPosition < g_DbcState.RxLength)
    {
        return;
    }

    g_DbcState.RxLength   = 0;
    g_DbcState.RxPosition = 0;

    DbcQueueNormalTrb(DBC_OFFSET_IN, &g_DbcState.InEnqueue, &g_DbcState.InCycle, DbcPhysical(DBC_OFFSET_RX), DBC_MAX_PACKET);
    g_DbcState.InPosted = TRUE;

    DbcRingDoorbell(DBC_DOORBELL_IN);
}

/**
 * @brief Turn DbC off if the host connected but did not configure it in time
 * @details The next poll sets DbC up again, so the host sees a new device.
 *
 * @param PortSc DbC port status read in this poll
 * @return VOID
 */
static VOID
DbcCheckConfigurationTimeout(UINT32 PortSc)
{
    UINT64 Now;
    UINT32 Control;

    if (PortSc == 0xFFFFFFFF || !(PortSc & DBC_DCPORTSC_CCS) ||
        g_DbcState.TscPerMs == 0 || g_DbcState.TscPerMs > MAXUINT64 / DBC_WATCHDOG_MAXIMUM_MS)
    {
        //
        // Time the configuration from the host connection
        // See xHCI 1.2 section 7.6.4.1
        //
        g_DbcState.WatchdogStartTsc = 0;
        return;
    }

    Now = __rdtsc();

    if (g_DbcState.WatchdogTimeoutMs == 0)
    {
        g_DbcState.WatchdogTimeoutMs = DBC_WATCHDOG_TIMEOUT_MS;
    }

    //
    // Restart the timer if this core reads an earlier timestamp
    //
    if (g_DbcState.WatchdogStartTsc == 0 || (INT64)(Now - g_DbcState.WatchdogStartTsc) < 0)
    {
        g_DbcState.WatchdogStartTsc = Now;
        return;
    }

    if (Now - g_DbcState.WatchdogStartTsc <= g_DbcState.TscPerMs * g_DbcState.WatchdogTimeoutMs)
    {
        return;
    }

    g_DbcState.WatchdogStartTsc = 0;

    //
    // Do not touch DbC if the host configured it meanwhile
    //
    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE) || (Control & DBC_DCCTRL_DCR))
    {
        return;
    }

    //
    // Wait longer after each reset in case the host never configures DbC
    //
    if (g_DbcState.WatchdogTimeoutMs < DBC_WATCHDOG_MAXIMUM_MS)
    {
        g_DbcState.WatchdogTimeoutMs = min(g_DbcState.WatchdogTimeoutMs * 2, DBC_WATCHDOG_MAXIMUM_MS);
    }

    g_DbcState.ConfigurationTimeoutResets++;
    DbcWrite32(DBC_REG_DCCTRL, Control & DBC_DCCTRL_RSVDP);
}

/**
 * @brief Poll DbC and recover from resets or halts in vmx-root
 *
 * @param DrainEvents TRUE to drain pending events during this poll
 * @return BOOLEAN TRUE when the debug host configured the device
 */
static BOOLEAN
DbcCheckStateInternal(BOOLEAN DrainEvents)
{
    volatile UINT32 * Context;
    UINT32            Control = DbcRead32(DBC_REG_DCCTRL);
    UINT32            PortSc;
    BOOLEAN           WasConfigured;
    BOOLEAN           Drained = !DrainEvents;

    if (Control == 0xFFFFFFFF)
    {
        //
        // The controller registers are inaccessible
        //
        return FALSE;
    }

    if (!(Control & DBC_DCCTRL_DCE))
    {
        //
        // Reuse the memory block after a controller reset
        //
        g_DbcState.Reprograms++;
        DbcProgram();
        return FALSE;
    }

    PortSc = DbcRead32(DBC_REG_DCPORTSC);
    if (PortSc != 0xFFFFFFFF && !(PortSc & DBC_DCPORTSC_PR))
    {
        if ((PortSc & (DBC_DCPORTSC_CCS | DBC_DCPORTSC_PED)) == DBC_DCPORTSC_CCS)
        {
            //
            // Enable the DbC port and clear its change bits
            //
            DbcWrite32(DBC_REG_DCPORTSC, DBC_DCPORTSC_PED | (PortSc & DBC_DCPORTSC_CHANGES));
        }
        else if (PortSc & DBC_DCPORTSC_CHANGES)
        {
            //
            // Clear change bits and preserve the port enable bit
            //
            DbcWrite32(DBC_REG_DCPORTSC, (PortSc & DBC_DCPORTSC_PED) | (PortSc & DBC_DCPORTSC_CHANGES));
        }
    }

    if (Control & DBC_DCCTRL_DRC)
    {
        g_DbcState.Configured = FALSE;

        if (!Drained)
        {
            DbcProcessEvents();
            Drained = TRUE;
        }
        g_DbcState.TxStuck   = FALSE;
        g_DbcState.OutHalted = FALSE;
        g_DbcState.InHalted  = FALSE;

        Context = DbcBlock32(DBC_OFFSET_CONTEXT);

        if ((Context[16 + 0] & DBC_EP_STATE_MASK) == DBC_EP_STATE_DISABLED &&
            (Context[32 + 0] & DBC_EP_STATE_MASK) == DBC_EP_STATE_DISABLED)
        {
            Control = DbcRead32(DBC_REG_DCCTRL);
            if (Control != 0xFFFFFFFF &&
                (Control & (DBC_DCCTRL_DCE | DBC_DCCTRL_DCR | DBC_DCCTRL_DRC)) == (DBC_DCCTRL_DCE | DBC_DCCTRL_DRC))
            {
                //
                // Reset the rings before the host configures DbC again
                //
                DbcResetTransferRings();
            }
        }

        //
        // Clear the run change bit to reenable the doorbell
        //
        Control = DbcRead32(DBC_REG_DCCTRL);
        if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE))
        {
            return FALSE;
        }

        DbcWrite32(DBC_REG_DCCTRL, (Control & DBC_DCCTRL_RSVDP) | DBC_DCCTRL_DCE | DBC_DCCTRL_DRC);
        Control = DbcRead32(DBC_REG_DCCTRL);
    }

    if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE) ||
        !(Control & DBC_DCCTRL_DCR) || (Control & DBC_DCCTRL_DRC))
    {
        g_DbcState.Configured = FALSE;
        DbcCheckConfigurationTimeout(PortSc);
        return FALSE;
    }

    WasConfigured                = g_DbcState.Configured;
    g_DbcState.Configured        = TRUE;
    g_DbcState.WatchdogStartTsc  = 0;
    g_DbcState.WatchdogTimeoutMs = DBC_WATCHDOG_TIMEOUT_MS;

    if (!Drained)
    {
        DbcProcessEvents();
    }

    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == 0xFFFFFFFF || !(Control & DBC_DCCTRL_DCE) ||
        !(Control & DBC_DCCTRL_DCR) || (Control & DBC_DCCTRL_DRC))
    {
        g_DbcState.Configured = FALSE;
        return FALSE;
    }

    if (Control & DBC_DCCTRL_HOT)
    {
        g_DbcState.OutHalted = TRUE;
    }
    else if (g_DbcState.OutHalted)
    {
        //
        // Resume OUT transfers after the host clears the halt
        //
        g_DbcState.OutHalted = FALSE;
        DbcRingDoorbell(DBC_DOORBELL_OUT);
    }
    else if (!WasConfigured && g_DbcState.OutPending != 0)
    {
        //
        // Resume queued OUT transfers after configuration
        //
        DbcRingDoorbell(DBC_DOORBELL_OUT);
    }

    if (Control & DBC_DCCTRL_HIT)
    {
        g_DbcState.InHalted = TRUE;
    }
    else if (g_DbcState.InHalted)
    {
        //
        // Resume IN transfers after the host clears the halt
        //
        g_DbcState.InHalted = FALSE;

        if (!g_DbcState.InPosted && g_DbcState.RxPosition >= g_DbcState.RxLength && g_DbcState.Ready)
        {
            DbcPostInTransfer();
        }
        else
        {
            DbcRingDoorbell(DBC_DOORBELL_IN);
        }
    }
    else if (!WasConfigured && !g_DbcState.InPosted && g_DbcState.RxPosition >= g_DbcState.RxLength && g_DbcState.Ready)
    {
        //
        // Resume IN transfers after configuration
        //
        DbcPostInTransfer();
    }
    else if (g_DbcState.InPosted)
    {
        //
        // Ring the IN doorbell on slow-path polls while an IN transfer is pending
        //
        DbcRingDoorbell(DBC_DOORBELL_IN);
    }

    //
    // Queue waiting packets once fewer OUT TRBs are pending
    //
    DbcTxFlushStaged(FALSE);

    return TRUE;
}

/**
 * @brief Poll DbC and recover from resets or halts in vmx-root
 *
 * @return BOOLEAN TRUE when the debug host configured the device
 */
static BOOLEAN
DbcCheckState()
{
    return DbcCheckStateInternal(TRUE);
}

/**
 * @brief Poll configured DbC after checking controller readiness
 * @details Falls back to DbcCheckStateInternal in every other state.
 *
 * @param DrainEvents TRUE to drain pending events during this poll
 * @param StateChanged TRUE if an earlier drain in this call reported a state change
 * @return BOOLEAN TRUE when the debug host configured the device
 */
static BOOLEAN
DbcCheckStateFastInternal(BOOLEAN DrainEvents, BOOLEAN StateChanged)
{
    UINT32 Control;

    if (g_DbcState.Configured && !g_DbcState.OutHalted && !g_DbcState.InHalted)
    {
        Control = DbcRead32(DBC_REG_DCCTRL);

        if (Control != DBC_INVALID_REGISTER_VALUE &&
            (Control & (DBC_DCCTRL_DCE | DBC_DCCTRL_DCR | DBC_DCCTRL_DRC | DBC_DCCTRL_HOT | DBC_DCCTRL_HIT)) ==
                (DBC_DCCTRL_DCE | DBC_DCCTRL_DCR))
        {
            if (DrainEvents)
            {
                StateChanged = DbcProcessEvents();
            }

            if (StateChanged)
            {
                Control = DbcRead32(DBC_REG_DCCTRL);
                if (Control == DBC_INVALID_REGISTER_VALUE ||
                    (Control & (DBC_DCCTRL_DCE | DBC_DCCTRL_DCR | DBC_DCCTRL_DRC | DBC_DCCTRL_HOT | DBC_DCCTRL_HIT)) !=
                        (DBC_DCCTRL_DCE | DBC_DCCTRL_DCR))
                {
                    return DbcCheckStateInternal(FALSE);
                }
            }

            if (!g_DbcState.OutHalted && !g_DbcState.InHalted)
            {
                DbcTxFlushStaged(FALSE);
                return TRUE;
            }

            return DbcCheckStateInternal(FALSE);
        }
    }

    return DbcCheckStateInternal(DrainEvents);
}

/**
 * @brief Poll configured DbC after checking controller readiness
 * @details Falls back to DbcCheckStateInternal in every other state.
 *
 * @return BOOLEAN TRUE when the debug host configured the device
 */
static BOOLEAN
DbcCheckStateFast()
{
    return DbcCheckStateFastInternal(TRUE, FALSE);
}

/**
 * @brief Check at the start of a packet whether the host still takes data
 * @details Keeps a handshake ping queued until the host reads it, and drops later packets.
 *
 * @return BOOLEAN FALSE when the packet is to be dropped
 */
static BOOLEAN
DbcTxCheckProgress()
{
    //
    // Long-wait packets ignore stuck state and get a full second for space
    //
    if (g_DbcState.LongWait)
    {
        return TRUE;
    }

    if (g_DbcState.OutPending != 0 && !g_DbcState.TxStuck &&
        DbcElapsedUs(g_DbcState.LastOutProgressTsc) > DBC_SEND_TIMEOUT_MS * DBC_US_PER_MS)
    {
        DbcCheckStateFast();

        if (g_DbcState.OutPending != 0 && DbcElapsedUs(g_DbcState.LastOutProgressTsc) > DBC_SEND_TIMEOUT_MS * DBC_US_PER_MS)
        {
            //
            // The host took no queued data for the long wait
            //
            g_DbcState.TxStuck = TRUE;
            g_DbcState.TxTimeouts++;
        }
    }

    if (g_DbcState.TxStuck)
    {
        if (g_DbcState.OutPending != 0)
        {
            DbcCheckStateFast();
        }

        if (g_DbcState.OutPending != 0)
        {
            return FALSE;
        }

        g_DbcState.TxStuck = FALSE;
    }

    return TRUE;
}

/**
 * @brief Wait until the next byte fits in the send ring
 * @details The wait is short while the debuggee runs and the packet has no queued bytes.
 * The long wait applies when the debuggee is halted or the packet has queued bytes.
 *
 * @return BOOLEAN FALSE when the packet was dropped or cut
 */
static BOOLEAN
DbcTxWaitForSpace()
{
    UINT64  StartTsc    = __rdtsc();
    UINT64  ProgressTsc = g_DbcState.LastOutProgressTsc;
    UINT64  Spins       = 0;
    UINT64  WaitUs;
    UINT32  LimitUs;
    BOOLEAN LongLimit = FALSE;
    BOOLEAN Fits      = FALSE;
    BOOLEAN TimedOut  = FALSE;

    g_DbcState.TxRingFullWaits++;

    for (;;)
    {
        //
        // Stop if the host is gone or a ring reset cut the packet
        //
        if (!DbcCheckStateFast() || g_DbcState.TxDropping)
        {
            break;
        }

        if (DbcTxFits())
        {
            Fits = TRUE;
            break;
        }

        if (g_DbcState.TxStaged != 0)
        {
            //
            // Give the complete packets to the host so that it frees space
            //
            DbcTxFlushStaged(TRUE);
            continue;
        }

        if (g_DbcState.TxUncommitted != 0 && (g_DbcState.TxPacketStarted || g_DbcState.OutPending == 0))
        {
            //
            // Stream a started packet or a packet that fills the ring alone
            //
            DbcTxCommitAll();
            continue;
        }

        LongLimit = g_DbcState.LongWait || g_DbcState.TxPacketStarted;
        LimitUs   = LongLimit ? DBC_SEND_TIMEOUT_MS * DBC_US_PER_MS : g_DbcState.TxRunningWaitUs - min(g_DbcState.TxPacketWaitUs, g_DbcState.TxRunningWaitUs);

        //
        // Skip the short wait after one ended with no progress
        //
        if ((!LongLimit && g_DbcState.TxNoProgress) || DbcTimeoutExpiredUs(StartTsc, LimitUs, &Spins))
        {
            TimedOut = TRUE;
            break;
        }

        _mm_pause();
    }

    //
    // Limit the total short wait of one packet to the running budget
    //
    WaitUs                    = DbcElapsedUs(StartTsc);
    g_DbcState.TxPacketWaitUs = (UINT32)min(g_DbcState.TxPacketWaitUs + WaitUs, MAXUINT32);

    if (WaitUs > g_DbcState.TxLongestWaitUs)
    {
        g_DbcState.TxLongestWaitUs = (UINT32)min(WaitUs, MAXUINT32);
    }

    if (Fits)
    {
        return TRUE;
    }

    if (g_DbcState.TxDropping)
    {
        //
        // The ring reset already counted the cut packet
        //
        return FALSE;
    }

    if (TimedOut)
    {
        if (LongLimit)
        {
            g_DbcState.TxStuck = TRUE;
            g_DbcState.TxTimeouts++;
        }
        else if (g_DbcState.LastOutProgressTsc == ProgressTsc)
        {
            g_DbcState.TxNoProgress = TRUE;
        }
    }

    DbcTxAbortPacket();
    return FALSE;
}

/**
 * @brief Write one byte of the current packet into the send ring
 *
 * @param Byte Byte to send
 * @return VOID
 */
static VOID
DbcTxWriteByte(UCHAR Byte)
{
    if (DbcTxNeedsNewTrb())
    {
        g_DbcState.TxUncommittedTrbs++;
        g_DbcState.TxChunkBytes = 0;
    }

    *DbcBlock8(DBC_OFFSET_TX + g_DbcState.TxHead) = Byte;

    g_DbcState.TxHead = (g_DbcState.TxHead + 1) % DBC_TX_BUFFER_SIZE;
    g_DbcState.TxUsed++;
    g_DbcState.TxUncommitted++;
    g_DbcState.TxChunkBytes++;
}

/**
 * @brief Keep a finished packet until its bytes are queued with later packets
 * @details Queue at once for long waits or when few OUT TRBs are pending.
 * Otherwise queue by a later packet or poll without waiting for the host.
 *
 * @return VOID
 */
static VOID
DbcTxEndPacket()
{
    BOOLEAN Checked      = FALSE;
    BOOLEAN Drained      = FALSE;
    BOOLEAN StateChanged = FALSE;

    if (!g_DbcState.TxDropping && (g_DbcState.LongWait || !g_DbcState.Configured))
    {
        Checked = DbcCheckStateFast();
        if (!Checked && !g_DbcState.TxDropping)
        {
            //
            // The host is gone, drop the whole packet
            //
            DbcTxAbortPacket();
        }
    }

    if (g_DbcState.TxDropping)
    {
        //
        // The end marker also ends the drop
        //
        g_DbcState.TxDropping = FALSE;
        return;
    }

    g_DbcState.TxStaged = g_DbcState.TxUncommitted;
    g_DbcState.TxStagedPackets++;
    g_DbcState.TxPacketStarted = FALSE;
    g_DbcState.TxPacketWaitUs  = 0;
    g_DbcState.TxPacketsSent++;

    DbcTxCountUncommittedTrbs();

    //
    // Read completions from memory and read a register only to queue packets or check a state change
    //
    if (!Checked && g_DbcState.OutPending != 0)
    {
        StateChanged = DbcProcessEvents();
        Drained      = TRUE;
    }

    if (StateChanged || g_DbcState.LongWait || g_DbcState.OutPending == 0 ||
        g_DbcState.OutPending < g_DbcState.TxCoalescePending ||
        g_DbcState.TxStaged >= g_DbcState.TxCoalesceBytes)
    {
        if (Checked)
        {
            DbcTxFlushStaged(FALSE);
        }
        else
        {
            DbcCheckStateFastInternal(!Drained, StateChanged);
        }
    }
}

/**
 * @brief Queue the bytes of a large packet before its end marker
 * @details The first queued part starts the packet.
 *
 * @return VOID
 */
static VOID
DbcTxStreamPacket()
{
    if (!DbcCheckStateFast() || g_DbcState.TxDropping)
    {
        if (!g_DbcState.TxDropping)
        {
            DbcTxAbortPacket();
        }

        return;
    }

    DbcTxCommitAll();
}

/**
 * @brief Add a byte to the send ring and finish the packet at its end marker
 * @details A packet is either fully queued or fully dropped. Only a hard
 * failure after part of the packet was queued cuts it.
 *
 * @param Byte Byte to send
 * @return VOID
 */
VOID
DbcSendByte(UCHAR Byte)
{
    BOOLEAN EndOfPacket;

    if (!g_DbcState.Ready)
    {
        return;
    }

    g_DbcState.TxWindow = (g_DbcState.TxWindow << 8) | Byte;
    if (g_DbcState.TxWindowBytes < 4)
    {
        g_DbcState.TxWindowBytes++;
    }

    EndOfPacket = (g_DbcState.TxWindowBytes == 4 && g_DbcState.TxWindow == DBC_END_OF_BUFFER_MARKER);
    if (EndOfPacket)
    {
        g_DbcState.TxWindow      = 0;
        g_DbcState.TxWindowBytes = 0;
    }

    if (!g_DbcState.TxDropping)
    {
        if (!g_DbcState.TxPacketStarted && g_DbcState.TxUncommitted == g_DbcState.TxStaged && !DbcTxCheckProgress())
        {
            //
            // Drop the whole packet while the host takes no data
            //
            DbcTxAbortPacket();
        }
        else if (DbcTxFits() || DbcTxWaitForSpace())
        {
            DbcTxWriteByte(Byte);

            if (EndOfPacket)
            {
                DbcTxEndPacket();
            }
            else if (g_DbcState.TxUncommitted - g_DbcState.TxStaged >= DBC_TX_STREAM_SIZE)
            {
                DbcTxStreamPacket();
            }

            return;
        }
    }

    //
    // Drop this byte, the end marker ends the drop
    //
    g_DbcState.TxDroppedBytes++;

    if (EndOfPacket)
    {
        g_DbcState.TxDropping = FALSE;
    }
}

/**
 * @brief Read one byte without waiting
 *
 * @param RecvByte Receives the byte when available
 * @return BOOLEAN
 */
BOOLEAN
DbcRecvByte(PUCHAR RecvByte)
{
    if (!g_DbcState.Ready)
    {
        return FALSE;
    }

    if (g_DbcState.RxPosition >= g_DbcState.RxLength)
    {
        if (!DbcCheckState())
        {
            return FALSE;
        }

        if (g_DbcState.RxPosition >= g_DbcState.RxLength)
        {
            DbcPostInTransfer();
            return FALSE;
        }
    }

    *RecvByte = *DbcBlock8(DBC_OFFSET_RX + g_DbcState.RxPosition);
    g_DbcState.RxPosition++;

    return TRUE;
}

//////////////////////////////////////////////////
//			Prepare at PASSIVE_LEVEL			//
//////////////////////////////////////////////////

/**
 * @brief Delay the current thread at PASSIVE_LEVEL
 *
 * @param Milliseconds Delay in milliseconds
 * @return VOID
 */
static VOID
DbcSleep(UINT32 Milliseconds)
{
    LARGE_INTEGER Interval;

    Interval.QuadPart = -10000LL * Milliseconds;
    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

/**
 * @brief Read a 32-bit PCI configuration register
 *
 * @param Bus PCI bus number
 * @param Device PCI device number
 * @param Function PCI function number
 * @param Offset Register offset in PCI configuration space
 * @return UINT32
 */
static UINT32
DbcPciRead32(UINT32 Bus, UINT32 Device, UINT32 Function, UINT32 Offset)
{
    PCI_SLOT_NUMBER Slot;
    UINT32          Value = 0xFFFFFFFF;

    Slot.u.AsULONG             = 0;
    Slot.u.bits.DeviceNumber   = Device;
    Slot.u.bits.FunctionNumber = Function;

#pragma warning(suppress : 4996)
    HalGetBusDataByOffset(PCIConfiguration, Bus, Slot.u.AsULONG, &Value, Offset, sizeof(Value));

    return Value;
}

/**
 * @brief Measure TSC ticks per millisecond from the OS performance counter
 *
 * @return VOID
 */
static VOID
DbcCalibrateTsc()
{
    LARGE_INTEGER Frequency;
    LARGE_INTEGER Counter0;
    LARGE_INTEGER Counter1;
    UINT64        Tsc0;
    UINT64        Tsc1;

    Counter0 = KeQueryPerformanceCounter(&Frequency);
    Tsc0     = __rdtsc();

    DbcSleep(50);

    Counter1 = KeQueryPerformanceCounter(NULL);
    Tsc1     = __rdtsc();

    if (Frequency.QuadPart > 0 && Counter1.QuadPart > Counter0.QuadPart && Tsc1 > Tsc0)
    {
        g_DbcState.TscPerMs = ((Tsc1 - Tsc0) * (UINT64)Frequency.QuadPart) /
                              ((UINT64)(Counter1.QuadPart - Counter0.QuadPart) * 1000);
    }

    if (g_DbcState.TscPerMs < 100000)
    {
        //
        // Fall back to 3 GHz when calibration fails
        //
        g_DbcState.TscPerMs = 3000000;
    }
}

/**
 * @brief Read elapsed time in milliseconds from the OS performance counter
 *
 * @return UINT64
 */
static UINT64
DbcMilliseconds()
{
    LARGE_INTEGER Frequency;
    LARGE_INTEGER Counter = KeQueryPerformanceCounter(&Frequency);

    return ((UINT64)Counter.QuadPart * 1000) / (UINT64)Frequency.QuadPart;
}

/**
 * @brief Clear DCE and wait until the controller stops the DbC
 *
 * @return VOID
 */
static VOID
DbcDisable()
{
    UINT64 Start   = DbcMilliseconds();
    UINT32 Control = DbcRead32(DBC_REG_DCCTRL);

    if (Control == 0xFFFFFFFF)
    {
        return;
    }

    DbcWrite32(DBC_REG_DCCTRL, Control & DBC_DCCTRL_RSVDP);

    for (;;)
    {
        Control = DbcRead32(DBC_REG_DCCTRL);
        if (Control == 0xFFFFFFFF)
        {
            return;
        }

        if (!(Control & DBC_DCCTRL_DCE) || DbcMilliseconds() - Start >= DBC_DISABLE_TIMEOUT_MS)
        {
            break;
        }

        DbcSleep(1);
    }

    //
    // Let the debug host observe the USB disconnect
    //
    DbcSleep(20);
}

/**
 * @brief Disable the DbC and unmap the controller MMIO pages at PASSIVE_LEVEL
 *
 * @return VOID
 */
static VOID
DbcReleaseController()
{
    if (g_DbcState.Dbc != NULL)
    {
        DbcDisable();
        g_DbcState.Dbc = NULL;
    }

    if (g_DbcState.Mmio != NULL)
    {
        MmUnmapIoSpace(g_DbcState.Mmio, g_DbcState.MmioSize);
        g_DbcState.Mmio     = NULL;
        g_DbcState.MmioSize = 0;
    }

    g_DbcState.UsbCmd = NULL;
    g_DbcState.UsbSts = NULL;
    if (g_DbcState.ControllerMmio != NULL)
    {
        MmUnmapIoSpace(g_DbcState.ControllerMmio, PAGE_SIZE);
        g_DbcState.ControllerMmio = NULL;
    }

    DbcTxResetRing();

    g_DbcState.Ac64              = FALSE;
    g_DbcState.Configured        = FALSE;
    g_DbcState.InPosted          = FALSE;
    g_DbcState.OutHalted         = FALSE;
    g_DbcState.InHalted          = FALSE;
    g_DbcState.TxHead            = 0;
    g_DbcState.TxTail            = 0;
    g_DbcState.TxUsed            = 0;
    g_DbcState.TxUncommitted     = 0;
    g_DbcState.TxUncommittedTrbs = 0;
    g_DbcState.TxChunkBytes      = 0;
    g_DbcState.TxPacketStarted   = FALSE;
    g_DbcState.TxPacketWaitUs    = 0;
    g_DbcState.TxDropping        = FALSE;
    g_DbcState.TxStaged          = 0;
    g_DbcState.TxStagedPackets   = 0;
}

/**
 * @brief Wait for a debug host connection on the DbC port
 *
 * @param Milliseconds Timeout in milliseconds
 * @return BOOLEAN
 */
static BOOLEAN
DbcWaitForConnection(UINT32 Milliseconds)
{
    UINT64 Start = DbcMilliseconds();

    for (;;)
    {
        if (DbcCheckState())
        {
            return TRUE;
        }

        UINT32 PortSc = DbcRead32(DBC_REG_DCPORTSC);
        if (PortSc != DBC_INVALID_REGISTER_VALUE && (PortSc & DBC_DCPORTSC_CCS))
        {
            return TRUE;
        }

        if (DbcMilliseconds() - Start > Milliseconds)
        {
            return FALSE;
        }

        DbcSleep(10);
    }
}

/**
 * @brief Wait for the host to configure the USB device
 *
 * @param Milliseconds Timeout in milliseconds
 * @return BOOLEAN
 */
static BOOLEAN
DbcWaitForConfiguration(UINT32 Milliseconds)
{
    UINT64 Start = DbcMilliseconds();

    for (;;)
    {
        if (DbcCheckState())
        {
            return TRUE;
        }

        if (DbcMilliseconds() - Start > Milliseconds)
        {
            return FALSE;
        }

        DbcSleep(10);
    }
}

/**
 * @brief Wait at PASSIVE_LEVEL for controller reset and readiness to clear
 * @details xHCI 1.2 sections 5.4.1 and 5.4.2 require HCRST and CNR to be zero
 * before register access. Never writes USBCMD.HCRST.
 *
 * @param UsbCmd Mapped USBCMD register
 * @param UsbSts Mapped USBSTS register
 * @param TimeoutMs Timeout in milliseconds
 * @return BOOLEAN TRUE when both HCRST and CNR are clear
 */
static BOOLEAN
DbcWaitForControllerReady(volatile ULONG * UsbCmd, volatile ULONG * UsbSts, UINT32 TimeoutMs)
{
    UINT64 Start = DbcMilliseconds();

    for (;;)
    {
        UINT32 Command = READ_REGISTER_ULONG(UsbCmd);
        UINT32 Status  = READ_REGISTER_ULONG(UsbSts);

        if (Command == DBC_INVALID_REGISTER_VALUE || Status == DBC_INVALID_REGISTER_VALUE)
        {
            return FALSE;
        }

        if (!(Command & DBC_XHCI_USBCMD_HCRST) && !(Status & DBC_XHCI_USBSTS_CNR))
        {
            return TRUE;
        }

        if (DbcMilliseconds() - Start >= TimeoutMs)
        {
            return FALSE;
        }

        DbcSleep(10);
    }
}

/**
 * @brief Allocate contiguous memory without crossing a 64 KB boundary
 * @details Try below 4 GB first for controllers that use 32-bit addresses.
 *
 * @return BOOLEAN
 */
static BOOLEAN
DbcAllocateBlock()
{
    PHYSICAL_ADDRESS Lowest;
    PHYSICAL_ADDRESS Highest;
    PHYSICAL_ADDRESS Boundary;

    Lowest.QuadPart   = 0;
    Highest.QuadPart  = 0xFFFFFFFF;
    Boundary.QuadPart = 0x10000;

    g_DbcState.Block = (PUCHAR)MmAllocateContiguousMemorySpecifyCache(DBC_BLOCK_SIZE, Lowest, Highest, Boundary, MmCached);

    if (g_DbcState.Block == NULL)
    {
        Highest.QuadPart = -1LL;
        g_DbcState.Block = (PUCHAR)MmAllocateContiguousMemorySpecifyCache(DBC_BLOCK_SIZE, Lowest, Highest, Boundary, MmCached);
    }

    if (g_DbcState.Block == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(g_DbcState.Block, DBC_BLOCK_SIZE);

    //
    // Save the physical address for use in vmx-root
    //
    g_DbcState.BlockPa = (UINT64)MmGetPhysicalAddress(g_DbcState.Block).QuadPart;

    return TRUE;
}

/**
 * @brief Try DbC on one controller and wait for the host to connect
 *
 * @param Bus PCI bus number
 * @param Device PCI device number
 * @param Function PCI function number
 * @return BOOLEAN
 */
static BOOLEAN
DbcTryController(UINT32 Bus, UINT32 Device, UINT32 Function)
{
    UINT32           Bar0Low  = DbcPciRead32(Bus, Device, Function, 0x10);
    UINT32           Bar0High = 0;
    UCHAR            CapLength;
    UINT32           HccParams1;
    BOOLEAN          Ac64;
    UINT32           Capability;
    UINT32           CapabilityOffset;
    UINT32           CapabilityPageOffset;
    UINT32           MappedPageOffset;
    UINT32           DbcOffset = 0;
    UINT32           DbcPageOffset;
    SIZE_T           DbcMapSize;
    UINT32           Control;
    PHYSICAL_ADDRESS Bar0;
    PHYSICAL_ADDRESS MapAddress;
    PUCHAR           PageMmio;
    PUCHAR           Mmio;

    if (Bar0Low == 0xFFFFFFFF || (Bar0Low & 0x1))
    {
        return FALSE;
    }

    if ((Bar0Low & 0x6) == 0x4)
    {
        Bar0High = DbcPciRead32(Bus, Device, Function, 0x14);
        if (Bar0High == 0xFFFFFFFF)
        {
            return FALSE;
        }
    }

    Bar0.QuadPart = (LONGLONG)(((UINT64)Bar0High << 32) | (Bar0Low & ~0xFu));

    if (Bar0.QuadPart == 0)
    {
        return FALSE;
    }

    //
    // Map the first page to read the controller capabilities and status
    //
    MappedPageOffset = 0;
    PageMmio         = (PUCHAR)MmMapIoSpaceEx(Bar0, PAGE_SIZE, PAGE_READWRITE | PAGE_NOCACHE);

    if (PageMmio == NULL)
    {
        return FALSE;
    }

    CapLength  = (UCHAR)(READ_REGISTER_ULONG((volatile ULONG *)PageMmio) & 0xFF);
    HccParams1 = READ_REGISTER_ULONG((volatile ULONG *)(PageMmio + DBC_XHCI_HCCPARAMS1));

    if (HccParams1 == 0xFFFFFFFF || CapLength < 0x20 || (CapLength & 0x3) != 0)
    {
        MmUnmapIoSpace(PageMmio, PAGE_SIZE);
        return FALSE;
    }

    Ac64 = (HccParams1 & 0x1) != 0;

    //
    // Wait for any active controller reset to finish and check address range
    //
    if ((!Ac64 && (g_DbcState.BlockPa + DBC_BLOCK_SIZE - 1) > 0xFFFFFFFFULL) ||
        !DbcWaitForControllerReady((volatile ULONG *)(PageMmio + CapLength + DBC_XHCI_USBCMD_OFFSET),
                                   (volatile ULONG *)(PageMmio + CapLength + DBC_XHCI_USBSTS_OFFSET),
                                   DBC_CONTROLLER_READY_TIMEOUT_MS))
    {
        MmUnmapIoSpace(PageMmio, PAGE_SIZE);
        return FALSE;
    }

    //
    // Search the extended capabilities for DbC one page at a time
    //
    CapabilityOffset = (HccParams1 >> 16) * 4;

    for (UINT32 Steps = 0; CapabilityOffset != 0 && Steps < DBC_MAX_EXT_CAPABILITIES; Steps++)
    {
        CapabilityPageOffset = CapabilityOffset & ~(UINT32)(PAGE_SIZE - 1);

        if (CapabilityPageOffset != MappedPageOffset)
        {
            MmUnmapIoSpace(PageMmio, PAGE_SIZE);
            MapAddress.QuadPart = Bar0.QuadPart + (LONGLONG)CapabilityPageOffset;
            PageMmio            = (PUCHAR)MmMapIoSpaceEx(MapAddress, PAGE_SIZE, PAGE_READWRITE | PAGE_NOCACHE);

            if (PageMmio == NULL)
            {
                return FALSE;
            }

            MappedPageOffset = CapabilityPageOffset;
        }

        Capability = READ_REGISTER_ULONG((volatile ULONG *)(PageMmio + (CapabilityOffset - MappedPageOffset)));

        if (Capability == 0xFFFFFFFF)
        {
            break;
        }

        if ((Capability & 0xFF) == DBC_XHCI_CAPABILITY_ID)
        {
            DbcOffset = CapabilityOffset;
            break;
        }

        if (((Capability >> 8) & 0xFF) == 0)
        {
            break;
        }

        CapabilityOffset += ((Capability >> 8) & 0xFF) * 4;
    }

    MmUnmapIoSpace(PageMmio, PAGE_SIZE);

    if (DbcOffset == 0)
    {
        return FALSE;
    }

    //
    // Map only the pages that contain the DbC registers
    //
    DbcPageOffset       = DbcOffset & ~(UINT32)(PAGE_SIZE - 1);
    DbcMapSize          = ((SIZE_T)(DbcOffset - DbcPageOffset) + DBC_REGISTER_BLOCK_SIZE + (PAGE_SIZE - 1)) & ~(SIZE_T)(PAGE_SIZE - 1);
    MapAddress.QuadPart = Bar0.QuadPart + (LONGLONG)DbcPageOffset;

    Mmio = (PUCHAR)MmMapIoSpaceEx(MapAddress, DbcMapSize, PAGE_READWRITE | PAGE_NOCACHE);

    if (Mmio == NULL)
    {
        return FALSE;
    }

    g_DbcState.Mmio     = Mmio;
    g_DbcState.MmioSize = DbcMapSize;
    g_DbcState.Dbc      = Mmio + (DbcOffset - DbcPageOffset);
    g_DbcState.Ac64     = Ac64;

    g_DbcState.ControllerMmio = (PUCHAR)MmMapIoSpaceEx(Bar0, PAGE_SIZE, PAGE_READWRITE | PAGE_NOCACHE);
    if (g_DbcState.ControllerMmio != NULL)
    {
        g_DbcState.UsbCmd = (volatile ULONG *)(g_DbcState.ControllerMmio + CapLength + DBC_XHCI_USBCMD_OFFSET);
        g_DbcState.UsbSts = (volatile ULONG *)(g_DbcState.ControllerMmio + CapLength + DBC_XHCI_USBSTS_OFFSET);
    }

    Control = DbcControllerResetIdle() ? DbcRead32(DBC_REG_DCCTRL) : DBC_INVALID_REGISTER_VALUE;
    if (Control == DBC_INVALID_REGISTER_VALUE || (Control & DBC_DCCTRL_DCE))
    {
        //
        // Skip controllers whose DbC is inaccessible or already owned,
        // for example by KDNET-USB or KDUSB
        //
        MmUnmapIoSpace(Mmio, DbcMapSize);
        g_DbcState.Mmio     = NULL;
        g_DbcState.MmioSize = 0;
        g_DbcState.Dbc      = NULL;
        g_DbcState.Ac64     = FALSE;

        g_DbcState.UsbCmd = NULL;
        g_DbcState.UsbSts = NULL;
        if (g_DbcState.ControllerMmio != NULL)
        {
            MmUnmapIoSpace(g_DbcState.ControllerMmio, PAGE_SIZE);
            g_DbcState.ControllerMmio = NULL;
        }
        return FALSE;
    }

    DbcProgram();

    if (!DbcWaitForConnection(DBC_CONNECT_TIMEOUT_MS))
    {
        DbcReleaseController();
        return FALSE;
    }

    g_DbcState.Bus      = Bus;
    g_DbcState.Device   = Device;
    g_DbcState.Function = Function;

    return TRUE;
}

/**
 * @brief Find a controller with a USB debug host attached
 *
 * @return BOOLEAN
 */
static BOOLEAN
DbcFindController()
{
    for (UINT32 Bus = 0; Bus < DBC_PCI_MAX_BUSES; Bus++)
    {
        for (UINT32 Device = 0; Device < DBC_PCI_MAX_DEVICES; Device++)
        {
            UINT32 HeaderType = 0;

            for (UINT32 Function = 0; Function < DBC_PCI_MAX_FUNCTIONS; Function++)
            {
                UINT32 Id = DbcPciRead32(Bus, Device, Function, 0x00);

                if ((Id & 0xFFFF) == 0xFFFF || (Id & 0xFFFF) == 0)
                {
                    if (Function == 0)
                    {
                        break;
                    }

                    continue;
                }

                if (Function == 0)
                {
                    HeaderType = (DbcPciRead32(Bus, Device, 0, 0x0C) >> 16) & 0xFF;
                }

                if ((DbcPciRead32(Bus, Device, Function, 0x08) >> 8) == DBC_PCI_CLASS_XHCI &&
                    (DbcPciRead32(Bus, Device, Function, 0x04) & DBC_PCI_COMMAND_MSE_BME) == DBC_PCI_COMMAND_MSE_BME &&
                    DbcTryController(Bus, Device, Function))
                {
                    return TRUE;
                }

                if (Function == 0 && !(HeaderType & 0x80))
                {
                    break;
                }
            }
        }
    }

    return FALSE;
}

/**
 * @brief Prepare DbC if the debuggee selects USB
 * @details Call at PASSIVE_LEVEL.
 *
 * @param PortAddress Port selector from the debuggee prepare request
 * @return BOOLEAN TRUE when SERIAL_PORT_DBC is selected
 */
BOOLEAN
DbcPrepare(UINT32 PortAddress)
{
    if (PortAddress != SERIAL_PORT_DBC)
    {
        g_DbcState.Ready  = FALSE;
        g_DbcState.Active = FALSE;
        KeMemoryBarrier();

        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            DbcReleaseController();
        }

        return FALSE;
    }

    g_DbcState.Ready         = FALSE;
    g_DbcState.Active        = TRUE;
    g_DbcState.TxWindow      = 0;
    g_DbcState.TxWindowBytes = 0;
    KeMemoryBarrier();

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        return TRUE;
    }

    DbcReleaseController();

    if (g_DbcState.TscPerMs == 0)
    {
        DbcCalibrateTsc();
    }

    if (g_DbcState.Block == NULL && !DbcAllocateBlock())
    {
        return TRUE;
    }

    if (DbcFindController())
    {
        if (DbcWaitForConfiguration(DBC_CONFIGURE_TIMEOUT_MS))
        {
            g_DbcState.Ready = TRUE;
        }
        else
        {
            DbcReleaseController();
        }
    }

    return TRUE;
}

/**
 * @brief Report whether the debuggee selected USB
 *
 * @return BOOLEAN
 */
BOOLEAN
DbcIsActive()
{
    return g_DbcState.Active;
}

/**
 * @brief Report USB selection even when the transport is not ready
 *
 * @return BOOLEAN TRUE if the debuggee selected USB
 */
BOOLEAN
KdHyperDbgDbcIsActive()
{
    return DbcIsActive();
}

/**
 * @brief Report whether the host configured the selected USB device during setup
 *
 * @return BOOLEAN
 */
BOOLEAN
KdHyperDbgDbcIsReady()
{
    return g_DbcState.Active && g_DbcState.Ready && DbcControllerReady();
}

/**
 * @brief Change the send wait mode without clearing delivery state
 *
 * @param Allow TRUE to use the long wait for space
 * @return BOOLEAN Previous wait mode
 */
BOOLEAN
KdHyperDbgDbcSetSendWait(BOOLEAN Allow)
{
    BOOLEAN Previous = g_DbcState.LongWait;

    g_DbcState.LongWait = Allow;

    return Previous;
}

/**
 * @brief Add or remove a protected send priority scope
 * @details Hold DebuggerResponseLock for every call.
 *
 * @param Enable TRUE to add a scope, FALSE to remove one
 * @return VOID
 */
VOID
KdHyperDbgDbcSetControlPriority(BOOLEAN Enable)
{
    if (Enable)
    {
        g_DbcState.TxControlWaiters++;
    }
    else if (g_DbcState.TxControlWaiters != 0)
    {
        g_DbcState.TxControlWaiters--;
    }
}

/**
 * @brief Drop a whole event packet while protected sends have priority
 * @details Hold DebuggerResponseLock before serializing any byte of the packet.
 *
 * @param Length Total packet length including its header and end marker
 * @return BOOLEAN TRUE if the packet was dropped without changing the loss state
 */
BOOLEAN
KdHyperDbgDbcDropEventPacket(UINT32 Length)
{
    if (!g_DbcState.Active || g_DbcState.TxControlWaiters == 0)
    {
        return FALSE;
    }

    g_DbcState.TxPriorityDrops++;
    g_DbcState.TxPacketsDropped++;
    g_DbcState.TxDroppedBytes += Length;

    return TRUE;
}

/**
 * @brief Check whether a complete USB packet fits without waiting for space
 * @details Hold DebuggerResponseLock through this check and the send.
 * Polls for completions and queues earlier staged packets before checking space.
 *
 * @param Length Total packet length including its header and end marker
 * @return BOOLEAN TRUE if the packet fits in the configured USB send ring
 */
BOOLEAN
KdHyperDbgDbcCanSendPacket(UINT64 Length)
{
    UINT32 Position;
    UINT32 Remaining;
    UINT32 StreamRemaining = DBC_TX_STREAM_SIZE;
    UINT32 Trbs            = 0;
    UINT32 Chunk;

    if (!g_DbcState.Active || !g_DbcState.Ready || Length > DBC_TX_BUFFER_SIZE ||
        g_DbcState.TxPacketStarted || g_DbcState.TxDropping ||
        g_DbcState.TxUncommitted != g_DbcState.TxStaged || !DbcCheckStateFast())
    {
        return FALSE;
    }

    DbcTxFlushStaged(TRUE);

    if (Length > DBC_TX_BUFFER_SIZE - g_DbcState.TxUsed)
    {
        return FALSE;
    }

    Position  = g_DbcState.TxHead;
    Remaining = (UINT32)Length;

    while (Remaining != 0)
    {
        Chunk = DbcTxChunkLength(Position, Remaining);
        Chunk = min(Chunk, StreamRemaining);
        Trbs++;

        Position = (Position + Chunk) % DBC_TX_BUFFER_SIZE;
        Remaining -= Chunk;
        StreamRemaining -= Chunk;
        if (StreamRemaining == 0)
        {
            StreamRemaining = DBC_TX_STREAM_SIZE;
        }
    }

    return g_DbcState.OutPending + g_DbcState.TxUncommittedTrbs + Trbs <= DBC_OUT_MAX_PENDING;
}

/**
 * @brief Clear delivery loss at the start of a USB setup or symbol pass
 * @details Hold DebuggerResponseLock for every call.
 *
 * @return VOID
 */
VOID
KdHyperDbgDbcClearSendLoss()
{
    g_DbcState.TxLoss      = FALSE;
    g_DbcState.TxLossCount = 0;
}

/**
 * @brief Read the delivery loss count for a USB setup or symbol pass
 * @details Hold DebuggerResponseLock for every call.
 *
 * @return UINT32 Number of delivery loss records since the pass started
 */
UINT32
KdHyperDbgDbcGetSendLossCount()
{
    return g_DbcState.TxLossCount;
}

/**
 * @brief Allow or forbid long send waits
 * @details Use TRUE while the debuggee is halted or sets up the session.
 * Resets the progress checks without clearing delivery loss.
 *
 * @param Allow TRUE to wait up to the long wait for space in the send ring
 * @return VOID
 */
VOID
KdHyperDbgDbcSetLongWait(BOOLEAN Allow)
{
    g_DbcState.LongWait           = Allow;
    g_DbcState.TxStuck            = FALSE;
    g_DbcState.TxNoProgress       = FALSE;
    g_DbcState.LastOutProgressTsc = __rdtsc();
}

/**
 * @brief Poll until the host took every queued byte or the time passes
 * @details Hold DebuggerResponseLock while polling. KdHyperDbgDbcClearSendLoss clears the loss state.
 *
 * @param Milliseconds Longest poll time in milliseconds
 * @return BOOLEAN TRUE when nothing is pending and no long-wait packet or queued data was lost
 */
BOOLEAN
KdHyperDbgDbcWaitForSend(UINT32 Milliseconds)
{
    return KdHyperDbgDbcWaitForSendEx(Milliseconds, FALSE);
}

/**
 * @brief Poll for completed USB transfers and optionally ignore earlier loss
 * @details Hold DebuggerResponseLock while polling. The loss state stays latched.
 *
 * @param Milliseconds Longest poll time in milliseconds
 * @param IgnoreLoss TRUE to drain queued transfers before sending an error finish
 * @return BOOLEAN TRUE when nothing is pending and the selected loss check passes
 */
BOOLEAN
KdHyperDbgDbcWaitForSendEx(UINT32 Milliseconds, BOOLEAN IgnoreLoss)
{
    UINT64  StartTsc = __rdtsc();
    UINT64  Spins    = 0;
    BOOLEAN Result;

    if (!g_DbcState.Active)
    {
        return TRUE;
    }

    if (!g_DbcState.Ready)
    {
        return FALSE;
    }

    KdHyperDbgDbcSetControlPriority(TRUE);

    while (g_DbcState.OutPending != 0 || g_DbcState.TxStaged != 0)
    {
        if (!DbcCheckStateFast())
        {
            break;
        }

        //
        // The caller waits for delivery, so queue waiting packets now
        //
        DbcTxFlushStaged(TRUE);

        if (g_DbcState.OutPending == 0 || DbcTimeoutExpired(StartTsc, Milliseconds, &Spins))
        {
            break;
        }

        _mm_pause();
    }

    Result = g_DbcState.OutPending == 0 && g_DbcState.TxUncommitted == 0 && !g_DbcState.TxPacketStarted &&
             !g_DbcState.TxDropping && (IgnoreLoss || !g_DbcState.TxLoss);

    KdHyperDbgDbcSetControlPriority(FALSE);

    return Result;
}

/**
 * @brief Disable DbC and free its resources on unload
 *
 * @return VOID
 */
VOID
DbcUnload()
{
    g_DbcState.Ready  = FALSE;
    g_DbcState.Active = FALSE;
    KeMemoryBarrier();

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        return;
    }

    DbcReleaseController();

    if (g_DbcState.Block != NULL)
    {
        MmFreeContiguousMemorySpecifyCache(g_DbcState.Block, DBC_BLOCK_SIZE, MmCached);
    }

    RtlZeroMemory(&g_DbcState, sizeof(DBC_STATE));
    g_DbcState.TxRunningWaitUs   = DBC_RUNNING_WAIT_US;
    g_DbcState.TxCoalescePending = DBC_COALESCE_PENDING;
    g_DbcState.TxCoalesceBytes   = DBC_COALESCE_BYTES;
}
