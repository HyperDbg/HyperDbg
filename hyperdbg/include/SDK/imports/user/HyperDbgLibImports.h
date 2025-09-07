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
// Testing parser
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_test_command_parser(CHAR *   command,
                               UINT32   number_of_tokens,
                               CHAR **  tokens_list,
                               UINT32 * failed_token_num,
                               UINT32 * failed_token_position);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_test_command_parser_show_tokens(CHAR * command);

//
// General imports/exports
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_setup_path_for_filename(const CHAR * filename, CHAR * file_location, UINT32 buffer_len, BOOLEAN check_file_existence);

IMPORT_EXPORT_LIBHYPERDBG INT
hyperdbg_u_run_command(CHAR * command);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_show_signature();

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_set_text_message_callback(PVOID handler);

IMPORT_EXPORT_LIBHYPERDBG PVOID
hyperdbg_u_set_text_message_callback_using_shared_buffer(PVOID handler);

IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_unset_text_message_callback();

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
// Connect to local or remote debugger
// Exported functionality of the '.connect' command
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_connect_local_debugger();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_connect_remote_debugger(const CHAR * ip, const CHAR * port);

//
// Connect to the debugger in the Debugger Mode
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_connect_remote_debugger_using_com_port(const CHAR * port_name, DWORD baudrate, BOOLEAN pause_after_connection);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_connect_remote_debugger_using_named_pipe(const CHAR * named_pipe, BOOLEAN pause_after_connection);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_connect_current_debugger_using_com_port(const CHAR * port_name, DWORD baudrate);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_debug_close_remote_debugger();

//
// Miscalenous functions
//
IMPORT_EXPORT_LIBHYPERDBG UINT64
hyperdbg_u_get_kernel_base();

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
// Writing memory
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_write_memory(PVOID                     destination_address,
                        DEBUGGER_EDIT_MEMORY_TYPE memory_type,
                        UINT32                    process_id,
                        PVOID                     source_address,
                        UINT32                    number_of_bytes);

//
// Reading/Writing registers
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_read_all_registers(GUEST_REGS * guest_registers, GUEST_EXTRA_REGISTERS * extra_registers);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_read_target_register(REGS_ENUM register_id, UINT64 * target_register);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_write_target_register(REGS_ENUM register_id, UINT64 value);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_show_all_registers();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_show_target_register(REGS_ENUM register_id);

//
// Continue debuggee
// Exported functionality of the 'g' command
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_continue_debuggee();

//
// Pause debuggee
// Exported functionality of the 'pause' command
//
IMPORT_EXPORT_LIBHYPERDBG VOID
hyperdbg_u_pause_debuggee();

//
// Set breakpoint
// Exported functionality of the 'bp' command
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_set_breakpoint(UINT64 address, UINT32 pid, UINT32 tid, UINT32 core_numer);

//
// Stepping and tracing instruction
// Exported functionality of 't', 'p', 'i', '!track', 'gu' commands
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_stepping_instrumentation_step_in();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_stepping_regular_step_in();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_stepping_step_over();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_stepping_instrumentation_step_in_for_tracking();

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_stepping_step_over_for_gu(BOOLEAN last_instruction);

//
// Start a process
// Exported functionality of the '.start' command
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_start_process(const WCHAR * path);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_start_process_with_args(const WCHAR * path, const WCHAR * arguments);

//
// APIC related command
// Exported functionality of the '!apic', and '!ioapic' commands
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_get_local_apic(PLAPIC_PAGE local_apic, BOOLEAN * is_using_x2apic);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_get_io_apic(IO_APIC_ENTRY_PACKETS * io_apic);

//
// IDT related command
// Exported functionality of the '!idt' command
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_get_idt_entry(INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS * idt_packet);

//
// SMM related command
// Exported functionality of the '!smi' command
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_perform_smi_operation(SMI_OPERATION_PACKETS * SmiOperation);

//
// Transparent mode related command
// Exported functionality of the '!hide', and '!unhide' commands
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_enable_transparent_mode(UINT32 ProcessId, CHAR * ProcessName, BOOLEAN IsProcessId);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_disable_transparent_mode();

//
// Assembler
// Exported functionality of the 'a' command
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_assemble_get_length(const CHAR * assembly_code, UINT64 start_address, UINT32 * length);

IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_assemble(const CHAR * assembly_code, UINT64 start_address, PVOID buffer_to_store_assembled_data, UINT32 buffer_size);

//
// hwdbg functions
// Exported functionality of the '!hw' and '!hw_*' commands
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hwdbg_script_run_script(const CHAR * script,
                        const CHAR * instance_filepath_to_read,
                        const CHAR * hardware_script_file_path_to_save,
                        UINT32       initial_bram_buffer_size);

IMPORT_EXPORT_LIBHYPERDBG VOID
hwdbg_script_engine_wrapper_test_parser(const CHAR * Expr);

//
// Run script and evaluate expression
//
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
hyperdbg_u_run_script(CHAR * Expr, BOOLEAN ShowErrorMessageIfAny);

IMPORT_EXPORT_LIBHYPERDBG UINT64
hyperdbg_u_eval_expression(CHAR * Expr, PBOOLEAN HasError);

#ifdef __cplusplus
}
#endif
