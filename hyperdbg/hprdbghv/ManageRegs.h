/**
 * @file ManageRegs.h
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief Headers relating to Management Registers (set and get regs value) 
 * @details
 * @version 0.1
 * @date 2021-03-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Functinsb                   //
//////////////////////////////////////////////////

VOID
SetGuestCs(PSEGMENT_SELECTOR Cs);

SEGMENT_SELECTOR
GetGuestCs();

VOID
SetGuestSs(PSEGMENT_SELECTOR Ss);

SEGMENT_SELECTOR
GetGuestSs();

VOID
SetGuestDs(PSEGMENT_SELECTOR Ds);

SEGMENT_SELECTOR
GetGuestDs();

VOID
SetGuestFs(PSEGMENT_SELECTOR Fs);

SEGMENT_SELECTOR
GetGuestFs();
VOID
SetGuestGs(PSEGMENT_SELECTOR Gs);

SEGMENT_SELECTOR
GetGuestGs();

VOID
SetGuestEs(PSEGMENT_SELECTOR Es);

SEGMENT_SELECTOR
GetGuestEs();

VOID
SetGuestRFlags(UINT64 RFlags);

UINT64
GetGuestRFlags();

VOID
SetGuestRIP(UINT64 RIP);

UINT64
GetGuestRIP();
