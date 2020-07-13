/**
 * @file Invept.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @return unsigned char 
 */
unsigned char
Invept(UINT32 Type, INVEPT_DESC * Descriptor)
{
    if (!Descriptor)
    {
        INVEPT_DESC ZeroDescriptor = {0};
        Descriptor                 = &ZeroDescriptor;
    }

    return AsmInvept(Type, Descriptor);
}

/**
 * @brief Invalidates a single context in ept cache table
 * 
 * @param EptPointer 
 * @return unsigned char 
 */
unsigned char
InveptSingleContext(UINT64 EptPointer)
{
    INVEPT_DESC Descriptor = {0};
    Descriptor.EptPointer  = EptPointer;
    Descriptor.Reserved    = 0;
    return Invept(INVEPT_SINGLE_CONTEXT, &Descriptor);
}

/**
 * @brief Invalidates all contexts in ept cache table
 * 
 * @return unsigned char 
 */
unsigned char
InveptAllContexts()
{
    return Invept(INVEPT_ALL_CONTEXTS, NULL);
}
