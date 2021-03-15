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
SetGuestIdtr(UINT64 Idtr);

UINT64
GetGuestIdtr();

VOID
SetGuestGdtr(UINT64 Gdtr);

UINT64
GetGuestGdtr();

VOID
SetGuestRFlags(UINT64 RFlags);

UINT64
GetGuestRFlags();

VOID
SetGuestRIP(UINT64 RIP);

VOID
SetGuestRSP(UINT64 RSP);

UINT64
GetGuestRIP();

UINT64
GetGuestCr0();

UINT64
GetGuestCr2();

UINT64
GetGuestCr3();

UINT64
GetGuestCr4();

UINT64
GetGuestCr8();

VOID
SetGuestCr0(UINT64 Cr0);

VOID
SetGuestCr2(UINT64 Cr2);

VOID
SetGuestCr3(UINT64 Cr3);

VOID
SetGuestCr4(UINT64 Cr4);

VOID
SetGuestCr8(UINT64 Cr8);
