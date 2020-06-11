/**
 * @file SegmentRegs.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Segment Registers
 * @details
 * @version 0.1
 * @date 2020-06-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

/**
 * @brief Get the Guest Cs Selector
 * 
 * @return VOID 
 */

#include "Vmx.h"


SEGMENT_SELECTOR
GetGuestCs()
{
    SEGMENT_SELECTOR Cs;

    __vmx_vmread(GUEST_CS_BASE, &Cs.BASE);
    __vmx_vmread(GUEST_CS_LIMIT, &Cs.LIMIT);
    __vmx_vmread(GUEST_CS_AR_BYTES, &Cs.ATTRIBUTES.UCHARs);
    __vmx_vmread(GUEST_CS_SELECTOR, &Cs.SEL);

    return Cs;
}

/**
 * @brief Set the Guest Cs selector
 * 
 * @param Cs The CS Selector for the guest
 * @return VOID 
 */
VOID
SetGuestCs(PSEGMENT_SELECTOR Cs)
{
    __vmx_vmwrite(GUEST_CS_BASE, Cs->BASE);
    __vmx_vmwrite(GUEST_CS_LIMIT, Cs->LIMIT);
    __vmx_vmwrite(GUEST_CS_AR_BYTES, Cs->ATTRIBUTES.UCHARs);
    __vmx_vmwrite(GUEST_CS_SELECTOR, Cs->SEL);
}

/**
 * @brief Set the Guest Ss selector
 * 
 * @param Ss The SS Selector for the guest
 * @return VOID 
 */
VOID
SetGuestSs(PSEGMENT_SELECTOR Ss)
{
    __vmx_vmwrite(GUEST_SS_BASE, Ss->BASE);
    __vmx_vmwrite(GUEST_SS_LIMIT, Ss->LIMIT);
    __vmx_vmwrite(GUEST_SS_AR_BYTES, Ss->ATTRIBUTES.UCHARs);
    __vmx_vmwrite(GUEST_SS_SELECTOR, Ss->SEL);
}
