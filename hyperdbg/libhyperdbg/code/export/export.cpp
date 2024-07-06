/**
 * @file export.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Exported functions from libhyperdbg interface
 * @details
 * @version 1.0
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
 * @brief Interprets the command
 *
 * @return INT Returns 0 if it was successful and 1 if it was failed
 */
INT
hyperdbg_u_interpreter(CHAR * command)
{
    return HyperDbgInterpreter(command);
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
 * needs to be shown
 *
 * @param handler Function that handles the messages
 *
 * @return VOID
 */
VOID
hyperdbg_u_set_text_message_callback(Callback handler)
{
    SetTextMessageCallback(handler);
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
 * @brief Pause the debuggee (equal to the 'pause' command or CTRL+C)
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
 * @return VOID
 */
VOID
hyperdbg_u_set_breakpoint(UINT64 address, UINT32 pid, UINT32 tid, UINT32 core_numer)
{
    CommandBpRequest(address, pid, tid, core_numer);
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
