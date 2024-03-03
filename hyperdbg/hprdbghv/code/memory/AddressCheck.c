/**
 * @file Conversion.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for address checks
 *
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function checks whether the address is valid or not using
 * Intel TSX
 *
 * @param Address Address to check
 *
 * @param UINT32 ProcId
 * @return BOOLEAN Returns true if the address is valid; otherwise, false
 */
BOOLEAN
CheckAddressValidityUsingTsx(CHAR * Address)
{
    UINT32  Status = 0;
    BOOLEAN Result = FALSE;
    CHAR    TempContent;

    if ((Status = _xbegin()) == _XBEGIN_STARTED)
    {
        //
        // Try to read the memory
        //
        TempContent = *(CHAR *)Address;
        _xend();

        //
        // No error, address is valid
        //
        Result = TRUE;
    }
    else
    {
        //
        // Address is not valid, it aborts the tsx rtm
        //
        Result = FALSE;
    }

    return Result;
}

/**
 * @brief Checks if the address is canonical based on x86 processor's
 * virtual address width or not
 * @param VAddr virtual address to check
 * @param IsKernelAddress Filled to show whether the address is a
 * kernel address or user-address
 * @brief IsKernelAddress wouldn't check for page attributes, it
 * just checks the address convention in Windows
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckAddressCanonicality(UINT64 VAddr, PBOOLEAN IsKernelAddress)
{
    UINT64 Addr = (UINT64)VAddr;
    UINT64 MaxVirtualAddrLowHalf, MinVirtualAddressHighHalf;

    //
    // Get processor's address width for VA
    //
    UINT32 AddrWidth = g_CompatibilityCheck.VirtualAddressWidth;

    //
    // get max address in lower-half canonical addr space
    // e.g. if width is 48, then 0x00007FFF_FFFFFFFF
    //
    MaxVirtualAddrLowHalf = ((UINT64)1ull << (AddrWidth - 1)) - 1;

    //
    // get min address in higher-half canonical addr space
    // e.g., if width is 48, then 0xFFFF8000_00000000
    //
    MinVirtualAddressHighHalf = ~MaxVirtualAddrLowHalf;

    //
    // Check to see if the address in a canonical address
    //
    if ((Addr > MaxVirtualAddrLowHalf) && (Addr < MinVirtualAddressHighHalf))
    {
        *IsKernelAddress = FALSE;
        return FALSE;
    }

    //
    // Set whether it's a kernel address or not
    //
    if (MinVirtualAddressHighHalf < Addr)
    {
        *IsKernelAddress = TRUE;
    }
    else
    {
        *IsKernelAddress = FALSE;
    }

    return TRUE;
}

/**
 * @brief Checks if the physical address is correct or not based on physical address width
 *
 * @param VAddr Physical address to check
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckAddressPhysical(UINT64 PAddr)
{
    UINT64 Addr = (UINT64)PAddr;
    UINT64 MaxPA;

    //
    // Get processor's address width for PS
    //
    UINT32 AddrWidth = g_CompatibilityCheck.PhysicalAddressWidth;

    //
    // get max address for physical address
    //
    MaxPA = ((UINT64)1ull << (AddrWidth - 1)) - 1;

    // LogInfo("Max physical address: %llx", MaxPA);

    //
    // Check to see if the address in a canonical address
    //
    if (Addr > MaxPA)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Check the safety to access the memory
 * @param TargetAddress
 * @param Size
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckAccessValidityAndSafety(UINT64 TargetAddress, UINT32 Size)
{
    CR3_TYPE GuestCr3;
    UINT64   OriginalCr3;
    BOOLEAN  IsKernelAddress;
    BOOLEAN  Result = FALSE;

    //
    // First, we check if the address is canonical based
    // on Intel processor's virtual address width
    //
    if (!CheckAddressCanonicality(TargetAddress, &IsKernelAddress))
    {
        //
        // No need for further check, address is invalid
        //
        Result = FALSE;
        goto Return;
    }

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3 = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // We'll only check address with TSX if the address is a kernel-mode
    // address because an exception is thrown if we access user-mode code
    // from vmx-root mode, thus, TSX will fail the transaction and the
    // result is not true, so we check each pages' page-table for user-mode
    // codes in both user-mode and kernel-mode
    //
    // if (g_RtmSupport && IsKernelAddress)
    // {
    //     //
    //     // The guest supports Intel TSX
    //     //
    //     UINT64 AlignedPage = (UINT64)PAGE_ALIGN(TargetAddress);
    //     UINT64 PageCount   = ((TargetAddress - AlignedPage) + Size) / PAGE_SIZE;
    //
    //     for (size_t i = 0; i <= PageCount; i++)
    //     {
    //         UINT64 CheckAddr = AlignedPage + (PAGE_SIZE * i);
    //         if (!CheckAddressValidityUsingTsx(CheckAddr))
    //         {
    //             //
    //             // Address is not valid
    //             //
    //             Result = FALSE;
    //
    //             goto RestoreCr3;
    //         }
    //     }
    // }

    //
    // We've realized that using TSX for address checking is not faster
    // than the legacy memory checking (traversing the page-tables),
    // based on our resultsm it's ~50 TSC clock cycles for a valid address
    // and ~180 TSC clock cycles for an invalid address slower to use TSX
    // for memory checking, that's why it is deprecated now
    //

    //
    // Check if memory is safe and present
    //
    UINT64 AddressToCheck = (CHAR *)TargetAddress + Size - ((CHAR *)PAGE_ALIGN(TargetAddress));

    if (AddressToCheck > PAGE_SIZE)
    {
        //
        // Address should be accessed in more than one page
        //
        UINT64 ReadSize = AddressToCheck;

        while (Size != 0)
        {
            ReadSize = (UINT64)PAGE_ALIGN(TargetAddress + PAGE_SIZE) - TargetAddress;

            if (ReadSize == PAGE_SIZE && Size < PAGE_SIZE)
            {
                ReadSize = Size;
            }

            if (!MemoryMapperCheckIfPageIsPresentByCr3((PVOID)TargetAddress, GuestCr3))
            {
                //
                // Address is not valid
                //
                Result = FALSE;

                goto RestoreCr3;
            }

            /*
            LogInfo("Addr From : %llx to Addr To : %llx | ReadSize : %llx\n",
                    TargetAddress,
                    TargetAddress + ReadSize,
                    ReadSize);
            */

            //
            // Apply the changes to the next addresses (if any)
            //
            Size          = (UINT32)(Size - ReadSize);
            TargetAddress = TargetAddress + ReadSize;
        }
    }
    else
    {
        if (!MemoryMapperCheckIfPageIsPresentByCr3((PVOID)TargetAddress, GuestCr3))
        {
            //
            // Address is not valid
            //
            Result = FALSE;

            goto RestoreCr3;
        }
    }

    //
    // If we've reached here, the address was valid
    //
    Result = TRUE;

RestoreCr3:

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3);

Return:
    return Result;
}

/**
 * @brief This function returns the maximum instruction length that can be read from this address
 * @param Address
 *
 * @return UINT32
 */
UINT32
CheckAddressMaximumInstructionLength(PVOID Address)
{
    UINT64 SizeOfSafeBufferToRead = 0;

    //
    // Compute the amount of buffer we can read without problem
    //
    SizeOfSafeBufferToRead = (UINT64)Address & 0xfff;
    SizeOfSafeBufferToRead += MAXIMUM_INSTR_SIZE;

    if (SizeOfSafeBufferToRead >= PAGE_SIZE)
    {
        SizeOfSafeBufferToRead = SizeOfSafeBufferToRead - PAGE_SIZE;

        //
        // When we reached here, we're sure the instruction is on the boundary of a
        // page table, so we have the maximum instruction length to read, but just
        // to make sure that the instruction is not continued into two pages, we'll
        // check the validity of the next page
        //
        if (CheckAccessValidityAndSafety((UINT64)Address + PAGE_SIZE, sizeof(CHAR)))
        {
            //
            // Address is safe to be read from the next page, so we just extend it
            // to the MAXIMUM_INSTR_SIZE
            //
            SizeOfSafeBufferToRead = MAXIMUM_INSTR_SIZE;
        }
        else
        {
            SizeOfSafeBufferToRead = MAXIMUM_INSTR_SIZE - SizeOfSafeBufferToRead;
        }
    }
    else
    {
        SizeOfSafeBufferToRead = MAXIMUM_INSTR_SIZE;
    }

    return (UINT32)SizeOfSafeBufferToRead;
}
