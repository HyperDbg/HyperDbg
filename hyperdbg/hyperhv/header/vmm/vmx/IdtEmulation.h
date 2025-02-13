/**
 * @file IdtEmulation.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for Handlers of Guest's IDT Emulator
 * @details
 * @version 0.1
 * @date 2020-06-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				     Constants		      		//
//////////////////////////////////////////////////

/**
 * @brief Whether the hypervisor should use the default OS's
 * IDT as the host IDT in VMCS or not
 *
 */
#define USE_DEFAULT_OS_IDT_AS_HOST_IDT FALSE

/**
 * @brief Maximum number of interrupt entries in IDT
 *
 */
#define HOST_IDT_DESCRIPTOR_COUNT 256

//////////////////////////////////////////////////
//				     Structures		      		//
//////////////////////////////////////////////////

#pragma pack(push, 1)

typedef struct _SIDT_ENTRY
{
    USHORT    IdtLimit;
    ULONG_PTR IdtBase;
} SIDT_ENTRY, *PSIDT_ENTRY;

typedef struct _KIDT_ENTRY
{
    ULONG LowPart : 16;
    ULONG SegmentSelector : 16;
    ULONG Reserved1 : 5;
    ULONG Reserved2 : 3;
    ULONG Type : 3;
    ULONG Size : 1;
    ULONG Reserved3 : 1;
    ULONG Dpl : 2;
    ULONG Present : 1;
    ULONG HighPart : 16;
#if defined _M_AMD64
    ULONG HighestPart;
    ULONG Reserved;
#endif
} KIDT_ENTRY, *PKIDT_ENTRY;

#pragma pack(pop)

/**
 * @brief Trap frame for interrupts
 *
 */
typedef struct _INTERRUPT_TRAP_FRAME
{
    //
    // general-purpose registers
    //
    union
    {
        UINT64 rax;
        UINT32 eax;
        UINT16 ax;
        UINT8  al;
    };
    union
    {
        UINT64 rcx;
        UINT32 ecx;
        UINT16 cx;
        UINT8  cl;
    };
    union
    {
        UINT64 rdx;
        UINT32 edx;
        UINT16 dx;
        UINT8  dl;
    };
    union
    {
        UINT64 rbx;
        UINT32 ebx;
        UINT16 bx;
        UINT8  bl;
    };
    union
    {
        UINT64 rbp;
        UINT32 ebp;
        UINT16 bp;
        UINT8  bpl;
    };
    union
    {
        UINT64 rsi;
        UINT32 esi;
        UINT16 si;
        UINT8  sil;
    };
    union
    {
        UINT64 rdi;
        UINT32 edi;
        UINT16 di;
        UINT8  dil;
    };
    union
    {
        UINT64 r8;
        UINT32 r8d;
        UINT16 r8w;
        UINT8  r8b;
    };
    union
    {
        UINT64 r9;
        UINT32 r9d;
        UINT16 r9w;
        UINT8  r9b;
    };
    union
    {
        UINT64 r10;
        UINT32 r10d;
        UINT16 r10w;
        UINT8  r10b;
    };
    union
    {
        UINT64 r11;
        UINT32 r11d;
        UINT16 r11w;
        UINT8  r11b;
    };
    union
    {
        UINT64 r12;
        UINT32 r12d;
        UINT16 r12w;
        UINT8  r12b;
    };
    union
    {
        UINT64 r13;
        UINT32 r13d;
        UINT16 r13w;
        UINT8  r13b;
    };
    union
    {
        UINT64 r14;
        UINT32 r14d;
        UINT16 r14w;
        UINT8  r14b;
    };
    union
    {
        UINT64 r15;
        UINT32 r15d;
        UINT16 r15w;
        UINT8  r15b;
    };

    // interrupt vector
    UINT8 vector;

    // _MACHINE_FRAME
    UINT64 error;
    UINT64 rip;
    UINT64 cs;
    UINT64 rflags;
    UINT64 rsp;
    UINT64 ss;
} INTERRUPT_TRAP_FRAME, *PINTERRUPT_TRAP_FRAME;

//
// remember to update this value in AsmInterruptHandlers.asm
//
static_assert(sizeof(INTERRUPT_TRAP_FRAME) == (0x78 + 0x38), "Size of INTERRUPT_TRAP_FRAME is not as expected");

/**
 * @brief Filled out when a host exception occurs
 *
 */
typedef struct _HOST_EXCEPTION_INFO
{
    //
    // whether an exception occurred or not
    //
    BOOLEAN ExceptionOccurred;

    //
    // interrupt vector
    //
    UINT64 Vector;

    //
    // error code
    //
    UINT64 Error;
} HOST_EXCEPTION_INFO, *PHOST_EXCEPTION_INFO;

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
IdtEmulationPrepareHostIdt(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);

VOID
IdtEmulationHandleExceptionAndNmi(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                  _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit);

VOID
IdtEmulationHandleExternalInterrupt(_Inout_ VIRTUAL_MACHINE_STATE *   VCpu,
                                    _In_ VMEXIT_INTERRUPT_INFORMATION InterruptExit);

VOID
IdtEmulationHandleNmiWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);

VOID
IdtEmulationHandleInterruptWindowExiting(_Inout_ VIRTUAL_MACHINE_STATE * VCpu);

VOID
IdtEmulationQueryIdtEntriesRequest(PINTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS IdtQueryRequest,
                                   BOOLEAN                                     ReadFromVmxRoot);
