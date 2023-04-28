/**
 * @file Vpid.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VPID Implementations
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief INVVPID Instruction
 *
 * @param Type
 * @param Descriptor
 * @return VOID
 */
_Use_decl_annotations_
VOID
VpidInvvpid(INVVPID_TYPE Type, INVVPID_DESCRIPTOR * Descriptor)
{
    INVVPID_DESCRIPTOR * TargetDescriptor = NULL;
    INVVPID_DESCRIPTOR   ZeroDescriptor   = {0};

    if (!Descriptor)
    {
        TargetDescriptor = &ZeroDescriptor;
    }
    else
    {
        TargetDescriptor = Descriptor;
    }

    AsmInvvpid(Type, TargetDescriptor);
}

/**
 * @brief INVVPID instruction to invalidate a special address
 *
 * @param Vpid
 * @param LinearAddress
 * @return VOID
 */
_Use_decl_annotations_
VOID
VpidInvvpidIndividualAddress(UINT16 Vpid, UINT64 LinearAddress)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, LinearAddress};
    VpidInvvpid(InvvpidIndividualAddress, &Descriptor);
}

/**
 * @brief INVVPID Single Context
 *
 * @param Vpid
 * @return VOID
 */
VOID
VpidInvvpidSingleContext(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, 0};
    VpidInvvpid(InvvpidSingleContext, &Descriptor);
}

/**
 * @brief INVVPID All Contexts
 *
 * @return VOID
 */
VOID
VpidInvvpidAllContext()
{
    VpidInvvpid(InvvpidAllContext, NULL);
}

/**
 * @brief INVVPID Single Context Retaining Globals
 *
 * @param Vpid
 * @return VOID
 */
VOID
VpidInvvpidSingleContextRetainingGlobals(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, 0};
    VpidInvvpid(InvvpidSingleContextRetainingGlobals, &Descriptor);
}
