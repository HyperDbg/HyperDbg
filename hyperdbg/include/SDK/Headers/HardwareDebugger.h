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
//                   Enums                      //
//////////////////////////////////////////////////

/**
 * @brief Different action of hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_ACTION_ENUMS
{
    hwdbgActionSendVersion           = 1,
    hwdbgActionSendPinInformation    = 2,
    hwdbgActionConfigureScriptBuffer = 3,

} HWDBG_ACTION_ENUMS;

/**
 * @brief Different responses come from hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_RESPONSE_ENUMS
{
    hwdbgResponseInvalidPacketOrError            = 1,
    hwdbgResponseVersion                         = 2,
    hwdbgResponsePinInformation                  = 3,
    hwdbgResponseScriptBufferConfigurationResult = 4,

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
 * @brief The structure of port information in hwdbg
 *
 */
typedef struct _HWDBG_PORT_INFORMATION
{
    UINT32 CountOfPorts;

    /*

    Here the pin information details will be available.

        UINT32
        Port Size

    */

} HWDBG_PORT_INFORMATION, *PHWDBG_PORT_INFORMATION;

/**
 * @brief The structure of port information (each item) in hwdbg
 *
 */
typedef struct _HWDBG_PORT_INFORMATION_ITEMS
{
    UINT32 PortSize;

} HWDBG_PORT_INFORMATION_ITEMS, *PHWDBG_PORT_INFORMATION_ITEMS;
