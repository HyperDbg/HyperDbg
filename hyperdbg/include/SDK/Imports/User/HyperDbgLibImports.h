/**
 * @file HyperDbgLibImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from controller interface
 * @version 0.2
 * @date 2023-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_LIBHYPERDBG
#    define IMPORT_EXPORT_LIBHYPERDBG __declspec(dllexport)
#else
#    define IMPORT_EXPORT_LIBHYPERDBG __declspec(dllimport)
#endif

//
// Header file of libhyperdbg
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

//
// Support Detection
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_detect_vmx_support();

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_read_vendor_string(CHAR *);

//
// VMM Module
//
IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_load_vmm();

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_unload_vmm();

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_install_vmm_driver();

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_uninstall_vmm_driver();

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_stop_vmm_driver();

//
// General imports
//
IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_interpreter(CHAR * command);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_show_signature();

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_set_text_message_callback(Callback handler);

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_script_read_file_and_execute_commandline(INT argc, CHAR * argv[]);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_continue_previous_command();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_check_multiline_command(CHAR * current_command, BOOLEAN reset);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_set_custom_driver_path(CHAR * driver_file_path, CHAR * driver_name);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_use_default_driver_path();

//
// Reading memory
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_read_memory(UINT64                              target_address,
                       DEBUGGER_READ_MEMORY_TYPE           memory_type,
                       DEBUGGER_READ_READING_TYPE          reading_Type,
                       UINT32                              pid,
                       UINT32                              size,
                       BOOLEAN                             get_address_mode,
                       DEBUGGER_READ_MEMORY_ADDRESS_MODE * address_mode,
                       BYTE *                              target_buffer_to_store,
                       UINT32 *                            return_length);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_show_memory_or_disassemble(DEBUGGER_SHOW_MEMORY_STYLE   style,
                                      UINT64                       address,
                                      DEBUGGER_READ_MEMORY_TYPE    memory_type,
                                      DEBUGGER_READ_READING_TYPE   reading_type,
                                      UINT32                       pid,
                                      UINT32                       size,
                                      PDEBUGGER_DT_COMMAND_OPTIONS dt_details);

//
// Connect to local or remote debugger
// Exported functionality of the '.connect' command
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_connect_local_debugger();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_connect_remote_debugger(const CHAR * ip, const CHAR * port);

//
// Continue debuggee
// Exported functionality of the 'g' command
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_continue_debuggee();

//
// Pause debuggee
// Exported functionality of the 'pause' command or CTRL+C
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_pause_debuggee();

//
// Set breakpoint
// Exported functionality of the 'bp' command
//
VOID
hyperdbg_u_set_breakpoint(UINT64 address, UINT32 pid, UINT32 tid, UINT32 core_numer);

#ifdef __cplusplus
}
#endif
