/**
 * @file ModeBasedExecHook.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hook headers for Mode-based execution
 * @details
 * @version 0.2
 * @date 2023-03-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Definitions	    			//
//////////////////////////////////////////////////

#define MAX_PHYSICAL_RAM_RANGE_COUNT 32

//////////////////////////////////////////////////
//				    Structures	    			//
//////////////////////////////////////////////////

/**
 * @brief The RAM regions
 *
 */
typedef struct MODE_BASED_RAM_REGIONS
{
    UINT64 RamPhysicalAddress;
    UINT64 RamSize;

} MODE_BASED_RAM_REGIONS, *PMODE_BASED_RAM_REGIONS;

//////////////////////////////////////////////////
//				     Globals	    			//
//////////////////////////////////////////////////

MODE_BASED_RAM_REGIONS PhysicalRamRegions[MAX_PHYSICAL_RAM_RANGE_COUNT];

//////////////////////////////////////////////////
//				      Functions					//
//////////////////////////////////////////////////

BOOLEAN
ModeBasedExecHookInitialize();

VOID
ModeBasedExecHookUninitialize();

VOID
ModeBasedExecHookHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, UINT64 NewCr3);

VOID
ModeBasedExecHookHandleMtfCallback(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ModeBasedExecHookChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ModeBasedExecHookRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
ModeBasedExecHookChangeToExecuteOnlyEptp(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
ModeBasedExecHookHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                          VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                          UINT64                                 GuestPhysicalAddr);

BOOLEAN
ModeBasedExecHookReversingMachineInitialize(PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevServiceRequest);

VOID
ModeBasedExecHookRestoreNormalStateInTargetProcess(VIRTUAL_MACHINE_STATE * VCpu);
