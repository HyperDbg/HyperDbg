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
#include "pch.h"

/**
 * @brief routines for !va2pa and !pa2va commands
 * 
 * @param AddressDetails 
 * @return VOID 
 */
VOID
ExtensionCommandVa2paAndPa2va(PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS AddressDetails)
{
    if (AddressDetails->ProcessId == PsGetCurrentProcessId())
    {
        //
        // It's on current process address space
        //
        if (AddressDetails->IsVirtual2Physical)
        {
            AddressDetails->PhysicalAddress = VirtualAddressToPhysicalAddress(AddressDetails->VirtualAddress);
        }
        else
        {
            AddressDetails->VirtualAddress = PhysicalAddressToVirtualAddress(AddressDetails->PhysicalAddress);
        }
    }
    else
    {
        //
        // It's on another process address space
        //
        if (AddressDetails->IsVirtual2Physical)
        {
            AddressDetails->PhysicalAddress = VirtualAddressToPhysicalAddressByProcessId(AddressDetails->VirtualAddress, AddressDetails->ProcessId);
        }
        else
        {
            AddressDetails->VirtualAddress = PhysicalAddressToVirtualAddressByProcessId(AddressDetails->PhysicalAddress, AddressDetails->ProcessId);
        }
    }
}

/**
 * @brief routines for !pte command
 * 
 * @param PteDetails 
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
        PteDetails->KernelStatus = DEBUGEER_ERROR_INVALID_ADDRESS;
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

    PteDetails->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    return TRUE;
}

/**
 * @brief routines for !msrread command which 
 * @details causes vm-exit on all msr reads 
 * @param BitmapMask Bit mask of msr to put on msr bitmap
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
 * @brief routines for disable (reset) !msrread command
 * @return VOID 
 */
VOID
ExtensionCommandResetChangeAllMsrBitmapReadAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcResetMsrBitmapReadOnAllCores, NULL);
}

/**
 * @brief routines for !msrwrite command which 
 * @details causes vm-exit on all msr writes 
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

/**
 * @brief routines for reset !msrwrite command which 
 * @return VOID 
 */
VOID
ExtensionCommandResetAllMsrBitmapWriteAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcResetMsrBitmapWriteOnAllCores, NULL);
}

/**
 * @brief routines for !tsc command
 * @details causes vm-exit on all execution of rdtsc/rdtscp 
 * @return VOID 
 */
VOID
ExtensionCommandEnableRdtscExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcEnableRdtscExitingAllCores, NULL);
}

/**
 * @brief routines for disabling !tsc command
 * @return VOID 
 */
VOID
ExtensionCommandDisableRdtscExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcDisableRdtscExitingAllCores, NULL);
}

/**
 * @brief routines for !pmc 
 * @details causes vm-exit on all execution of rdpmc 
 * @return VOID 
 */
VOID
ExtensionCommandEnableRdpmcExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcEnableRdpmcExitingAllCores, NULL);
}

/**
 * @brief routines for disabling !pmc 
 * @return VOID 
 */
VOID
ExtensionCommandDisableRdpmcExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcDisableRdpmcExitingAllCores, NULL);
}

/**
 * @brief routines for !exception command which 
 * @details causes vm-exit when exception occurred 
 * @param ExceptionIndex index of exception on IDT
 * @return VOID 
 */
VOID
ExtensionCommandSetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcSetExceptionBitmapOnAllCores, ExceptionIndex);
}

/**
 * @brief routines for reset !exception command 
 * @return VOID 
 */
VOID
ExtensionCommandResetExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcResetExceptionBitmapOnAllCores, NULL);
}

/**
 * @brief routines for !dr 
 * @details causes vm-exit on all accesses to debug registers 
 * @return VOID 
 */
VOID
ExtensionCommandEnableMovDebugRegistersExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcEnableMovDebigRegisterExitingAllCores, NULL);
}

/**
 * @brief routines for disabling !dr 
 * @return VOID 
 */
VOID
ExtensionCommandDisableMovDebugRegistersExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcDisableMovDebigRegisterExitingAllCores, NULL);
}

/**
 * @brief routines for !interrupt command which 
 * @details causes vm-exit when external interrupt occurs 
 * @return VOID 
 */
VOID
ExtensionCommandSetExternalInterruptExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcSetEnableExternalInterruptExitingOnAllCores, NULL);
}

/**
 * @brief routines for terminate !interrupt command  
 * @return VOID 
 */
VOID
ExtensionCommandUnsetExternalInterruptExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcSetDisableExternalInterruptExitingOnAllCores, NULL);
}

/**
 * @brief routines for !ioin and !ioout command which 
 * @details causes vm-exit on all i/o instructions or one port 
 * @return VOID 
 */
VOID
ExtensionCommandIoBitmapChangeAllCores(UINT64 Port)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcChangeIoBitmapOnAllCores, Port);
}

/**
 * @brief routines for reset !ioin and !ioout command  
 * @return VOID 
 */
VOID
ExtensionCommandIoBitmapResetAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcResetIoBitmapOnAllCores, NULL);
}
