/**
 * @file PlatformIntrinsicsVmx.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for intrinsic functions
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
// #    include "../../general/header/GeneralTypes.h"
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//             VMX Instructions                 //
//////////////////////////////////////////////////

//
// VMPTRST
//
extern inline VOID
VmxVmptrst(UINT64 * VmcsPhysicalAddress);

//
// VMPTRLD
//
extern inline UCHAR
VmxVmptrld(UINT64 * VmcsPhysicalAddress);

//
// VMCLEAR
//
extern inline UCHAR
VmxVmclear(UINT64 * VmcsPhysicalAddress);

//
// VMXON
//
extern inline UCHAR
VmxVmxon(UINT64 * VmxonRegionPhysicalAddress);

//
// VMLAUNCH
//
extern inline VOID
    VmxVmlaunch(VOID);

//
// VMRESUME
//
extern inline VOID
    VmxVmresume(VOID);

//
// VMXOFF
//
extern inline VOID
    VmxVmxoff(VOID);

//
// VMREAD
//
extern inline UCHAR
VmxVmread64(size_t Field, UINT64 FieldValue);

extern inline UCHAR
VmxVmread32(size_t Field, UINT32 FieldValue);

extern inline UCHAR
VmxVmread16(size_t Field, UINT16 FieldValue);

extern inline UCHAR
VmxVmread64P(size_t Field, UINT64 * FieldValue);

extern inline UCHAR
VmxVmread32P(size_t Field, UINT32 * FieldValue);

extern inline UCHAR
VmxVmread16P(size_t Field, UINT16 * FieldValue);

//
// VMWRITE
//
extern inline UCHAR
VmxVmwrite64(size_t Field, UINT64 FieldValue);

extern inline UCHAR
VmxVmwrite32(size_t Field, UINT32 FieldValue);

extern inline UCHAR
VmxVmwrite16(size_t Field, UINT16 FieldValue);
