/**
 * @file export.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Exported functions from libhyperdbg interface
 * @details
 * @version 0.10
 * @date 2024-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern TCHAR   g_DriverLocation[MAX_PATH];
extern TCHAR   g_DriverName[MAX_PATH];
extern BOOLEAN g_UseCustomDriverLocation;

/**
 * @brief Detects the support of VMX
 *
 * @return BOOLEAN Returns true if the CPU supports VMX
 */
BOOLEAN
hyperdbg_u_detect_vmx_support()
{
    return VmxSupportDetection();
}

/**
 * @brief Read the vendor string of the CPU
 *
 * @param vendor_string The buffer to store the vendor string
 * @return VOID
 */
VOID
hyperdbg_u_read_vendor_string(CHAR * vendor_string)
{
    CpuReadVendorString(vendor_string);
}

/**
 * @brief Load the VMM
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_load_vmm()
{
    return HyperDbgLoadVmmModule();
}

/**
 * @brief Unload the VMM
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_unload_vmm()
{
    return HyperDbgUnloadVmm();
}

/**
 * @brief Install the VMM driver
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_install_vmm_driver()
{
    return HyperDbgInstallVmmDriver();
}

/**
 * @brief Uninstall the VMM driver
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_uninstall_vmm_driver()
{
    return HyperDbgUninstallVmmDriver();
}

/**
 * @brief Stop the VMM driver
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_stop_vmm_driver()
{
    return HyperDbgStopVmmDriver();
}

/**
 * @brief Run a command
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_run_command(CHAR * command)
{
    return HyperDbgInterpreter(command);
}

/**
 * @brief Parse the command (used for testing purposes)
 *
 * @param command The text of command
 * @param number_of_tokens The number of tokens
 * @param tokens_list The list of tokens
 *
 * @return BOOLEAN returns true if the command was parsed successfully and false if there was an error
 */
BOOLEAN
hyperdbg_u_test_command_parser(CHAR *   command,
                               UINT32   number_of_tokens,
                               CHAR **  tokens_list,
                               UINT32 * failed_token_num,
                               UINT32 * failed_token_position)
{
    return HyperDbgTestCommandParser(command, number_of_tokens, tokens_list, failed_token_num, failed_token_position);
}

/**
 * @brief Parse and show tokens for the command (used for testing purposes)
 *
 * @param command The text of command
 *
 * @return VOID
 */
VOID
hyperdbg_u_test_command_parser_show_tokens(CHAR * command)
{
    return HyperDbgTestCommandParserShowTokens(command);
}

/**
 * @brief Show the signature of the debugger
 *
 * @return VOID
 */
VOID
hyperdbg_u_show_signature()
{
    HyperDbgShowSignature();
}

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown (by passing message as a parameter)
 *
 * @param handler Function that handles the messages
 *
 * @return VOID
 */
VOID
hyperdbg_u_set_text_message_callback(PVOID handler)
{
    SetTextMessageCallback(handler);
}

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown (using shared buffer method)
 *
 * @param handler Function that handles the messages
 *
 * @return PVOID
 */
PVOID
hyperdbg_u_set_text_message_callback_using_shared_buffer(PVOID handler)
{
    return SetTextMessageCallbackUsingSharedBuffer(handler);
}

/**
 * @brief Unset the function callback that will be called if any message
 * needs to be shown
 *
 * @return VOID
 */
VOID
hyperdbg_u_unset_text_message_callback()
{
    UnsetTextMessageCallback();
}

/**
 * @brief Parsing the command line options for scripts
 * @param argc
 * @param argv
 *
 * @return INT
 */
INT
hyperdbg_u_script_read_file_and_execute_commandline(INT argc, CHAR * argv[])
{
    return ScriptReadFileAndExecuteCommandline(argc, argv);
}

/**
 * @brief Some of commands like stepping commands (i, p, t) and etc.
 * need to be repeated when the user press enter, this function shows
 * whether we should continue the previous command or not
 *
 * @return TRUE means the command should be continued, FALSE means command
 * should be ignored
 */
BOOLEAN
hyperdbg_u_continue_previous_command()
{
    return ContinuePreviousCommand();
}

/**
 * @brief Check if the command is a multiline command or not
 *
 * @param current_command The current command
 * @param reset If it's true, it will reset the multiline command
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_check_multiline_command(CHAR * current_command, BOOLEAN reset)
{
    return CheckMultilineCommand(current_command, reset);
}

/**
 * @brief Connect to the local debugger
 *
 * @return VOID
 */
VOID
hyperdbg_u_connect_local_debugger()
{
    ConnectLocalDebugger();
}
/**
 * @brief Connect to the remote debugger
 *
 * @param ip The IP address of the remote debugger
 * @param port The port of the remote debugger
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_connect_remote_debugger(const CHAR * ip, const CHAR * port)
{
    return ConnectRemoteDebugger(ip, port);
}

/**
 * @brief Continue the debuggee (equal to the 'g' command)
 *
 * @return VOID
 */
VOID
hyperdbg_u_continue_debuggee()
{
    CommandGRequest();
}

/**
 * @brief Pause the debuggee (equal to the 'pause' command)
 *
 * @return VOID
 */
VOID
hyperdbg_u_pause_debuggee()
{
    CommandPauseRequest();
}

/**
 * @brief Set a breakpoint
 * @param address The address of the breakpoint
 * @param pid The process ID of the breakpoint
 * @param tid The thread ID of the breakpoint
 * @param core_numer The core number of the breakpoint
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_set_breakpoint(UINT64 address, UINT32 pid, UINT32 tid, UINT32 core_numer)
{
    return CommandBpRequest(address, pid, tid, core_numer);
}

/**
 * @brief Set custom driver path
 *
 * @param driver_file_path The path of the driver
 * @param driver_name The name of the driver
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_set_custom_driver_path(CHAR * driver_file_path, CHAR * driver_name)
{
    if (strlen(driver_file_path) > MAX_PATH)
    {
        ShowMessages("The driver path is too long, the maximum length is %d\n", MAX_PATH);
        return FALSE;
    }

    if (strlen(driver_name) > MAX_PATH)
    {
        ShowMessages("The driver name is too long, the maximum length is %d\n", MAX_PATH);
        return FALSE;
    }

    //
    // Copy the driver path
    //
    strcpy_s(g_DriverLocation, MAX_PATH, driver_file_path);

    //
    // Copy the driver name
    //
    strcpy_s(g_DriverName, MAX_PATH, driver_name);

    //
    // Set the flag to use the custom driver path
    //
    g_UseCustomDriverLocation = TRUE;

    return TRUE;
}

/**
 * @brief Use the default driver path
 *
 * @return VOID
 */
VOID
hyperdbg_u_use_default_driver_path()
{
    //
    // Set the flag to use the default driver path
    //
    g_UseCustomDriverLocation = FALSE;
}

/**
 * @brief Read memory and disassembler
 *
 * @param target_address location of where to read the memory
 * @param memory_type type of memory (phyical or virtual)
 * @param reading_Type read from kernel or vmx-root
 * @param pid The target process id
 * @param size size of memory to read
 * @param get_address_mode check for address mode
 * @param address_mode Address mode (32 or 64)
 * @param target_buffer_to_store The buffer to store the read memory
 * @param return_length The length of the read memory
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_read_memory(UINT64                              target_address,
                       DEBUGGER_READ_MEMORY_TYPE           memory_type,
                       DEBUGGER_READ_READING_TYPE          reading_Type,
                       UINT32                              pid,
                       UINT32                              size,
                       BOOLEAN                             get_address_mode,
                       DEBUGGER_READ_MEMORY_ADDRESS_MODE * address_mode,
                       BYTE *                              target_buffer_to_store,
                       UINT32 *                            return_length)
{
    return HyperDbgReadMemory(target_address, memory_type, reading_Type, pid, size, get_address_mode, address_mode, target_buffer_to_store, return_length);
}

/**
 * @brief Show memory or disassembler
 *
 * @param style style of show memory (as byte, dwrod, qword)
 * @param address location of where to read the memory
 * @param memory_type type of memory (phyical or virtual)
 * @param reading_type read from kernel or vmx-root
 * @param pid The target process id
 * @param size size of memory to read
 * @param dt_details Options for dt structure show details
 *
 * @return VOID
 */
VOID
hyperdbg_u_show_memory_or_disassemble(DEBUGGER_SHOW_MEMORY_STYLE   style,
                                      UINT64                       address,
                                      DEBUGGER_READ_MEMORY_TYPE    memory_type,
                                      DEBUGGER_READ_READING_TYPE   reading_type,
                                      UINT32                       pid,
                                      UINT32                       size,
                                      PDEBUGGER_DT_COMMAND_OPTIONS dt_details)
{
    HyperDbgShowMemoryOrDisassemble(style, address, memory_type, reading_type, pid, size, dt_details);
}

/**
 * @brief Read all registers
 * @param guest_registers The buffer to store the registers
 * @param extra_registers The buffer to store the extra registers
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_read_all_registers(GUEST_REGS * guest_registers, GUEST_EXTRA_REGISTERS * extra_registers)
{
    return HyperDbgReadAllRegisters(guest_registers, extra_registers);
}

/**
 * @brief Read target register
 * @param register_id The target register
 * @param target_register The buffer to store the register
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_read_target_register(REGS_ENUM register_id, UINT64 * target_register)
{
    return HyperDbgReadTargetRegister(register_id, target_register);
}

/**
 * @brief Write target register
 * @param register_id The target register
 * @param value The value to write
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_write_target_register(REGS_ENUM register_id, UINT64 value)
{
    return HyperDbgWriteTargetRegister(register_id, value);
}

/**
 * @brief Show all registers
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_show_all_registers()
{
    return HyperDbgRegisterShowAll();
}

/**
 * @brief Show target register
 * @param register_id The target register
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_show_target_register(REGS_ENUM register_id)
{
    return HyperDbgRegisterShowTargetRegister(register_id);
}

/**
 * @brief Write memory
 * @param destination_address The destination address
 * @param memory_type The type of memory (physical or virtual)
 * @param process_id The target process id (if it's virtual memory)
 * @param source_address The source address
 * @param number_of_bytes The number of bytes to write
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
hyperdbg_u_write_memory(PVOID                     destination_address,
                        DEBUGGER_EDIT_MEMORY_TYPE memory_type,
                        UINT32                    process_id,
                        PVOID                     source_address,
                        UINT32                    number_of_bytes)
{
    return HyperDbgWriteMemory(destination_address, memory_type, process_id, source_address, number_of_bytes);
}

/**
 * @brief Get the kernel base
 *
 * @return UINT64 The kernel base
 */
UINT64
hyperdbg_u_get_kernel_base()
{
    return DebuggerGetKernelBase();
}

/**
 * @brief Connect to the remote debugger using COM port
 *
 * @param port_name The port name
 * @param baudrate The baudrate
 * @param pause_after_connection Pause after connection
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_connect_remote_debugger_using_com_port(const CHAR * port_name, DWORD baudrate, BOOLEAN pause_after_connection)
{
    return HyperDbgDebugRemoteDeviceUsingComPort(port_name, baudrate, pause_after_connection);
}

/**
 * @brief Connect to the remote debugger using named pipe
 *
 * @param named_pipe The named pipe
 * @param pause_after_connection Pause after connection
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_connect_remote_debugger_using_named_pipe(const CHAR * named_pipe, BOOLEAN pause_after_connection)
{
    return HyperDbgDebugRemoteDeviceUsingNamedPipe(named_pipe, pause_after_connection);
}

/**
 * @brief Close the remote debugger
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_debug_close_remote_debugger()
{
    return HyperDbgDebugCloseRemoteDebugger();
}

/**
 * @brief Connect to the current debugger using COM port
 *
 * @param port_name The port name
 * @param baudrate The baudrate
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_connect_current_debugger_using_com_port(const CHAR * port_name, DWORD baudrate)
{
    return HyperDbgDebugCurrentDeviceUsingComPort(port_name, baudrate);
}

/**
 * @brief Start a new process
 *
 * @param path The path of the process
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_start_process(const WCHAR * path)
{
    return UdAttachToProcess(NULL,
                             path,
                             NULL,
                             FALSE);
}

/**
 * @brief Start a new process
 *
 * @param path The path of the process
 * @param arguments The arguments of the process
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_start_process_with_args(const WCHAR * path, const WCHAR * arguments)
{
    return UdAttachToProcess(NULL,
                             path,
                             arguments,
                             FALSE);
}

/**
 * @brief Assembler function to get the length of the assembly code
 *
 * @param assembly_code The assembly code
 * @param start_address The start address of the assembly code
 * @param length The length of the assembly code
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_assemble_get_length(const CHAR * assembly_code, UINT64 start_address, UINT32 * length)
{
    return HyperDbgAssembleGetLength(assembly_code, start_address, length);
}

/**
 * @brief Assembler function
 *
 * @param assembly_code The assembly code
 * @param start_address The start address of the assembly code
 * @param buffer_to_store_assembled_data The buffer to store the assembled data
 * @param buffer_size The size of the buffer
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
hyperdbg_u_assemble(const CHAR * assembly_code, UINT64 start_address, PVOID buffer_to_store_assembled_data, UINT32 buffer_size)
{
    return HyperDbgAssemble(assembly_code, start_address, buffer_to_store_assembled_data, buffer_size);
}

/**
 * @brief Setip the path for the filename
 *
 * @param filename The filename
 * @param file_location
 * @param buffer_len
 * @param check_file_existence
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_setup_path_for_filename(const CHAR * filename, CHAR * file_location, UINT32 buffer_len, BOOLEAN check_file_existence)
{
    return SetupPathForFileName(filename, file_location, buffer_len, check_file_existence);
}

/**
 * @brief Perform instrumentation step-in
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_stepping_instrumentation_step_in()
{
    return SteppingInstrumentationStepIn();
}

/**
 * @brief Perform regular step-in
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_stepping_regular_step_in()
{
    return SteppingRegularStepIn();
}

/**
 * @brief Perform step-over
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_stepping_step_over()
{
    return SteppingStepOver();
}

/**
 * @brief Perform instrumentation step-in for tracking
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_stepping_instrumentation_step_in_for_tracking()
{
    return SteppingInstrumentationStepInForTracking();
}

/**
 * @brief Perform step-over for gu
 *
 * @param last_instruction
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_stepping_step_over_for_gu(BOOLEAN last_instruction)
{
    return SteppingStepOverForGu(last_instruction);
}

/**
 * @brief Get Local APIC
 * @details The system automatically detects whether to read it
 * in the xAPIC or x2APIC mode
 *
 * @param local_apic
 * @param is_using_x2apic
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_get_local_apic(PLAPIC_PAGE local_apic, BOOLEAN * is_using_x2apic)
{
    return HyperDbgGetLocalApic(local_apic, is_using_x2apic);
}

/**
 * @brief Get I/O APIC
 *
 * @param io_apic
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_get_io_apic(IO_APIC_ENTRY_PACKETS * io_apic)
{
    return HyperDbgGetIoApic(io_apic);
}

/**
 * @brief Get IDT entry
 *
 * @param idt_packet
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_get_idt_entry(INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS * idt_packet)
{
    return HyperDbgGetIdtEntry(idt_packet);
}

/**
 * @brief Perform SMI operation
 *
 * @param SmiOperation The SMI operation packet
 *
 * @return
 */
BOOLEAN
hyperdbg_u_perform_smi_operation(SMI_OPERATION_PACKETS * SmiOperation)
{
    return HyperDbgPerformSmiOperation(SmiOperation);
}

/**
 * @brief Run hwdbg script
 *
 * @param script
 * @param instance_filepath_to_read
 * @param hardware_script_file_path_to_save
 * @param initial_bram_buffer_size
 *
 * @return BOOLEAN
 */
BOOLEAN
hwdbg_script_run_script(const CHAR * script,
                        const CHAR * instance_filepath_to_read,
                        const CHAR * hardware_script_file_path_to_save,
                        UINT32       initial_bram_buffer_size)
{
    return HwdbgScriptRunScript(script,
                                instance_filepath_to_read,
                                hardware_script_file_path_to_save,
                                initial_bram_buffer_size);
}

/**
 * @brief Run (test evaluation) hwdbg script
 *
 * @param Expr
 *
 * @return VOID
 */
VOID
hwdbg_script_engine_wrapper_test_parser(const char * Expr)
{
    std::string StrExpr = Expr; // Convert const char* to std::string
    ScriptEngineWrapperTestParserForHwdbg(StrExpr);
}

/**
 * @brief Enable transparent mode
 * @param ProcessId The process ID to enable transparent mode for
 * @param ProcessName The process name to enable transparent mode for
 * @param IsProcessId If true, ProcessId is used, otherwise ProcessName is used
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_enable_transparent_mode(UINT32 ProcessId, CHAR * ProcessName, BOOLEAN IsProcessId)
{
    return HyperDbgEnableTransparentMode(ProcessId, ProcessName, IsProcessId);
}

/**
 * @brief Disable transparent mode
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_disable_transparent_mode()
{
    return HyperDbgDisableTransparentMode();
}

/**
 * @brief Run HyperDbg scripts in the target process (user debugger) or
 * debuggee (kernel debugger)
 * @param Expr The expression to run
 * @param ShowErrorMessageIfAny If true, show error message if any
 *
 * @return BOOLEAN
 */
BOOLEAN
hyperdbg_u_run_script(CHAR * Expr, BOOLEAN ShowErrorMessageIfAny)
{
    return ScriptEngineExecuteSingleExpression(Expr, ShowErrorMessageIfAny, FALSE);
}

/**
 * @brief Evaluate expression and return result based on the target
 * process (user debugger) or debuggee (kernel debugger) context
 *
 * @param Expr The expression to evaluate
 * @param HasError If true, there was an error in evaluation
 *
 * @return UINT64 The result of the evaluated expression
 */
UINT64
hyperdbg_u_eval_expression(CHAR * Expr, PBOOLEAN HasError)
{
    return ScriptEngineEvalSingleExpression(Expr, HasError);
}
