/**
 * @file TraceApi.h
 * @author
 * @brief Header for general tracing routines for HyperTrace module
 * @details
 * @version 0.19
 * @date 2026-04-25
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

//
// Most of the functions are defined and exported
//

//////////////////////////////////////////////////
//             Platform Wrappers                //
//////////////////////////////////////////////////

#define xrdmsr(msr, pval) (*(pval) = __readmsr(msr))
#define xwrmsr(msr, val)  (msr, val)

// CPUID (Fixed C6001: initialized CpuInfo)
#define xcpuid(code, a, b, c, d) \
    {                            \
        int CpuInfo[4] = {0};    \
        __cpuid(CpuInfo, code);  \
        *a = CpuInfo[0];         \
        *b = CpuInfo[1];         \
        *c = CpuInfo[2];         \
        *d = CpuInfo[3];         \
    }
