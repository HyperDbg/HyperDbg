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
#define DBC_CC_SUCCESS         1
#define DBC_CC_BABBLE_ERROR    3
#define DBC_CC_USB_TRANS_ERROR 4
#define DBC_CC_TRB_ERROR       5
#define DBC_CC_STALL_ERROR     6
#define DBC_CC_SHORT_PACKET    13
#define DBC_TRB_LENGTH_MASK    0x1FFFFu

//
// Ring and packet sizes
//
#define DBC_RING_TRBS  256
#define DBC_EVENT_TRBS 256
#define DBC_MAX_PACKET 1024

//
// Offsets in the contiguous memory block
//
#define DBC_OFFSET_CONTEXT 0x0000 // Info and endpoint contexts
#define DBC_OFFSET_STRINGS 0x0100 // USB strings
#define DBC_OFFSET_ERST    0x0200 // Event ring segment table
#define DBC_OFFSET_EVENT   0x1000
#define DBC_OFFSET_OUT     0x2000
#define DBC_OFFSET_IN      0x3000
#define DBC_OFFSET_RX      0x4000 // One receive packet
#define DBC_OFFSET_TX      0x5000 // Send buffer
#define DBC_TX_BUFFER_SIZE 0x4000
#define DBC_BLOCK_SIZE     (DBC_OFFSET_TX + DBC_TX_BUFFER_SIZE)

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
    BOOLEAN TxStuck;    // Send timed out
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

    UINT32 TxLength;
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

} DBC_STATE, *PDBC_STATE;

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

DBC_STATE g_DbcState = {0};

static const WCHAR g_DbcStringLangId[]       = {0x0409, 0};
static const WCHAR g_DbcStringManufacturer[] = L"Intel";
static const WCHAR g_DbcStringProduct[]      = L"USB 3.0 Debug Cable";
static const WCHAR g_DbcStringSerial[]       = L"1";

//////////////////////////////////////////////////
//			Registers and Memory				//
//////////////////////////////////////////////////

/**
 * @brief Read a 32-bit DbC register
 *
 * @param Offset Byte offset from the DbC capability base
 * @return UINT32
 */
static UINT32
DbcRead32(UINT32 Offset)
{
    return READ_REGISTER_ULONG((volatile ULONG *)(g_DbcState.Dbc + Offset));
}

/**
 * @brief Write a 32-bit DbC register
 *
 * @param Offset Byte offset from the DbC capability base
 * @param Value Value to write
 * @return VOID
 */
static VOID
DbcWrite32(UINT32 Offset, UINT32 Value)
{
    WRITE_REGISTER_ULONG((volatile ULONG *)(g_DbcState.Dbc + Offset), Value);
}

/**
 * @brief Write a DbC address and preserve reserved bits
 * @details Write the high 32 bits only if the controller supports 64-bit addresses.
 *
 * @param Offset Byte offset from the DbC capability base
 * @param Value Physical address value to write
 * @param PreserveMask Reserved bits to preserve in the low 32 bits
 * @return VOID
 */
static VOID
DbcWrite64(UINT32 Offset, UINT64 Value, UINT32 PreserveMask)
{
    UINT32 Low = ((UINT32)Value & ~PreserveMask) | (DbcRead32(Offset) & PreserveMask);

    DbcWrite32(Offset, Low);

    if (g_DbcState.Ac64)
    {
        DbcWrite32(Offset + 4, (UINT32)(Value >> 32));
    }
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
 * @return VOID
 */
static VOID
DbcInitializeEndpointContexts()
{
    volatile UINT32 * Context  = DbcBlock32(DBC_OFFSET_CONTEXT);
    UINT32            MaxBurst = (DbcRead32(DBC_REG_DCCTRL) >> 16) & 0xFF;
    UINT32            EpInfo2  = (MaxBurst << 8) | ((UINT32)DBC_MAX_PACKET << 16);

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

    Trb[0] = (UINT32)DataPa;
    Trb[1] = (UINT32)(DataPa >> 32);
    Trb[2] = Length & DBC_TRB_LENGTH_MASK;
    KeMemoryBarrier();

    //
    // Write the cycle bit last to hand the TRB to the controller
    //
    Trb[3] = (DBC_TRB_NORMAL << 10) | 0x20 | 0x4 | (*Cycle & 1);

    (*Enqueue)++;

    if (*Enqueue == DBC_RING_TRBS - 1)
    {
        //
        // Hand the Link TRB to the controller before wrapping
        //
        Link[3]  = (Link[3] & ~1u) | (*Cycle & 1);
        *Enqueue = 0;
        *Cycle ^= 1;
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
    DbcInitializeEndpointContexts();

    //
    // Use one segment for the event ring
    //
    DbcZeroBlock(DBC_OFFSET_EVENT, DBC_EVENT_TRBS * 16);
    Erst[0] = (UINT32)DbcPhysical(DBC_OFFSET_EVENT);
    Erst[1] = (UINT32)(DbcPhysical(DBC_OFFSET_EVENT) >> 32);
    Erst[2] = DBC_EVENT_TRBS;
    Erst[3] = 0;

    g_DbcState.OutEnqueue   = 0;
    g_DbcState.OutCycle     = 1;
    g_DbcState.InEnqueue    = 0;
    g_DbcState.InCycle      = 1;
    g_DbcState.EventDequeue = 0;
    g_DbcState.EventCycle   = 1;
    g_DbcState.OutPending   = 0;
    g_DbcState.InPosted     = FALSE;
    g_DbcState.TxStuck      = FALSE;
    g_DbcState.OutHalted    = FALSE;
    g_DbcState.InHalted     = FALSE;
    g_DbcState.Configured   = FALSE;
    g_DbcState.RxLength     = 0;
    g_DbcState.RxPosition   = 0;

    g_DbcState.OutRetainedTrb   = 0;
    g_DbcState.WatchdogStartTsc = 0;
    KeMemoryBarrier();

    //
    // Set the event ring dequeue pointer before the segment table address
    //
    DbcWrite32(DBC_REG_DCERSTSZ, (DbcRead32(DBC_REG_DCERSTSZ) & DBC_DCERSTSZ_RSVDP) | 1u);
    DbcWrite64(DBC_REG_DCERDP, DbcPhysical(DBC_OFFSET_EVENT), DBC_DCERDP_RSVDP);
    DbcWrite64(DBC_REG_DCERSTBA, DbcPhysical(DBC_OFFSET_ERST), DBC_DCERSTBA_RSVDP);
    DbcWrite64(DBC_REG_DCCP, DbcPhysical(DBC_OFFSET_CONTEXT), DBC_DCCP_RSVDP);
    DbcWrite32(DBC_REG_DCDDI1, ((UINT32)DBC_VID << 16) | DBC_PROTOCOL);
    DbcWrite32(DBC_REG_DCDDI2, ((UINT32)DBC_REVISION << 16) | DBC_PID);
    KeMemoryBarrier();

    //
    // Clear the run change bit so the doorbell works
    //
    DbcWrite32(DBC_REG_DCCTRL, (DbcRead32(DBC_REG_DCCTRL) & DBC_DCCTRL_RSVDP) | DBC_DCCTRL_DRC | DBC_DCCTRL_DCE);
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

    g_DbcState.OutEnqueue = 0;
    g_DbcState.OutCycle   = 1;
    g_DbcState.InEnqueue  = 0;
    g_DbcState.InCycle    = 1;
    g_DbcState.OutPending = 0;
    g_DbcState.InPosted   = FALSE;
    g_DbcState.TxStuck    = FALSE;
    g_DbcState.OutHalted  = FALSE;
    g_DbcState.InHalted   = FALSE;
    g_DbcState.RingResets++;

    g_DbcState.OutRetainedTrb = 0;
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
        g_DbcState.OutPending    -= Distance;
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
 * @brief Drain pending DbC events in vmx-root
 * @details Keep received bytes in the buffer until the caller reads them.
 *
 * @return VOID
 */
static VOID
DbcProcessEvents()
{
    UINT32 Unflushed = 0;

    for (UINT32 Processed = 0; Processed < DBC_EVENT_TRBS; Processed++)
    {
        volatile UINT32 * Event = DbcBlock32(DBC_OFFSET_EVENT + g_DbcState.EventDequeue * 16);
        UINT32            Dw3   = Event[3];
        UINT32            Type;
        UINT32            CompletionCode;
        UINT32            Residual;
        UINT64            TrbPa;
        BOOLEAN           Success;
        BOOLEAN           Stall;
        BOOLEAN           InRing;
        BOOLEAN           OutRing;

        if ((Dw3 & 1) != g_DbcState.EventCycle)
        {
            break;
        }

        KeMemoryBarrier();

        Type           = (Dw3 >> 10) & 0x3F;
        TrbPa          = Event[0] | ((UINT64)Event[1] << 32);
        CompletionCode = Event[2] >> 24;
        Residual       = Event[2] & 0xFFFFFF;

        if (Type == DBC_TRB_TRANSFER_EVENT)
        {
            Success = (CompletionCode == DBC_CC_SUCCESS || CompletionCode == DBC_CC_SHORT_PACKET);
            Stall   = (CompletionCode == DBC_CC_STALL_ERROR);
            InRing  = DbcIsTrbInRing(DBC_OFFSET_IN, TrbPa);
            OutRing = DbcIsTrbInRing(DBC_OFFSET_OUT, TrbPa);

            if (!InRing && !OutRing)
            {
                g_DbcState.UnknownTrbEvents++;
            }

            if (!Success)
            {
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
                        g_DbcState.OutPending--;
                        g_DbcState.OutRetainedTrb = 0;
                    }
                    else
                    {
                        g_DbcState.OutRetainedTrb = TrbPa;
                    }
                }
            }
        }

        g_DbcState.EventDequeue++;

        if (g_DbcState.EventDequeue == DBC_EVENT_TRBS)
        {
            g_DbcState.EventDequeue = 0;
            g_DbcState.EventCycle ^= 1;
        }

        Unflushed++;

        if (Unflushed == DBC_EVENT_TRBS / 2)
        {
            //
            // Advance the pointer mid-ring so a full wrap changes the value
            //
            DbcWrite64(DBC_REG_DCERDP,
                       DbcPhysical(DBC_OFFSET_EVENT) + (UINT64)g_DbcState.EventDequeue * 16,
                       DBC_DCERDP_RSVDP);
            Unflushed = 0;
        }
    }

    if (Unflushed != 0)
    {
        DbcWrite64(DBC_REG_DCERDP,
                   DbcPhysical(DBC_OFFSET_EVENT) + (UINT64)g_DbcState.EventDequeue * 16,
                   DBC_DCERDP_RSVDP);
    }
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
 * @return BOOLEAN TRUE when the debug host configured the device
 */
static BOOLEAN
DbcCheckState()
{
    volatile UINT32 * Context;
    UINT32            Control = DbcRead32(DBC_REG_DCCTRL);
    UINT32            PortSc;
    BOOLEAN           WasConfigured;

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

        DbcProcessEvents();
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

    DbcProcessEvents();

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
    else if (!WasConfigured)
    {
        //
        // Resume IN transfers after configuration
        //
        if (!g_DbcState.InPosted && g_DbcState.RxPosition >= g_DbcState.RxLength && g_DbcState.Ready)
        {
            DbcPostInTransfer();
        }
        else if (g_DbcState.InPosted)
        {
            DbcRingDoorbell(DBC_DOORBELL_IN);
        }
    }

    return TRUE;
}

/**
 * @brief Poll OUT transfers until they finish or time out in vmx-root
 *
 * @param Milliseconds Timeout in milliseconds
 * @return BOOLEAN
 */
static BOOLEAN
DbcWaitForOutTransfers(UINT32 Milliseconds)
{
    UINT64 StartTsc = __rdtsc();
    UINT64 Spins    = 0;

    while (g_DbcState.OutPending != 0)
    {
        if (!DbcCheckState())
        {
            break;
        }

        if (g_DbcState.OutPending == 0 || DbcTimeoutExpired(StartTsc, Milliseconds, &Spins))
        {
            break;
        }

        _mm_pause();
    }

    return g_DbcState.OutPending == 0;
}

/**
 * @brief Send buffered bytes through the OUT endpoint in vmx-root
 *
 * @return VOID
 */
static VOID
DbcFlush()
{
    UINT32 Offset;
    UINT32 Chunk;

    if (g_DbcState.TxLength == 0)
    {
        return;
    }

    if (!DbcCheckState() || g_DbcState.OutHalted)
    {
        g_DbcState.TxDroppedBytes += g_DbcState.TxLength;
        g_DbcState.TxLength = 0;
        return;
    }

    for (Offset = 0; Offset < g_DbcState.TxLength; Offset += Chunk)
    {
        Chunk = g_DbcState.TxLength - Offset;

        if (Chunk > DBC_MAX_PACKET)
        {
            Chunk = DBC_MAX_PACKET;
        }

        DbcQueueNormalTrb(DBC_OFFSET_OUT, &g_DbcState.OutEnqueue, &g_DbcState.OutCycle, DbcPhysical(DBC_OFFSET_TX + Offset), Chunk);
        g_DbcState.OutPending++;
    }

    g_DbcState.TxBytes += g_DbcState.TxLength;
    g_DbcState.TxLength = 0;

    DbcRingDoorbell(DBC_DOORBELL_OUT);

    if (!DbcWaitForOutTransfers(DBC_SEND_TIMEOUT_MS))
    {
        //
        // The controller still owns the send buffer
        //
        g_DbcState.TxStuck = TRUE;
        g_DbcState.TxTimeouts++;
    }
}

/**
 * @brief Buffer a byte and send when the buffer is full or the marker arrives
 *
 * @param Byte Byte to send
 * @return VOID
 */
VOID
DbcSendByte(UCHAR Byte)
{
    volatile UCHAR * Tx;
    UINT32           Length;
    BOOLEAN          EndOfPacket;

    if (!g_DbcState.Ready)
    {
        return;
    }

    if (g_DbcState.OutPending != 0)
    {
        //
        // Do not reuse the send buffer while transfers are pending
        //
        DbcCheckState();

        if (g_DbcState.OutPending != 0)
        {
            g_DbcState.TxWindow      = 0;
            g_DbcState.TxWindowBytes = 0;
            g_DbcState.TxDroppedBytes++;
            return;
        }

        g_DbcState.TxStuck = FALSE;
    }

    Tx     = DbcBlock8(DBC_OFFSET_TX);
    Length = g_DbcState.TxLength;

    Tx[Length] = Byte;
    Length++;
    g_DbcState.TxLength = Length;

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

    if (Length == DBC_TX_BUFFER_SIZE || EndOfPacket)
    {
        DbcFlush();
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

    g_DbcState.Ac64       = FALSE;
    g_DbcState.Configured = FALSE;
    g_DbcState.InPosted   = FALSE;
    g_DbcState.TxStuck    = FALSE;
    g_DbcState.OutHalted  = FALSE;
    g_DbcState.InHalted   = FALSE;
    g_DbcState.OutPending = 0;
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
        if (DbcCheckState() || (DbcRead32(DBC_REG_DCPORTSC) & DBC_DCPORTSC_CCS))
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
    UINT32           UsbSts;
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

    UsbSts = READ_REGISTER_ULONG((volatile ULONG *)(PageMmio + CapLength + DBC_XHCI_USBSTS_OFFSET));
    Ac64   = (HccParams1 & 0x1) != 0;

    //
    // Skip controllers that are not ready or cannot address the memory block
    //
    if (UsbSts == 0xFFFFFFFF || (UsbSts & DBC_XHCI_USBSTS_CNR) ||
        (!Ac64 && (g_DbcState.BlockPa + DBC_BLOCK_SIZE - 1) > 0xFFFFFFFFULL))
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

    Control = DbcRead32(DBC_REG_DCCTRL);
    if (Control == 0xFFFFFFFF || (Control & DBC_DCCTRL_DCE))
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
    g_DbcState.TxLength      = 0;
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
 * @brief Report whether the host configured the selected USB device during setup
 *
 * @return BOOLEAN
 */
BOOLEAN
KdHyperDbgDbcIsReady()
{
    return g_DbcState.Active && g_DbcState.Ready;
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
}
