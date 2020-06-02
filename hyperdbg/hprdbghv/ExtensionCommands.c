/**
 * @file ExtensionCommands.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger Commands (Extensions)
 * @details Debugger Commands that start with "!"
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "Broadcast.h"
#include "Dpc.h"
#include "Debugger.h"
#include "Logging.h"
#include "Common.h"
#include "Hooks.h"
#include "GlobalVariables.h"

/**
 * @brief routines for !pte command
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ExtensionCommandPte(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteDetails)
{
    //
    // Check if address is valid
    //
    if (!VirtualAddressToPhysicalAddress(PteDetails->VirtualAddress))
    {
        //
        // Address is not valid (doesn't have Physical Address)
        //
        return FALSE;
    }

    //
    // Read the PML4E
    //
    PPAGE_ENTRY Pml4e = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PML4);
    if (Pml4e)
    {
        PteDetails->Pml4eVirtualAddress = Pml4e;
        PteDetails->Pml4eValue          = Pml4e->Flags;
    }

    //
    // Read the PDPTE
    //
    PPAGE_ENTRY Pdpte = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PDPT);
    if (Pdpte)
    {
        PteDetails->PdpteVirtualAddress = Pdpte;
        PteDetails->PdpteValue          = Pdpte->Flags;
    }

    //
    // Read the PDE
    //
    PPAGE_ENTRY Pde = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PD);
    if (Pde)
    {
        PteDetails->PdeVirtualAddress = Pde;
        PteDetails->PdeValue          = Pde->Flags;
    }

    //
    // Read the PTE
    //
    PPAGE_ENTRY Pte = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PT);
    if (Pte)
    {
        PteDetails->PteVirtualAddress = Pte;
        PteDetails->PteValue          = Pte->Flags;
    }

    return TRUE;
}

/**
 * @brief routines for !msrread and !msrwrite command which 
 * causes vm-exit on all msr reads 
 * @return VOID 
 */
VOID
ExtensionCommandChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcChangeMsrBitmapReadOnAllCores, BitmapMask);
}

/**
 * @brief routines for !msrread and !msrwrite command which 
 * causes vm-exit on all msr writes 
 * @return VOID 
 */
VOID
ExtensionCommandChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcChangeMsrBitmapWriteOnAllCores, BitmapMask);
}
