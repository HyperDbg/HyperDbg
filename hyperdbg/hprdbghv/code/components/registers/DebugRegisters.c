/**
 * @file DebugRegisters.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debug registers functions
 * @details
 * 
 * @version 0.1
 * @date 2021-11-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Configure hardware debug register for access, write and 
 * fetch breakpoints
 * @details if apply to vmcs is true then should be called at vmx-root mode
 * keep in mind that it applies only on one core
 * Also, the caller must be sure that Load Debug Controls and Save Debug
 * Controls on VM-entry and VM-exit controls on the VMCS of the
 * target core, vmcalls VMCALL_SET_VM_ENTRY_LOAD_DEBUG_CONTROLS and
 * VMCALL_SET_VM_EXIT_SAVE_DEBUG_CONTROLS are designd for this purpose
 * should be called on vmx-root mode if the ApplyToVmcs is TRUE
 * 
 * @param DebugRegNum Debug register that want to apply to it (can be between
 * 0 to 3 as current processors support only 4 locations on hardware debug
 * register)
 * @param ActionType Type of breakpoint (Access, write, fetch)
 * @param ApplyToVmcs Apply on GUEST_RIP register of VMCS, see details above
 * for more information
 * @param TargetAddress Target breakpoint virtual address
 * @return BOOLEAN If TRUE, shows the request configuration is correct, otherwise
 * it's either not supported or not correct configuration
 */
BOOLEAN
DebugRegistersSet(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress)
{
    DR7 Dr7 = {0};

    //
    // Debug registers can be dr0, dr1, dr2, dr3
    //
    if (DebugRegNum >= 4)
    {
        return FALSE;
    }

    //
    // Configure the dr7 (dr6 is only to show the status)
    // the configuration derived from https://stackoverflow.com/questions/40818920/
    //
    // Check-list:
    //     - Set the reserved bits to their right values
    //     - Set DR7.LE and DR7.GE to 1
    //     - Set DR7.L0(L1, L2, L3) to 1 [local breakpoint]
    //     - Make sure DR7.RW/0 (RW/1, RW/2, RW/3) is 0 [break on instruction exec]
    //     - Make sure DR7.LEN0 (LEN1, LEN2, LEN3) is 0 [1 byte length]
    //     - Set DR0 (1, 2, 3) to the instruction linear address
    //     - Make sure linear address [DR0 to DR3] falls on the first byte of the instruction
    //

    //
    // Must be 1
    //
    Dr7.Reserved1 = 1;

    //
    // Based on Intel Manual:
    // we recommend that the LE and GE flags be set to 1 if exact breakpoints are required
    //
    Dr7.LocalExactBreakpoint  = 1;
    Dr7.GlobalExactBreakpoint = 1;

    //
    // Set the target address and enable it on dr7
    //
    if (DebugRegNum == 0)
    {
        __writedr(0, TargetAddress);

        Dr7.GlobalBreakpoint0 = 1;

        //
        // Based on SDM :
        //   00 - Break on instruction execution only.
        //   01 - Break on data writes only.
        //   10 - Break on I/O reads or writes.
        //   11 - Break on data reads or writes but not instruction fetches
        // Also 10, is based on another bit so it is configured based on
        // other bits, read the SDM for more.
        //

        switch (ActionType)
        {
        case BREAK_ON_INSTRUCTION_FETCH:
            Dr7.ReadWrite0 = 0b00; // 0b00 => 0
            break;
        case BREAK_ON_WRITE_ONLY:
            Dr7.ReadWrite0 = 0b01; // 0b01 => 1
            break;
        case BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED:
            Dr7.ReadWrite0 = 0b10; // 0b10 => 2
            LogError("Err, I/O access breakpoint by debug regs are not supported");
            return FALSE;
            break;
        case BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH:
            Dr7.ReadWrite0 = 0b11; // 0b11 => 3
            break;

        default:
            //
            // what?
            //
            LogError("Err, unknown parameter as debug reg action type");
            return FALSE;
            break;
        }
    }
    else if (DebugRegNum == 1)
    {
        __writedr(1, TargetAddress);
        Dr7.GlobalBreakpoint1 = 1;

        //
        // Based on SDM :
        //   00 - Break on instruction execution only.
        //   01 - Break on data writes only.
        //   10 - Break on I/O reads or writes.
        //   11 - Break on data reads or writes but not instruction fetches
        // Also 10, is based on another bit so it is configured based on
        // other bits, read the SDM for more.
        //

        switch (ActionType)
        {
        case BREAK_ON_INSTRUCTION_FETCH:
            Dr7.ReadWrite1 = 0b00; // 0b00 => 0
            break;
        case BREAK_ON_WRITE_ONLY:
            Dr7.ReadWrite1 = 0b01; // 0b01 => 1
            break;
        case BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED:
            Dr7.ReadWrite1 = 0b10; // 0b10 => 2
            LogError("Err, I/O access breakpoint by debug regs are not supported");
            return FALSE;
            break;
        case BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH:
            Dr7.ReadWrite1 = 0b11; // 0b11 => 3
            break;

        default:
            //
            // what?
            //
            LogError("Err, unknown parameter as debug reg action type");
            return FALSE;
            break;
        }
    }
    else if (DebugRegNum == 2)
    {
        __writedr(2, TargetAddress);
        Dr7.GlobalBreakpoint2 = 1;

        //
        // Based on SDM :
        //   00 - Break on instruction execution only.
        //   01 - Break on data writes only.
        //   10 - Break on I/O reads or writes.
        //   11 - Break on data reads or writes but not instruction fetches
        // Also 10, is based on another bit so it is configured based on
        // other bits, read the SDM for more.
        //

        switch (ActionType)
        {
        case BREAK_ON_INSTRUCTION_FETCH:
            Dr7.ReadWrite2 = 0b00; // 0b00 => 0
            break;
        case BREAK_ON_WRITE_ONLY:
            Dr7.ReadWrite2 = 0b01; // 0b01 => 1
            break;
        case BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED:
            Dr7.ReadWrite2 = 0b10; // 0b10 => 2
            LogError("Err, I/O access breakpoint by debug regs are not supported");
            return FALSE;
            break;
        case BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH:
            Dr7.ReadWrite2 = 0b11; // 0b11 => 3
            break;

        default:
            //
            // what?
            //
            LogError("Err, unknown parameter as debug reg action type");
            return FALSE;
            break;
        }
    }
    else if (DebugRegNum == 3)
    {
        __writedr(3, TargetAddress);
        Dr7.GlobalBreakpoint3 = 1;

        //
        // Based on SDM :
        //   00 - Break on instruction execution only.
        //   01 - Break on data writes only.
        //   10 - Break on I/O reads or writes.
        //   11 - Break on data reads or writes but not instruction fetches
        // Also 10, is based on another bit so it is configured based on
        // other bits, read the SDM for more.
        //

        switch (ActionType)
        {
        case BREAK_ON_INSTRUCTION_FETCH:
            Dr7.ReadWrite3 = 0b00; // 0b00 => 0
            break;
        case BREAK_ON_WRITE_ONLY:
            Dr7.ReadWrite3 = 0b01; // 0b01 => 1
            break;
        case BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED:
            Dr7.ReadWrite3 = 0b10; // 0b10 => 2
            LogError("Err, I/O access breakpoint by debug regs are not supported");
            return FALSE;
            break;
        case BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH:
            Dr7.ReadWrite3 = 0b11; // 0b11 => 3
            break;

        default:
            //
            // what?
            //
            LogError("Err, unknown parameter as debug reg action type");
            return FALSE;
            break;
        }
    }

    //
    // Applies to debug register 7, the caller must be sure that Load Debug
    // Controls and Save Debug Controls on VM-entry and VM-exit controls
    // on the VMCS of the target core
    //
    if (ApplyToVmcs)
    {
        __vmx_vmwrite(VMCS_GUEST_DR7, Dr7.AsUInt);
    }
    else
    {
        __writedr(7, Dr7.AsUInt);
    }

    return TRUE;
}
