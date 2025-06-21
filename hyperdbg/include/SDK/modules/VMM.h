/**
 * @file VMM.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for VMM project
 * @details This file contains definitions of HyperLog routines
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
                                                                  const char * Fmt,
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
 * @brief Check for commands in user-debugger
 *
 */
typedef BOOLEAN (*UD_CHECK_FOR_COMMAND)();

/**
 * @brief Handle registered MTF callback
 *
 */
typedef VOID (*VMM_CALLBACK_REGISTERED_MTF_HANDLER)(UINT32 CoreId);

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
 * @brief Handle cr3 process change callbacks
 *
 */
typedef VOID (*INTERCEPTION_CALLBACK_TRIGGER_CR3_CHANGE)(UINT32 CoreId);

/**
 * @brief Check for process or thread change callback
 *
 */
typedef BOOLEAN (*INTERCEPTION_CALLBACK_TRIGGER_CLOCK_AND_IPI)(_In_ UINT32 CoreId);

/**
 * @brief Check and handle reapplying breakpoint
 *
 */
typedef BOOLEAN (*BREAKPOINT_CHECK_AND_HANDLE_REAPPLYING_BREAKPOINT)(UINT32 CoreId);

/**
 * @brief Handle NMI broadcast
 *
 */
typedef VOID (*VMM_CALLBACK_NMI_BROADCAST_REQUEST_HANDLER)(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler);

/**
 * @brief Check and handle NMI callbacks
 *
 */
typedef BOOLEAN (*KD_CHECK_AND_HANDLE_NMI_CALLBACK)(UINT32 CoreId);

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
 * @brief Query debugger thread or process tracing details by core ID
 *
 */
typedef BOOLEAN (*KD_QUERY_DEBUGGER_THREAD_OR_PROCESS_TRACING_DETAILS_BY_CORE_ID)(UINT32                          CoreId,
                                                                                  DEBUGGER_THREAD_PROCESS_TRACING TracingType);
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
    LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogCallbackPrepareAndSendMessageToQueueWrapper; // Fixed
    LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE             LogCallbackSendMessageToQueue;                  // Fixed
    LOG_CALLBACK_SEND_BUFFER                       LogCallbackSendBuffer;                          // Fixed
    LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL           LogCallbackCheckIfBufferIsFull;                 // Fixed

    //
    // VMM callbacks
    //
    VMM_CALLBACK_TRIGGER_EVENTS                     VmmCallbackTriggerEvents;                   // Fixed
    VMM_CALLBACK_SET_LAST_ERROR                     VmmCallbackSetLastError;                    // Fixed
    VMM_CALLBACK_VMCALL_HANDLER                     VmmCallbackVmcallHandler;                   // Fixed
    VMM_CALLBACK_NMI_BROADCAST_REQUEST_HANDLER      VmmCallbackNmiBroadcastRequestHandler;      // Fixed
    VMM_CALLBACK_QUERY_TERMINATE_PROTECTED_RESOURCE VmmCallbackQueryTerminateProtectedResource; // Fixed
    VMM_CALLBACK_RESTORE_EPT_STATE                  VmmCallbackRestoreEptState;                 // Fixed
    VMM_CALLBACK_CHECK_UNHANDLED_EPT_VIOLATION      VmmCallbackCheckUnhandledEptViolations;     // Fixed

    //
    // Debugging callbacks
    //
    DEBUGGING_CALLBACK_HANDLE_BREAKPOINT_EXCEPTION       DebuggingCallbackHandleBreakpointException;      // Fixed
    DEBUGGING_CALLBACK_HANDLE_DEBUG_BREAKPOINT_EXCEPTION DebuggingCallbackHandleDebugBreakpointException; // Fixed
    DEBUGGING_CALLBACK_CHECK_THREAD_INTERCEPTION         DebuggingCallbackCheckThreadInterception;        // Fixed

    //
    // Interception callbacks
    //
    INTERCEPTION_CALLBACK_TRIGGER_CR3_CHANGE InterceptionCallbackTriggerCr3ProcessChange; // Fixed

    //
    // Callbacks to be removed
    //
    BREAKPOINT_CHECK_AND_HANDLE_REAPPLYING_BREAKPOINT              BreakpointCheckAndHandleReApplyingBreakpoint;
    UD_CHECK_FOR_COMMAND                                           UdCheckForCommand;
    KD_CHECK_AND_HANDLE_NMI_CALLBACK                               KdCheckAndHandleNmiCallback;
    VMM_CALLBACK_REGISTERED_MTF_HANDLER                            VmmCallbackRegisteredMtfHandler; // Fixed but not good
    INTERCEPTION_CALLBACK_TRIGGER_CLOCK_AND_IPI                    DebuggerCheckProcessOrThreadChange;
    KD_QUERY_DEBUGGER_THREAD_OR_PROCESS_TRACING_DETAILS_BY_CORE_ID KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId;

} VMM_CALLBACKS, *PVMM_CALLBACKS;
