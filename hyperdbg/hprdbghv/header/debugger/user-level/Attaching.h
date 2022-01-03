/**
 * @file Attaching.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for attaching and detaching for debugging user-mode processes
 * @details 
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief Description of user-mode attaching mechanism
 * 
 */
typedef struct _USERMODE_ATTACHING_DETAILS
{
    PVOID   PebAddressToMonitor;
    BOOLEAN IsWaitingForUserModeModuleEntrypointToBeCalled;
    BOOLEAN IsWaitingForReturnAndRunFromPageFault;
    UINT64  Entrypoint;
    UINT64  BaseAddress;
    UINT32  ProcessId;
    UINT32  ThreadId;
    BOOLEAN Is32Bit;

} USERMODE_ATTACHING_DETAILS, *PUSERMODE_ATTACHING_DETAILS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
AttachingInitialize();

VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest);

VOID
AttachingHandleEntrypointDebugBreak(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);
