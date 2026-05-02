/**
 * @file PlatformIntrinsics.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for intrinsic functions
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformIntrinsics.h"

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
