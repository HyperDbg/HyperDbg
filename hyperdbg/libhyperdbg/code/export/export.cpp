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
extern BOOLEAN g_UseCustomDriverLocation;

BOOLEAN
hyperdbg_u_detect_vmx_support()
{
    return VmxSupportDetection();
}

VOID
hyperdbg_u_read_vendor_string(CHAR * vendor_string)
{
    CpuReadVendorString(vendor_string);
}

INT
hyperdbg_u_load_vmm()
{
    return HyperDbgLoadVmm();
}

INT
hyperdbg_u_unload_vmm()
{
    return HyperDbgUnloadVmm();
}

INT
hyperdbg_u_install_vmm_driver()
{
    return HyperDbgInstallVmmDriver();
}

INT
hyperdbg_u_uninstall_vmm_driver()
{
    return HyperDbgUninstallVmmDriver();
}

INT
hyperdbg_u_stop_vmm_driver()
{
    return HyperDbgStopVmmDriver();
}

INT
hyperdbg_u_interpreter(CHAR * command)
{
    return HyperDbgInterpreter(command);
}

VOID
hyperdbg_u_show_signature()
{
    HyperDbgShowSignature();
}

VOID
hyperdbg_u_set_text_message_callback(Callback handler)
{
    SetTextMessageCallback(handler);
}

INT
hyperdbg_u_script_read_file_and_execute_commandline(INT argc, CHAR * argv[])
{
    return ScriptReadFileAndExecuteCommandline(argc, argv);
}

BOOLEAN
hyperdbg_u_continue_previous_command()
{
    return ContinuePreviousCommand();
}

BOOLEAN
hyperdbg_u_check_multiline_command(CHAR * current_command, BOOLEAN reset)
{
    return CheckMultilineCommand(current_command, reset);
}

VOID
hyperdbg_u_connect_local_debugger()
{
    ConnectLocalDebugger();
}

BOOLEAN
hyperdbg_u_connect_remote_debugger(const CHAR * ip, const CHAR * port)
{
    return ConnectRemoteDebugger(ip, port);
}

VOID
hyperdbg_u_continue_debuggee()
{
    CommandGRequest();
}

VOID
hyperdbg_u_pause_debuggee()
{
    CommandPauseRequest();
}

VOID
hyperdbg_u_set_breakpoint(UINT64 Address, UINT32 Pid, UINT32 Tid, UINT32 CoreNumer)
{
    CommandBpRequest(Address, Pid, Tid, CoreNumer);
}

BOOLEAN
hyperdbg_u_set_custom_driver_path(CHAR * DriverPath)
{
    if (strlen(DriverPath) > MAX_PATH)
    {
        ShowMessages("The driver path is too long, the maximum length is %d\n", MAX_PATH);
        return FALSE;
    }

    //
    // Copy the driver path
    //
    strcpy_s(g_DriverLocation, MAX_PATH, DriverPath);

    //
    // Set the flag to use the custom driver path
    //
    g_UseCustomDriverLocation = TRUE;

    return TRUE;
}

VOID
hyperdbg_u_use_default_driver_path()
{
    //
    // Set the flag to use the default driver path
    //
    g_UseCustomDriverLocation = FALSE;
}
