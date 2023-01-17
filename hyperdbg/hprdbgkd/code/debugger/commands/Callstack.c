/**
 * @file Callstack.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Kernel routines for callstack
 * @details 
 * @version 0.1
 * @date 2021-03-05
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Walkthrough the stack
 * 
 * @param AddressToSaveFrames  
 * @param StackBaseAddress  
 * @param Size  
 * @param Is32Bit  
 * 
 * @return BOOLEAN
 */
BOOLEAN
CallstackWalkthroughStack(PDEBUGGER_SINGLE_CALLSTACK_FRAME AddressToSaveFrames,
                          UINT64                           StackBaseAddress,
                          UINT32                           Size,
                          BOOLEAN                          Is32Bit)
{
    UINT32 FrameIndex          = 0;
    UINT16 AddressMode         = 0;
    UINT64 Value               = NULL;
    UINT64 CurrentStackAddress = NULL;

    if (Size == 0)
    {
        return FALSE;
    }

    if (Is32Bit)
    {
        //
        // 32-bit interpretation
        //
        AddressMode = sizeof(UINT32);
        FrameIndex  = Size / AddressMode;
    }
    else
    {
        //
        // 64-bit interpretation
        //
        AddressMode = sizeof(UINT64);
        FrameIndex  = Size / AddressMode;
    }

    //
    // Walkthrough the stack
    //
    for (size_t i = 0; i < FrameIndex; i++)
    {
        //
        // Compute the current stack position address
        //
        CurrentStackAddress = StackBaseAddress + (i * AddressMode);

        if (!CheckMemoryAccessSafety(CurrentStackAddress, AddressMode))
        {
            AddressToSaveFrames[i].IsStackAddressValid = FALSE;

            //
            // Stack is no longer valid or available to access from here
            //
            return FALSE;
        }

        //
        // Stack address is valid
        //
        AddressToSaveFrames[i].IsStackAddressValid = TRUE;

        //
        // Read the 4 or 8 byte from the target stack
        //
        MemoryMapperReadMemorySafeOnTargetProcess(CurrentStackAddress, &Value, AddressMode);

        //
        // Set the value
        //
        AddressToSaveFrames[i].Value = Value;

        //
        // This implementation has a problem, if the target jump is between two page were the second
        // page is not available, it fails to set it as the valid address,
        // We should check it for this page attribute (check boundary) but for now, i'm lazy enough
        // to let it unimplemented
        //
        // Check if value is a valid address
        //
        if (CheckMemoryAccessSafety(Value, MAXIMUM_CALL_INSTR_SIZE))
        {
            //
            // It's a valid address
            //
            AddressToSaveFrames[i].IsValidAddress = TRUE;

            //
            // Check if the target page has NX bit (executable page)
            //
            AddressToSaveFrames[i].IsExecutable = MemoryMapperCheckIfPageIsNxBitSetOnTargetProcess(Value);

            //
            // Read the memory at the target address
            //
            MemoryMapperReadMemorySafeOnTargetProcess(Value - MAXIMUM_CALL_INSTR_SIZE,
                                                      AddressToSaveFrames[i].InstructionBytesOnRip,
                                                      MAXIMUM_CALL_INSTR_SIZE);
        }
    }

    //
    // Stack walk is finished
    //
    return TRUE;
}
