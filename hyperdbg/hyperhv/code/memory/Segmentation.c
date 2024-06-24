/**
 * @file Segmentation.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for handling memory segmentations
 *
 * @version 0.9
 * @date 2024-06-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Get Segment Descriptor
 *
 * @param SegmentSelector
 * @param Selector
 * @param GdtBase
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
SegmentGetDescriptor(PUCHAR                GdtBase,
                     UINT16                Selector,
                     PVMX_SEGMENT_SELECTOR SegmentSelector)
{
    SEGMENT_DESCRIPTOR_32 * DescriptorTable32;
    SEGMENT_DESCRIPTOR_32 * Descriptor32;
    SEGMENT_SELECTOR        SegSelector = {.AsUInt = Selector};

    if (!SegmentSelector)
        return FALSE;

#define SELECTOR_TABLE_LDT 0x1
#define SELECTOR_TABLE_GDT 0x0

    //
    // Ignore LDT
    //
    if ((Selector == 0x0) || (SegSelector.Table != SELECTOR_TABLE_GDT))
    {
        return FALSE;
    }

    DescriptorTable32 = (SEGMENT_DESCRIPTOR_32 *)(GdtBase);
    Descriptor32      = &DescriptorTable32[SegSelector.Index];

    SegmentSelector->Selector = Selector;
    SegmentSelector->Limit    = __segmentlimit(Selector);
    SegmentSelector->Base     = ((UINT64)Descriptor32->BaseAddressLow | (UINT64)Descriptor32->BaseAddressMiddle << 16 | (UINT64)Descriptor32->BaseAddressHigh << 24);

    SegmentSelector->Attributes.AsUInt = (AsmGetAccessRights(Selector) >> 8);

    if (SegSelector.Table == 0 && SegSelector.Index == 0)
    {
        SegmentSelector->Attributes.Unusable = TRUE;
    }

    if ((Descriptor32->Type == SEGMENT_DESCRIPTOR_TYPE_TSS_BUSY) || (Descriptor32->Type == SEGMENT_DESCRIPTOR_TYPE_CALL_GATE))
    {
        //
        // this is a TSS or callgate etc, save the base high part
        //

        UINT64 SegmentLimitHigh;
        SegmentLimitHigh      = (*(UINT64 *)((PUCHAR)Descriptor32 + 8));
        SegmentSelector->Base = (SegmentSelector->Base & 0xffffffff) | (SegmentLimitHigh << 32);
    }

    if (SegmentSelector->Attributes.Granularity)
    {
        //
        // 4096-bit granularity is enabled for this segment, scale the limit
        //
        SegmentSelector->Limit = (SegmentSelector->Limit << 12) + 0xfff;
    }

    return TRUE;
}

/**
 * @brief Initialize the host GDT
 *
 * @param OsGdtBase
 * @param OsGdtLimit
 * @param TrSelector
 * @param HostStack
 * @param AllocatedHostGdt
 * @param AllocatedHostTss
 *
 * @return VOID
 */
VOID
SegmentPrepareHostGdt(
    SEGMENT_DESCRIPTOR_32 * OsGdtBase,
    UINT16                  OsGdtLimit,
    UINT16                  TrSelector,
    UINT64                  HostStack,
    SEGMENT_DESCRIPTOR_32 * AllocatedHostGdt,
    TASK_STATE_SEGMENT_64 * AllocatedHostTss)
{
    //
    // Copy current OS GDT into host GDT
    // Note that the limit is the maximum addressable byte offset within the segment,
    // so the actual size of the GDT is limit + 1
    //
    RtlCopyBytes(AllocatedHostGdt, OsGdtBase, OsGdtLimit + 1);

    //
    // Make sure host TSS is empty
    //
    RtlZeroBytes(AllocatedHostTss, sizeof(TASK_STATE_SEGMENT_64));

#if USE_INTERRUPT_STACK_TABLE == TRUE

    UINT64 EndOfStack = 0;

    //
    // Setup TSS memory for host (same host stack is used for all interrupts and privilege levels)
    //
    EndOfStack = ((UINT64)HostStack + HOST_INTERRUPT_STACK_SIZE - 1);
    EndOfStack = ((UINT64)((ULONG_PTR)(EndOfStack) & ~(16 - 1)));

    LogDebugInfo("Host Interrupt Stack, from: %llx, to: %llx", HostStack, EndOfStack);

    AllocatedHostTss->Rsp0 = EndOfStack;
    AllocatedHostTss->Rsp1 = EndOfStack;
    AllocatedHostTss->Rsp2 = EndOfStack;
    AllocatedHostTss->Ist1 = EndOfStack;
    AllocatedHostTss->Ist2 = EndOfStack;
    AllocatedHostTss->Ist3 = EndOfStack;
    AllocatedHostTss->Ist4 = EndOfStack;
    AllocatedHostTss->Ist5 = EndOfStack;
    AllocatedHostTss->Ist6 = EndOfStack;
    AllocatedHostTss->Ist7 = EndOfStack;

#else

    UNREFERENCED_PARAMETER(HostStack);

#endif // USE_INTERRUPT_STACK_TABLE == TRUE

    //
    // Setup the TSS segment descriptor
    //
    SEGMENT_DESCRIPTOR_64 * GdtTssDesc = (SEGMENT_DESCRIPTOR_64 *)&AllocatedHostGdt[TrSelector];

    //
    // Point the TSS descriptor to our TSS
    //
    UINT64 Base                   = (UINT64)AllocatedHostTss;
    GdtTssDesc->BaseAddressLow    = (Base >> 00) & 0xFFFF;
    GdtTssDesc->BaseAddressMiddle = (Base >> 16) & 0xFF;
    GdtTssDesc->BaseAddressHigh   = (Base >> 24) & 0xFF;
    GdtTssDesc->BaseAddressUpper  = (Base >> 32) & 0xFFFFFFFF;

    ////////////////////////////////////////////////////////////////////

    // SEGMENT_SELECTOR HostCsSelector = {0, 0, 1};
    // SEGMENT_SELECTOR HostTrSelector = {0, 0, 2};
    //
    // //
    // // Setup the CS segment descriptor
    // //
    // SEGMENT_DESCRIPTOR_32 CsDesc    = AllocatedHostGdt[HostCsSelector.Index];
    // CsDesc.Type                     = SEGMENT_DESCRIPTOR_TYPE_CODE_EXECUTE_READ;
    // CsDesc.DescriptorType           = SEGMENT_DESCRIPTOR_TYPE_CODE_OR_DATA;
    // CsDesc.DescriptorPrivilegeLevel = 0;
    // CsDesc.Present                  = 1;
    // CsDesc.LongMode                 = 1;
    // CsDesc.DefaultBig               = 0;
    // CsDesc.Granularity              = 0;
    //
    // //
    // // Setup the TSS segment descriptor
    // //
    // SEGMENT_DESCRIPTOR_64 * TssDesc   = (SEGMENT_DESCRIPTOR_64 *)&AllocatedHostGdt[HostTrSelector.Index];
    // TssDesc->Type                     = SEGMENT_DESCRIPTOR_TYPE_TSS_BUSY;
    // TssDesc->DescriptorType           = SEGMENT_DESCRIPTOR_TYPE_SYSTEM;
    // TssDesc->DescriptorPrivilegeLevel = 0;
    // TssDesc->Present                  = 1;
    // TssDesc->Granularity              = 0;
    // TssDesc->SegmentLimitLow          = 0x67;
    // TssDesc->SegmentLimitHigh         = 0;
    //
    // //
    // // Point the TSS descriptor to our TSS
    // //
    // UINT64 Base                = (UINT64)AllocatedHostTss;
    // TssDesc->BaseAddressLow    = (Base >> 00) & 0xFFFF;
    // TssDesc->BaseAddressMiddle = (Base >> 16) & 0xFF;
    // TssDesc->BaseAddressHigh   = (Base >> 24) & 0xFF;
    // TssDesc->BaseAddressUpper  = (Base >> 32) & 0xFFFFFFFF;
}
