/**
 * @file Ioctls.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK IOCTL codes
 * @details This file contains definitions of IOCTLs used in HyperDbg
 * @version 0.2
 * @date 2022-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                 Definitions                  //
//////////////////////////////////////////////////

//
// The following controls are mainly defined in <winioctl.h>
//

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//
#ifndef CTL_CODE

#    define CTL_CODE(DeviceType, Function, Method, Access) ( \
        ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#endif // ! CTL_CODE

#ifndef FILE_ANY_ACCESS

#    define FILE_ANY_ACCESS 0

#endif // !FILE_ANY_ACCESS

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#ifndef METHOD_BUFFERED

#    define METHOD_BUFFERED 0

#endif // !METHOD_BUFFERED

#ifndef FILE_DEVICE_UNKNOWN

#    define FILE_DEVICE_UNKNOWN 0x00000022

#endif // !FILE_DEVICE_UNKNOWN

/**
 * @brief Extract the function from an IOCTL code
 *
 */
#define CTL_CODE_FUNCTION(Code) (((Code) >> 2) & 0xFFF)

/**
 * @brief Base code for IOCTLs, to avoid conflicts with other drivers
 *
 */
#define IOCTL_START_CODE 0x800

//////////////////////////////////////////////////
//                   IOCTLs                     //
//////////////////////////////////////////////////

/**
 * @brief ioctl, initialize the VMM module
 *
 */
#define IOCTL_INIT_VMM \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, initialize the HyperTrace module
 *
 */
#define IOCTL_INIT_HYPERTRACE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, register a new event
 *
 */
#define IOCTL_REGISTER_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, irp pending mechanism for reading from message tracing buffers
 *
 */
#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to terminate vmx and exit form debugger
 *
 */
#define IOCTL_TERMINATE_VMX \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x12, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read memory
 *
 */
#define IOCTL_DEBUGGER_READ_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x13, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read or write on a special MSR
 *
 */
#define IOCTL_DEBUGGER_READ_OR_WRITE_MSR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x14, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to read page table entries
 *
 */
#define IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x15, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, register an event
 *
 */
#define IOCTL_DEBUGGER_REGISTER_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x16, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, add action to event
 *
 */
#define IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x17, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to enable or disable transparent-mode
 *
 */
#define IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x18, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, for !va2pa and !pa2va commands
 *
 */
#define IOCTL_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x19, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to edit virtual and physical memory
 *
 */
#define IOCTL_DEBUGGER_EDIT_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1a, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to search virtual and physical memory
 *
 */
#define IOCTL_DEBUGGER_SEARCH_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1b, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to modify an event (enable/disable/clear)
 *
 */
#define IOCTL_DEBUGGER_MODIFY_EVENTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1c, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, flush the kernel buffers
 *
 */
#define IOCTL_DEBUGGER_FLUSH_LOGGING_BUFFERS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1d, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, attach or detach user-mode processes
 *
 */
#define IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1e, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, print states (Deprecated)
 *
 *
 */
#define IOCTL_DEBUGGER_PRINT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x1f, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, prepare debuggee
 *
 */
#define IOCTL_PREPARE_DEBUGGEE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x20, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, pause and halt the system
 *
 */
#define IOCTL_PAUSE_PACKET_RECEIVED \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x21, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send a signal that execution of command finished
 *
 */
#define IOCTL_SEND_SIGNAL_EXECUTION_IN_DEBUGGEE_FINISHED \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x22, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send user-mode messages to the debugger
 *
 */
#define IOCTL_SEND_USERMODE_MESSAGES_TO_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x23, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, send general buffer from debuggee to debugger
 *
 */
#define IOCTL_SEND_GENERAL_BUFFER_FROM_DEBUGGEE_TO_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x24, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform kernel-side tests
 *
 */
#define IOCTL_PERFORM_KERNEL_SIDE_TESTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x25, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to reserve pre-allocated pools
 *
 */
#define IOCTL_RESERVE_PRE_ALLOCATED_POOLS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x26, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to send user debugger commands
 *
 */
#define IOCTL_SEND_USER_DEBUGGER_COMMANDS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x27, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to get active threads/processes that are debugging
 *
 */
#define IOCTL_GET_DETAIL_OF_ACTIVE_THREADS_AND_PROCESSES \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x28, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to get user mode modules details
 *
 */
#define IOCTL_GET_USER_MODE_MODULE_DETAILS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x29, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, query count of active threads or processes
 *
 */
#define IOCTL_QUERY_COUNT_OF_ACTIVE_PROCESSES_OR_THREADS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2a, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to get list threads/processes
 *
 */
#define IOCTL_GET_LIST_OF_THREADS_AND_PROCESSES \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2b, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, query the current process details
 *
 */
#define IOCTL_QUERY_CURRENT_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2c, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, query the current thread details
 *
 */
#define IOCTL_QUERY_CURRENT_THREAD \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2d, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request service from the reversing machine
 *
 */
#define IOCTL_REQUEST_REV_MACHINE_SERVICE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2e, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, request to bring pages in
 *
 */
#define IOCTL_DEBUGGER_BRING_PAGES_IN \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x2f, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to preactivate a functionality
 *
 */
#define IOCTL_PREACTIVATE_FUNCTIONALITY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x30, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to enumerate PCIe endpoints
 *
 */
#define IOCTL_PCIE_ENDPOINT_ENUM \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x31, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform actions related to APIC
 *
 */
#define IOCTL_PERFORM_ACTIONS_ON_APIC \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x32, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to query for PCI endpoint info
 *
 */
#define IOCTL_PCIDEVINFO_ENUM \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x33, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to query the IDT entries
 *
 */
#define IOCTL_QUERY_IDT_ENTRY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x34, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to set breakpoint for the user debugger
 *
 */
#define IOCTL_SET_BREAKPOINT_USER_DEBUGGER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x35, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform SMI operations
 *
 */
#define IOCTL_PERFORM_SMI_OPERATION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x36, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform HyperTrace LBR operations
 *
 */
#define IOCTL_PERFORM_HYPERTRACE_LBR_OPERATION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x37, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform HyperTrace LBR dump
 *
 */
#define IOCTL_PERFORM_HYPERTRACE_LBR_DUMP \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x38, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief ioctl, to perform HyperTrace PT operations
 *
 */
#define IOCTL_PERFORM_HYPERTRACE_PT_OPERATION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_START_CODE + 0x39, METHOD_BUFFERED, FILE_ANY_ACCESS)
