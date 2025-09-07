/**
 * @file steppings.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for stepping instructions
 * @details
 * @version 0.11
 * @date 2024-09-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief Perform Instrumentation Step-in
 *
 * @return BOOLEAN
 */
BOOLEAN
SteppingInstrumentationStepIn()
{
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Check if we're in VMI mode
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("the instrumentation step-in is only supported in Debugger Mode\n");
        return FALSE;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN;

    return KdSendStepPacketToDebuggee(RequestFormat);
}

/**
 * @brief Perform Instrumentation Step-in for Tracking
 *
 * @return BOOLEAN
 */
BOOLEAN
SteppingInstrumentationStepInForTracking()
{
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Check if we're in VMI mode
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("the instrumentation step-in is only supported in Debugger Mode\n");
        return FALSE;
    }

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN_FOR_TRACKING;

    return KdSendStepPacketToDebuggee(RequestFormat);
}

/**
 * @brief Perform Regular Step-in
 *
 * @return BOOLEAN
 */
BOOLEAN
SteppingRegularStepIn()
{
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's stepping over serial connection in kernel debugger
        //
        return KdSendStepPacketToDebuggee(RequestFormat);
    }
    else if (g_ActiveProcessDebuggingState.IsActive && g_ActiveProcessDebuggingState.IsPaused)
    {
        //
        // It's stepping over user debugger
        //
        return UdSendStepPacketToDebuggee(g_ActiveProcessDebuggingState.ProcessDebuggingToken,
                                          g_ActiveProcessDebuggingState.ThreadId,
                                          RequestFormat);
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Perform Step-over
 *
 * @return BOOLEAN
 */
BOOLEAN
SteppingStepOver()
{
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Set type of step
    //
    RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's stepping over serial connection in kernel debugger
        //
        return KdSendStepPacketToDebuggee(RequestFormat);
    }
    else if (g_ActiveProcessDebuggingState.IsActive && g_ActiveProcessDebuggingState.IsPaused)
    {
        //
        // It's stepping over user debugger
        //
        return UdSendStepPacketToDebuggee(g_ActiveProcessDebuggingState.ProcessDebuggingToken,
                                          g_ActiveProcessDebuggingState.ThreadId,
                                          RequestFormat);
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Perform Step-over for GU
 * @param LastInstruction
 *
 * @return BOOLEAN
 */
BOOLEAN
SteppingStepOverForGu(BOOLEAN LastInstruction)
{
    DEBUGGER_REMOTE_STEPPING_REQUEST RequestFormat;

    //
    // Set type of step
    //
    if (!LastInstruction)
    {
        RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER_FOR_GU;
    }
    else
    {
        RequestFormat = DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER_FOR_GU_LAST_INSTRUCTION;
    }

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's stepping over serial connection in kernel debugger
        //
        return KdSendStepPacketToDebuggee(RequestFormat);
    }
    else if (g_ActiveProcessDebuggingState.IsActive && g_ActiveProcessDebuggingState.IsPaused)
    {
        //
        // It's stepping over user debugger
        //
        return UdSendStepPacketToDebuggee(g_ActiveProcessDebuggingState.ProcessDebuggingToken,
                                          g_ActiveProcessDebuggingState.ThreadId,
                                          RequestFormat);
    }
    else
    {
        //
        // The target is not paused, or not a valid context
        //
        return FALSE;
    }
}
