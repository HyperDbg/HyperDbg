/**
 * @file Vpid.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 */
void
Invvpid(INVVPID_ENUM Type, INVVPID_DESCRIPTOR * Descriptor)
{
    if (!Descriptor)
    {
        static INVVPID_DESCRIPTOR ZeroDescriptor = {0};
        Descriptor                               = &ZeroDescriptor;
    }

    return AsmInvvpid(Type, Descriptor);
}

/**
 * @brief INVVPID instruction to invalidate a special address
 * 
 * @param Vpid 
 * @param LinearAddress 
 */
void
InvvpidIndividualAddress(UINT16 Vpid, UINT64 LinearAddress)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, LinearAddress};
    return Invvpid(INVVPID_INDIVIDUAL_ADDRESS, &Descriptor);
}

/**
 * @brief INVVPID Single Context
 * 
 * @param Vpid 
 */
void
InvvpidSingleContext(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0};
    return Invvpid(INVVPID_SINGLE_CONTEXT, &Descriptor);
}

/**
 * @brief INVVPID All Contexts
 * 
 */
void
InvvpidAllContexts()
{
    return Invvpid(INVVPID_ALL_CONTEXT, NULL);
}

/**
 * @brief INVVPID Single Context Retaining Globals
 * 
 * @param Vpid 
 */
void
InvvpidSingleContextRetainingGlobals(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0};
    return Invvpid(INVVPID_SINGLE_CONTEXT_RETAINING_GLOBALS, &Descriptor);
}
