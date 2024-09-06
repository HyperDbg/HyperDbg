/**
 * @file steppings.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for stepping instructions
 * @details
 * @version 0.11
 * @date 2024-09-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            	    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
SteppingInstrumentationStepIn();

BOOLEAN
SteppingRegularStepIn();

BOOLEAN
SteppingStepOver();

BOOLEAN
SteppingInstrumentationStepInForTracking();

BOOLEAN
SteppingStepOverForGu(BOOLEAN LastInstruction);
