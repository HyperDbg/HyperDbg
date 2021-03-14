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
//#pragma once

//////////////////////////////////////////////////
//					Functinsb                   //
//////////////////////////////////////////////////

VOID
SetGuestCsSel(PSEGMENT_SELECTOR Cs);

VOID
SetGuestCs(PSEGMENT_SELECTOR Cs);

SEGMENT_SELECTOR
GetGuestCs();

VOID
SetGuestSsSel(PSEGMENT_SELECTOR Ss);

VOID
SetGuestSs(PSEGMENT_SELECTOR Ss);

SEGMENT_SELECTOR
GetGuestSs();

VOID
SetGuestDsSel(PSEGMENT_SELECTOR Ds);

VOID
SetGuestDs(PSEGMENT_SELECTOR Ds);

SEGMENT_SELECTOR
GetGuestDs();

VOID
SetGuestFsSel(PSEGMENT_SELECTOR Fs);

VOID
SetGuestFs(PSEGMENT_SELECTOR Fs);

SEGMENT_SELECTOR
GetGuestFs();

VOID
SetGuestGsSel(PSEGMENT_SELECTOR Gs);

VOID
SetGuestGs(PSEGMENT_SELECTOR Gs);

SEGMENT_SELECTOR
GetGuestGs();

VOID
SetGuestEsSel(PSEGMENT_SELECTOR Es);

VOID
SetGuestEs(PSEGMENT_SELECTOR Es);

SEGMENT_SELECTOR
GetGuestEs();

VOID
SetGuestIdtr(PSEGMENT_SELECTOR Idtr);

SEGMENT_SELECTOR
GetGuestIdtr();

VOID
SetGuestGdtr(PSEGMENT_SELECTOR Gdtr);

SEGMENT_SELECTOR
GetGuestGdtr();

VOID
SetGuestRFlags(UINT64 RFlags);

UINT64
GetGuestRFlags();

VOID
SetGuestRIP(UINT64 RIP);

UINT64
GetGuestRIP();
