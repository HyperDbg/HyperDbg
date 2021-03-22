/**
 * @file transparency.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Measurements for debugger transparency
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

/**
 * @brief get the difference clock cycles between two rdtsc(s)
 *
 * @return unsigned long long
 */
unsigned long long
TransparentModeRdtscDiffVmexit()
{
    unsigned long long ret, ret2;
    unsigned           eax, edx;
    int                cpuid_result[4] = {0};

    //
    // GCC
    //
    // __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
    // ret = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

    //
    // Win32
    //
    ret = __rdtsc();

    /* vm exit forced here. it uses: eax = 0; cpuid; */

    //
    // GCC
    //
    //__asm__ volatile("cpuid" : /* no output */ : "a"(0x00));

    //
    // WIN32
    //
    __cpuid(cpuid_result, 0);

    //
    // GCC
    //
    // __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
    // ret2 = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

    //
    // WIN32
    //
    ret2 = __rdtsc();

    return ret2 - ret;
}

/**
 * @brief get the difference clock cycles between rdtsc+cpuid+rdtsc
 *
 * @return unsigned long long
 */
unsigned long long
TransparentModeRdtscVmexitTracing()
{
    unsigned long long ret, ret2;
    unsigned           eax, edx;

    //
    // GCC
    //
    // __asm__ volatile("rdtsc" : "=a" (eax), "=d" (edx));
    // ret = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

    //
    // WIN32
    //
    ret = __rdtsc();

    //
    // GCC
    //
    // __asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
    // ret2 = ((unsigned long long)eax) | (((unsigned long long)edx) << 32);

    //
    // WIN32
    //
    ret2 = __rdtsc();

    return ret2 - ret;
}

/**
 * @brief compute the average, standard deviation and median if
 * rdtsc+cpuid+rdtsc
 *
 * @param Average a pointer to save average on it
 * @param StandardDeviation a pointer to standard deviation average on it
 * @param Median a pointer to save median on it
 * @return int
 */
int
TransparentModeCpuidTimeStampCounter(UINT64 * Average,
                                     UINT64 * StandardDeviation,
                                     UINT64 * Median)
{
    unsigned long long Avg          = 0;
    unsigned long long MeasuredTime = 0;
    vector<double>     Results;

    for (int i = 0; i < TestCount; i++)
    {
        MeasuredTime = TransparentModeRdtscDiffVmexit();
        Avg          = Avg + MeasuredTime;

        Results.push_back(MeasuredTime);

        /*
    ShowMessages("(%d) Measured time : %d\n", i, MeasuredTime);
    */
    }

    if (Average != NULL && StandardDeviation != NULL && Median != NULL)
    {
        //
        // Compute the average and variance
        //
        GuassianGenerateRandom(Results, Average, StandardDeviation, Median);
    }

    Avg = Avg / TestCount;
    return (Avg < 1000 && Avg > 0) ? FALSE : TRUE;
}

/**
 * @brief compute the average, standard deviation and median if
 * rdtsc+rdtsc
 *
 * @param Average a pointer to save average on it
 * @param StandardDeviation a pointer to standard deviation average on it
 * @param Median a pointer to save median on it
 * @return int
 */
int
TransparentModeRdtscEmulationDetection(UINT64 * Average,
                                       UINT64 * StandardDeviation,
                                       UINT64 * Median)
{
    unsigned long long Avg          = 0;
    unsigned long long MeasuredTime = 0;
    vector<double>     Results;

    for (int i = 0; i < TestCount; i++)
    {
        MeasuredTime = TransparentModeRdtscVmexitTracing();
        Avg          = Avg + MeasuredTime;

        Results.push_back(MeasuredTime);

        /*
    ShowMessages("(%d) Measured time : %d\n", i, MeasuredTime);
    */
    }

    if (Average != NULL && StandardDeviation != NULL && Median != NULL)
    {
        //
        // Compute the average and variance
        //
        GuassianGenerateRandom(Results, Average, StandardDeviation, Median);
    }

    Avg = Avg / TestCount;
    return (Avg < 750 && Avg > 0) ? FALSE : TRUE;
}

/**
 * @brief compute the average, standard deviation and median if
 * rdtsc+cpuid+rdtsc
 * @details detects the presence of hypervisor
 *
 * @param Average a pointer to save average on it
 * @param StandardDeviation a pointer to standard deviation average on it
 * @param Median a pointer to save median on it
 * @return int
 */
BOOLEAN
TransparentModeCheckHypervisorPresence(UINT64 * Average,
                                       UINT64 * StandardDeviation,
                                       UINT64 * Median)
{
    //
    // Check whether the hypervisor is detected or not
    //
    if (TransparentModeCpuidTimeStampCounter(Average, StandardDeviation, Median))
    {
        ShowMessages("hypervisor detected\n");
        return TRUE;
    }
    else
    {
        ShowMessages("hypervisor not detected\n");
        return FALSE;
    }
}

/**
 * @brief compute the average, standard deviation and median if
 * rdtsc+rdtsc
 * @details detects the presence of rdtsc/p vm-exits
 *
 * @param Average a pointer to save average on it
 * @param StandardDeviation a pointer to standard deviation average on it
 * @param Median a pointer to save median on it
 * @return int
 */
BOOLEAN
TransparentModeCheckRdtscpVmexit(UINT64 * Average,
                                 UINT64 * StandardDeviation,
                                 UINT64 * Median)
{
    //
    // Check whether the system emulating rdtsc/p or not
    //
    if (TransparentModeRdtscEmulationDetection(Average, StandardDeviation, Median))
    {
        ShowMessages("rdtsc/p emulation detected\n");
        return TRUE;
    }
    else
    {
        ShowMessages("rdtsc/p emulation not detected\n");
        return FALSE;
    }
}
