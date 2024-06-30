/**
 * @file Invept.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of functions that perform different INVEPT functions
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Invoke the Invept instruction
 *
 * @param Type
 * @param Descriptor
 * @return UCHAR
 */
UCHAR
EptInvept(_In_ UINT32 Type, _In_ INVEPT_DESCRIPTOR * Descriptor)
{
    INVEPT_DESCRIPTOR ZeroDescriptor = {0};
    if (!Descriptor)
    {
        Descriptor                       = &ZeroDescriptor;
    }

    return AsmInvept(Type, Descriptor);
}

/**
 * @brief Invalidates a single context in ept cache table
 *
 * @param EptPointer
 * @return UCHAR
 */
UCHAR
EptInveptSingleContext(_In_ UINT64 EptPointer)
{
    INVEPT_DESCRIPTOR Descriptor = {0};
    Descriptor.EptPointer        = EptPointer;
    Descriptor.Reserved          = 0;
    return EptInvept(InveptSingleContext, &Descriptor);
}

/**
 * @brief Invalidates all contexts in EPT cache table
 *
 * @return UCHAR
 */
UCHAR
EptInveptAllContexts()
{
    return EptInvept(InveptAllContext, NULL);
}
