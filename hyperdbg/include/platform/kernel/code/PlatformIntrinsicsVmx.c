/**
 * @file PlatformIntrinsicsVmx.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for intrinsic functions (VMX instructions)
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformIntrinsicsVmx.h"
#endif // defined(__linux__)

#if defined(__linux__)

/**
 * @brief Linux inline-asm helper for VMREAD
 *        Mirrors __vmx_vmread: returns 0=success, 1=VMfail valid (ZF), 2=VMfail invalid (CF)
 */
static inline UCHAR
__linux_vmx_vmread(size_t Field, size_t * FieldValue)
{
    unsigned char cf, zf;
    __asm__ __volatile__(
        "vmread %[field], %[val] \n\t"
        "setc   %[cf]           \n\t"
        "setz   %[zf]           \n\t"
        : [val] "=rm"(*FieldValue),
          [cf] "=qm"(cf),
          [zf] "=qm"(zf)
        : [field] "r"(Field)
        : "cc");
    return cf ? 2 : (zf ? 1 : 0);
}

/**
 * @brief Linux inline-asm helper for VMWRITE
 *        Mirrors __vmx_vmwrite: returns 0=success, 1=VMfail valid (ZF), 2=VMfail invalid (CF)
 */
static inline UCHAR
__linux_vmx_vmwrite(size_t Field, size_t FieldValue)
{
    unsigned char cf, zf;
    __asm__ __volatile__(
        "vmwrite %[val], %[field] \n\t"
        "setc    %[cf]            \n\t"
        "setz    %[zf]            \n\t"
        : [cf] "=qm"(cf),
          [zf] "=qm"(zf)
        : [val] "rm"(FieldValue),
          [field] "r"(Field)
        : "cc");
    return cf ? 2 : (zf ? 1 : 0);
}

/**
 * @brief Linux inline-asm helper for VMPTRST
 *        Stores the current VMCS pointer into the given physical address
 */
static inline VOID
__linux_vmx_vmptrst(UINT64 * VmcsPhysicalAddress)
{
    __asm__ __volatile__(
        "vmptrst %[addr]"
        :
        : [addr] "m"(*VmcsPhysicalAddress)
        : "memory");
}

/**
 * @brief Linux inline-asm helper for VMPTRLD
 *        Returns 0=success, 1=VMfail valid (ZF), 2=VMfail invalid (CF)
 */
static inline UCHAR
__linux_vmx_vmptrld(UINT64 * VmcsPhysicalAddress)
{
    unsigned char cf, zf;
    __asm__ __volatile__(
        "vmptrld %[addr] \n\t"
        "setc    %[cf]   \n\t"
        "setz    %[zf]   \n\t"
        : [cf] "=qm"(cf),
          [zf] "=qm"(zf)
        : [addr] "m"(*VmcsPhysicalAddress)
        : "cc", "memory");
    return cf ? 2 : (zf ? 1 : 0);
}

/**
 * @brief Linux inline-asm helper for VMCLEAR
 *        Returns 0=success, 1=VMfail valid (ZF), 2=VMfail invalid (CF)
 */
static inline UCHAR
__linux_vmx_vmclear(UINT64 * VmcsPhysicalAddress)
{
    unsigned char cf, zf;
    __asm__ __volatile__(
        "vmclear %[addr] \n\t"
        "setc    %[cf]   \n\t"
        "setz    %[zf]   \n\t"
        : [cf] "=qm"(cf),
          [zf] "=qm"(zf)
        : [addr] "m"(*VmcsPhysicalAddress)
        : "cc", "memory");
    return cf ? 2 : (zf ? 1 : 0);
}

/**
 * @brief Linux inline-asm helper for VMXON
 *        Returns 0=success, 1=VMfail valid (ZF), 2=VMfail invalid (CF)
 */
static inline UCHAR
__linux_vmx_vmxon(UINT64 * VmxonRegionPhysicalAddress)
{
    unsigned char cf, zf;
    __asm__ __volatile__(
        "vmxon %[addr] \n\t"
        "setc  %[cf]   \n\t"
        "setz  %[zf]   \n\t"
        : [cf] "=qm"(cf),
          [zf] "=qm"(zf)
        : [addr] "m"(*VmxonRegionPhysicalAddress)
        : "cc", "memory");
    return cf ? 2 : (zf ? 1 : 0);
}

#endif // defined(__linux__)

/**
 * @brief VMX VMREAD instruction (64-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread64(size_t Field,
            UINT64 FieldValue)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)FieldValue);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)FieldValue);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMREAD instruction (32-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread32(size_t Field,
            UINT32 FieldValue)
{
    UINT64 TargetField = 0ull;
    TargetField        = FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)TargetField);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMREAD instruction (16-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread16(size_t Field,
            UINT16 FieldValue)
{
    UINT64 TargetField = 0ull;
    TargetField        = FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)TargetField);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMREAD instruction (64-bit, pointer variant)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread64P(size_t   Field,
             UINT64 * FieldValue)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)FieldValue);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)FieldValue);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMREAD instruction (32-bit, pointer variant)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread32P(size_t   Field,
             UINT32 * FieldValue)
{
    UINT64 TargetField = 0ull;
    TargetField        = (UINT64)FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)TargetField);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMREAD instruction (16-bit, pointer variant)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmread16P(size_t   Field,
             UINT16 * FieldValue)
{
    UINT64 TargetField = 0ull;
    TargetField        = (UINT64)FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmread((size_t)Field, (size_t *)TargetField);
#elif defined(__linux__)
    return __linux_vmx_vmread((size_t)Field, (size_t *)TargetField);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMWRITE instruction (64-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite64(size_t Field,
             UINT64 FieldValue)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmwrite((size_t)Field, (size_t)FieldValue);
#elif defined(__linux__)
    return __linux_vmx_vmwrite((size_t)Field, (size_t)FieldValue);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMWRITE instruction (32-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite32(size_t Field,
             UINT32 FieldValue)
{
    UINT64 TargetValue = NULL64_ZERO;
    TargetValue        = (UINT64)FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmwrite((size_t)Field, (size_t)TargetValue);
#elif defined(__linux__)
    return __linux_vmx_vmwrite((size_t)Field, (size_t)TargetValue);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMWRITE instruction (16-bit)
 * @param Field
 * @param FieldValue
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmwrite16(size_t Field,
             UINT16 FieldValue)
{
    UINT64 TargetValue = NULL64_ZERO;
    TargetValue        = (UINT64)FieldValue;

#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmwrite((size_t)Field, (size_t)TargetValue);
#elif defined(__linux__)
    return __linux_vmx_vmwrite((size_t)Field, (size_t)TargetValue);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMPTRST instruction
 *
 * @param VmcsPhysicalAddress
 *
 * @return VOID
 */
inline VOID
VmxVmptrst(UINT64 * VmcsPhysicalAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    __vmx_vmptrst(VmcsPhysicalAddress);
#elif defined(__linux__)
    __linux_vmx_vmptrst(VmcsPhysicalAddress);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMRESUME instruction
 *
 * @return VOID
 */
inline VOID
VmxVmresume(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    __vmx_vmresume();
#elif defined(__linux__)
    __asm__ __volatile__("vmresume" ::: "cc", "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMXOFF instruction
 *
 * @return VOID
 */
inline VOID
VmxVmxoff(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    __vmx_off();
#elif defined(__linux__)
    __asm__ __volatile__("vmxoff" ::: "cc", "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMLAUNCH instruction
 *
 * @return VOID
 */
inline VOID
VmxVmlaunch(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    __vmx_vmlaunch();
#elif defined(__linux__)
    __asm__ __volatile__("vmlaunch" ::: "cc", "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMPTRLD instruction
 *
 * @param VmcsPhysicalAddress
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmptrld(UINT64 * VmcsPhysicalAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmptrld(VmcsPhysicalAddress);
#elif defined(__linux__)
    return __linux_vmx_vmptrld(VmcsPhysicalAddress);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMCLEAR instruction
 *
 * @param VmcsPhysicalAddress
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmclear(UINT64 * VmcsPhysicalAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_vmclear(VmcsPhysicalAddress);
#elif defined(__linux__)
    return __linux_vmx_vmclear(VmcsPhysicalAddress);
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief VMX VMXON instruction
 *
 * @param VmxonRegionPhysicalAddress
 *
 * @return UCHAR
 */
inline UCHAR
VmxVmxon(UINT64 * VmxonRegionPhysicalAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return __vmx_on(VmxonRegionPhysicalAddress);
#elif defined(__linux__)
    return __linux_vmx_vmxon(VmxonRegionPhysicalAddress);
#else
#    error "Unsupported platform"
#endif
}
