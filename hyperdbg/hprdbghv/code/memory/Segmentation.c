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
SegmentGetDescriptor(PUCHAR GdtBase, UINT16 Selector, PVMX_SEGMENT_SELECTOR SegmentSelector)
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
