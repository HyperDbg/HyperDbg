/**
 * @file PtDefinitions.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Intel Processor Trace (PT) related data structures and hardware
 *        definitions shared between the kernel and user-mode components.
 * @details
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                MSR Addresses                 //
//   (Intel SDM Vol. 3, Chapter 32)             //
//////////////////////////////////////////////////

#define MSR_IA32_RTIT_OUTPUT_BASE      0x00000560
#define MSR_IA32_RTIT_OUTPUT_MASK_PTRS 0x00000561
#define MSR_IA32_RTIT_CTL              0x00000570
#define MSR_IA32_RTIT_STATUS           0x00000571
#define MSR_IA32_RTIT_CR3_MATCH        0x00000572

#define MSR_IA32_RTIT_ADDR0_A 0x00000580
#define MSR_IA32_RTIT_ADDR0_B 0x00000581
#define MSR_IA32_RTIT_ADDR1_A 0x00000582
#define MSR_IA32_RTIT_ADDR1_B 0x00000583
#define MSR_IA32_RTIT_ADDR2_A 0x00000584
#define MSR_IA32_RTIT_ADDR2_B 0x00000585
#define MSR_IA32_RTIT_ADDR3_A 0x00000586
#define MSR_IA32_RTIT_ADDR3_B 0x00000587

//
// Used in PMI acknowledgment
//
#define MSR_IA32_PERF_GLOBAL_STATUS   0x0000038E
#define MSR_IA32_PERF_GLOBAL_OVF_CTRL 0x00000390

//
// Bit 55 of IA32_PERF_GLOBAL_STATUS: ToPA PMI pending
//
#define PERF_GLOBAL_STATUS_TOPA_PMI (1ULL << 55)

//////////////////////////////////////////////////
//                  Constants                   //
//////////////////////////////////////////////////

#define PT_PAGE_SIZE           0x1000ULL    /* 4 KB                                  */
#define PT_DEFAULT_BUFFER_SIZE 0x200000ULL  /* 2 MB                                  */
#define PT_OVERFLOW_SIZE       PT_PAGE_SIZE /* 4 KB overflow landing zone            */
#define PT_MAX_ADDR_RANGES     4

//
// Maximum CPUs that the PT user-mode mmap surface can describe in a
// single request. Sized to fit one HYPERTRACE_PT_MMAP_PACKETS within a
// typical IOCTL buffer and to cover realistic host topologies.
//
#define PT_MAX_CPUS_FOR_MMAP 64

//
// ToPA entry Size field encoding: value N = 4KB * 2^N
//
#define PT_TOPA_SIZE_4K   0
#define PT_TOPA_SIZE_8K   1
#define PT_TOPA_SIZE_16K  2
#define PT_TOPA_SIZE_32K  3
#define PT_TOPA_SIZE_64K  4
#define PT_TOPA_SIZE_128K 5
#define PT_TOPA_SIZE_256K 6
#define PT_TOPA_SIZE_512K 7
#define PT_TOPA_SIZE_1M   8
#define PT_TOPA_SIZE_2M   9
#define PT_TOPA_SIZE_4M   10
#define PT_TOPA_SIZE_8M   11
#define PT_TOPA_SIZE_16M  12
#define PT_TOPA_SIZE_32M  13
#define PT_TOPA_SIZE_64M  14
#define PT_TOPA_SIZE_128M 15

//////////////////////////////////////////////////
//                 MSR Structures               //
//////////////////////////////////////////////////

/**
 * @brief IA32_RTIT_CTL — PT master control register
 * @details Intel SDM Vol. 3, Section 32.2.7.2
 */
typedef union _PT_RTIT_CTL_REGISTER
{
    struct
    {
        UINT64 TraceEn   : 1;  /* [0]     Enable tracing                         */
        UINT64 CycEn     : 1;  /* [1]     CYC packets                            */
        UINT64 Os        : 1;  /* [2]     Trace CPL 0                            */
        UINT64 User      : 1;  /* [3]     Trace CPL > 0                          */
        UINT64 PwrEvtEn  : 1;  /* [4]     Power event trace                      */
        UINT64 FupOnPtw  : 1;  /* [5]     FUP on PTWRITE                         */
        UINT64 FabricEn  : 1;  /* [6]     Trace to fabric (must be 0)            */
        UINT64 Cr3Filter : 1;  /* [7]     Filter by CR3                          */
        UINT64 ToPA      : 1;  /* [8]     Use ToPA output scheme                 */
        UINT64 MtcEn     : 1;  /* [9]     MTC packets                            */
        UINT64 TscEn     : 1;  /* [10]    TSC packets                            */
        UINT64 DisRetc   : 1;  /* [11]    Disable RET compression                */
        UINT64 PtwEn     : 1;  /* [12]    PTWRITE packets                        */
        UINT64 BranchEn  : 1;  /* [13]    Branch trace (TNT, TIP, FUP)           */
        UINT64 MtcFreq   : 4;  /* [14:17] MTC frequency                          */
        UINT64 Reserved0 : 1;  /* [18]    Must be 0                              */
        UINT64 CycThresh : 4;  /* [19:22] CYC threshold                          */
        UINT64 Reserved1 : 1;  /* [23]    Must be 0                              */
        UINT64 PsbFreq   : 4;  /* [24:27] PSB frequency                          */
        UINT64 Reserved2 : 4;  /* [28:31] Must be 0                              */
        UINT64 Addr0Cfg  : 4;  /* [32:35] Range 0 mode (1=filter / 2=stop)       */
        UINT64 Addr1Cfg  : 4;  /* [36:39] Range 1 mode                           */
        UINT64 Addr2Cfg  : 4;  /* [40:43] Range 2 mode                           */
        UINT64 Addr3Cfg  : 4;  /* [44:47] Range 3 mode                           */
        UINT64 Reserved3 : 16; /* [48:63] Must be 0                              */
    };
    UINT64 Value;
} PT_RTIT_CTL_REGISTER, *PPT_RTIT_CTL_REGISTER;

/**
 * @brief IA32_RTIT_STATUS — PT status / error register
 * @details Intel SDM Vol. 3, Section 32.2.7.4
 */
typedef union _PT_RTIT_STATUS_REGISTER
{
    struct
    {
        UINT64 FilterEn      : 1;  /* [0]     RO: IP filter allowing trace      */
        UINT64 ContextEn     : 1;  /* [1]     RO: Context (CR3) allowing trace   */
        UINT64 TriggerEn     : 1;  /* [2]     RO: Trigger conditions met        */
        UINT64 Reserved0     : 1;  /* [3]     Must be 0                         */
        UINT64 Error         : 1;  /* [4]     RO/Sticky: Operational error      */
        UINT64 Stopped       : 1;  /* [5]     RO: TraceStop hit                 */
        UINT64 PendTopaPmi   : 1;  /* [6]     RW: ToPA PMI pending — clear this */
        UINT64 PendPsbPmi    : 1;  /* [7]     RW: PSB+ PMI pending — clear this */
        UINT64 Reserved1     : 24; /* [8:31]  Must be 0                         */
        UINT64 PacketByteCnt : 17; /* [32:48] Bytes since last PSB              */
        UINT64 Reserved2     : 15; /* [49:63] Must be 0                         */
    };
    UINT64 Value;
} PT_RTIT_STATUS_REGISTER, *PPT_RTIT_STATUS_REGISTER;

/**
 * @brief IA32_RTIT_OUTPUT_MASK_PTRS — Output position tracker
 * @details Intel SDM Vol. 3, Section 32.2.7.8
 */
typedef union _PT_OUTPUT_MASK_PTRS_REGISTER
{
    struct
    {
        UINT64 LowerMask         : 7;  /* [0:6]   Forced to 0x7F               */
        UINT64 MaskOrTableOffset : 25; /* [7:31]  ToPA: table entry index       */
        UINT64 OutputOffset      : 32; /* [32:63] Byte offset in current entry  */
    };
    UINT64 Value;
} PT_OUTPUT_MASK_PTRS_REGISTER, *PPT_OUTPUT_MASK_PTRS_REGISTER;

/**
 * @brief ToPA Table Entry
 * @details Intel SDM Vol. 3, Section 32.2.7.2 (Table of Physical Addresses)
 */
typedef union _PT_TOPA_ENTRY
{
    struct
    {
        UINT64 End       : 1;  /* [0]     Last entry — wraps to next table  */
        UINT64 Reserved0 : 1;  /* [1]     Must be 0                         */
        UINT64 Int       : 1;  /* [2]     Generate PMI when region fills    */
        UINT64 Reserved1 : 1;  /* [3]     Must be 0                         */
        UINT64 Stop      : 1;  /* [4]     Stop tracing when region fills    */
        UINT64 Reserved2 : 1;  /* [5]     Must be 0                         */
        UINT64 Size      : 4;  /* [6:9]   Region size (4K*2^N)              */
        UINT64 Reserved3 : 2;  /* [10:11] Must be 0                         */
        UINT64 BaseAddr  : 36; /* [12:47] Physical address >> 12            */
        UINT64 Reserved4 : 16; /* [48:63] Must be 0                         */
    };
    UINT64 Value;
} PT_TOPA_ENTRY, *PPT_TOPA_ENTRY;

//////////////////////////////////////////////////
//             Capability / Config              //
//////////////////////////////////////////////////

/**
 * @brief Discovered Intel PT capabilities (populated from CPUID leaf 0x14).
 */
typedef struct _PT_CAPABILITIES
{
    UINT32 Cr3Filtering       : 1; /* Can filter by process CR3            */
    UINT32 PsbCycConfigurable : 1; /* PSBFreq and CYC configurable         */
    UINT32 IpFiltering        : 1; /* IP filtering and TraceStop supported */
    UINT32 MtcSupport         : 1; /* MTC packets supported                */
    UINT32 PtwriteSupport     : 1; /* PTWRITE instruction supported        */
    UINT32 PowerEventTrace    : 1; /* Power event trace supported          */
    UINT32 VmxSupport         : 1; /* PT works in VMX operations           */
    UINT32 TopaOutput         : 1; /* ToPA output scheme supported         */
    UINT32 TopaMultiEntry     : 1; /* ToPA tables can have >1 entry        */
    UINT32 SingleRangeOutput  : 1; /* Single contiguous range output       */
    UINT32 TransportOutput    : 1; /* Trace transport subsystem output     */
    UINT32 IpPayloadsAreLip   : 1; /* IP payloads are LIP (not RIP)        */
    UINT32 Reserved           : 20;

    UINT32 NumAddrRanges;       /* Number of ADDRn_CFG pairs (0-4)         */
    UINT16 MtcPeriodBitmap;     /* Supported MTC period values             */
    UINT16 CycThresholdBitmap;  /* Supported CYC threshold values          */
    UINT16 PsbFreqBitmap;       /* Supported PSB frequency values          */

} PT_CAPABILITIES, *PPT_CAPABILITIES;

/**
 * @brief Intel PT trace state machine.
 */
typedef enum _PT_STATE
{
    PT_STATE_DISABLED = 0, /* No buffers allocated, PT off                */
    PT_STATE_READY,        /* Buffers allocated, MSRs not yet programmed  */
    PT_STATE_TRACING,      /* TraceEn=1, actively generating packets      */
    PT_STATE_PAUSED,       /* TraceEn=0 temporarily (PMI or user pause)   */
    PT_STATE_STOPPED,      /* Tracing done, buffer has valid data         */
    PT_STATE_ERROR         /* Hardware error (check IA32_RTIT_STATUS)     */
} PT_STATE;

/**
 * @brief Intel PT IP filter range.
 */
typedef struct _PT_ADDR_RANGE
{
    UINT64  Start;       /* Start virtual address — written to ADDRn_A       */
    UINT64  End;         /* End virtual address — written to ADDRn_B         */
    BOOLEAN IsStopRange; /* FALSE = filter (only trace inside range)
                            TRUE  = trace-stop (stop when entering range)    */
} PT_ADDR_RANGE, *PPT_ADDR_RANGE;

/**
 * @brief Intel PT trace configuration — what the user specifies.
 */
typedef struct _PT_TRACE_CONFIG
{
    //
    // What to trace
    //
    BOOLEAN TraceUser;   /* Trace CPL 3 (user mode)              */
    BOOLEAN TraceKernel; /* Trace CPL 0 (kernel mode)            */
    UINT64  TargetCr3;   /* Process to trace (0 = trace all)     */

    //
    // IP filtering
    //
    UINT32        NumAddrRanges; /* 0-4 active ranges            */
    PT_ADDR_RANGE AddrRanges[PT_MAX_ADDR_RANGES];

    //
    // Packet options
    //
    BOOLEAN EnableBranch;         /* BranchEn — must be TRUE for useful traces  */
    BOOLEAN EnableTsc;            /* TscEn — timestamps at PSB boundaries       */
    BOOLEAN EnableMtc;            /* MtcEn — wall-clock timing packets          */
    BOOLEAN EnableCyc;            /* CycEn — cycle-accurate packets             */
    BOOLEAN EnableRetCompression; /* TRUE keeps RET compression                 */
    UINT8   MtcFreq;              /* 0-15: MTC period (must be in capabilities) */
    UINT8   CycThresh;            /* 0-15: CYC threshold                        */
    UINT8   PsbFreq;              /* 0-15: PSB frequency (0 = every 2KB)        */

    //
    // Buffer size
    //
    UINT64 BufferSize; /* Main output buffer size in bytes
                          Must be 4KB * 2^N (4KB, 8KB, ..., 128MB)
                          Default: PT_DEFAULT_BUFFER_SIZE (2MB)      */
} PT_TRACE_CONFIG, *PPT_TRACE_CONFIG;

//////////////////////////////////////////////////
//             Per-CPU Trace State              //
//////////////////////////////////////////////////

/**
 * @brief Per-CPU PT buffer layout
 *
 * ToPA Table (one 4KB page, 3 entries used):
 *   Entry[0]  — Main data buffer (BufferSize), INT=1
 *   Entry[1]  — Overflow zone (4KB), INT=0
 *   Entry[2]  — END, points back to ToPA table (circular)
 */
typedef struct _PT_BUFFER
{
    //
    // ToPA table
    //
    PT_TOPA_ENTRY * TopaVa;       /* Virtual addr of the ToPA table          */
    UINT64          TopaPhysical; /* Physical addr — IA32_RTIT_OUTPUT_BASE   */

    //
    // Main output buffer
    //
    PVOID  OutputVa;       /* Virtual addr — read trace data from here      */
    UINT64 OutputPhysical; /* Physical addr — goes into ToPA entry[0]       */
    UINT64 OutputSize;     /* Bytes (e.g., 0x200000 for 2MB)                */

    //
    // Overflow landing zone (4KB)
    //
    PVOID  OverflowVa;       /* Virtual addr of overflow page               */
    UINT64 OverflowPhysical; /* Physical addr — ToPA entry[1]               */

} PT_BUFFER, *PPT_BUFFER;

/**
 * @brief Per-CPU Intel PT state — one of these per logical processor.
 */
typedef struct _PT_PER_CPU
{
    PT_BUFFER            Buffer;             /* ToPA + output buffers          */
    PT_RTIT_CTL_REGISTER SavedCtl;           /* Last written IA32_RTIT_CTL     */
    PT_TRACE_CONFIG      Config;             /* Current trace configuration    */
    PT_STATE             State;              /* Current state machine position */
    UINT64               TotalBytesCaptured; /* Accumulated across PMI events  */

} PT_PER_CPU, *PPT_PER_CPU;

//////////////////////////////////////////////////
//             Trace Output Descriptor          //
//////////////////////////////////////////////////

/**
 * @brief Trace output descriptor
 *
 * Passed to the engine to receive trace data. WriteOffset serves dual purpose:
 *   - Input:  where in Buffer to start writing new data.
 *   - Output: updated to Buffer[0..WriteOffset) = valid data after the call.
 *
 * If the remaining space (Length - WriteOffset) is smaller than the new data,
 * the copy is skipped and WriteOffset is not updated.
 * Pass NULL instead of a PT_OUTPUT_BUFFER * to skip copying entirely.
 */
typedef struct _PT_OUTPUT_BUFFER
{
    PVOID  Buffer;      /* Destination buffer for trace data              */
    UINT64 Length;      /* Total size of Buffer in bytes                  */
    UINT64 WriteOffset; /* Next write position; valid data is [0..WriteOffset) */

} PT_OUTPUT_BUFFER, *PPT_OUTPUT_BUFFER;

//////////////////////////////////////////////////
//          User-mode mmap descriptor           //
//////////////////////////////////////////////////

/**
 * @brief One per-CPU descriptor returned by the PT mmap surface.
 *
 *        The main output buffer and the 4 KB overflow page are stitched
 *        into a single virtually contiguous region in the calling user
 *        process — main first, then overflow, matching the order PT
 *        writes them on a ToPA PMI. Consumers read the whole stream
 *        as Size bytes starting at UserVa.
 *
 *        UserVa is valid only in the address space of the process that
 *        issued the mmap IOCTL, and only until PT is disabled / flushed
 *        (at which point the underlying kernel buffers are torn down).
 */
typedef struct _PT_USER_BUFFER_DESC
{
    UINT32 CpuId;
    UINT32 Reserved;
    UINT64 UserVa; /* base of the combined main + overflow mapping */
    UINT64 Size;   /* total bytes in the mapping (main + overflow) */

} PT_USER_BUFFER_DESC, *PPT_USER_BUFFER_DESC;
