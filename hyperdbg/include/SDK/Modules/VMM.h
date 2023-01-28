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
 * @brief A function that handles trigger events
 *
 */
typedef VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE (*VMM_CALLBACK_TRIGGER_EVENTS)(VMM_EVENT_TYPE_ENUM                   EventType,
                                                                                 VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE CallingStage,
                                                                                 PVOID                                 Context,
                                                                                 BOOLEAN *                             PostEventRequired,
                                                                                 GUEST_REGS *                          Regs);
/**
 * @brief A function that checks and handles debug breakpoints
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_HANDLE_DEBUG_BREAKPOINT_EXCEPTION)(UINT32 CoreId);

/**
 * @brief A function that checks and handles debug breakpoints
 *
 */
typedef BOOLEAN (*DEBUGGING_CALLBACK_HANDLE_BREAKPOINT_EXCEPTION)(UINT32 CoreId);

/**
 * @brief Check for commands in user-debugger
 *
 */
typedef BOOLEAN (*UD_CHECK_FOR_COMMAND)();

/**
 * @brief Handle registered MTF callback
 *
 */
typedef VOID (*KD_HANDLE_REGISTERED_MTF_CALLBACK)(UINT32 CoreId);

/**
 * @brief Handle registered MTF callback
 *
 */
typedef VOID (*PROCESS_TRIGGER_CR3_PROCESS_CHANGE)(UINT32 CoreId);

/**
 * @brief Handle breakpoint and debug breakpoint callback
 *
 */
typedef VOID (*PROCESS_TRIGGER_CR3_PROCESS_CHANGE)(UINT32 CoreId);

/**
 * @brief Check for process or thread change callback
 *
 */
typedef BOOLEAN (*DEBUGGER_CHECK_PROCESS_OR_THREAD_CHANGE)(_In_ UINT32 CoreId);

/**
 * @brief Check for page-faults in user-debugger
 *
 */
typedef BOOLEAN (*ATTACHING_CHECK_PAGE_FAULTS_WITH_USER_DEBUGGER)(UINT32 CoreId,
                                                                  UINT64 Address,
                                                                  ULONG  ErrorCode);

/**
 * @brief Check to handle cr3 events for thread interception
 *
 */
typedef BOOLEAN (*ATTACHING_HANDLE_CR3_EVENTS_FOR_THREAD_INTERCEPTION)(UINT32 CoreId, CR3_TYPE NewCr3);

/**
 * @brief Check for user-mode access for loaded module details
 *
 */
typedef BOOLEAN (*USER_ACCESS_CHECK_FOR_LOADED_MODULE_DETAILS)();

/**
 * @brief Check and handle reapplying breakpoint
 *
 */
typedef BOOLEAN (*BREAKPOINT_CHECK_AND_HANDLE_REAPPLYING_BREAKPOINT)(UINT32 CoreId);

/**
 * @brief Handle NMI broadcast and debug breaks
 *
 */
typedef VOID (*KD_HANDLE_NMI_BROADCAST_DEBUG_BREAKS)(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler);

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
typedef BOOLEAN (*TERMINATE_QUERY_DEBUGGER_RESOURCE)(UINT32                               CoreId,
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

    //
    // VMM callbacks
    //
    VMM_CALLBACK_TRIGGER_EVENTS VmmCallbackTriggerEvents; // Fixed
    VMM_CALLBACK_SET_LAST_ERROR VmmCallbackSetLastError;  // Fixed
    VMM_CALLBACK_VMCALL_HANDLER VmmCallbackVmcallHandler; // Fixed

    //
    // Debugging callbacks
    //
    DEBUGGING_CALLBACK_HANDLE_BREAKPOINT_EXCEPTION       DebuggingCallbackHandleBreakpointException;      // Fixed
    DEBUGGING_CALLBACK_HANDLE_DEBUG_BREAKPOINT_EXCEPTION DebuggingCallbackHandleDebugBreakpointException; // Fixed

    BREAKPOINT_CHECK_AND_HANDLE_REAPPLYING_BREAKPOINT BreakpointCheckAndHandleReApplyingBreakpoint;
    TERMINATE_QUERY_DEBUGGER_RESOURCE                 TerminateQueryDebuggerResource;
    KD_HANDLE_REGISTERED_MTF_CALLBACK                 KdHandleRegisteredMtfCallback;
    KD_HANDLE_NMI_BROADCAST_DEBUG_BREAKS              KdHandleNmiBroadcastDebugBreaks;
    KD_CHECK_AND_HANDLE_NMI_CALLBACK                  KdCheckAndHandleNmiCallback;

    //
    // User Debugger
    //
    UD_CHECK_FOR_COMMAND                        UdCheckForCommand;
    USER_ACCESS_CHECK_FOR_LOADED_MODULE_DETAILS UserAccessCheckForLoadedModuleDetails;

    //
    // Process/Thread interception mechanism
    //
    PROCESS_TRIGGER_CR3_PROCESS_CHANGE                             ProcessTriggerCr3ProcessChange;
    DEBUGGER_CHECK_PROCESS_OR_THREAD_CHANGE                        DebuggerCheckProcessOrThreadChange;
    ATTACHING_CHECK_PAGE_FAULTS_WITH_USER_DEBUGGER                 AttachingCheckPageFaultsWithUserDebugger;
    ATTACHING_HANDLE_CR3_EVENTS_FOR_THREAD_INTERCEPTION            AttachingHandleCr3VmexitsForThreadInterception;
    KD_QUERY_DEBUGGER_THREAD_OR_PROCESS_TRACING_DETAILS_BY_CORE_ID KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId;

} VMM_CALLBACKS, *PVMM_CALLBACKS;
