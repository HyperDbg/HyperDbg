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
