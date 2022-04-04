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
#include "..\hprdbghv\pch.h"

/**
 * @brief INVVPID Instruction
 * 
 * @param Type 
 * @param Descriptor 
 * @return VOID
 */
VOID
Invvpid_fn(INVVPID_ENUM Type, INVVPID_DESCRIPTOR * Descriptor)
{
    if (!Descriptor)
    {
        static INVVPID_DESCRIPTOR ZeroDescriptor = {0};
        Descriptor                               = &ZeroDescriptor;
    }

    AsmInvvpid(Type, Descriptor);
}

/**
 * @brief INVVPID instruction to invalidate a special address
 * 
 * @param Vpid 
 * @param LinearAddress 
 * @return VOID
 */
VOID
InvvpidIndividualAddress_fn(UINT16 Vpid, UINT64 LinearAddress)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, LinearAddress};
    Invvpid_fn(INVVPID_INDIVIDUAL_ADDRESS, &Descriptor);
}

/**
 * @brief INVVPID Single Context
 * 
 * @param Vpid 
 * @return VOID
 */
VOID
InvvpidSingleContext_fn(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, 0};
    Invvpid_fn(INVVPID_SINGLE_CONTEXT, &Descriptor);
}

/**
 * @brief INVVPID All Contexts
 * 
 * @return VOID
 */
VOID
InvvpidAllContexts_fn()
{
    Invvpid_fn(INVVPID_ALL_CONTEXT, NULL);
}

/**
 * @brief INVVPID Single Context Retaining Globals
 * 
 * @param Vpid 
 * @return VOID
 */
VOID
InvvpidSingleContextRetainingGlobals_fn(UINT16 Vpid)
{
    INVVPID_DESCRIPTOR Descriptor = {Vpid, 0, 0, 0};
    Invvpid_fn(INVVPID_SINGLE_CONTEXT_RETAINING_GLOBALS, &Descriptor);
}
