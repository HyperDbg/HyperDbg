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

/**
 * @brief Initial debuggee to debugger offset
 *
 */
#define DEFAULT_INITIAL_DEBUGGEE_TO_DEBUGGER_OFFSET 0x200

/**
 * @brief Initial debugger to debuggee offset
 *
 */
#define DEFAULT_INITIAL_DEBUGGER_TO_DEBUGGEE_OFFSET 0x0

/**
 * @brief Initial default buffer size (BRAN Size)
 * @details Number of 4-Byte intergers (256 * 4 Byte * 8 bits = 8-kilobits)
 *
 */
#define DEFAULT_INITIAL_BRAM_BUFFER_SIZE 256

/**
 * @brief Path to read the sample of the instance info
 *
 */
#define HWDBG_TEST_READ_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\sim\\hwdbg\\DebuggerModuleTestingBRAM\\bram_instance_info.txt"

/**
 * @brief Path to write the sample of the script buffer
 *
 */
#define HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH "..\\..\\..\\..\\hwdbg\\src\\test\\bram\\script_buffer.hex.txt"

/**
 * @brief Path to write the sample of the instance info requests
 *
 */
#define HWDBG_TEST_WRITE_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\src\\test\\bram\\instance_info.hex.txt"

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
    hwdbgResponseSuccessOrErrorMessage = 1,
    hwdbgResponseInstanceInfo          = 2,

} HWDBG_RESPONSE_ENUMS;

/**
 * @brief Different success or error codes in hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_SUCCESS_OR_ERROR_ENUMS
{
    hwdbgOperationWasSuccessful = 0x7FFFFFFF,
    hwdbgErrorInvalidPacket     = 1,

} HWDBG_SUCCESS_OR_ERROR_ENUMS;

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
#pragma pack(push, 4) // This is to make sure the structure is packed (without padding alignment)
typedef struct _HWDBG_INSTANCE_INFORMATION
{
    //
    // ANY ADDITION TO THIS STRUCTURE SHOULD BE SYNCHRONIZED WITH SCALA AND INSTANCE INFO SENDER MODULE
    //
    UINT32 version;                                    // Target version of HyperDbg (same as hwdbg)
    UINT32 maximumNumberOfStages;                      // Number of stages that this instance of hwdbg supports (NumberOfSupportedStages == 0 means script engine is disabled)
    UINT32 scriptVariableLength;                       // Maximum length of variables (and other script elements)
    UINT32 numberOfSupportedLocalAndGlobalVariables;   // Number of supported local (and global) variables
    UINT32 numberOfSupportedTemporaryVariables;        // Number of supported temporary variables
    UINT32 maximumNumberOfSupportedGetScriptOperators; // Maximum supported GET operators in a single func
    UINT32 maximumNumberOfSupportedSetScriptOperators; // Maximum supported SET operators in a single func
    UINT32 sharedMemorySize;                           // Size of shared memory
    UINT32 debuggerAreaOffset;                         // The memory offset of debugger
    UINT32 debuggeeAreaOffset;                         // The memory offset of debuggee
    UINT32 numberOfPins;                               // Number of pins
    UINT32 numberOfPorts;                              // Number of ports

    //
    // ANY ADDITION TO THIS STRUCTURE SHOULD BE SYNCHRONIZED WITH SCALA AND INSTANCE INFO SENDER MODULE
    //

    struct _HWDBG_SCRIPT_CAPABILITIES
    {
        //
        // ANY ADDITION TO THIS MASK SHOULD BE ADDED TO HwdbgInterpreterShowScriptCapabilities
        // and HwdbgInterpreterCheckScriptBufferWithScriptCapabilities as well Scala file
        //
        UINT64 assign_local_global_var : 1;
        UINT64 assign_registers : 1;
        UINT64 assign_pseudo_registers : 1;
        UINT64 conditional_statements_and_comparison_operators : 1;
        UINT64 stack_assignments : 1;

        UINT64 func_or : 1;
        UINT64 func_xor : 1;
        UINT64 func_and : 1;
        UINT64 func_asr : 1;
        UINT64 func_asl : 1;
        UINT64 func_add : 1;
        UINT64 func_sub : 1;
        UINT64 func_mul : 1;
        UINT64 func_div : 1;
        UINT64 func_mod : 1;
        UINT64 func_gt : 1;
        UINT64 func_lt : 1;
        UINT64 func_egt : 1;
        UINT64 func_elt : 1;
        UINT64 func_equal : 1;
        UINT64 func_neq : 1;
        UINT64 func_jmp : 1;
        UINT64 func_jz : 1;
        UINT64 func_jnz : 1;
        UINT64 func_mov : 1;
        UINT64 func_printf : 1;

        //
        // ANY ADDITION TO THIS MASK SHOULD BE ADDED TO HwdbgInterpreterShowScriptCapabilities
        // and HwdbgInterpreterCheckScriptBufferWithScriptCapabilities as well Scala file
        //

    } scriptCapabilities;

    UINT32 bramAddrWidth; // BRAM address width
    UINT32 bramDataWidth; // BRAM data width

    //
    // Here the details of port arrangements are located (HWDBG_PORT_INFORMATION_ITEMS)
    // As the following type:
    //   HWDBG_PORT_INFORMATION_ITEMS portsConfiguration[numberOfPorts]   ; Port arrangement
    //

} HWDBG_INSTANCE_INFORMATION, *PHWDBG_INSTANCE_INFORMATION;
#pragma pack(pop) // This is to make sure the structure is packed (without padding alignment)

/**
 * @brief The structure of script buffer in hwdbg
 *
 */
typedef struct _HWDBG_SCRIPT_BUFFER
{
    UINT32 scriptNumberOfSymbols; // Number of symbols in the script

    //
    // Here the script buffer is located
    //
    // UINT8 scriptBuffer[scriptNumberOfSymbols]; // The script buffer
    //

} HWDBG_SCRIPT_BUFFER, *PHWDBG_SCRIPT_BUFFER;
