/**
 * @file Apic.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating to Advanced Programmable Interrupt Controller (APIC)
 * @details Some of the constants are copied from KVM project
 * @version 0.1
 * @date 2020-12-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Definition					//
//////////////////////////////////////////////////

#define X2_MSR_BASE 0x800
#define ICROffset   0x300
#define TO_X2(x)    (x / 0x10)

#define APIC_DEFAULT_PHYS_BASE 0xfee00000
#define APIC_BSP               (1UL << 8)
#define APIC_EXTD              (1UL << 10)
#define APIC_EN                (1UL << 11)

#define APIC_LVR            0x30
#define APIC_LVR_MASK       0xFF00FF
#define GET_APIC_VERSION(x) ((x) & 0xFFu)
#define GET_APIC_MAXLVT(x)  (((x) >> 16) & 0xFFu)

#define APIC_INTEGRATED(x) (1)
#define APIC_XAPIC(x)      ((x) >= 0x14)
#define APIC_TASKPRI       0x80
#define APIC_TPRI_MASK     0xFFu
#define APIC_ARBPRI        0x90
#define APIC_ARBPRI_MASK   0xFFu
#define APIC_PROCPRI       0xA0

#define APIC_EIO_ACK                 0x0
#define APIC_RRR                     0xC0
#define APIC_LDR                     0xD0
#define APIC_LDR_MASK                (0xFFu << 24)
#define GET_APIC_LOGICAL_ID(x)       (((x) >> 24) & 0xFFu)
#define SET_APIC_LOGICAL_ID(x)       (((x) << 24))
#define APIC_ALL_CPUS                0xFFu
#define APIC_DFR                     0xE0
#define APIC_DFR_CLUSTER             0x0FFFFFFFul
#define APIC_DFR_FLAT                0xFFFFFFFFul
#define APIC_SPIV                    0xF0
#define APIC_SPIV_FOCUS_DISABLED     (1 << 9)
#define APIC_SPIV_APIC_ENABLED       (1 << 8)
#define APIC_ISR                     0x100
#define APIC_ISR_NR                  0x8 /* Number of 32 bit ISR registers. */
#define APIC_TMR                     0x180
#define APIC_IRR                     0x200
#define APIC_ESR                     0x280
#define APIC_ESR_SEND_CS             0x00001
#define APIC_ESR_RECV_CS             0x00002
#define APIC_ESR_SEND_ACC            0x00004
#define APIC_ESR_RECV_ACC            0x00008
#define APIC_ESR_SENDILL             0x00020
#define APIC_ESR_RECVILL             0x00040
#define APIC_ESR_ILLREGA             0x00080
#define APIC_CMCI                    0x2F0
#define APIC_ICR                     0x300
#define APIC_DEST_SELF               0x40000
#define APIC_DEST_ALLINC             0x80000
#define APIC_DEST_ALLBUT             0xC0000
#define APIC_ICR_RR_MASK             0x30000
#define APIC_ICR_RR_INVALID          0x00000
#define APIC_ICR_RR_INPROG           0x10000
#define APIC_ICR_RR_VALID            0x20000
#define APIC_INT_LEVELTRIG           0x08000
#define APIC_INT_ASSERT              0x04000
#define APIC_ICR_BUSY                0x01000
#define APIC_DEST_LOGICAL            0x00800
#define APIC_DEST_PHYSICAL           0x00000
#define APIC_DM_FIXED                0x00000
#define APIC_DM_LOWEST               0x00100
#define APIC_DM_SMI                  0x00200
#define APIC_DM_REMRD                0x00300
#define APIC_DM_NMI                  0x00400
#define APIC_DM_INIT                 0x00500
#define APIC_DM_STARTUP              0x00600
#define APIC_DM_EXTINT               0x00700
#define APIC_VECTOR_MASK             0x000FF
#define APIC_ICR2                    0x310
#define GET_APIC_DEST_FIELD(x)       (((x) >> 24) & 0xFF)
#define SET_APIC_DEST_FIELD(x)       ((x) << 24)
#define APIC_LVTT                    0x320
#define APIC_LVTTHMR                 0x330
#define APIC_LVTPC                   0x340
#define APIC_LVT0                    0x350
#define APIC_LVT_TIMER_BASE_MASK     (0x3 << 18)
#define GET_APIC_TIMER_BASE(x)       (((x) >> 18) & 0x3)
#define SET_APIC_TIMER_BASE(x)       (((x) << 18))
#define APIC_TIMER_BASE_CLKIN        0x0
#define APIC_TIMER_BASE_TMBASE       0x1
#define APIC_TIMER_BASE_DIV          0x2
#define APIC_LVT_TIMER_MASK          (3 << 17)
#define APIC_LVT_TIMER_ONESHOT       (0 << 17)
#define APIC_LVT_TIMER_PERIODIC      (1 << 17)
#define APIC_LVT_TIMER_TSCDEADLINE   (2 << 17)
#define APIC_LVT_MASKED              (1 << 16)
#define APIC_LVT_LEVEL_TRIGGER       (1 << 15)
#define APIC_LVT_REMOTE_IRR          (1 << 14)
#define APIC_INPUT_POLARITY          (1 << 13)
#define APIC_SEND_PENDING            (1 << 12)
#define APIC_MODE_MASK               0x700
#define GET_APIC_DELIVERY_MODE(x)    (((x) >> 8) & 0x7)
#define SET_APIC_DELIVERY_MODE(x, y) (((x) & ~0x700) | ((y) << 8))
#define APIC_MODE_FIXED              0x0
#define APIC_MODE_NMI                0x4
#define APIC_MODE_EXTINT             0x7
#define APIC_LVT1                    0x360
#define APIC_LVTERR                  0x370
#define APIC_TMICT                   0x380
#define APIC_TMCCT                   0x390
#define APIC_TDCR                    0x3E0
#define APIC_SELF_IPI                0x3F0
#define APIC_TDR_DIV_TMBASE          (1 << 2)
#define APIC_TDR_DIV_1               0xB
#define APIC_TDR_DIV_2               0x0
#define APIC_TDR_DIV_4               0x1
#define APIC_TDR_DIV_8               0x2
#define APIC_TDR_DIV_16              0x3
#define APIC_TDR_DIV_32              0x8
#define APIC_TDR_DIV_64              0x9
#define APIC_TDR_DIV_128             0xA
#define APIC_EILVT0                  0x500
#define APIC_EILVT_NR_AMD_K8         1 /* # of extended interrupts */
#define APIC_EILVT_NR_AMD_10H        4
#define APIC_EILVT_LVTOFF(x)         (((x) >> 4) & 0xF)
#define APIC_EILVT_MSG_FIX           0x0
#define APIC_EILVT_MSG_SMI           0x2
#define APIC_EILVT_MSG_NMI           0x4
#define APIC_EILVT_MSG_EXT           0x7
#define APIC_EILVT_MASKED            (1 << 16)
#define APIC_EILVT1                  0x510
#define APIC_EILVT2                  0x520
#define APIC_EILVT3                  0x530

#define APIC_BASE_MSR 0x800

#define LAPIC_LVT_FLAG_ENTRY_MASKED     (1UL << 16)
#define LAPIC_LVT_DELIVERY_MODE_EXT_INT (7UL << 8)
#define LAPIC_SVR_FLAG_SW_ENABLE        (1UL << 8)

/**
 * @brief LAPIC structure size
 */
#define LAPIC_SIZE 0x400

/**
 * @brief LAPIC structure and offsets
 */
typedef struct _LAPIC_PAGE
{
    UINT8 Reserved000[0x10];
    UINT8 Reserved010[0x10];

    UINT32 Id; // offset 0x020
    UINT8  Reserved024[0x0C];

    UINT32 Version; // offset 0x030
    UINT8  Reserved034[0x0C];

    UINT8 Reserved040[0x40];

    UINT32 TPR; // offset 0x080
    UINT8  Reserved084[0x0C];

    UINT32 ArbitrationPriority; // offset 0x090
    UINT8  Reserved094[0x0C];

    UINT32 ProcessorPriority; // offset 0x0A0
    UINT8  Reserved0A4[0x0C];

    UINT32 EOI; // offset 0x0B0
    UINT8  Reserved0B4[0x0C];

    UINT32 RemoteRead; // offset 0x0C0
    UINT8  Reserved0C4[0x0C];

    UINT32 LogicalDestination; // offset 0x0D0
    UINT8  Reserved0D4[0x0C];

    UINT32 DestinationFormat; // offset 0x0E0
    UINT8  Reserved0E4[0x0C];

    UINT32 SpuriousInterruptVector; // offset 0x0F0
    UINT8  Reserved0F4[0x0C];

    UINT32 ISR[32]; // offset 0x100

    UINT32 TMR[32]; // offset 0x180

    UINT32 IRR[32]; // offset 0x200

    UINT32 ErrorStatus; // offset 0x280
    UINT8  Reserved284[0x0C];

    UINT8 Reserved290[0x60];

    UINT32 LvtCmci; // offset 0x2F0
    UINT8  Reserved2F4[0x0C];

    UINT32 IcrLow; // offset 0x300
    UINT8  Reserved304[0x0C];

    UINT32 IcrHigh; // offset 0x310
    UINT8  Reserved314[0x0C];

    UINT32 LvtTimer; // offset 0x320
    UINT8  Reserved324[0x0C];

    UINT32 LvtThermalSensor; // offset 0x330
    UINT8  Reserved334[0x0C];

    UINT32 LvtPerfMonCounters; // offset 0x340
    UINT8  Reserved344[0x0C];

    UINT32 LvtLINT0; // offset 0x350
    UINT8  Reserved354[0x0C];

    UINT32 LvtLINT1; // offset 0x360
    UINT8  Reserved364[0x0C];

    UINT32 LvtError; // offset 0x370
    UINT8  Reserved374[0x0C];

    UINT32 InitialCount; // offset 0x380
    UINT8  Reserved384[0x0C];

    UINT32 CurrentCount; // offset 0x390
    UINT8  Reserved394[0x0C];

    UINT8 Reserved3A0[0x40]; // offset 0x3A0

    UINT32 DivideConfiguration; // offset 0x3E0
    UINT8  Reserved3E4[0x0C];

    UINT32 SelfIpi;           // offset 0x3F0
    UINT8  Reserved3F4[0x0C]; // valid only for X2APIC
} LAPIC_PAGE, *PLAPIC_PAGE;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
ApicInitialize();

VOID
ApicUninitialize();

VOID
ApicShowDetails();

VOID
ApicSelfIpi(UINT32 Vector);

VOID
ApicTriggerGenericNmi();
