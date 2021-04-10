/**
 * @file GlobalVariables.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Here we put global variables that are used more or less in all part of our hypervisor (not all of them)
 * @details Note : All the global variables are not here, just those that
 * will be used in all project. Special use global variables are located 
 * in their corresponding headers
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

/**
 * @brief Save the state and variables related to each to logical core
 * 
 */
VIRTUAL_MACHINE_STATE * g_GuestState;

/**
 * @brief Save the state and variables related to EPT
 * 
 */
EPT_STATE * g_EptState;

/**
 * @brief events list (for debugger)
 * 
 */
DEBUGGER_CORE_EVENTS * g_Events;

/**
 * @brief Save the state of the thread that waits for messages to deliver to user-mode
 * 
 */
NOTIFY_RECORD * g_GlobalNotifyRecord;

/**
 * @brief Support for execute-only pages (indicating that data accesses are
 *  not allowed while instruction fetches are allowed)
 * 
 */
BOOLEAN g_ExecuteOnlySupport;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 * 
 */
BOOLEAN g_AllowIOCTLFromUsermode;

/**
 * @brief Determines whether the debugger events should be active or not
 * 
 */
BOOLEAN g_EnableDebuggerEvents;

/**
 * @brief Determines whether the one application gets the handle or not
 * this is used to ensure that only one application can get the handle
 * 
 */
BOOLEAN g_HandleInUse;

/**
 * @brief List header of hidden hooks detour
 * 
 */
LIST_ENTRY g_EptHook2sDetourListHead;

/**
 * @brief List header of breakpoints for debugger-mode
 * 
 */
LIST_ENTRY g_BreakpointsListHead;

/**
 * @brief Seed for setting id of breakpoints
 * 
 */
UINT64 g_MaximumBreakpointId;

/**
 * @brief Shows whether the debugger transparent mode 
 * is enabled (true) or not (false)
 * 
 */
BOOLEAN g_TransparentMode;

/**
 * @brief holds the measurements from the user-mode and kernel-mode
 * 
 */
TRANSPARENCY_MEASUREMENTS * g_TransparentModeMeasurements;

/**
 * @brief details relating to nop-sled page
 * 
 */
DEBUGGER_STEPPINGS_NOP_SLED g_SteppingsNopSledState;

/**
 * @brief shows whether the stepping mechanism is enabled or disabled
 * 
 */
BOOLEAN g_EnableDebuggerSteppings;

/**
 * @brief holds the thread states for debugger on steppings
 * 
 */
LIST_ENTRY g_ThreadDebuggingStates;

/**
 * @brief shows whether the kernel debugger is enabled or disabled
 * 
 */
BOOLEAN g_KernelDebuggerState;

/**
 * @brief X2APIC or XAPIC routine
 * 
 */
BOOLEAN g_IsX2Apic;

/**
 * @brief APIC Base
 * 
 */
VOID * g_ApicBase;

/**
 * @brief Reason that the debuggee is halted
 * 
 */
DEBUGGEE_PAUSING_REASON g_DebuggeeHaltReason;

/**
 * @brief Optional context as the debuggee is halted
 * 
 */
PVOID g_DebuggeeHaltContext;

/**
 * @brief Optional tag as the debuggee is halted
 * 
 */
UINT64 g_DebuggeeHaltTag;

/**
 * @brief Dpc state for debuggee
 * 
 */
PKDPC g_DebuggeeDpc;

/**
 * @brief NMI handler pointer for KeDeregisterNmiCallback 
 * 
 */
PVOID g_NmiHandlerForKeDeregisterNmiCallback;

/**
 * @brief check for RTM support
 * 
 */
BOOLEAN g_RtmSupport;

/**
 * @brief Shows whether the debuggee is waiting for an 
 * trap step or not
 * 
 */
BOOLEAN g_WaitForStepTrap;

/**
 * @brief Holds the requests to pause the break of debuggee until
 * a special event happens
 * 
 */
DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT g_IgnoreBreaksToDebugger;

/**
 * @brief Holds the state of hardware debug register for step-over
 * 
 */
HARDWARE_DEBUG_REGISTER_DETAILS g_HardwareDebugRegisterDetailsForStepOver;
