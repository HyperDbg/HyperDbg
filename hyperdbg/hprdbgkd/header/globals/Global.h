/**
 * @file Global.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for global variables
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/**
 * @brief Save the state and variables related to debugging on each to logical core
 *
 */
PROCESSOR_DEBUGGING_STATE * g_DbgState;

/**
 * @brief Holder of script engines global variables
 *
 */
UINT64 * g_ScriptGlobalVariables;

/**
 * @brief State of the trap-flag
 *
 */
DEBUGGER_TRAP_FLAG_STATE g_TrapFlagState;

/**
 * @brief Determines whether the one application gets the handle or not
 * this is used to ensure that only one application can get the handle
 *
 */
BOOLEAN g_HandleInUse;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 *
 */
BOOLEAN g_AllowIOCTLFromUsermode;

/**
 * @brief events list (for debugger)
 *
 */
DEBUGGER_CORE_EVENTS * g_Events;

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

/**
 * @brief Process switch to EPROCESS or Process ID
 *
 */
DEBUGGEE_REQUEST_TO_CHANGE_PROCESS g_ProcessSwitch;

/**
 * @brief Thread switch to ETHREAD or Thread ID
 *
 */
DEBUGGEE_REQUEST_TO_CHANGE_THREAD g_ThreadSwitch;

/**
 * @brief The value of last error
 *
 */
UINT32 g_LastError;

/**
 * @brief Determines whether the debugger events should be active or not
 *
 */
BOOLEAN g_EnableDebuggerEvents;

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
 * @brief shows whether the kernel debugger is enabled or disabled
 *
 */
BOOLEAN g_KernelDebuggerState;

/**
 * @brief shows whether the user debugger is enabled or disabled
 *
 */
BOOLEAN g_UserDebuggerState;

/**
 * @brief shows whether the debugger should intercept breakpoints (#BP) or not
 *
 */
BOOLEAN g_InterceptBreakpoints;

/**
 * @brief shows whether the debugger should intercept breakpoints (#DB) or not
 *
 */
BOOLEAN g_InterceptDebugBreaks;

/**
 * @brief Reason that the debuggee is halted
 *
 */
DEBUGGEE_PAUSING_REASON g_DebuggeeHaltReason;

/**
 * @brief Trigger event details
 *
 */
DEBUGGER_TRIGGERED_EVENT_DETAILS g_EventTriggerDetail;

/**
 * @brief Seed for tokens of unique details buffer for threads
 *
 */
UINT64 g_SeedOfUserDebuggingDetails;

/**
 * @brief Whether the thread attaching mechanism is waiting for a page-fault
 * finish or not
 *
 */
BOOLEAN g_IsWaitingForReturnAndRunFromPageFault;

/**
 * @brief List header of thread debugging details
 *
 */
LIST_ENTRY g_ProcessDebuggingDetailsListHead;

/**
 * @brief Target function for kernel tests
 *
 */
PVOID g_KernelTestTargetFunction;

/**
 * @brief Tag1 for kernel tests
 *
 */
UINT64 g_KernelTestTag1;

/**
 * @brief Tag2 for kernel tests
 *
 */
UINT64 g_KernelTestTag2;

/**
 * @brief Temp registers for kernel tests
 *
 */
UINT64 g_KernelTestR15;
UINT64 g_KernelTestR14;
UINT64 g_KernelTestR13;
UINT64 g_KernelTestR12;

/**
 * @brief Whether the thread attaching mechanism is waiting for #DB or not
 *
 */
BOOLEAN g_IsWaitingForUserModeProcessEntryToBeCalled;

/**
 * @brief To avoid getting stuck from getting hit from the breakpoints while executing
 * the commands in the remote computer, for example, bp NtQuerySystemInformation and lm,
 * the debugger should intercept the breakponts and events.
 *
 */
BOOLEAN g_InterceptBreakpointsAndEventsForCommandsInRemoteComputer;
