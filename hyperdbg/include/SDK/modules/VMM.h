/**
 * @file VMM.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for VMM project
 * @details This file contains definitions of VMM routines
 * @version 0.2
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			     Callback Types                 //
//////////////////////////////////////////////////

/**
 * @brief A function from the message tracer that send the inputs to the
 * queue of the messages
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE)(UINT32       OperationCode,
                                                                  BOOLEAN      IsImmediateMessage,
                                                                  BOOLEAN      ShowCurrentSystemTime,
                                                                  BOOLEAN      Priority,
                                                                  const CHAR * Fmt,
                                                                  va_list      ArgList);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE)(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_BUFFER)(_In_ UINT32                          OperationCode,
                                            _In_reads_bytes_(BufferLength) PVOID Buffer,
                                            _In_ UINT32                          BufferLength,
                                            _In_ BOOLEAN                         Priority);

/**
 * @brief A function that checks whether the priority or regular buffer is full or not
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL)(BOOLEAN Priority);

/**
 * @brief A function that checks if LBR is supported on the current CPU and gets its capacity
 *
 */
typedef BOOLEAN (*HYPERTRACE_LBR_IS_SUPPORTED)(UINT32 * Capacity, BOOLEAN * IsArchLbr);

/**
 * @brief A function that handles trigger events
 *
 */
typedef VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE (*VMM_CALLBACK_TRIGGER_EVENTS)(VMM_EVENT_TYPE_ENUM                   EventType,
                                                                                 VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE CallingStage,
                                                                                 PVOID                                 Context,
                                                                                 BOOLEAN *                             PostEventRequired,
                                                                                 GUEST_REGS *                          Regs);

/**
 * @brief A function that checks and handles breakpoints
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_HANDLE_BREAKPOINT_EXCEPTION)(UINT32 CoreId);

/**
 * @brief A function that checks and handles debug breakpoints
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_HANDLE_DEBUG_BREAKPOINT_EXCEPTION)(UINT32 CoreId);

/**
 * @brief Check for thread interception in user-debugger
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_CHECK_THREAD_INTERCEPTION)(UINT32 CoreId);

/**
 * @brief Trigger on clock and IPI events for checking process or thread change
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_TRIGGER_ON_CLOCK_AND_IPI_EVENTS)(_In_ UINT32 CoreId);

/**
 * @brief routine callback to ignore handling mov 2 debug registers
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_IGNORE_HANDLING_MOV_2_DEBUG_REGS)(_In_ UINT32 CoreId);

/**
 * @brief Request pool allocation
 *
 */
typedef BOOLEAN (*POOL_MANAGER_REQUEST_ALLOCATION)(SIZE_T Size, UINT32 Count, POOL_ALLOCATION_INTENTION Intention);

/**
 * @brief Request pool
 *
 */
typedef UINT64 (*POOL_MANAGER_REQUEST_POOL)(POOL_ALLOCATION_INTENTION Intention, BOOLEAN RequestNewPool, UINT32 Size);

/**
 * @brief Free pool
 *
 */
typedef BOOLEAN (*POOL_MANAGER_FREE_POOL)(UINT64 AddressToFree);

/**
 * @brief Check for user-mode access for loaded module details
 *
 */
typedef BOOLEAN (*VMM_CALLBACK_RESTORE_EPT_STATE)(UINT32 CoreId);

/**
 * @brief Check for unhandled EPT violations
 *
 */
typedef BOOLEAN (*VMM_CALLBACK_CHECK_UNHANDLED_EPT_VIOLATION)(UINT32 CoreId, UINT64 ViolationQualification, UINT64 GuestPhysicalAddr);

/**
 * @brief Handle MTF callback
 *
 */
typedef BOOLEAN (*VMM_CALLBACK_HANDLE_MTF_CALLBACK)(UINT32 CoreId);

/**
 * @brief Handle cr3 process change callbacks
 *
 */
typedef VOID (*INTERCEPTION_CALLBACK_TRIGGER_CR3_CHANGE)(UINT32 CoreId);

/**
 * @brief Handle NMI broadcast
 *
 */
typedef VOID (*VMM_CALLBACK_NMI_BROADCAST_REQUEST_HANDLER)(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler);

/**
 * @brief Set the top-level driver's error status
 *
 */
typedef VOID (*VMM_CALLBACK_SET_LAST_ERROR)(UINT32 LastError);

/**
 * @brief Check and modify the protected resources of the hypervisor
 *
 */
typedef BOOLEAN (*VMM_CALLBACK_QUERY_TERMINATE_PROTECTED_RESOURCE)(UINT32                               CoreId,
                                                                   PROTECTED_HV_RESOURCES_TYPE          ResourceType,
                                                                   PVOID                                Context,
                                                                   PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver);

/**
 * @brief Handler of debugger specific VMCALLs
 *
 */
typedef BOOLEAN (*VMM_CALLBACK_VMCALL_HANDLER)(UINT32 CoreId,
                                               UINT64 VmcallNumber,
                                               UINT64 OptionalParam1,
                                               UINT64 OptionalParam2,
                                               UINT64 OptionalParam3);

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by VMM module
 *
 */
typedef struct _VMM_CALLBACKS
{
    //
    // Log (Hyperlog) callbacks
    //
    LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogCallbackPrepareAndSendMessageToQueueWrapper;
    LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE             LogCallbackSendMessageToQueue;
    LOG_CALLBACK_SEND_BUFFER                       LogCallbackSendBuffer;
    LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL           LogCallbackCheckIfBufferIsFull;

    //
    // HyperTrace callback(s)
    //
    HYPERTRACE_LBR_IS_SUPPORTED HyperTraceCallbackLbrIsSupported;

    //
    // VMM callbacks
    //
    VMM_CALLBACK_TRIGGER_EVENTS                     VmmCallbackTriggerEvents;
    VMM_CALLBACK_SET_LAST_ERROR                     VmmCallbackSetLastError;
    VMM_CALLBACK_VMCALL_HANDLER                     VmmCallbackVmcallHandler;
    VMM_CALLBACK_NMI_BROADCAST_REQUEST_HANDLER      VmmCallbackNmiBroadcastRequestHandler;
    VMM_CALLBACK_QUERY_TERMINATE_PROTECTED_RESOURCE VmmCallbackQueryTerminateProtectedResource;
    VMM_CALLBACK_RESTORE_EPT_STATE                  VmmCallbackRestoreEptState;
    VMM_CALLBACK_CHECK_UNHANDLED_EPT_VIOLATION      VmmCallbackCheckUnhandledEptViolations;
    VMM_CALLBACK_HANDLE_MTF_CALLBACK                VmmCallbackHandleMtfCallback;

    //
    // Debugging callbacks
    //
    DEBUGGING_CALLBACK_HANDLE_BREAKPOINT_EXCEPTION       DebuggingCallbackHandleBreakpointException;
    DEBUGGING_CALLBACK_HANDLE_DEBUG_BREAKPOINT_EXCEPTION DebuggingCallbackHandleDebugBreakpointException;
    DEBUGGING_CALLBACK_CHECK_THREAD_INTERCEPTION         DebuggingCallbackCheckThreadInterception;
    DEBUGGING_CALLBACK_TRIGGER_ON_CLOCK_AND_IPI_EVENTS   DebuggingCallbackTriggerOnClockAndIpiEvents;
    DEBUGGING_CALLBACK_IGNORE_HANDLING_MOV_2_DEBUG_REGS  DebuggingCallbackIgnoreHandlingMov2DebugRegs;

    //
    // Pool manager callbacks
    //
    POOL_MANAGER_REQUEST_ALLOCATION PoolManagerCallbackRequestAllocation;
    POOL_MANAGER_REQUEST_POOL       PoolManagerCallbackRequestPool;
    POOL_MANAGER_FREE_POOL          PoolManagerCallbackFreePool;

    //
    // Interception callbacks
    //
    INTERCEPTION_CALLBACK_TRIGGER_CR3_CHANGE InterceptionCallbackTriggerCr3ProcessChange;

} VMM_CALLBACKS, *PVMM_CALLBACKS;
