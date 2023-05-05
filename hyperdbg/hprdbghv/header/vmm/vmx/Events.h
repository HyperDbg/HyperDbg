/**
 * @file Events.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating to Exception Bitmap and Event (Interrupt and Exception) Injection
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Definitions					//
//////////////////////////////////////////////////

#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x4000109F

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

/**
 * @brief Type of interrupts
 *
 */
typedef enum _INTERRUPT_TYPE
{
    INTERRUPT_TYPE_EXTERNAL_INTERRUPT            = 0,
    INTERRUPT_TYPE_RESERVED                      = 1,
    INTERRUPT_TYPE_NMI                           = 2,
    INTERRUPT_TYPE_HARDWARE_EXCEPTION            = 3,
    INTERRUPT_TYPE_SOFTWARE_INTERRUPT            = 4,
    INTERRUPT_TYPE_PRIVILEGED_SOFTWARE_INTERRUPT = 5,
    INTERRUPT_TYPE_SOFTWARE_EXCEPTION            = 6,
    INTERRUPT_TYPE_OTHER_EVENT                   = 7
} INTERRUPT_TYPE;

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Interrupt injection and event format
 *
 */
typedef union _INTERRUPT_INFO
{
    struct
    {
        UINT32 Vector : 8;
        /* 0=Ext Int, 1=Rsvd, 2=NMI, 3=Exception, 4=Soft INT,
         * 5=Priv Soft Trap, 6=Unpriv Soft Trap, 7=Other */
        UINT32 InterruptType : 3;
        UINT32 DeliverCode : 1; /* 0=Do not deliver, 1=Deliver */
        UINT32 Reserved : 19;
        UINT32 Valid : 1;       /* 0=Not valid, 1=Valid. Must be checked first */
    } Fields;
    UINT32 Flags;
} INTERRUPT_INFO, *PINTERRUPT_INFO;

/**
 * @brief Event information
 *
 */
typedef struct _EVENT_INFORMATION
{
    INTERRUPT_INFO InterruptInfo;
    UINT32         InstructionLength;
    UINT64         ErrorCode;
} EVENT_INFORMATION, *PEVENT_INFORMATION;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
EventInjectInterruptOrException(_In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit);

VOID
EventInjectBreakpoint();

VOID
EventInjectInterruption(INTERRUPT_TYPE InterruptionType, EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode, UINT32 ErrorCode);

VOID
EventInjectGeneralProtection();

VOID
EventInjectUndefinedOpcode(VIRTUAL_MACHINE_STATE * VCpu);

VOID
EventInjectPageFault(UINT64 PageFaultAddress);

VOID
EventInjectDebugBreakpoint();

VOID
EventInjectPageFaultWithCr2(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Address);
