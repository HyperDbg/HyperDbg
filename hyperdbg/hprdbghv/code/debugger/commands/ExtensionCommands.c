/**
 * @file ExtensionCommands.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @param OperateOnVmxRoot 
 * @return VOID 
 */
VOID
ExtensionCommandVa2paAndPa2va(PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS AddressDetails, BOOLEAN OperateOnVmxRoot)
{
    if (OperateOnVmxRoot)
    {
        //
        // *** !va2pa and !pa2va in Debugger Mode
        //
        if (AddressDetails->IsVirtual2Physical)
        {
            AddressDetails->PhysicalAddress = VirtualAddressToPhysicalAddressOnTargetProcess(AddressDetails->VirtualAddress);

            //
            // Check if address is valid or invalid
            //
            if (AddressDetails->PhysicalAddress == NULL)
            {
                //
                // Invalid address
                //
                AddressDetails->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
            }
            else
            {
                //
                // Operation was successful
                //
                AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            }
        }
        else
        {
            AddressDetails->VirtualAddress = PhysicalAddressToVirtualAddressOnTargetProcess(AddressDetails->PhysicalAddress);

            //
            // We don't know a way for checking physical address validity
            //
            AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
        }
    }
    else
    {
        //
        // *** regular !va2pa and !pa2va in VMI Mode
        //

        if (AddressDetails->ProcessId == PsGetCurrentProcessId())
        {
            //
            // It's on current process address space (we process the request
            // based on system process layout (pid = 4))
            //
            if (AddressDetails->IsVirtual2Physical)
            {
                AddressDetails->PhysicalAddress = VirtualAddressToPhysicalAddress(AddressDetails->VirtualAddress);

                //
                // Check if address is valid or invalid
                //
                if (AddressDetails->PhysicalAddress == NULL)
                {
                    //
                    // Invalid address
                    //
                    AddressDetails->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
                }
                else
                {
                    //
                    // Operation was successful
                    //
                    AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
            }
            else
            {
                AddressDetails->VirtualAddress = PhysicalAddressToVirtualAddress(AddressDetails->PhysicalAddress);

                //
                // We don't know a way for checking physical address validity
                //
                AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            }
        }
        else
        {
            //
            // It's on another process address space
            //

            //
            // Check if pid is valid
            //
            if (!IsProcessExist(AddressDetails->ProcessId))
            {
                //
                // Process id is invalid
                //
                AddressDetails->KernelStatus = DEBUGGER_ERROR_INVALID_PROCESS_ID;
                return;
            }

            if (AddressDetails->IsVirtual2Physical)
            {
                AddressDetails->PhysicalAddress = VirtualAddressToPhysicalAddressByProcessId(AddressDetails->VirtualAddress, AddressDetails->ProcessId);

                //
                // Check if address is valid or invalid
                //
                if (AddressDetails->PhysicalAddress == NULL)
                {
                    //
                    // Invalid address
                    //
                    AddressDetails->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
                }
                else
                {
                    //
                    // Operation was successful
                    //
                    AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
            }
            else
            {
                AddressDetails->VirtualAddress = PhysicalAddressToVirtualAddressByProcessId(AddressDetails->PhysicalAddress, AddressDetails->ProcessId);

                //
                // We don't know a way for checking physical address validity
                //
                AddressDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            }
        }
    }
}

/**
 * @brief routines for !pte command
 * 
 * @param PteDetails 
 * @param IsOperatingInVmxRoot 
 * @return BOOLEAN 
 */
BOOLEAN
ExtensionCommandPte(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteDetails, BOOLEAN IsOperatingInVmxRoot)
{
    BOOLEAN  Result     = FALSE;
    CR3_TYPE RestoreCr3 = {0};

    //
    // Check for validations
    //
    if (IsOperatingInVmxRoot)
    {
        if (!VirtualAddressToPhysicalAddressOnTargetProcess(PteDetails->VirtualAddress))
        {
            //
            // Address is not valid (doesn't have Physical Address)
            //
            PteDetails->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }

        //
        // Switch on running process's cr3
        //
        RestoreCr3.Flags = SwitchOnMemoryLayoutOfTargetProcess().Flags;
    }
    else
    {
        if (PteDetails->ProcessId != PsGetCurrentProcessId())
        {
            //
            // It's on another process address space
            //

            //
            // Check if pid is valid
            //
            if (!IsProcessExist(PteDetails->ProcessId))
            {
                //
                // Process id is invalid
                //
                PteDetails->KernelStatus = DEBUGGER_ERROR_INVALID_PROCESS_ID;
                return FALSE;
            }

            //
            // Switch to new process's memory layout
            //
            RestoreCr3.Flags = SwitchOnAnotherProcessMemoryLayout(PteDetails->ProcessId).Flags;
        }

        //
        // Check if address is valid
        //
        if (!VirtualAddressToPhysicalAddress(PteDetails->VirtualAddress))
        {
            //
            // Address is not valid (doesn't have Physical Address)
            //
            PteDetails->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
            Result                   = FALSE;
            goto RestoreTheState;
        }
    }

    //
    // Read the PML4E
    //
    PPAGE_ENTRY Pml4e = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PagingLevelPageMapLevel4);
    if (Pml4e)
    {
        PteDetails->Pml4eVirtualAddress = Pml4e;
        PteDetails->Pml4eValue          = Pml4e->Flags;
    }

    //
    // Read the PDPTE
    //
    PPAGE_ENTRY Pdpte = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PagingLevelPageDirectoryPointerTable);
    if (Pdpte)
    {
        PteDetails->PdpteVirtualAddress = Pdpte;
        PteDetails->PdpteValue          = Pdpte->Flags;
    }

    //
    // Read the PDE
    //
    PPAGE_ENTRY Pde = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PagingLevelPageDirectory);
    if (Pde)
    {
        PteDetails->PdeVirtualAddress = Pde;
        PteDetails->PdeValue          = Pde->Flags;
    }

    //
    // Read the PTE
    //
    PPAGE_ENTRY Pte = MemoryMapperGetPteVa(PteDetails->VirtualAddress, PagingLevelPageTable);
    if (Pte)
    {
        PteDetails->PteVirtualAddress = Pte;
        PteDetails->PteValue          = Pte->Flags;
    }

    PteDetails->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    Result                   = TRUE;

RestoreTheState:

    //
    // Check to restore the current cr3 if it's changed
    //
    if (RestoreCr3.Flags != NULL)
    {
        RestoreToPreviousProcess(RestoreCr3);
    }

    return Result;
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
    KeGenericCallDpc(DpcRoutineChangeMsrBitmapReadOnAllCores, BitmapMask);
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
    KeGenericCallDpc(DpcRoutineResetMsrBitmapReadOnAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineChangeMsrBitmapWriteOnAllCores, BitmapMask);
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
    KeGenericCallDpc(DpcRoutineResetMsrBitmapWriteOnAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineEnableRdtscExitingAllCores, NULL);
}

/**
 * @brief routines for disabling rdtsc/p exiting
 * @return VOID 
 */
VOID
ExtensionCommandDisableRdtscExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableRdtscExitingAllCores, NULL);
}

/**
 * @brief routines ONLY for disabling !tsc command 
 * @return VOID 
 */
VOID
ExtensionCommandDisableRdtscExitingForClearingEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableRdtscExitingForClearingTscEventsAllCores, NULL);
}

/**
 * @brief routines ONLY for disabling !crwrite command
 * @param Event
 * @return VOID
 */
VOID
ExtensionCommandDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_EVENT Event)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMov2CrExitingForClearingCrEventsAllCores, Event);
}

/**
 * @brief routines ONLY for disabling !dr command 
 * @return VOID 
 */
VOID
ExtensionCommandDisableMov2DebugRegsExitingForClearingEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMov2DrExitingForClearingDrEventsAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineEnableRdpmcExitingAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineDisableRdpmcExitingAllCores, NULL);
}

/**
 * @brief routines for !exception command which 
 * @details causes vm-exit when exception occurred 
 * @param ExceptionIndex index of exception on IDT
 * 
 * @return VOID 
 */
VOID
ExtensionCommandSetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetExceptionBitmapOnAllCores, ExceptionIndex);
}

/**
 * @brief routines for disabling exception bitmap 
 * @details removes vm-exit when exception occurred 
 * @param ExceptionIndex index of exception on IDT
 * 
 * @return VOID 
 */
VOID
ExtensionCommandUnsetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineUnsetExceptionBitmapOnAllCores, ExceptionIndex);
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
    KeGenericCallDpc(DpcRoutineResetExceptionBitmapOnlyOnClearingExceptionEventsOnAllCores, NULL);
}

/**
 * @brief routines for !crwrite
 * @details causes vm-exit on all accesses to debug registers
 * @param Event
 * @return VOID
 */
VOID
ExtensionCommandEnableMovControlRegisterExitingAllCores(PDEBUGGER_EVENT Event)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableMovControlRegisterExitingAllCores, Event);
}

/**
 * @brief routines for disabling !crwrite
 * @param Event
 * @return VOID
 */
VOID
ExtensionCommandDisableMovToControlRegistersExitingAllCores(PDEBUGGER_EVENT Event)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMovControlRegisterExitingAllCores, Event);
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
    KeGenericCallDpc(DpcRoutineEnableMovDebigRegisterExitingAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineDisableMovDebigRegisterExitingAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineSetEnableExternalInterruptExitingOnAllCores, NULL);
}

/**
 * @brief routines for ONLY terminate !interrupt command 
 * @return VOID 
 */
VOID
ExtensionCommandUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetDisableExternalInterruptExitingOnlyOnClearingInterruptEventsOnAllCores, NULL);
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
    KeGenericCallDpc(DpcRoutineChangeIoBitmapOnAllCores, Port);
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
    KeGenericCallDpc(DpcRoutineResetIoBitmapOnAllCores, NULL);
}
