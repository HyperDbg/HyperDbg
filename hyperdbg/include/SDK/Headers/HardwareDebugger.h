/**
 * @file HardwareDebugger.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's Hardware Debugger (hwdbg) types and constants
 * @details This file contains definitions of hwdbg elements
 * used in HyperDbg
 * @version 0.9
 * @date 2024-04-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                 Definitions                  //
//////////////////////////////////////////////////

#define HWDBG_TEST_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\sim\\hwdbg\\DebuggerModuleTestingBRAM\\bram_content_after_emulation.txt"

//////////////////////////////////////////////////
//                   Enums                      //
//////////////////////////////////////////////////

/**
 * @brief Different action of hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_ACTION_ENUMS
{
    hwdbgActionSendInstanceInfo      = 1,
    hwdbgActionConfigureScriptBuffer = 2,

} HWDBG_ACTION_ENUMS;

/**
 * @brief Different responses come from hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_RESPONSE_ENUMS
{
    hwdbgResponseInvalidPacketOrError            = 1,
    hwdbgResponseInstanceInfo                    = 2,
    hwdbgResponseScriptBufferConfigurationResult = 3,

} HWDBG_RESPONSE_ENUMS;

/**
 * @brief Different error codes in hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_ERROR_ENUMS
{
    hwdbgErrorInvalidPacket = 1,

} HWDBG_ERROR_ENUMS;

//////////////////////////////////////////////////
//                   Structures                 //
//////////////////////////////////////////////////

/**
 * @brief The structure of port information (each item) in hwdbg
 *
 */
typedef struct _HWDBG_PORT_INFORMATION_ITEMS
{
    UINT32 PortSize;

} HWDBG_PORT_INFORMATION_ITEMS, *PHWDBG_PORT_INFORMATION_ITEMS;

/**
 * @brief The structure of script capabilities information in hwdbg
 *
 */
typedef struct _HWDBG_INSTANCE_INFORMATION
{
    UINT32 Version;                                 // Target version of HyperDbg (same as hwdbg)
    UINT32 MaximumNumberOfStages;                   // Number of stages that this instance of hwdbg supports (NumberOfSupportedStages == 0 means script engine is disabled)
    UINT32 scriptVariableLength;                    // maximum length of variables (and other script elements)
    UINT32 maximumNumberOfSupportedScriptOperators; // maximum supported operators in a single func
    UINT32 numberOfPins;                            // Number of pins
    UINT32 numberOfPorts;                           // Number of ports

    struct _HWDBG_SCRIPT_CAPABILITIES
    {
        UINT64 op_inc : 1;
        UINT64 op_dec : 1;
        UINT64 op_or : 1;
        UINT64 op_xor : 1;
        UINT64 op_and : 1;
        UINT64 op_asr : 1;
        UINT64 op_asl : 1;
        UINT64 op_add : 1;
        UINT64 op_sub : 1;
        UINT64 op_mul : 1;
        UINT64 op_div : 1;
        UINT64 op_mod : 1;
        UINT64 op_gt : 1;
        UINT64 op_lt : 1;
        UINT64 op_egt : 1;
        UINT64 op_elt : 1;
        UINT64 op_equal : 1;
        UINT64 op_neq : 1;
        UINT64 op_jmp : 1;
        UINT64 op_jz : 1;
        UINT64 op_jnz : 1;
        UINT64 op_mov : 1;
        UINT64 op_printf : 1;

    } scriptCapabilities;

    //
    // Here the details of port arrangements are located (HWDBG_PORT_INFORMATION_ITEMS)
    // As the following type:
    //   HWDBG_PORT_INFORMATION_ITEMS portsConfiguration[numberOfPorts]   ; Port arrangement
    //

} HWDBG_SCRIPT_CAPABILITIES_INFORMATION, *PHWDBG_SCRIPT_CAPABILITIES_INFORMATION;
