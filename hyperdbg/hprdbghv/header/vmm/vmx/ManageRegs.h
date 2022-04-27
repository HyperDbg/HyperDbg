/**
 * @file ManageRegs.h
 * @author Alee Amini (alee@hyperdbg.org)
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
//					Functions                    //
//////////////////////////////////////////////////

VOID
SetGuestCsSel(PVMX_SEGMENT_SELECTOR Cs);

VOID
SetGuestCs(PVMX_SEGMENT_SELECTOR Cs);

VMX_SEGMENT_SELECTOR
GetGuestCs();

VOID
SetGuestSsSel(PVMX_SEGMENT_SELECTOR Ss);

VOID
SetGuestSs(PVMX_SEGMENT_SELECTOR Ss);

VMX_SEGMENT_SELECTOR
GetGuestSs();

VOID
SetGuestDsSel(PVMX_SEGMENT_SELECTOR Ds);

VOID
SetGuestDs(PVMX_SEGMENT_SELECTOR Ds);

VMX_SEGMENT_SELECTOR
GetGuestDs();

VOID
SetGuestFsSel(PVMX_SEGMENT_SELECTOR Fs);

VOID
SetGuestFs(PVMX_SEGMENT_SELECTOR Fs);

VMX_SEGMENT_SELECTOR
GetGuestFs();

VOID
SetGuestGsSel(PVMX_SEGMENT_SELECTOR Gs);

VOID
SetGuestGs(PVMX_SEGMENT_SELECTOR Gs);

VMX_SEGMENT_SELECTOR
GetGuestGs();

VOID
SetGuestEsSel(PVMX_SEGMENT_SELECTOR Es);

VOID
SetGuestEs(PVMX_SEGMENT_SELECTOR Es);

VMX_SEGMENT_SELECTOR
GetGuestEs();

VOID
SetGuestIdtr(UINT64 Idtr);

UINT64
GetGuestIdtr();

VOID
SetGuestLdtr(UINT64 Ldtr);

UINT64
GetGuestLdtr();

VOID
SetGuestGdtr(UINT64 Gdtr);

UINT64
GetGuestGdtr();

VOID
SetGuestTr(UINT64 Tr);

UINT64
GetGuestTr();

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

UINT64
GetGuestDr0();

UINT64
GetGuestDr1();

UINT64
GetGuestDr2();

UINT64
GetGuestDr3();

UINT64
GetGuestDr6();

UINT64
GetGuestDr7();

VOID
SetGuestDr0(UINT64 value);

VOID
SetGuestDr1(UINT64 value);

VOID
SetGuestDr2(UINT64 value);

VOID
SetGuestDr3(UINT64 value);

VOID
SetGuestDr6(UINT64 value);

VOID
SetGuestDr7(UINT64 value);
