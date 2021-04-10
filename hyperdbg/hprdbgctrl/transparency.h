/**
 * @file transparency.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief headers for test functions
 * @details
 * @version 0.1
 * @date 2020-07-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/**
 * @brief Number of tests for each instruction sets
 * @details used to generate test cases for rdts+cpuid+rdtsc
 * and rdtsc+rdtsc commands
 */
#define TestCount 1000

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

void
GuassianGenerateRandom(vector<double> Data, UINT64 * AverageOfData, UINT64 * StandardDeviationOfData, UINT64 * MedianOfData);

BOOLEAN
TransparentModeCheckHypervisorPresence(UINT64 * Average,
                                       UINT64 * StandardDeviation,
                                       UINT64 * Median);

BOOLEAN
TransparentModeCheckRdtscpVmexit(UINT64 * Average,
                                 UINT64 * StandardDeviation,
                                 UINT64 * Median);

double
Randn(double mu, double sigma);

double
Median(vector<double> Cases);

unsigned long long
TransparentModeRdtscDiffVmexit();

unsigned long long
TransparentModeRdtscVmexitTracing();
