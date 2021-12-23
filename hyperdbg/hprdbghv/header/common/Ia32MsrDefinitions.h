/**
 * @file Ia32MsrDefinitions.h
 * @author rust communit
 * @brief Definition of different Model-Specific Register (MSRs)
 * @details Thanks to all rust developers, this file is copied from:
 *	        https://github.com/gz/rust-x86/blob/master/src/msr.rs
 * @version 0.1
 * @date 2021-12-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// What follows is a long list of all MSR register taken from Intel's manual.
// Some of the register values appear duplicated as they may be
// called differently for different architectures or they just have
// different meanings on different platforms.
//

/**
 * @brief See Section 35.16, MSRs in Pentium Processors,  and see  Table 35-2.
 *
 */
#define P5_MC_ADDR 0x0

/**
 * @brief See Section 35.16, MSRs in Pentium Processors.
 *
 */
#define IA32_P5_MC_ADDR 0x0

/**
 * @brief See Section 35.16, MSRs in Pentium Processors,  and see  Table 35-2.
 *
 */
#define P5_MC_TYPE 0x1

/**
 * @brief See Section 35.16, MSRs in Pentium Processors.
 *
 */
#define IA32_P5_MC_TYPE 0x1

/**
 * @brief See Section 8.10.5, Monitor/Mwait Address Range Determination,
 * and see Table 35-2.
 *
 */
#define IA32_MONITOR_FILTER_SIZE 0x6

/**
 * @brief See Section 8.10.5, Monitor/Mwait Address  Range Determination.
 *
 */
#define IA32_MONITOR_FILTER_LINE_SIZE 0x6

/**
 * @brief See Section 17.13, Time-Stamp Counter,  and see Table 35-2.
 *
 */
#define IA32_TIME_STAMP_COUNTER 0x10

/**
 * @brief See Section 17.13, Time-Stamp Counter.
 *
 */
#define TSC 0x10

/**
 * @brief Model Specific Platform ID (R)
 *
 */
#define MSR_PLATFORM_ID 0x17

/**
 * @brief Platform ID (R)  See Table 35-2. 
 * The operating system can use this MSR to  determine slot information for
 * the processor and the proper microcode update to load.
 *
 */
#define IA32_PLATFORM_ID 0x17

/**
 * @brief Section 10.4.4, Local APIC Status and Location.
 *
 */
#define APIC_BASE 0x1b

/**
 * @brief APIC Location and Status (R/W) See Table 35-2. See Section 10.4.4, Local APIC  
 * Status and Location.
 *
 */
#define IA32_APIC_BASE 0x1b

/**
 * @brief Processor Hard Power-On Configuration  (R/W) Enables and disables
 * processor features  (R) indicates current processor configuration.
 *
 */
#define EBL_CR_POWERON 0x2a

/**
 * @brief Processor Hard Power-On Configuration (R/W) Enables and
 * disables processor features  (R) indicates current processor configuration.
 *
 */
#define MSR_EBL_CR_POWERON 0x2a

/**
 * @brief Processor Hard Power-On Configuration (R/W) Enables and 
 * disables processor features (R) indicates current processor configuration.
 *
 */
#define MSR_EBC_HARD_POWERON 0x2a

/**
 * @brief Processor Soft Power-On Configuration (R/W)  
 * Enables and disables processor features.
 *
 */
#define MSR_EBC_SOFT_POWERON 0x2b

/**
 * @brief Processor Frequency Configuration The bit field layout of this 
 * MSR varies according to  the MODEL value in the CPUID version information. 
 * The following bit field layout applies to Pentium 4 and Xeon Processors with MODEL  
 * encoding equal or greater than 2.  (R) The field Indicates the current processor  
 * frequency configuration.
 *
 */
#define MSR_EBC_FREQUENCY_ID 0x2c

/**
 * @brief Test Control Register
 *
 */
#define TEST_CTL 0x33

/**
 * @brief SMI Counter (R/O)
 *
 */
#define MSR_SMI_COUNT 0x34

/**
 * @brief Control Features in IA-32 Processor (R/W) 
 * See Table 35-2 (If CPUID.01H:ECX.[bit 5])
 *
 */
#define IA32_FEATURE_CONTROL 0x3a

/**
 * @brief Per-Logical-Processor TSC ADJUST (R/W) See Table 35-2.
 *
 */
#define IA32_TSC_ADJUST 0x3b

/**
 * @brief Last Branch Record 0 From IP (R/W) One of eight pairs of last 
 * branch record registers on the last branch  record stack. 
 * This part of the stack contains pointers to the source  instruction 
 * for one of the last eight branches, exceptions, or  interrupts taken 
 * by the processor. See also: Last Branch Record Stack TOS at 
 * 1C9H Section 17.11, Last Branch, Interrupt, and Exception 
 * Recording  (Pentium M Processors).
 *
 */
#define MSR_LASTBRANCH_0_FROM_IP 0x40

/**
 * @brief Last Branch Record 1 (R/W) See description of MSR_LASTBRANCH_0.
 *
 */
#define MSR_LASTBRANCH_1 0x41

/**
 * @brief Last Branch Record 1 From IP (R/W) 
 * See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_1_FROM_IP 0x41

/**
 * @brief Last Branch Record 2 From IP (R/W) 
 * See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_2_FROM_IP 0x42

/**
 * @brief Last Branch Record 3 From IP (R/W) 
 * See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_3_FROM_IP 0x43

/**
 * @brief Last Branch Record 4 (R/W) See description of MSR_LASTBRANCH_0.
 *
 */
#define MSR_LASTBRANCH_4 0x44

/**
 * @brief Last Branch Record 4 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_4_FROM_IP 0x44

/**
 * @brief Last Branch Record 5 (R/W) See description of MSR_LASTBRANCH_0.
 *
 */
#define MSR_LASTBRANCH_5 0x45

/**
 * @brief Last Branch Record 5 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_5_FROM_IP 0x45

/**
 * @brief Last Branch Record 6 (R/W) See description of MSR_LASTBRANCH_0.
 *
 */
#define MSR_LASTBRANCH_6 0x46

/**
 * @brief Last Branch Record 6 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_6_FROM_IP 0x46

/**
 * @brief Last Branch Record 7 (R/W) See description of MSR_LASTBRANCH_0.
 *
 */
#define MSR_LASTBRANCH_7 0x47

/**
 * @brief Last Branch Record 7 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_7_FROM_IP 0x47

/**
 * @brief IA32_SPEC_CTRL[bit 0] – Indirect Branch Restricted Speculation (IBRS)
 * If IBRS is set, near returns and near indirect jumps/calls will not allow their
 * predicted target address to be controlled by code that executed in a less privileged 
 * prediction mode before the IBRS mode was last written with a value of 1 or on another
 * logical processor so long as all RSB entries from the previous less privileged 
 * prediction mode are overwritten.
 * 
 */
#define IA32_SPEC_CTRL 0x48

/**
 * @brief IA32_PRED_CMD,  bit0 – Indirect Branch Prediction Barrier (IBPB)
 * Setting of IBPB ensures that earlier code's behavior does not control later
 * indirect branch predictions.  It is used when context switching to new 
 * untrusted address space.  Unlike IBRS, it is a command MSR and does not 
 * retain its state.
 *
 */
#define IA32_PRED_CMD 0x49

/**
 * @brief Last Branch Record 0 (R/W)  One of 16 pairs of last branch record registers on  
 * the last branch record stack (6C0H-6CFH). This  part of the stack contains pointers 
 * to the  destination instruction for one of the last 16  branches, exceptions, or 
 * interrupts that the  processor took. See Section 17.9, Last Branch, Interrupt, and  
 * Exception Recording (Processors based on Intel  NetBurst® Microarchitecture).
 *
 */
#define MSR_LASTBRANCH_0_TO_IP 0x6c0

/**
 * @brief Last Branch Record 1 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_1_TO_IP 0x61

/**
 * @brief Last Branch Record 2 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_2_TO_IP 0x62

/**
 * @brief Last Branch Record 3 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_3_TO_IP 0x63

/**
 * @brief Last Branch Record 4 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_4_TO_IP 0x64

/**
 * @brief Last Branch Record 5 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_5_TO_IP 0x65

/**
 * @brief Last Branch Record 6 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_6_TO_IP 0x66

/**
 * @brief Last Branch Record 7 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_7_TO_IP 0x67

/**
 * @brief BIOS Update Trigger Register (W)  See Table 35-2.
 *
 */
#define IA32_BIOS_UPDT_TRIG 0x79

/**
 * @brief BIOS Update Trigger Register.
 *
 */
#define BIOS_UPDT_TRIG 0x79

/**
 * @brief BIOS Update Signature ID (R/W) See Table 35-2.
 *
 */
#define IA32_BIOS_SIGN_ID 0x8b

/**
 * @brief SMM Monitor Configuration (R/W) See Table 35-2.
 *
 */
#define IA32_SMM_MONITOR_CTL 0x9b

/**
 * @brief If IA32_VMX_MISC[bit 15])
 *
 */
#define IA32_SMBASE 0x9e

/**
 * @brief System Management Mode Physical Address Mask register  (WO in SMM) 
 * Model-specific implementation of SMRR-like interface, read visible  
 * and write only in SMM.
 *
 */
#define MSR_SMRR_PHYSMASK 0xa1

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC0 0xc1

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC1 0xc2

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC2 0xc3

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC3 0xc4

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC4 0xc5

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC5 0xc6

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC6 0xc7

/**
 * @brief Performance Counter Register  See Table 35-2.
 *
 */
#define IA32_PMC7 0xc8

/**
 * @brief Scaleable Bus Speed(RO) This field indicates the intended 
 * scaleable bus clock speed for  processors based on Intel Atom 
 * microarchitecture:
 *
 */
#define MSR_FSB_FREQ 0xcd

/**
 * @brief see http://biosbits.org.
 *
 */
#define MSR_PLATFORM_INFO 0xce

/**
 * @brief C-State Configuration Control (R/W)  Note: C-state values are processor 
 * specific C-state code names,  unrelated to MWAIT extension C-state 
 * parameters or ACPI C- States. See http://biosbits.org.
 *
 */
#define MSR_PKG_CST_CONFIG_CONTROL 0xe2

/**
 * @brief Power Management IO Redirection in C-state (R/W)  See http://biosbits.org.
 *
 */
#define MSR_PMG_IO_CAPTURE_BASE 0xe4

/**
 * @brief Maximum Performance Frequency Clock Count (RW)  See Table 35-2.
 *
 */
#define IA32_MPERF 0xe7

/**
 * @brief Actual Performance Frequency Clock Count (RW)  See Table 35-2.
 *
 */
#define IA32_APERF 0xe8

/**
 * @brief MTRR Information See Section 11.11.1, MTRR Feature  Identification. .
 *
 */
#define IA32_MTRRCAP 0xfe

#define MSR_BBL_CR_CTL 0x119

#define MSR_BBL_CR_CTL3 0x11e

/**
 * @brief TSX Ctrl Register for TSX Async Abot (TAA) Migration. See Volume 3A, Section 2.1, Table 2-2.
 *
 */
#define MSR_IA32_TSX_CTRL 0x122

/**
 * @brief CS register target for CPL 0 code (R/W) See Table 35-2. See Section 5.8.7, Performing 
 * Fast Calls to  System Procedures with the SYSENTER and  SYSEXIT Instructions.
 *
 */
#define IA32_SYSENTER_CS 0x174

/**
 * @brief CS register target for CPL 0 code
 *
 */
#define SYSENTER_CS_MSR 0x174

/**
 * @brief Stack pointer for CPL 0 stack (R/W) See Table 35-2. See Section 5.8.7, 
 * Performing Fast Calls to  System Procedures with the SYSENTER and  SYSEXIT Instructions.
 *
 */
#define IA32_SYSENTER_ESP 0x175

/**
 * @brief Stack pointer for CPL 0 stack
 *
 */
#define SYSENTER_ESP_MSR 0x175

/**
 * @brief CPL 0 code entry point (R/W) See Table 35-2. See Section 5.8.7, Performing  
 * Fast Calls to System Procedures with the SYSENTER and SYSEXIT Instructions.
 *
 */
#define IA32_SYSENTER_EIP 0x176

/**
 * @brief CPL 0 code entry point
 *
 */
#define SYSENTER_EIP_MSR 0x176

#define MCG_CAP 0x179

/**
 * @brief Machine Check Capabilities (R) See Table 35-2. See Section 15.3.1.1,  IA32_MCG_CAP MSR.
 *
 */
#define IA32_MCG_CAP 0x179

/**
 * @brief Machine Check Status. (R) See Table 35-2. See Section 15.3.1.2,  IA32_MCG_STATUS MSR.
 *
 */
#define IA32_MCG_STATUS 0x17a

#define MCG_STATUS 0x17a

#define MCG_CTL 0x17b

/**
 * @brief Machine Check Feature Enable (R/W) See Table 35-2. See Section 15.3.1.3, IA32_MCG_CTL MSR.
 *
 */
#define IA32_MCG_CTL 0x17b

/**
 * @brief Enhanced SMM Capabilities (SMM-RO) Reports SMM capability Enhancement. Accessible only while in  SMM.
 *
 */
#define MSR_SMM_MCA_CAP 0x17d

/**
 * @brief MC Bank Error Configuration (R/W)
 *
 */
#define MSR_ERROR_CONTROL 0x17f

/**
 * @brief Machine Check EAX/RAX Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RAX 0x180

/**
 * @brief Machine Check EBX/RBX Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RBX 0x181

/**
 * @brief Machine Check ECX/RCX Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RCX 0x182

/**
 * @brief Machine Check EDX/RDX Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RDX 0x183

/**
 * @brief Machine Check ESI/RSI Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RSI 0x184

/**
 * @brief Machine Check EDI/RDI Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RDI 0x185

/**
 * @brief Machine Check EBP/RBP Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RBP 0x186

/**
 * @brief Performance Event Select for Counter 0 (R/W) Supports all fields described inTable 35-2 and the fields below.
 *
 */
#define IA32_PERFEVTSEL0 0x186

/**
 * @brief Performance Event Select for Counter 1 (R/W) Supports all fields described inTable 35-2 and the fields below.
 *
 */
#define IA32_PERFEVTSEL1 0x187

/**
 * @brief Performance Event Select for Counter 2 (R/W) Supports all fields described inTable 35-2 and the fields below.
 *
 */
#define IA32_PERFEVTSEL2 0x188

/**
 * @brief Machine Check EFLAGS/RFLAG Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RFLAGS 0x188

/**
 * @brief Performance Event Select for Counter 3 (R/W) Supports all fields described inTable 35-2 and the fields below.
 *
 */
#define IA32_PERFEVTSEL3 0x189

/**
 * @brief Machine Check EIP/RIP Save State See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_RIP 0x189

/**
 * @brief Machine Check Miscellaneous See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_MISC 0x18a

/**
 * @brief See Table 35-2 If CPUID.0AH:EAX[15:8] = 8
 *
 */
#define IA32_PERFEVTSEL4 0x18a

/**
 * @brief See Table 35-2 If CPUID.0AH:EAX[15:8] = 8
 *
 */
#define IA32_PERFEVTSEL5 0x18b

/**
 * @brief See Table 35-2 If CPUID.0AH:EAX[15:8] = 8
 *
 */
#define IA32_PERFEVTSEL6 0x18c

/**
 * @brief See Table 35-2 If CPUID.0AH:EAX[15:8] = 8
 *
 */
#define IA32_PERFEVTSEL7 0x18d

/**
 * @brief Machine Check R8 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R8 0x190

/**
 * @brief Machine Check R9D/R9 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R9 0x191

/**
 * @brief Machine Check R10 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R10 0x192

/**
 * @brief Machine Check R11 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R11 0x193

/**
 * @brief Machine Check R12 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R12 0x194

/**
 * @brief Machine Check R13 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R13 0x195

/**
 * @brief Machine Check R14 See Section 15.3.2.6, IA32_MCG Extended  Machine Check State MSRs.
 *
 */
#define MSR_MCG_R14 0x196

#define MSR_PERF_STATUS 0x198

/**
 * @brief See Table 35-2. See Section 14.1, Enhanced Intel  Speedstep® Technology.
 *
 */
#define IA32_PERF_STATUS 0x198

/**
 * @brief See Table 35-2. See Section 14.1, Enhanced Intel  Speedstep® Technology.
 *
 */
#define IA32_PERF_CTL 0x199

/**
 * @brief Clock Modulation (R/W)  See Table 35-2. IA32_CLOCK_MODULATION MSR was 
 * originally named  IA32_THERM_CONTROL MSR.
 *
 */
#define IA32_CLOCK_MODULATION 0x19a

/**
 * @brief Thermal Interrupt Control (R/W) See Section 14.5.2, Thermal Monitor,  
 * and see Table 35-2.
 *
 */
#define IA32_THERM_INTERRUPT 0x19b

/**
 * @brief Thermal Monitor Status (R/W) See Section 14.5.2, Thermal Monitor,  and see  Table 35-2.
 *
 */
#define IA32_THERM_STATUS 0x19c

/**
 * @brief Thermal Monitor 2 Control.
 *
 */
#define MSR_THERM2_CTL 0x19d

#define IA32_MISC_ENABLE 0x1a0

/**
 * @brief Platform Feature Requirements (R)
 *
 */
#define MSR_PLATFORM_BRV 0x1a1

#define MSR_TEMPERATURE_TARGET 0x1a2

/**
 * @brief Offcore Response Event Select Register (R/W)
 *
 */
#define MSR_OFFCORE_RSP_0 0x1a6

/**
 * @brief Offcore Response Event Select Register (R/W)
 *
 */
#define MSR_OFFCORE_RSP_1 0x1a7

/**
 * @brief See http://biosbits.org.
 *
 */
#define MSR_MISC_PWR_MGMT 0x1aa

/**
 * @brief See http://biosbits.org.
 *
 */
#define MSR_TURBO_POWER_CURRENT_LIMIT 0x1ac

/**
 * @brief Maximum Ratio Limit of Turbo Mode RO if MSR_PLATFORM_INFO.[28] = 0, 
 * RW if MSR_PLATFORM_INFO.[28] = 1
 *
 */
#define MSR_TURBO_RATIO_LIMIT 0x1ad

/**
 * @brief if CPUID.6H:ECX[3] = 1
 *
 */
#define IA32_ENERGY_PERF_BIAS 0x1b0

/**
 * @brief If CPUID.06H: EAX[6] = 1
 *
 */
#define IA32_PACKAGE_THERM_STATUS 0x1b1

/**
 * @brief If CPUID.06H: EAX[6] = 1
 *
 */
#define IA32_PACKAGE_THERM_INTERRUPT 0x1b2

/**
 * @brief Last Branch Record Filtering Select Register (R/W)  See Section 17.6.2, 
 * Filtering of Last Branch Records.
 *
 */
#define MSR_LBR_SELECT 0x1c8

/**
 * @brief Last Branch Record Stack TOS (R/W)  Contains an index (0-3 or 0-15) that 
 * points to the  top of the last branch record stack (that is, that points the 
 * index of the MSR containing the most  recent branch record). See Section 17.9.2, 
 * LBR Stack for Processors Based on Intel NetBurst® Microarchitecture and addresses 
 * 1DBH-1DEH and 680H-68FH.
 *
 */
#define MSR_LASTBRANCH_TOS 0x1da

#define DEBUGCTLMSR 0x1d9

/**
 * @brief Debug Control (R/W)  Controls how several debug features are used. Bit  definitions 
 * are discussed in the referenced section. See Section 17.9.1, MSR_DEBUGCTLA MSR.
 *
 */
#define MSR_DEBUGCTLA 0x1d9

/**
 * @brief Debug Control (R/W)  Controls how several debug features are used. Bit definitions 
 * are discussed in the referenced section. See Section 17.11, Last Branch, Interrupt, and 
 * Exception Recording  (Pentium M Processors).
 *
 */
#define MSR_DEBUGCTLB 0x1d9

/**
 * @brief Debug Control (R/W)  Controls how several debug features are used. Bit definitions 
 * are  discussed in the referenced section.
 *
 */
#define IA32_DEBUGCTL 0x1d9

#define LASTBRANCHFROMIP 0x1db

/**
 * @brief Last Branch Record 0 (R/W)  One of four last branch record registers on the last  
 * branch record stack. It contains pointers to the  source and destination instruction for 
 * one of the  last four branches, exceptions, or interrupts that  the processor took. 
 * MSR_LASTBRANCH_0 through  MSR_LASTBRANCH_3 at 1DBH-1DEH are  available only on 
 * family 0FH, models 0H-02H.  They have been replaced by the MSRs at 680H- 68FH and 6C0H-6CFH.
 *
 */
#define MSR_LASTBRANCH_0 0x1db

#define LASTBRANCHTOIP 0x1dc

#define LASTINTFROMIP 0x1dd

/**
 * @brief Last Branch Record 2 See description of the MSR_LASTBRANCH_0 MSR at 1DBH.
 *
 */
#define MSR_LASTBRANCH_2 0x1dd

/**
 * @brief Last Exception Record From Linear IP (R)  Contains a pointer to the last branch 
 * instruction that the processor  executed prior to the last exception that was generated 
 * or the last  interrupt that was handled. See Section 17.11, Last Branch, Interrupt, and 
 * Exception Recording  (Pentium M Processors)  and Section 17.12.2, Last Branch and Last  
 * Exception MSRs.
 *
 */
#define MSR_LER_FROM_LIP 0x1de

#define LASTINTTOIP 0x1de

/**
 * @brief Last Branch Record 3 See description of the MSR_LASTBRANCH_0 MSR  at 1DBH.
 *
 */
#define MSR_LASTBRANCH_3 0x1de

/**
 * @brief Last Exception Record To Linear IP (R)  This area contains a pointer to the target 
 * of the last branch instruction  that the processor executed prior to the last exception 
 * that was  generated or the last interrupt that was handled. See Section 17.11, Last Branch, 
 * Interrupt, and Exception Recording  (Pentium M Processors)  and Section 17.12.2, Last Branch 
 * and Last  Exception MSRs.
 *
 */
#define MSR_LER_TO_LIP 0x1dd

#define ROB_CR_BKUPTMPDR6 0x1e0

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_SMRR_PHYSBASE 0x1f2

/**
 * @brief If IA32_MTRR_CAP[SMRR]  = 1
 *
 */
#define IA32_SMRR_PHYSMASK 0x1f3

/**
 * @brief 06_0FH
 *
 */
#define IA32_PLATFORM_DCA_CAP 0x1f8

#define IA32_CPU_DCA_CAP 0x1f9

/**
 * @brief 06_2EH
 *
 */
#define IA32_DCA_0_CAP 0x1fa

/**
 * @brief Power Control Register. See http://biosbits.org.
 *
 */
#define MSR_POWER_CTL 0x1fc

/**
 * @brief Variable Range Base MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE0 0x200

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK0 0x201

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE1 0x202

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK1 0x203

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE2 0x204

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs .
 *
 */
#define IA32_MTRR_PHYSMASK2 0x205

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE3 0x206

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK3 0x207

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE4 0x208

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK4 0x209

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE5 0x20a

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK5 0x20b

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE6 0x20c

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK6 0x20d

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSBASE7 0x20e

/**
 * @brief Variable Range Mask MTRR See Section 11.11.2.3, Variable Range MTRRs.
 *
 */
#define IA32_MTRR_PHYSMASK7 0x20f

/**
 * @brief if IA32_MTRR_CAP[7:0] >  8
 *
 */
#define IA32_MTRR_PHYSBASE8 0x210

/**
 * @brief if IA32_MTRR_CAP[7:0] >  8
 *
 */
#define IA32_MTRR_PHYSMASK8 0x211

/**
 * @brief if IA32_MTRR_CAP[7:0] >  9
 *
 */
#define IA32_MTRR_PHYSBASE9 0x212

/**
 * @brief if IA32_MTRR_CAP[7:0] >  9
 *
 */
#define IA32_MTRR_PHYSMASK9 0x213

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX64K_00000 0x250

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX16K_80000 0x258

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX16K_A0000 0x259

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_C0000 0x268

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs .
 *
 */
#define IA32_MTRR_FIX4K_C8000 0x269

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs .
 *
 */
#define IA32_MTRR_FIX4K_D0000 0x26a

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_D8000 0x26b

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_E0000 0x26c

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_E8000 0x26d

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_F0000 0x26e

/**
 * @brief Fixed Range MTRR See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_MTRR_FIX4K_F8000 0x26f

/**
 * @brief Page Attribute Table See Section 11.11.2.2, Fixed Range MTRRs.
 *
 */
#define IA32_PAT 0x277

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC0_CTL2 0x280

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC1_CTL2 0x281

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC2_CTL2 0x282

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC3_CTL2 0x283

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC4_CTL2 0x284

/**
 * @brief Always 0 (CMCI not supported).
 *
 */
#define MSR_MC4_CTL2 0x284

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC5_CTL2 0x285

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC6_CTL2 0x286

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC7_CTL2 0x287

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC8_CTL2 0x288

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC9_CTL2 0x289

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC10_CTL2 0x28a

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC11_CTL2 0x28b

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC12_CTL2 0x28c

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC13_CTL2 0x28d

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC14_CTL2 0x28e

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC15_CTL2 0x28f

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC16_CTL2 0x290

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC17_CTL2 0x291

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC18_CTL2 0x292

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC19_CTL2 0x293

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC20_CTL2 0x294

/**
 * @brief See Table 35-2.
 *
 */
#define IA32_MC21_CTL2 0x295

/**
 * @brief Default Memory Types (R/W)  Sets the memory type for the regions of physical 
 * memory that are not  mapped by the MTRRs.  See Section 11.11.2.1, IA32_MTRR_DEF_TYPE MSR.
 *
 */
#define IA32_MTRR_DEF_TYPE 0x2ff

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_BPU_COUNTER0 0x300

#define MSR_GQ_SNOOP_MESF 0x301

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_BPU_COUNTER1 0x301

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_BPU_COUNTER2 0x302

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_BPU_COUNTER3 0x303

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_MS_COUNTER0 0x304

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_MS_COUNTER1 0x305

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_MS_COUNTER2 0x306

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_MS_COUNTER3 0x307

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_FLAME_COUNTER0 0x308

/**
 * @brief Fixed-Function Performance Counter Register 0 (R/W)
 *
 */
#define MSR_PERF_FIXED_CTR0 0x309

/**
 * @brief Fixed-Function Performance Counter Register 0 (R/W)  See Table 35-2.
 *
 */
#define IA32_FIXED_CTR0 0x309

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_FLAME_COUNTER1 0x309

/**
 * @brief Fixed-Function Performance Counter Register 1 (R/W)
 *
 */
#define MSR_PERF_FIXED_CTR1 0x30a

/**
 * @brief Fixed-Function Performance Counter Register 1 (R/W)  See Table 35-2.
 *
 */
#define IA32_FIXED_CTR1 0x30a

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_FLAME_COUNTER2 0x30a

/**
 * @brief Fixed-Function Performance Counter Register 2 (R/W)
 *
 */
#define MSR_PERF_FIXED_CTR2 0x30b

/**
 * @brief Fixed-Function Performance Counter Register 2 (R/W)  See Table 35-2.
 *
 */
#define IA32_FIXED_CTR2 0x30b

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_FLAME_COUNTER3 0x30b

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_IQ_COUNTER4 0x310

/**
 * @brief See Section 18.12.2, Performance Counters.
 *
 */
#define MSR_IQ_COUNTER5 0x311

/**
 * @brief See Table 35-2. See Section 17.4.1, IA32_DEBUGCTL MSR.
 *
 */
#define IA32_PERF_CAPABILITIES 0x345

/**
 * @brief RO. This applies to processors that do not support architectural  perfmon version 2.
 *
 */
#define MSR_PERF_CAPABILITIES 0x345

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_BPU_CCCR0 0x360

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_BPU_CCCR1 0x361

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_BPU_CCCR2 0x362

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_BPU_CCCR3 0x363

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_MS_CCCR0 0x364

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_MS_CCCR1 0x365

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_MS_CCCR2 0x366

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_MS_CCCR3 0x367

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_FLAME_CCCR0 0x368

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_FLAME_CCCR1 0x369

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_FLAME_CCCR2 0x36a

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_FLAME_CCCR3 0x36b

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR0 0x36c

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR1 0x36d

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR2 0x36e

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR3 0x36f

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR4 0x370

/**
 * @brief See Section 18.12.3, CCCR MSRs.
 *
 */
#define MSR_IQ_CCCR5 0x371

/**
 * @brief Fixed-Function-Counter Control Register (R/W)
 *
 */
#define MSR_PERF_FIXED_CTR_CTRL 0x38d

/**
 * @brief Fixed-Function-Counter Control Register (R/W)  See Table 35-2.
 *
 */
#define IA32_FIXED_CTR_CTRL 0x38d

/**
 * @brief See Section 18.4.2, Global Counter Control Facilities.
 *
 */
#define MSR_PERF_GLOBAL_STAUS 0x38e

/**
 * @brief See Table 35-2. See Section 18.4.2, Global Counter Control  Facilities.
 *
 */
#define IA32_PERF_GLOBAL_STAUS 0x38e

/**
 * @brief See Section 18.4.2, Global Counter Control Facilities.
 *
 */
#define MSR_PERF_GLOBAL_CTRL 0x38f

/**
 * @brief See Table 35-2. See Section 18.4.2, Global Counter Control  Facilities.
 *
 */
#define IA32_PERF_GLOBAL_CTRL 0x38f

/**
 * @brief See Section 18.4.2, Global Counter Control Facilities.
 *
 */
#define MSR_PERF_GLOBAL_OVF_CTRL 0x390

/**
 * @brief See Table 35-2. See Section 18.4.2, Global Counter Control  Facilities.
 *
 */
#define IA32_PERF_GLOBAL_OVF_CTRL 0x390

/**
 * @brief See Section 18.7.2.1, Uncore Performance Monitoring  Management Facility.
 *
 */
#define MSR_UNCORE_PERF_GLOBAL_CTRL 0x391

/**
 * @brief Uncore PMU global control
 *
 */
#define MSR_UNC_PERF_GLOBAL_CTRL 0x391

/**
 * @brief See Section 18.7.2.1, Uncore Performance Monitoring  Management Facility.
 *
 */
#define MSR_UNCORE_PERF_GLOBAL_STATUS 0x392

/**
 * @brief Uncore PMU main status
 *
 */
#define MSR_UNC_PERF_GLOBAL_STATUS 0x392

/**
 * @brief See Section 18.7.2.1, Uncore Performance Monitoring  Management Facility.
 *
 */
#define MSR_UNCORE_PERF_GLOBAL_OVF_CTRL 0x393

/**
 * @brief See Section 18.7.2.1, Uncore Performance Monitoring  Management Facility.
 *
 */
#define MSR_UNCORE_FIXED_CTR0 0x394

/**
 * @brief Uncore W-box perfmon fixed counter
 *
 */
#define MSR_W_PMON_FIXED_CTR 0x394

/**
 * @brief Uncore fixed counter control (R/W)
 *
 */
#define MSR_UNC_PERF_FIXED_CTRL 0x394

/**
 * @brief See Section 18.7.2.1, Uncore Performance Monitoring  Management Facility.
 *
 */
#define MSR_UNCORE_FIXED_CTR_CTRL 0x395

/**
 * @brief Uncore U-box perfmon fixed counter control MSR
 *
 */
#define MSR_W_PMON_FIXED_CTR_CTL 0x395

/**
 * @brief Uncore fixed counter
 *
 */
#define MSR_UNC_PERF_FIXED_CTR 0x395

/**
 * @brief See Section 18.7.2.3, Uncore Address/Opcode Match MSR.
 *
 */
#define MSR_UNCORE_ADDR_OPCODE_MATCH 0x396

/**
 * @brief Uncore C-Box configuration information (R/O)
 *
 */
#define MSR_UNC_CBO_CONFIG 0x396

#define MSR_PEBS_NUM_ALT 0x39c

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_BSU_ESCR0 0x3a0

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_BSU_ESCR1 0x3a1

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FSB_ESCR0 0x3a2

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FSB_ESCR1 0x3a3

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FIRM_ESCR0 0x3a4

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FIRM_ESCR1 0x3a5

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FLAME_ESCR0 0x3a6

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_FLAME_ESCR1 0x3a7

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_DAC_ESCR0 0x3a8

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_DAC_ESCR1 0x3a9

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_MOB_ESCR0 0x3aa

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_MOB_ESCR1 0x3ab

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_PMH_ESCR0 0x3ac

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_PMH_ESCR1 0x3ad

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_SAAT_ESCR0 0x3ae

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_SAAT_ESCR1 0x3af

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_U2L_ESCR0 0x3b0

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration  Facility.
 *
 */
#define MSR_UNCORE_PMC0 0x3b0

/**
 * @brief Uncore Arb unit, performance counter 0
 *
 */
#define MSR_UNC_ARB_PER_CTR0 0x3b0

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_U2L_ESCR1 0x3b1

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration  Facility.
 *
 */
#define MSR_UNCORE_PMC1 0x3b1

/**
 * @brief Uncore Arb unit, performance counter 1
 *
 */
#define MSR_UNC_ARB_PER_CTR1 0x3b1

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_BPU_ESCR0 0x3b2

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration  Facility.
 *
 */
#define MSR_UNCORE_PMC2 0x3b2

/**
 * @brief Uncore Arb unit, counter 0 event select MSR
 *
 */
#define MSR_UNC_ARB_PERFEVTSEL0 0x3b2

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_BPU_ESCR1 0x3b3

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PMC3 0x3b3

/**
 * @brief Uncore Arb unit, counter 1 event select MSR
 *
 */
#define MSR_UNC_ARB_PERFEVTSEL1 0x3b3

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_IS_ESCR0 0x3b4

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PMC4 0x3b4

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_IS_ESCR1 0x3b5

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration  Facility.
 *
 */
#define MSR_UNCORE_PMC5 0x3b5

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_ITLB_ESCR0 0x3b6

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PMC6 0x3b6

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_ITLB_ESCR1 0x3b7

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PMC7 0x3b7

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR0 0x3b8

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR1 0x3b9

/**
 * @brief See Section 18.12.1, ESCR MSRs. This MSR is not available on later processors.
 * It is  only available on processor family 0FH, models  01H-02H.
 *
 */
#define MSR_IQ_ESCR0 0x3ba

/**
 * @brief See Section 18.12.1, ESCR MSRs. This MSR is not available on later processors.
 * It is  only available on processor family 0FH, models  01H-02H.
 *
 */
#define MSR_IQ_ESCR1 0x3bb

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_RAT_ESCR0 0x3bc

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_RAT_ESCR1 0x3bd

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_SSU_ESCR0 0x3be

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_MS_ESCR0 0x3c0

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL0 0x3c0

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_MS_ESCR1 0x3c1

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL1 0x3c1

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_TBPU_ESCR0 0x3c2

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL2 0x3c2

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_TBPU_ESCR1 0x3c3

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL3 0x3c3

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_TC_ESCR0 0x3c4

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL4 0x3c4

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_TC_ESCR1 0x3c5

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL5 0x3c5

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL6 0x3c6

/**
 * @brief See Section 18.7.2.2, Uncore Performance Event Configuration Facility.
 *
 */
#define MSR_UNCORE_PERFEVTSEL7 0x3c7

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_IX_ESCR0 0x3c8

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_ALF_ESCR0 0x3ca

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_ALF_ESCR1 0x3cb

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR2 0x3cc

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR3 0x3cd

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR4 0x3e0

/**
 * @brief See Section 18.12.1, ESCR MSRs.
 *
 */
#define MSR_CRU_ESCR5 0x3e1

#define IA32_PEBS_ENABLE 0x3f1

/**
 * @brief Precise Event-Based Sampling (PEBS) (R/W)  Controls the enabling of precise event sampling  and replay tagging.
 *
 */
#define MSR_PEBS_ENABLE 0x3f1

/**
 * @brief See Table 19-26.
 *
 */
#define MSR_PEBS_MATRIX_VERT 0x3f2

/**
 * @brief see See Section 18.7.1.2, Load Latency Performance Monitoring  Facility.
 *
 */
#define MSR_PEBS_LD_LAT 0x3f6

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_PKG_C3_RESIDENCY 0x3f8

/**
 * @brief Package C2 Residency Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C-States
 *
 */
#define MSR_PKG_C2_RESIDENCY 0x3f8

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_PKG_C6C_RESIDENCY 0x3f9

/**
 * @brief Package C4 Residency Note: C-state values are processor specific 
 * C-state code names, unrelated to MWAIT extension C-state parameters or ACPI C-States
 *
 */
#define MSR_PKG_C4_RESIDENCY 0x3f9

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_PKG_C7_RESIDENCY 0x3fa

/**
 * @brief Package C6 Residency Note: C-state values are processor specific 
 * C-state code names,  unrelated to MWAIT extension C-state parameters or ACPI C-States
 *
 */
#define MSR_PKG_C6_RESIDENCY 0x3fa

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_CORE_C3_RESIDENCY 0x3fc

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_CORE_C4_RESIDENCY 0x3fc

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_CORE_C6_RESIDENCY 0x3fd

/**
 * @brief Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_CORE_C7_RESIDENCY 0x3fe

#define MC0_CTL 0x400

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define IA32_MC0_CTL 0x400

#define MC0_STATUS 0x401

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define IA32_MC0_STATUS 0x401

#define MC0_ADDR 0x402

/**
 * @brief P6 Family Processors
 *
 */
#define IA32_MC0_ADDR1 0x402

/**
 * @brief See Section 14.3.2.3., IA32_MCi_ADDR MSRs .  The IA32_MC0_ADDR register is 
 * either not implemented or contains no address if the ADDRV flag in the IA32_MC0_STATUS 
 * register is clear.  When not implemented in the processor, all reads and writes to 
 * this MSR  will cause a general-protection exception.
 *
 */
#define IA32_MC0_ADDR 0x402

/**
 * @brief Defined in MCA architecture but not implemented in the P6 family  processors.
 *
 */
#define MC0_MISC 0x403

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs. The IA32_MC0_MISC MSR is either not  
 * implemented or does not contain additional  information if the MISCV flag in the  IA32_MC0_STATUS 
 * register is clear. When not implemented in the processor, all reads  and writes to this MSR 
 * will cause a general- protection exception.
 *
 */
#define IA32_MC0_MISC 0x403

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC0_MISC 0x403

#define MC1_CTL 0x404

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define IA32_MC1_CTL 0x404

/**
 * @brief Bit definitions same as MC0_STATUS.
 *
 */
#define MC1_STATUS 0x405

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define IA32_MC1_STATUS 0x405

#define MC1_ADDR 0x406

/**
 * @brief P6 Family Processors
 *
 */
#define IA32_MC1_ADDR2 0x406

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The IA32_MC1_ADDR register is either 
 * not implemented or  contains no address if the ADDRV flag in the IA32_MC1_STATUS  
 * register is clear.  When not implemented in the processor, all reads and writes to this  
 * MSR will cause a general-protection exception.
 *
 */
#define IA32_MC1_ADDR 0x406

/**
 * @brief Defined in MCA architecture but not implemented in the P6 family  processors.
 *
 */
#define MC1_MISC 0x407

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs. The IA32_MC1_MISC MSR is either not  
 * implemented or does not contain additional  information if the MISCV flag in the  
 * IA32_MC1_STATUS register is clear. When not implemented in the processor, all reads  
 * and writes to this MSR will cause a general- protection exception.
 *
 */
#define IA32_MC1_MISC 0x407

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC1_MISC 0x407

#define MC2_CTL 0x408

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define IA32_MC2_CTL 0x408

/**
 * @brief Bit definitions same as MC0_STATUS.
 *
 */
#define MC2_STATUS 0x409

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define IA32_MC2_STATUS 0x409

#define MC2_ADDR 0x40a

/**
 * @brief P6 Family Processors
 *
 */
#define IA32_MC2_ADDR1 0x40a

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The IA32_MC2_ADDR register is either not  
 * implemented or contains no address if the ADDRV  flag in the IA32_MC2_STATUS register is clear.  
 * When not implemented in the processor, all reads  and writes to this MSR will cause a general- protection exception.
 *
 */
#define IA32_MC2_ADDR 0x40a

/**
 * @brief Defined in MCA architecture but not implemented in the P6 family  processors.
 *
 */
#define MC2_MISC 0x40b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs. The IA32_MC2_MISC MSR is either not  
 * implemented or does not contain additional  information if the MISCV flag in the IA32_MC2_STATUS 
 * register is clear.  When not implemented in the processor, all reads  and writes to this 
 * MSR will cause a general- protection exception.
 *
 */
#define IA32_MC2_MISC 0x40b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC2_MISC 0x40b

#define MC4_CTL 0x40c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define IA32_MC3_CTL 0x40c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC4_CTL 0x40c

/**
 * @brief Bit definitions same as MC0_STATUS, except bits 0, 4, 57, and 61 are  hardcoded to 1.
 *
 */
#define MC4_STATUS 0x40d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define IA32_MC3_STATUS 0x40d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS.
 *
 */
#define MSR_MC4_STATUS 0x40d

/**
 * @brief Defined in MCA architecture but not implemented in P6 Family processors.
 *
 */
#define MC4_ADDR 0x40e

/**
 * @brief P6 Family Processors
 *
 */
#define IA32_MC3_ADDR1 0x40e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The IA32_MC3_ADDR register is either not  
 * implemented or contains no address if the ADDRV  flag in the IA32_MC3_STATUS register is clear. 
 * When not implemented in the processor, all reads  and writes to this MSR will cause a 
 * general- protection exception.
 *
 */
#define IA32_MC3_ADDR 0x40e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The MSR_MC4_ADDR register is either not 
 * implemented or  contains no address if the ADDRV flag in the MSR_MC4_STATUS  register is clear. 
 * When not implemented in the processor, all reads and writes to this  MSR will cause a 
 * general-protection exception.
 *
 */
#define MSR_MC4_ADDR 0x412

/**
 * @brief Defined in MCA architecture but not implemented in the P6 family  processors.
 *
 */
#define MC4_MISC 0x40f

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs. The IA32_MC3_MISC MSR is either not  
 * implemented or does not contain additional  information if the MISCV flag in the  
 * IA32_MC3_STATUS register is clear. When not implemented in the processor, all reads  
 * and writes to this MSR will cause a general- protection exception.
 *
 */
#define IA32_MC3_MISC 0x40f

#define MC3_CTL 0x410

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define IA32_MC4_CTL 0x410

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC3_CTL 0x410

/**
 * @brief Bit definitions same as MC0_STATUS.
 *
 */
#define MC3_STATUS 0x411

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define IA32_MC4_STATUS 0x411

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS.
 *
 */
#define MSR_MC3_STATUS 0x411

#define MC3_ADDR 0x412

/**
 * @brief P6 Family Processors
 *
 */
#define IA32_MC4_ADDR1 0x412

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The IA32_MC2_ADDR register is either not  
 * implemented or contains no address if the ADDRV  flag in the IA32_MC4_STATUS register is clear.  
 * When not implemented in the processor, all reads  and writes to this MSR will cause a general-protection exception.
 *
 */
#define IA32_MC4_ADDR 0x412

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The MSR_MC3_ADDR register is either not implemented or  
 * contains no address if the ADDRV flag in the MSR_MC3_STATUS register is clear.  When not implemented in the processor, 
 * all reads and writes to this  MSR will cause a general-protection exception.
 *
 */
#define MSR_MC3_ADDR 0x412

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC3_MISC 0x40f

/**
 * @brief Defined in MCA architecture but not implemented in the P6 family  processors.
 *
 */
#define MC3_MISC 0x413

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.   The IA32_MC2_MISC MSR is either not  
 * implemented or does not contain additional  information if the MISCV flag in the  IA32_MC4_STATUS register is clear.  
 * When not implemented in the processor, all reads  and writes to this MSR will cause a general- protection exception.
 *
 */
#define IA32_MC4_MISC 0x413

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC4_MISC 0x413

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC5_CTL 0x414

/**
 * @brief 06_0FH
 *
 */
#define IA32_MC5_CTL 0x414

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC5_STATUS 0x415

/**
 * @brief 06_0FH
 *
 */
#define IA32_MC5_STATUS 0x415

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs. The MSR_MC4_ADDR register is either not implemented or  
 * contains no address if the ADDRV flag in the MSR_MC4_STATUS  register is clear. When not implemented in 
 * the processor, all reads and writes to this  MSR will cause a general-protection exception.
 *
 */
#define MSR_MC5_ADDR 0x416

/**
 * @brief 06_0FH
 *
 */
#define IA32_MC5_ADDR1 0x416

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC5_MISC 0x417

/**
 * @brief 06_0FH
 *
 */
#define IA32_MC5_MISC 0x417

/**
 * @brief 06_1DH
 *
 */
#define IA32_MC6_CTL 0x418

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC6_CTL 0x418

/**
 * @brief 06_1DH
 *
 */
#define IA32_MC6_STATUS 0x419

/**
 * @brief Apply to Intel Xeon processor 7400 series (processor signature  06_1D) only. 
 * See Section 15.3.2.2, IA32_MCi_STATUS MSRS.  and  Chapter 23.
 *
 */
#define MSR_MC6_STATUS 0x419

/**
 * @brief 06_1DH
 *
 */
#define IA32_MC6_ADDR1 0x41a

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC6_ADDR 0x41a

/**
 * @brief Misc MAC information of Integrated I/O. (R/O) see Section 15.3.2.4
 *
 */
#define IA32_MC6_MISC 0x41b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC6_MISC 0x41b

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC7_CTL 0x41c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC7_CTL 0x41c

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC7_STATUS 0x41d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC7_STATUS 0x41d

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC7_ADDR1 0x41e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC7_ADDR 0x41e

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC7_MISC 0x41f

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC7_MISC 0x41f

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC8_CTL 0x420

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC8_CTL 0x420

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC8_STATUS 0x421

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC8_STATUS 0x421

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC8_ADDR1 0x422

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC8_ADDR 0x422

/**
 * @brief 06_1AH
 *
 */
#define IA32_MC8_MISC 0x423

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC8_MISC 0x423

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC9_CTL 0x424

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC9_CTL 0x424

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC9_STATUS 0x425

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC9_STATUS 0x425

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC9_ADDR1 0x426

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC9_ADDR 0x426

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC9_MISC 0x427

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC9_MISC 0x427

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC10_CTL 0x428

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC10_CTL 0x428

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC10_STATUS 0x429

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC10_STATUS 0x429

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC10_ADDR1 0x42a

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC10_ADDR 0x42a

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC10_MISC 0x42b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC10_MISC 0x42b

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC11_CTL 0x42c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC11_CTL 0x42c

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC11_STATUS 0x42d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC11_STATUS 0x42d

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC11_ADDR1 0x42e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC11_ADDR 0x42e

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC11_MISC 0x42f

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC11_MISC 0x42f

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC12_CTL 0x430

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC12_CTL 0x430

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC12_STATUS 0x431

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC12_STATUS 0x431

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC12_ADDR1 0x432

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC12_ADDR 0x432

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC12_MISC 0x433

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC12_MISC 0x433

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC13_CTL 0x434

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC13_CTL 0x434

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC13_STATUS 0x435

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC13_STATUS 0x435

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC13_ADDR1 0x436

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC13_ADDR 0x436

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC13_MISC 0x437

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC13_MISC 0x437

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC14_CTL 0x438

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC14_CTL 0x438

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC14_STATUS 0x439

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC14_STATUS 0x439

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC14_ADDR1 0x43a

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC14_ADDR 0x43a

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC14_MISC 0x43b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC14_MISC 0x43b

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC15_CTL 0x43c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC15_CTL 0x43c

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC15_STATUS 0x43d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC15_STATUS 0x43d

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC15_ADDR1 0x43e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC15_ADDR 0x43e

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC15_MISC 0x43f

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC15_MISC 0x43f

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC16_CTL 0x440

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC16_CTL 0x440

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC16_STATUS 0x441

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC16_STATUS 0x441

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC16_ADDR1 0x442

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC16_ADDR 0x442

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC16_MISC 0x443

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC16_MISC 0x443

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC17_CTL 0x444

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC17_CTL 0x444

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC17_STATUS 0x445

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC17_STATUS 0x445

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC17_ADDR1 0x446

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC17_ADDR 0x446

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC17_MISC 0x447

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC17_MISC 0x447

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC18_CTL 0x448

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC18_CTL 0x448

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC18_STATUS 0x449

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC18_STATUS 0x449

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC18_ADDR1 0x44a

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC18_ADDR 0x44a

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC18_MISC 0x44b

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC18_MISC 0x44b

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC19_CTL 0x44c

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC19_CTL 0x44c

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC19_STATUS 0x44d

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC19_STATUS 0x44d

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC19_ADDR1 0x44e

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC19_ADDR 0x44e

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC19_MISC 0x44f

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC19_MISC 0x44f

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC20_CTL 0x450

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC20_CTL 0x450

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC20_STATUS 0x451

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC20_STATUS 0x451

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC20_ADDR1 0x452

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC20_ADDR 0x452

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC20_MISC 0x453

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC20_MISC 0x453

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC21_CTL 0x454

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC21_CTL 0x454

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC21_STATUS 0x455

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC21_STATUS 0x455

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC21_ADDR1 0x456

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC21_ADDR 0x456

/**
 * @brief 06_2EH
 *
 */
#define IA32_MC21_MISC 0x457

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC21_MISC 0x457

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC22_CTL 0x458

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC22_STATUS 0x459

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC22_ADDR 0x45a

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC22_MISC 0x45b

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC23_CTL 0x45c

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC23_STATUS 0x45d

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC23_ADDR 0x45e

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC23_MISC 0x45f

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC24_CTL 0x460

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC24_STATUS 0x461

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC24_ADDR 0x462

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC24_MISC 0x463

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC25_CTL 0x464

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC25_STATUS 0x465

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC25_ADDR 0x466

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC25_MISC 0x467

/**
 * @brief See Section 15.3.2.1,  IA32_MCi_CTL MSRs.
 *
 */
#define MSR_MC26_CTL 0x468

/**
 * @brief See Section 15.3.2.2, IA32_MCi_STATUS MSRS,  and Chapter 16.
 *
 */
#define MSR_MC26_STATUS 0x469

/**
 * @brief See Section 15.3.2.3, IA32_MCi_ADDR MSRs.
 *
 */
#define MSR_MC26_ADDR 0x46a

/**
 * @brief See Section 15.3.2.4,  IA32_MCi_MISC MSRs.
 *
 */
#define MSR_MC26_MISC 0x46b

/**
 * @brief Reporting Register of Basic VMX Capabilities (R/O) See Table 35-2. 
 * See Appendix A.1, Basic VMX Information (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_BASIC 0x480

/**
 * @brief Capability Reporting Register of Pin-based VM-execution  Controls (R/O) 
 * See Appendix A.3, VM-Execution Controls (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_PINBASED_CTLS 0x481

/**
 * @brief Capability Reporting Register of Primary Processor-based  
 * VM-execution Controls (R/O) See Appendix A.3, VM-Execution Controls (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_PROCBASED_CTLS 0x482

/**
 * @brief Capability Reporting Register of VM-exit Controls (R/O) See Appendix A.4, 
 * VM-Exit Controls (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_EXIT_CTLS 0x483

/**
 * @brief Capability Reporting Register of VM-entry Controls (R/O) See Appendix A.5, 
 * VM-Entry Controls (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_ENTRY_CTLS 0x484

/**
 * @brief Reporting Register of Miscellaneous VMX Capabilities (R/O) See Appendix A.6, 
 * Miscellaneous Data (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_MISC 0x485

/**
 * @brief Capability Reporting Register of CR0 Bits Fixed to 0 (R/O) See Appendix A.7,
 *  VMX-Fixed Bits in CR0 (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_CR0_FIXED0 0x486

/**
 * @brief If CPUID.01H:ECX.[bit 5] = 1
 *
 */
#define IA32_VMX_CRO_FIXED0 0x486

/**
 * @brief Capability Reporting Register of CR0 Bits Fixed to 1 (R/O) See Appendix A.7, 
 * VMX-Fixed Bits in CR0 (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_CR0_FIXED1 0x487

/**
 * @brief If CPUID.01H:ECX.[bit 5] = 1
 *
 */
#define IA32_VMX_CRO_FIXED1 0x487

/**
 * @brief Capability Reporting Register of CR4 Bits Fixed to 0 (R/O) See Appendix A.8, 
 * VMX-Fixed Bits in CR4 (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_CR4_FIXED0 0x488

/**
 * @brief Capability Reporting Register of CR4 Bits Fixed to 1 (R/O) See Appendix A.8, 
 * VMX-Fixed Bits in CR4 (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_CR4_FIXED1 0x489

/**
 * @brief Capability Reporting Register of VMCS Field Enumeration (R/O) See Appendix A.9, 
 * VMCS Enumeration (If CPUID.01H:ECX.[bit 9])
 *
 */
#define IA32_VMX_VMCS_ENUM 0x48a

/**
 * @brief Capability Reporting Register of Secondary Processor-based  VM-execution Controls (R/O) 
 * See Appendix A.3, VM-Execution Controls (If CPUID.01H:ECX.[bit 9] and  IA32_VMX_PROCBASED_CTLS[bit 63])
 *
 */
#define IA32_VMX_PROCBASED_CTLS2 0x48b

/**
 * @brief Capability Reporting Register of EPT and VPID (R/O)  See Table 35-2
 *
 */
#define IA32_VMX_EPT_VPID_ENUM 0x48c

/**
 * @brief If ( CPUID.01H:ECX.[bit 5],  IA32_VMX_PROCBASED_C TLS[bit 63], and either  
 * IA32_VMX_PROCBASED_C TLS2[bit 33] or  IA32_VMX_PROCBASED_C TLS2[bit 37])
 *
 */
#define IA32_VMX_EPT_VPID_CAP 0x48c

/**
 * @brief Capability Reporting Register of Pin-based VM-execution Flex  Controls (R/O) See Table 35-2
 *
 */
#define IA32_VMX_TRUE_PINBASED_CTLS 0x48d

/**
 * @brief Capability Reporting Register of Primary Processor-based  VM-execution Flex Controls (R/O) See Table 35-2
 *
 */
#define IA32_VMX_TRUE_PROCBASED_CTLS 0x48e

/**
 * @brief Capability Reporting Register of VM-exit Flex Controls (R/O) See Table 35-2
 *
 */
#define IA32_VMX_TRUE_EXIT_CTLS 0x48f

/**
 * @brief Capability Reporting Register of VM-entry Flex Controls (R/O) See Table 35-2
 *
 */
#define IA32_VMX_TRUE_ENTRY_CTLS 0x490

/**
 * @brief Capability Reporting Register of VM-function Controls (R/O) See Table 35-2
 *
 */
#define IA32_VMX_FMFUNC 0x491

/**
 * @brief If( CPUID.01H:ECX.[bit 5] =  1 and IA32_VMX_BASIC[bit 55] )
 *
 */
#define IA32_VMX_VMFUNC 0x491

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  0) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC0 0x4c1

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  1) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC1 0x4c2

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  2) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC2 0x4c3

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  3) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC3 0x4c4

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  4) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC4 0x4c5

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  5) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC5 0x4c6

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  6) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC6 0x4c7

/**
 * @brief (If CPUID.0AH: EAX[15:8] >  7) & IA32_PERF_CAPABILITIES[ 13] = 1
 *
 */
#define IA32_A_PMC7 0x4c8

/**
 * @brief Enhanced SMM Feature Control (SMM-RW) Reports SMM capability Enhancement. Accessible only while in  SMM.
 *
 */
#define MSR_SMM_FEATURE_CONTROL 0x4e0

/**
 * @brief SMM Delayed (SMM-RO) Reports the interruptible state of all logical processors in the  package . 
 * Available only while in SMM and  MSR_SMM_MCA_CAP[LONG_FLOW_INDICATION] == 1.
 *
 */
#define MSR_SMM_DELAYED 0x4e2

/**
 * @brief SMM Blocked (SMM-RO) Reports the blocked state of all logical processors in the package .  
 * Available only while in SMM.
 *
 */
#define MSR_SMM_BLOCKED 0x4e3

/**
 * @brief Trace Output Base Register (R/W)
 *
 */
#define MSR_IA32_RTIT_OUTPUT_BASE 0x560

/**
 * @brief Trace Output Mask Pointers Register (R/W)
 *
 */
#define MSR_IA32_RTIT_OUTPUT_MASK_PTRS 0x561

/**
 * @brief Trace Control Register (R/W)
 *
 */
#define MSR_IA32_RTIT_CTL 0x570

/**
 * @brief Tracing Status Register (R/W)
 *
 */
#define MSR_IA32_RTIT_STATUS 0x571

/**
 * @brief Trace Filter CR3 Match Register (R/W)
 *
 */
#define MSR_IA32_CR3_MATCH 0x572

/**
 * @brief Trace Start Address 0
 *
 */
#define MSR_IA32_ADDR0_START 0x580

/**
 * @brief Trace End Address 0
 *
 */
#define MSR_IA32_ADDR0_END 0x581

/**
 * @brief Trace Start Address 1
 *
 */
#define MSR_IA32_ADDR1_START 0x582

/**
 * @brief Trace End Address 1
 *
 */
#define MSR_IA32_ADDR1_END 0x583

/**
 * @brief Trace Start Address 3
 *
 */
#define MSR_IA32_ADDR2_START 0x584

/**
 * @brief Trace End Address 3
 *
 */
#define MSR_IA32_ADDR2_END 0x585

/**
 * @brief Trace Start Address 4
 *
 */
#define MSR_IA32_ADDR3_START 0x586

/**
 * @brief Trace End Address 4
 *
 */
#define MSR_IA32_ADDR3_END 0x587

/**
 * @brief DS Save Area (R/W) See Table 35-2. Points to the DS buffer management area, which is used to manage the  
 * BTS and PEBS buffers. See Section 18.12.4, Debug Store (DS)  Mechanism.
 *
 */
#define IA32_DS_AREA 0x600

/**
 * @brief Unit Multipliers used in RAPL Interfaces (R/O)  See Section 14.7.1, RAPL Interfaces.
 *
 */
#define MSR_RAPL_POWER_UNIT 0x606

/**
 * @brief Package C3 Interrupt Response Limit (R/W)  Note: C-state values are processor specific C-state code names,  
 * unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_PKGC3_IRTL 0x60a

/**
 * @brief Package C6 Interrupt Response Limit (R/W)  This MSR defines the budget allocated for the package to 
 * exit from  C6 to a C0 state, where interrupt request can be delivered to the  core and serviced. Additional 
 * core-exit latency amy be applicable  depending on the actual C-state the core is in.  Note: C-state values are 
 * processor specific C-state code names,  unrelated to MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_PKGC6_IRTL 0x60b

/**
 * @brief Package C7 Interrupt Response Limit (R/W)  This MSR defines the budget allocated for the package to 
 * exit from  C7 to a C0 state, where interrupt request can be delivered to the  core and serviced. Additional 
 * core-exit latency amy be applicable  depending on the actual C-state the core is in.  Note: C-state values 
 * are processor specific C-state code names,  unrelated to MWAIT extension C-state parameters or ACPI C-States.
 *
 */
#define MSR_PKGC7_IRTL 0x60c

/**
 * @brief PKG RAPL Power Limit Control (R/W)  See Section 14.7.3, Package RAPL Domain.
 *
 */
#define MSR_PKG_POWER_LIMIT 0x610

/**
 * @brief PKG Energy Status (R/O)  See Section 14.7.3, Package RAPL Domain.
 *
 */
#define MSR_PKG_ENERGY_STATUS 0x611

/**
 * @brief Package RAPL Perf Status (R/O)
 *
 */
#define MSR_PKG_PERF_STATUS 0x613

/**
 * @brief PKG RAPL Parameters (R/W) See Section 14.7.3,  Package RAPL  Domain.
 *
 */
#define MSR_PKG_POWER_INFO 0x614

/**
 * @brief DRAM RAPL Power Limit Control (R/W)  See Section 14.7.5, DRAM RAPL Domain.
 *
 */
#define MSR_DRAM_POWER_LIMIT 0x618

/**
 * @brief DRAM Energy Status (R/O)  See Section 14.7.5, DRAM RAPL Domain.
 *
 */
#define MSR_DRAM_ENERGY_STATUS 0x619

/**
 * @brief DRAM Performance Throttling Status (R/O) See Section 14.7.5,  DRAM RAPL Domain.
 *
 */
#define MSR_DRAM_PERF_STATUS 0x61b

/**
 * @brief DRAM RAPL Parameters (R/W) See Section 14.7.5, DRAM RAPL Domain.
 *
 */
#define MSR_DRAM_POWER_INFO 0x61c

/**
 * @brief Note: C-state values are processor specific C-state code names, unrelated to MWAIT 
 * extension C-state parameters or ACPI C-States.
 *
 */
#define MSR_PKG_C9_RESIDENCY 0x631

/**
 * @brief Note: C-state values are processor specific C-state code names,  unrelated to MWAIT 
 * extension C-state parameters or ACPI C-States.
 *
 */
#define MSR_PKG_C10_RESIDENCY 0x632

/**
 * @brief PP0 RAPL Power Limit Control (R/W)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP0_POWER_LIMIT 0x638

/**
 * @brief PP0 Energy Status (R/O)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP0_ENERGY_STATUS 0x639

/**
 * @brief PP0 Balance Policy (R/W)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP0_POLICY 0x63a

/**
 * @brief PP0 Performance Throttling Status (R/O) See Section 14.7.4,  PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP0_PERF_STATUS 0x63b

/**
 * @brief PP1 RAPL Power Limit Control (R/W)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP1_POWER_LIMIT 0x640

/**
 * @brief PP1 Energy Status (R/O)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP1_ENERGY_STATUS 0x641

/**
 * @brief PP1 Balance Policy (R/W)  See Section 14.7.4, PP0/PP1 RAPL Domains.
 *
 */
#define MSR_PP1_POLICY 0x642

/**
 * @brief Nominal TDP Ratio (R/O)
 *
 */
#define MSR_CONFIG_TDP_NOMINAL 0x648

/**
 * @brief ConfigTDP Level 1 ratio and power level (R/O)
 *
 */
#define MSR_CONFIG_TDP_LEVEL1 0x649

/**
 * @brief ConfigTDP Level 2 ratio and power level (R/O)
 *
 */
#define MSR_CONFIG_TDP_LEVEL2 0x64a

/**
 * @brief ConfigTDP Control (R/W)
 *
 */
#define MSR_CONFIG_TDP_CONTROL 0x64b

/**
 * @brief ConfigTDP Control (R/W)
 *
 */
#define MSR_TURBO_ACTIVATION_RATIO 0x64c

/**
 * @brief Note: C-state values are processor specific C-state code names,  unrelated to 
 * MWAIT extension C-state parameters or ACPI C- States.
 *
 */
#define MSR_CORE_C1_RESIDENCY 0x660

/**
 * @brief Last Branch Record 8 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_8_FROM_IP 0x688

/**
 * @brief Last Branch Record 9 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_9_FROM_IP 0x689

/**
 * @brief Last Branch Record 10 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_10_FROM_IP 0x68a

/**
 * @brief Last Branch Record 11 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_11_FROM_IP 0x68b

/**
 * @brief Last Branch Record 12 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_12_FROM_IP 0x68c

/**
 * @brief Last Branch Record 13 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_13_FROM_IP 0x68d

/**
 * @brief Last Branch Record 14 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_14_FROM_IP 0x68e

/**
 * @brief Last Branch Record 15 From IP (R/W) See description of MSR_LASTBRANCH_0_FROM_IP.
 *
 */
#define MSR_LASTBRANCH_15_FROM_IP 0x68f

/**
 * @brief Last Branch Record 8 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_8_TO_IP 0x6c8

/**
 * @brief Last Branch Record 9 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_9_TO_IP 0x6c9

/**
 * @brief Last Branch Record 10 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_10_TO_IP 0x6ca

/**
 * @brief Last Branch Record 11 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_11_TO_IP 0x6cb

/**
 * @brief Last Branch Record 12 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_12_TO_IP 0x6cc

/**
 * @brief Last Branch Record 13 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_13_TO_IP 0x6cd

/**
 * @brief Last Branch Record 14 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_14_TO_IP 0x6ce

/**
 * @brief Last Branch Record 15 To IP (R/W) See description of MSR_LASTBRANCH_0_TO_IP.
 *
 */
#define MSR_LASTBRANCH_15_TO_IP 0x6cf

/**
 * @brief TSC Target of Local APIC s TSC Deadline Mode (R/W)  See Table 35-2
 *
 */
#define IA32_TSC_DEADLINE 0x6e0

/**
 * @brief Uncore C-Box 0, counter 0 event select MSR
 *
 */
#define MSR_UNC_CBO_0_PERFEVTSEL0 0x700

/**
 * @brief Uncore C-Box 0, counter 1 event select MSR
 *
 */
#define MSR_UNC_CBO_0_PERFEVTSEL1 0x701

/**
 * @brief Uncore C-Box 0, performance counter 0
 *
 */
#define MSR_UNC_CBO_0_PER_CTR0 0x706

/**
 * @brief Uncore C-Box 0, performance counter 1
 *
 */
#define MSR_UNC_CBO_0_PER_CTR1 0x707

/**
 * @brief Uncore C-Box 1, counter 0 event select MSR
 *
 */
#define MSR_UNC_CBO_1_PERFEVTSEL0 0x710

/**
 * @brief Uncore C-Box 1, counter 1 event select MSR
 *
 */
#define MSR_UNC_CBO_1_PERFEVTSEL1 0x711

/**
 * @brief Uncore C-Box 1, performance counter 0
 *
 */
#define MSR_UNC_CBO_1_PER_CTR0 0x716

/**
 * @brief Uncore C-Box 1, performance counter 1
 *
 */
#define MSR_UNC_CBO_1_PER_CTR1 0x717

/**
 * @brief Uncore C-Box 2, counter 0 event select MSR
 *
 */
#define MSR_UNC_CBO_2_PERFEVTSEL0 0x720

/**
 * @brief Uncore C-Box 2, counter 1 event select MSR
 *
 */
#define MSR_UNC_CBO_2_PERFEVTSEL1 0x721

/**
 * @brief Uncore C-Box 2, performance counter 0
 *
 */
#define MSR_UNC_CBO_2_PER_CTR0 0x726

/**
 * @brief Uncore C-Box 2, performance counter 1
 *
 */
#define MSR_UNC_CBO_2_PER_CTR1 0x727

/**
 * @brief Uncore C-Box 3, counter 0 event select MSR
 *
 */
#define MSR_UNC_CBO_3_PERFEVTSEL0 0x730

/**
 * @brief Uncore C-Box 3, counter 1 event select MSR.
 *
 */
#define MSR_UNC_CBO_3_PERFEVTSEL1 0x731

/**
 * @brief Uncore C-Box 3, performance counter 0.
 *
 */
#define MSR_UNC_CBO_3_PER_CTR0 0x736

/**
 * @brief Uncore C-Box 3, performance counter 1.
 *
 */
#define MSR_UNC_CBO_3_PER_CTR1 0x737

/**
 * @brief x2APIC ID register (R/O) See x2APIC Specification.
 *
 */
#define IA32_X2APIC_APICID 0x802

/**
 * @brief x2APIC Version. If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_VERSION 0x803

/**
 * @brief x2APIC Task Priority register (R/W)
 *
 */
#define IA32_X2APIC_TPR 0x808

/**
 * @brief x2APIC Processor Priority register (R/O)
 *
 */
#define IA32_X2APIC_PPR 0x80a

/**
 * @brief x2APIC End of Interrupt. If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_EOI 0x80b

/**
 * @brief x2APIC Logical Destination register (R/O)
 *
 */
#define IA32_X2APIC_LDR 0x80d

/**
 * @brief x2APIC Spurious Interrupt Vector register (R/W)
 *
 */
#define IA32_X2APIC_SIVR 0x80f

/**
 * @brief x2APIC In-Service register bits [31:0] (R/O)
 *
 */
#define IA32_X2APIC_ISR0 0x810

/**
 * @brief x2APIC In-Service register bits [63:32] (R/O)
 *
 */
#define IA32_X2APIC_ISR1 0x811

/**
 * @brief x2APIC In-Service register bits [95:64] (R/O)
 *
 */
#define IA32_X2APIC_ISR2 0x812

/**
 * @brief x2APIC In-Service register bits [127:96] (R/O)
 *
 */
#define IA32_X2APIC_ISR3 0x813

/**
 * @brief x2APIC In-Service register bits [159:128] (R/O)
 *
 */
#define IA32_X2APIC_ISR4 0x814

/**
 * @brief x2APIC In-Service register bits [191:160] (R/O)
 *
 */
#define IA32_X2APIC_ISR5 0x815

/**
 * @brief x2APIC In-Service register bits [223:192] (R/O)
 *
 */
#define IA32_X2APIC_ISR6 0x816

/**
 * @brief x2APIC In-Service register bits [255:224] (R/O)
 *
 */
#define IA32_X2APIC_ISR7 0x817

/**
 * @brief x2APIC Trigger Mode register bits [31:0] (R/O)
 *
 */
#define IA32_X2APIC_TMR0 0x818

/**
 * @brief x2APIC Trigger Mode register bits [63:32] (R/O)
 *
 */
#define IA32_X2APIC_TMR1 0x819

/**
 * @brief x2APIC Trigger Mode register bits [95:64] (R/O)
 *
 */
#define IA32_X2APIC_TMR2 0x81a

/**
 * @brief x2APIC Trigger Mode register bits [127:96] (R/O)
 *
 */
#define IA32_X2APIC_TMR3 0x81b

/**
 * @brief x2APIC Trigger Mode register bits [159:128] (R/O)
 *
 */
#define IA32_X2APIC_TMR4 0x81c

/**
 * @brief x2APIC Trigger Mode register bits [191:160] (R/O)
 *
 */
#define IA32_X2APIC_TMR5 0x81d

/**
 * @brief x2APIC Trigger Mode register bits [223:192] (R/O)
 *
 */
#define IA32_X2APIC_TMR6 0x81e

/**
 * @brief x2APIC Trigger Mode register bits [255:224] (R/O)
 *
 */
#define IA32_X2APIC_TMR7 0x81f

/**
 * @brief x2APIC Interrupt Request register bits [31:0] (R/O)
 *
 */
#define IA32_X2APIC_IRR0 0x820

/**
 * @brief x2APIC Interrupt Request register bits [63:32] (R/O)
 *
 */
#define IA32_X2APIC_IRR1 0x821

/**
 * @brief x2APIC Interrupt Request register bits [95:64] (R/O)
 *
 */
#define IA32_X2APIC_IRR2 0x822

/**
 * @brief x2APIC Interrupt Request register bits [127:96] (R/O)
 *
 */
#define IA32_X2APIC_IRR3 0x823

/**
 * @brief x2APIC Interrupt Request register bits [159:128] (R/O)
 *
 */
#define IA32_X2APIC_IRR4 0x824

/**
 * @brief x2APIC Interrupt Request register bits [191:160] (R/O)
 *
 */
#define IA32_X2APIC_IRR5 0x825

/**
 * @brief x2APIC Interrupt Request register bits [223:192] (R/O)
 *
 */
#define IA32_X2APIC_IRR6 0x826

/**
 * @brief x2APIC Interrupt Request register bits [255:224] (R/O)
 *
 */
#define IA32_X2APIC_IRR7 0x827

/**
 * @brief Error Status Register. If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_ESR 0x828

/**
 * @brief x2APIC LVT Corrected Machine Check Interrupt register (R/W)
 *
 */
#define IA32_X2APIC_LVT_CMCI 0x82f

/**
 * @brief x2APIC Interrupt Command register (R/W)
 *
 */
#define IA32_X2APIC_ICR 0x830

/**
 * @brief x2APIC LVT Timer Interrupt register (R/W)
 *
 */
#define IA32_X2APIC_LVT_TIMER 0x832

/**
 * @brief x2APIC LVT Thermal Sensor Interrupt register (R/W)
 *
 */
#define IA32_X2APIC_LVT_THERMAL 0x833

/**
 * @brief x2APIC LVT Performance Monitor register (R/W)
 *
 */
#define IA32_X2APIC_LVT_PMI 0x834

/**
 * @brief If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_LVT_LINT0 0x835

/**
 * @brief If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_LVT_LINT1 0x836

/**
 * @brief If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_LVT_ERROR 0x837

/**
 * @brief x2APIC Initial Count register (R/W)
 *
 */
#define IA32_X2APIC_INIT_COUNT 0x838

/**
 * @brief x2APIC Current Count register (R/O)
 *
 */
#define IA32_X2APIC_CUR_COUNT 0x839

/**
 * @brief x2APIC Divide Configuration register (R/W)
 *
 */
#define IA32_X2APIC_DIV_CONF 0x83e

/**
 * @brief If ( CPUID.01H:ECX.[bit 21]  = 1 )
 *
 */
#define IA32_X2APIC_SELF_IPI 0x83f

/**
 * @brief Uncore U-box perfmon global control MSR.
 *
 */
#define MSR_U_PMON_GLOBAL_CTRL 0xc00

/**
 * @brief Uncore U-box perfmon global status MSR.
 *
 */
#define MSR_U_PMON_GLOBAL_STATUS 0xc01

/**
 * @brief Uncore U-box perfmon global overflow control MSR.
 *
 */
#define MSR_U_PMON_GLOBAL_OVF_CTRL 0xc02

/**
 * @brief Uncore U-box perfmon event select MSR.
 *
 */
#define MSR_U_PMON_EVNT_SEL 0xc10

/**
 * @brief Uncore U-box perfmon counter MSR.
 *
 */
#define MSR_U_PMON_CTR 0xc11

/**
 * @brief Uncore B-box 0 perfmon local box control MSR.
 *
 */
#define MSR_B0_PMON_BOX_CTRL 0xc20

/**
 * @brief Uncore B-box 0 perfmon local box status MSR.
 *
 */
#define MSR_B0_PMON_BOX_STATUS 0xc21

/**
 * @brief Uncore B-box 0 perfmon local box overflow control MSR.
 *
 */
#define MSR_B0_PMON_BOX_OVF_CTRL 0xc22

/**
 * @brief Uncore B-box 0 perfmon event select MSR.
 *
 */
#define MSR_B0_PMON_EVNT_SEL0 0xc30

/**
 * @brief Uncore B-box 0 perfmon counter MSR.
 *
 */
#define MSR_B0_PMON_CTR0 0xc31

/**
 * @brief Uncore B-box 0 perfmon event select MSR.
 *
 */
#define MSR_B0_PMON_EVNT_SEL1 0xc32

/**
 * @brief Uncore B-box 0 perfmon counter MSR.
 *
 */
#define MSR_B0_PMON_CTR1 0xc33

/**
 * @brief Uncore B-box 0 perfmon event select MSR.
 *
 */
#define MSR_B0_PMON_EVNT_SEL2 0xc34

/**
 * @brief Uncore B-box 0 perfmon counter MSR.
 *
 */
#define MSR_B0_PMON_CTR2 0xc35

/**
 * @brief Uncore B-box 0 perfmon event select MSR.
 *
 */
#define MSR_B0_PMON_EVNT_SEL3 0xc36

/**
 * @brief Uncore B-box 0 perfmon counter MSR.
 *
 */
#define MSR_B0_PMON_CTR3 0xc37

/**
 * @brief Uncore S-box 0 perfmon local box control MSR.
 *
 */
#define MSR_S0_PMON_BOX_CTRL 0xc40

/**
 * @brief Uncore S-box 0 perfmon local box status MSR.
 *
 */
#define MSR_S0_PMON_BOX_STATUS 0xc41

/**
 * @brief Uncore S-box 0 perfmon local box overflow control MSR.
 *
 */
#define MSR_S0_PMON_BOX_OVF_CTRL 0xc42

/**
 * @brief Uncore S-box 0 perfmon event select MSR.
 *
 */
#define MSR_S0_PMON_EVNT_SEL0 0xc50

/**
 * @brief Uncore S-box 0 perfmon counter MSR.
 *
 */
#define MSR_S0_PMON_CTR0 0xc51

/**
 * @brief Uncore S-box 0 perfmon event select MSR.
 *
 */
#define MSR_S0_PMON_EVNT_SEL1 0xc52

/**
 * @brief Uncore S-box 0 perfmon counter MSR.
 *
 */
#define MSR_S0_PMON_CTR1 0xc53

/**
 * @brief Uncore S-box 0 perfmon event select MSR.
 *
 */
#define MSR_S0_PMON_EVNT_SEL2 0xc54

/**
 * @brief Uncore S-box 0 perfmon counter MSR.
 *
 */
#define MSR_S0_PMON_CTR2 0xc55

/**
 * @brief Uncore S-box 0 perfmon event select MSR.
 *
 */
#define MSR_S0_PMON_EVNT_SEL3 0xc56

/**
 * @brief Uncore S-box 0 perfmon counter MSR.
 *
 */
#define MSR_S0_PMON_CTR3 0xc57

/**
 * @brief Uncore B-box 1 perfmon local box control MSR.
 *
 */
#define MSR_B1_PMON_BOX_CTRL 0xc60

/**
 * @brief Uncore B-box 1 perfmon local box status MSR.
 *
 */
#define MSR_B1_PMON_BOX_STATUS 0xc61

/**
 * @brief Uncore B-box 1 perfmon local box overflow control MSR.
 *
 */
#define MSR_B1_PMON_BOX_OVF_CTRL 0xc62

/**
 * @brief Uncore B-box 1 perfmon event select MSR.
 *
 */
#define MSR_B1_PMON_EVNT_SEL0 0xc70

/**
 * @brief Uncore B-box 1 perfmon counter MSR.
 *
 */
#define MSR_B1_PMON_CTR0 0xc71

/**
 * @brief Uncore B-box 1 perfmon event select MSR.
 *
 */
#define MSR_B1_PMON_EVNT_SEL1 0xc72

/**
 * @brief Uncore B-box 1 perfmon counter MSR.
 *
 */
#define MSR_B1_PMON_CTR1 0xc73

/**
 * @brief Uncore B-box 1 perfmon event select MSR.
 *
 */
#define MSR_B1_PMON_EVNT_SEL2 0xc74

/**
 * @brief Uncore B-box 1 perfmon counter MSR.
 *
 */
#define MSR_B1_PMON_CTR2 0xc75

/**
 * @brief Uncore B-box 1vperfmon event select MSR.
 *
 */
#define MSR_B1_PMON_EVNT_SEL3 0xc76

/**
 * @brief Uncore B-box 1 perfmon counter MSR.
 *
 */
#define MSR_B1_PMON_CTR3 0xc77

/**
 * @brief Uncore W-box perfmon local box control MSR.
 *
 */
#define MSR_W_PMON_BOX_CTRL 0xc80

/**
 * @brief Uncore W-box perfmon local box status MSR.
 *
 */
#define MSR_W_PMON_BOX_STATUS 0xc81

/**
 * @brief Uncore W-box perfmon local box overflow control MSR.
 *
 */
#define MSR_W_PMON_BOX_OVF_CTRL 0xc82

/**
 * @brief If ( CPUID.(EAX=07H,  ECX=0):EBX.[bit 12] = 1 )
 *
 */
#define IA32_QM_EVTSEL 0xc8d

/**
 * @brief If ( CPUID.(EAX=07H,  ECX=0):EBX.[bit 12] = 1 )
 *
 */
#define IA32_QM_CTR 0xc8e

/**
 * @brief If ( CPUID.(EAX=07H,  ECX=0):EBX.[bit 12] = 1 )
 *
 */
#define IA32_PQR_ASSOC 0xc8f

/**
 * @brief Uncore W-box perfmon event select MSR.
 *
 */
#define MSR_W_PMON_EVNT_SEL0 0xc90

/**
 * @brief Uncore W-box perfmon counter MSR.
 *
 */
#define MSR_W_PMON_CTR0 0xc91

/**
 * @brief Uncore W-box perfmon event select MSR.
 *
 */
#define MSR_W_PMON_EVNT_SEL1 0xc92

/**
 * @brief Uncore W-box perfmon counter MSR.
 *
 */
#define MSR_W_PMON_CTR1 0xc93

/**
 * @brief Uncore W-box perfmon event select MSR.
 *
 */
#define MSR_W_PMON_EVNT_SEL2 0xc94

/**
 * @brief Uncore W-box perfmon counter MSR.
 *
 */
#define MSR_W_PMON_CTR2 0xc95

/**
 * @brief Uncore W-box perfmon event select MSR.
 *
 */
#define MSR_W_PMON_EVNT_SEL3 0xc96

/**
 * @brief Uncore W-box perfmon counter MSR.
 *
 */
#define MSR_W_PMON_CTR3 0xc97

/**
 * @brief Uncore M-box 0 perfmon local box control MSR.
 *
 */
#define MSR_M0_PMON_BOX_CTRL 0xca0

/**
 * @brief Uncore M-box 0 perfmon local box status MSR.
 *
 */
#define MSR_M0_PMON_BOX_STATUS 0xca1

/**
 * @brief Uncore M-box 0 perfmon local box overflow control MSR.
 *
 */
#define MSR_M0_PMON_BOX_OVF_CTRL 0xca2

/**
 * @brief Uncore M-box 0 perfmon time stamp unit select MSR.
 *
 */
#define MSR_M0_PMON_TIMESTAMP 0xca4

/**
 * @brief Uncore M-box 0 perfmon DSP unit select MSR.
 *
 */
#define MSR_M0_PMON_DSP 0xca5

/**
 * @brief Uncore M-box 0 perfmon ISS unit select MSR.
 *
 */
#define MSR_M0_PMON_ISS 0xca6

/**
 * @brief Uncore M-box 0 perfmon MAP unit select MSR.
 *
 */
#define MSR_M0_PMON_MAP 0xca7

/**
 * @brief Uncore M-box 0 perfmon MIC THR select MSR.
 *
 */
#define MSR_M0_PMON_MSC_THR 0xca8

/**
 * @brief Uncore M-box 0 perfmon PGT unit select MSR.
 *
 */
#define MSR_M0_PMON_PGT 0xca9

/**
 * @brief Uncore M-box 0 perfmon PLD unit select MSR.
 *
 */
#define MSR_M0_PMON_PLD 0xcaa

/**
 * @brief Uncore M-box 0 perfmon ZDP unit select MSR.
 *
 */
#define MSR_M0_PMON_ZDP 0xcab

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL0 0xcb0

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR0 0xcb1

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL1 0xcb2

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR1 0xcb3

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL2 0xcb4

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR2 0xcb5

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL3 0xcb6

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR3 0xcb7

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL4 0xcb8

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR4 0xcb9

/**
 * @brief Uncore M-box 0 perfmon event select MSR.
 *
 */
#define MSR_M0_PMON_EVNT_SEL5 0xcba

/**
 * @brief Uncore M-box 0 perfmon counter MSR.
 *
 */
#define MSR_M0_PMON_CTR5 0xcbb

/**
 * @brief Uncore S-box 1 perfmon local box control MSR.
 *
 */
#define MSR_S1_PMON_BOX_CTRL 0xcc0

/**
 * @brief Uncore S-box 1 perfmon local box status MSR.
 *
 */
#define MSR_S1_PMON_BOX_STATUS 0xcc1

/**
 * @brief Uncore S-box 1 perfmon local box overflow control MSR.
 *
 */
#define MSR_S1_PMON_BOX_OVF_CTRL 0xcc2

/**
 * @brief Uncore S-box 1 perfmon event select MSR.
 *
 */
#define MSR_S1_PMON_EVNT_SEL0 0xcd0

/**
 * @brief Uncore S-box 1 perfmon counter MSR.
 *
 */
#define MSR_S1_PMON_CTR0 0xcd1

/**
 * @brief Uncore S-box 1 perfmon event select MSR.
 *
 */
#define MSR_S1_PMON_EVNT_SEL1 0xcd2

/**
 * @brief Uncore S-box 1 perfmon counter MSR.
 *
 */
#define MSR_S1_PMON_CTR1 0xcd3

/**
 * @brief Uncore S-box 1 perfmon event select MSR.
 *
 */
#define MSR_S1_PMON_EVNT_SEL2 0xcd4

/**
 * @brief Uncore S-box 1 perfmon counter MSR.
 *
 */
#define MSR_S1_PMON_CTR2 0xcd5

/**
 * @brief Uncore S-box 1 perfmon event select MSR.
 *
 */
#define MSR_S1_PMON_EVNT_SEL3 0xcd6

/**
 * @brief Uncore S-box 1 perfmon counter MSR.
 *
 */
#define MSR_S1_PMON_CTR3 0xcd7

/**
 * @brief Uncore M-box 1 perfmon local box control MSR.
 *
 */
#define MSR_M1_PMON_BOX_CTRL 0xce0

/**
 * @brief Uncore M-box 1 perfmon local box status MSR.
 *
 */
#define MSR_M1_PMON_BOX_STATUS 0xce1

/**
 * @brief Uncore M-box 1 perfmon local box overflow control MSR.
 *
 */
#define MSR_M1_PMON_BOX_OVF_CTRL 0xce2

/**
 * @brief Uncore M-box 1 perfmon time stamp unit select MSR.
 *
 */
#define MSR_M1_PMON_TIMESTAMP 0xce4

/**
 * @brief Uncore M-box 1 perfmon DSP unit select MSR.
 *
 */
#define MSR_M1_PMON_DSP 0xce5

/**
 * @brief Uncore M-box 1 perfmon ISS unit select MSR.
 *
 */
#define MSR_M1_PMON_ISS 0xce6

/**
 * @brief Uncore M-box 1 perfmon MAP unit select MSR.
 *
 */
#define MSR_M1_PMON_MAP 0xce7

/**
 * @brief Uncore M-box 1 perfmon MIC THR select MSR.
 *
 */
#define MSR_M1_PMON_MSC_THR 0xce8

/**
 * @brief Uncore M-box 1 perfmon PGT unit select MSR.
 *
 */
#define MSR_M1_PMON_PGT 0xce9

/**
 * @brief Uncore M-box 1 perfmon PLD unit select MSR.
 *
 */
#define MSR_M1_PMON_PLD 0xcea

/**
 * @brief Uncore M-box 1 perfmon ZDP unit select MSR.
 *
 */
#define MSR_M1_PMON_ZDP 0xceb

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL0 0xcf0

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR0 0xcf1

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL1 0xcf2

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR1 0xcf3

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL2 0xcf4

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR2 0xcf5

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL3 0xcf6

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR3 0xcf7

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL4 0xcf8

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR4 0xcf9

/**
 * @brief Uncore M-box 1 perfmon event select MSR.
 *
 */
#define MSR_M1_PMON_EVNT_SEL5 0xcfa

/**
 * @brief Uncore M-box 1 perfmon counter MSR.
 *
 */
#define MSR_M1_PMON_CTR5 0xcfb

/**
 * @brief Uncore C-box 0 perfmon local box control MSR.
 *
 */
#define MSR_C0_PMON_BOX_CTRL 0xd00

/**
 * @brief Uncore C-box 0 perfmon local box status MSR.
 *
 */
#define MSR_C0_PMON_BOX_STATUS 0xd01

/**
 * @brief Uncore C-box 0 perfmon local box overflow control MSR.
 *
 */
#define MSR_C0_PMON_BOX_OVF_CTRL 0xd02

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL0 0xd10

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR0 0xd11

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL1 0xd12

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR1 0xd13

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL2 0xd14

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR2 0xd15

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL3 0xd16

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR3 0xd17

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL4 0xd18

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR4 0xd19

/**
 * @brief Uncore C-box 0 perfmon event select MSR.
 *
 */
#define MSR_C0_PMON_EVNT_SEL5 0xd1a

/**
 * @brief Uncore C-box 0 perfmon counter MSR.
 *
 */
#define MSR_C0_PMON_CTR5 0xd1b

/**
 * @brief Uncore C-box 4 perfmon local box control MSR.
 *
 */
#define MSR_C4_PMON_BOX_CTRL 0xd20

/**
 * @brief Uncore C-box 4 perfmon local box status MSR.
 *
 */
#define MSR_C4_PMON_BOX_STATUS 0xd21

/**
 * @brief Uncore C-box 4 perfmon local box overflow control MSR.
 *
 */
#define MSR_C4_PMON_BOX_OVF_CTRL 0xd22

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL0 0xd30

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR0 0xd31

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL1 0xd32

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR1 0xd33

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL2 0xd34

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR2 0xd35

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL3 0xd36

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR3 0xd37

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL4 0xd38

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR4 0xd39

/**
 * @brief Uncore C-box 4 perfmon event select MSR.
 *
 */
#define MSR_C4_PMON_EVNT_SEL5 0xd3a

/**
 * @brief Uncore C-box 4 perfmon counter MSR.
 *
 */
#define MSR_C4_PMON_CTR5 0xd3b

/**
 * @brief Uncore C-box 2 perfmon local box control MSR.
 *
 */
#define MSR_C2_PMON_BOX_CTRL 0xd40

/**
 * @brief Uncore C-box 2 perfmon local box status MSR.
 *
 */
#define MSR_C2_PMON_BOX_STATUS 0xd41

/**
 * @brief Uncore C-box 2 perfmon local box overflow control MSR.
 *
 */
#define MSR_C2_PMON_BOX_OVF_CTRL 0xd42

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL0 0xd50

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR0 0xd51

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL1 0xd52

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR1 0xd53

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL2 0xd54

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR2 0xd55

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL3 0xd56

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR3 0xd57

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL4 0xd58

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR4 0xd59

/**
 * @brief Uncore C-box 2 perfmon event select MSR.
 *
 */
#define MSR_C2_PMON_EVNT_SEL5 0xd5a

/**
 * @brief Uncore C-box 2 perfmon counter MSR.
 *
 */
#define MSR_C2_PMON_CTR5 0xd5b

/**
 * @brief Uncore C-box 6 perfmon local box control MSR.
 *
 */
#define MSR_C6_PMON_BOX_CTRL 0xd60

/**
 * @brief Uncore C-box 6 perfmon local box status MSR.
 *
 */
#define MSR_C6_PMON_BOX_STATUS 0xd61

/**
 * @brief Uncore C-box 6 perfmon local box overflow control MSR.
 *
 */
#define MSR_C6_PMON_BOX_OVF_CTRL 0xd62

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL0 0xd70

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR0 0xd71

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL1 0xd72

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR1 0xd73

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL2 0xd74

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR2 0xd75

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL3 0xd76

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR3 0xd77

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL4 0xd78

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR4 0xd79

/**
 * @brief Uncore C-box 6 perfmon event select MSR.
 *
 */
#define MSR_C6_PMON_EVNT_SEL5 0xd7a

/**
 * @brief Uncore C-box 6 perfmon counter MSR.
 *
 */
#define MSR_C6_PMON_CTR5 0xd7b

/**
 * @brief Uncore C-box 1 perfmon local box control MSR.
 *
 */
#define MSR_C1_PMON_BOX_CTRL 0xd80

/**
 * @brief Uncore C-box 1 perfmon local box status MSR.
 *
 */
#define MSR_C1_PMON_BOX_STATUS 0xd81

/**
 * @brief Uncore C-box 1 perfmon local box overflow control MSR.
 *
 */
#define MSR_C1_PMON_BOX_OVF_CTRL 0xd82

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL0 0xd90

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR0 0xd91

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL1 0xd92

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR1 0xd93

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL2 0xd94

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR2 0xd95

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL3 0xd96

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR3 0xd97

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL4 0xd98

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR4 0xd99

/**
 * @brief Uncore C-box 1 perfmon event select MSR.
 *
 */
#define MSR_C1_PMON_EVNT_SEL5 0xd9a

/**
 * @brief Uncore C-box 1 perfmon counter MSR.
 *
 */
#define MSR_C1_PMON_CTR5 0xd9b

/**
 * @brief Uncore C-box 5 perfmon local box control MSR.
 *
 */
#define MSR_C5_PMON_BOX_CTRL 0xda0

/**
 * @brief Uncore C-box 5 perfmon local box status MSR.
 *
 */
#define MSR_C5_PMON_BOX_STATUS 0xda1

/**
 * @brief Uncore C-box 5 perfmon local box overflow control MSR.
 *
 */
#define MSR_C5_PMON_BOX_OVF_CTRL 0xda2

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL0 0xdb0

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR0 0xdb1

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL1 0xdb2

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR1 0xdb3

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL2 0xdb4

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR2 0xdb5

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL3 0xdb6

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR3 0xdb7

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL4 0xdb8

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR4 0xdb9

/**
 * @brief Uncore C-box 5 perfmon event select MSR.
 *
 */
#define MSR_C5_PMON_EVNT_SEL5 0xdba

/**
 * @brief Uncore C-box 5 perfmon counter MSR.
 *
 */
#define MSR_C5_PMON_CTR5 0xdbb

/**
 * @brief Uncore C-box 3 perfmon local box control MSR.
 *
 */
#define MSR_C3_PMON_BOX_CTRL 0xdc0

/**
 * @brief Uncore C-box 3 perfmon local box status MSR.
 *
 */
#define MSR_C3_PMON_BOX_STATUS 0xdc1

/**
 * @brief Uncore C-box 3 perfmon local box overflow control MSR.
 *
 */
#define MSR_C3_PMON_BOX_OVF_CTRL 0xdc2

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL0 0xdd0

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR0 0xdd1

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL1 0xdd2

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR1 0xdd3

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL2 0xdd4

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR2 0xdd5

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL3 0xdd6

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR3 0xdd7

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL4 0xdd8

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR4 0xdd9

/**
 * @brief Uncore C-box 3 perfmon event select MSR.
 *
 */
#define MSR_C3_PMON_EVNT_SEL5 0xdda

/**
 * @brief Uncore C-box 3 perfmon counter MSR.
 *
 */
#define MSR_C3_PMON_CTR5 0xddb

/**
 * @brief Uncore C-box 7 perfmon local box control MSR.
 *
 */
#define MSR_C7_PMON_BOX_CTRL 0xde0

/**
 * @brief Uncore C-box 7 perfmon local box status MSR.
 *
 */
#define MSR_C7_PMON_BOX_STATUS 0xde1

/**
 * @brief Uncore C-box 7 perfmon local box overflow control MSR.
 *
 */
#define MSR_C7_PMON_BOX_OVF_CTRL 0xde2

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL0 0xdf0

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR0 0xdf1

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL1 0xdf2

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR1 0xdf3

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL2 0xdf4

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR2 0xdf5

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL3 0xdf6

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR3 0xdf7

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL4 0xdf8

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR4 0xdf9

/**
 * @brief Uncore C-box 7 perfmon event select MSR.
 *
 */
#define MSR_C7_PMON_EVNT_SEL5 0xdfa

/**
 * @brief Uncore C-box 7 perfmon counter MSR.
 *
 */
#define MSR_C7_PMON_CTR5 0xdfb

/**
 * @brief Uncore R-box 0 perfmon local box control MSR.
 *
 */
#define MSR_R0_PMON_BOX_CTRL 0xe00

/**
 * @brief Uncore R-box 0 perfmon local box status MSR.
 *
 */
#define MSR_R0_PMON_BOX_STATUS 0xe01

/**
 * @brief Uncore R-box 0 perfmon local box overflow control MSR.
 *
 */
#define MSR_R0_PMON_BOX_OVF_CTRL 0xe02

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 0 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P0 0xe04

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 1 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P1 0xe05

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 2 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P2 0xe06

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 3 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P3 0xe07

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 4 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P4 0xe08

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 5 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P5 0xe09

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 6 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P6 0xe0a

/**
 * @brief Uncore R-box 0 perfmon IPERF0 unit Port 7 select MSR.
 *
 */
#define MSR_R0_PMON_IPERF0_P7 0xe0b

/**
 * @brief Uncore R-box 0 perfmon QLX unit Port 0 select MSR.
 *
 */
#define MSR_R0_PMON_QLX_P0 0xe0c

/**
 * @brief Uncore R-box 0 perfmon QLX unit Port 1 select MSR.
 *
 */
#define MSR_R0_PMON_QLX_P1 0xe0d

/**
 * @brief Uncore R-box 0 perfmon QLX unit Port 2 select MSR.
 *
 */
#define MSR_R0_PMON_QLX_P2 0xe0e

/**
 * @brief Uncore R-box 0 perfmon QLX unit Port 3 select MSR.
 *
 */
#define MSR_R0_PMON_QLX_P3 0xe0f

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL0 0xe10

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR0 0xe11

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL1 0xe12

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR1 0xe13

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL2 0xe14

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR2 0xe15

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL3 0xe16

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR3 0xe17

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL4 0xe18

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR4 0xe19

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL5 0xe1a

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR5 0xe1b

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL6 0xe1c

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR6 0xe1d

/**
 * @brief Uncore R-box 0 perfmon event select MSR.
 *
 */
#define MSR_R0_PMON_EVNT_SEL7 0xe1e

/**
 * @brief Uncore R-box 0 perfmon counter MSR.
 *
 */
#define MSR_R0_PMON_CTR7 0xe1f

/**
 * @brief Uncore R-box 1 perfmon local box control MSR.
 *
 */
#define MSR_R1_PMON_BOX_CTRL 0xe20

/**
 * @brief Uncore R-box 1 perfmon local box status MSR.
 *
 */
#define MSR_R1_PMON_BOX_STATUS 0xe21

/**
 * @brief Uncore R-box 1 perfmon local box overflow control MSR.
 *
 */
#define MSR_R1_PMON_BOX_OVF_CTRL 0xe22

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 8 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P8 0xe24

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 9 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P9 0xe25

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 10 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P10 0xe26

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 11 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P11 0xe27

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 12 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P12 0xe28

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 13 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P13 0xe29

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 14 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P14 0xe2a

/**
 * @brief Uncore R-box 1 perfmon IPERF1 unit Port 15 select MSR.
 *
 */
#define MSR_R1_PMON_IPERF1_P15 0xe2b

/**
 * @brief Uncore R-box 1 perfmon QLX unit Port 4 select MSR.
 *
 */
#define MSR_R1_PMON_QLX_P4 0xe2c

/**
 * @brief Uncore R-box 1 perfmon QLX unit Port 5 select MSR.
 *
 */
#define MSR_R1_PMON_QLX_P5 0xe2d

/**
 * @brief Uncore R-box 1 perfmon QLX unit Port 6 select MSR.
 *
 */
#define MSR_R1_PMON_QLX_P6 0xe2e

/**
 * @brief Uncore R-box 1 perfmon QLX unit Port 7 select MSR.
 *
 */
#define MSR_R1_PMON_QLX_P7 0xe2f

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL8 0xe30

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR8 0xe31

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL9 0xe32

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR9 0xe33

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL10 0xe34

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR10 0xe35

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL11 0xe36

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR11 0xe37

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL12 0xe38

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR12 0xe39

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL13 0xe3a

/**
 * @brief Uncore R-box 1perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR13 0xe3b

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL14 0xe3c

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR14 0xe3d

/**
 * @brief Uncore R-box 1 perfmon event select MSR.
 *
 */
#define MSR_R1_PMON_EVNT_SEL15 0xe3e

/**
 * @brief Uncore R-box 1 perfmon counter MSR.
 *
 */
#define MSR_R1_PMON_CTR15 0xe3f

/**
 * @brief Uncore B-box 0 perfmon local box match MSR.
 *
 */
#define MSR_B0_PMON_MATCH 0xe45

/**
 * @brief Uncore B-box 0 perfmon local box mask MSR.
 *
 */
#define MSR_B0_PMON_MASK 0xe46

/**
 * @brief Uncore S-box 0 perfmon local box match MSR.
 *
 */
#define MSR_S0_PMON_MATCH 0xe49

/**
 * @brief Uncore S-box 0 perfmon local box mask MSR.
 *
 */
#define MSR_S0_PMON_MASK 0xe4a

/**
 * @brief Uncore B-box 1 perfmon local box match MSR.
 *
 */
#define MSR_B1_PMON_MATCH 0xe4d

/**
 * @brief Uncore B-box 1 perfmon local box mask MSR.
 *
 */
#define MSR_B1_PMON_MASK 0xe4e

/**
 * @brief Uncore M-box 0 perfmon local box address match/mask config MSR.
 *
 */
#define MSR_M0_PMON_MM_CONFIG 0xe54

/**
 * @brief Uncore M-box 0 perfmon local box address match MSR.
 *
 */
#define MSR_M0_PMON_ADDR_MATCH 0xe55

/**
 * @brief Uncore M-box 0 perfmon local box address mask MSR.
 *
 */
#define MSR_M0_PMON_ADDR_MASK 0xe56

/**
 * @brief Uncore S-box 1 perfmon local box match MSR.
 *
 */
#define MSR_S1_PMON_MATCH 0xe59

/**
 * @brief Uncore S-box 1 perfmon local box mask MSR.
 *
 */
#define MSR_S1_PMON_MASK 0xe5a

/**
 * @brief Uncore M-box 1 perfmon local box address match/mask config MSR.
 *
 */
#define MSR_M1_PMON_MM_CONFIG 0xe5c

/**
 * @brief Uncore M-box 1 perfmon local box address match MSR.
 *
 */
#define MSR_M1_PMON_ADDR_MATCH 0xe5d

/**
 * @brief Uncore M-box 1 perfmon local box address mask MSR.
 *
 */
#define MSR_M1_PMON_ADDR_MASK 0xe5e

/**
 * @brief Uncore C-box 8 perfmon local box control MSR.
 *
 */
#define MSR_C8_PMON_BOX_CTRL 0xf40

/**
 * @brief Uncore C-box 8 perfmon local box status MSR.
 *
 */
#define MSR_C8_PMON_BOX_STATUS 0xf41

/**
 * @brief Uncore C-box 8 perfmon local box overflow control MSR.
 *
 */
#define MSR_C8_PMON_BOX_OVF_CTRL 0xf42

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL0 0xf50

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR0 0xf51

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL1 0xf52

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR1 0xf53

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL2 0xf54

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR2 0xf55

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL3 0xf56

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR3 0xf57

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL4 0xf58

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR4 0xf59

/**
 * @brief Uncore C-box 8 perfmon event select MSR.
 *
 */
#define MSR_C8_PMON_EVNT_SEL5 0xf5a

/**
 * @brief Uncore C-box 8 perfmon counter MSR.
 *
 */
#define MSR_C8_PMON_CTR5 0xf5b

/**
 * @brief Uncore C-box 9 perfmon local box control MSR.
 *
 */
#define MSR_C9_PMON_BOX_CTRL 0xfc0

/**
 * @brief Uncore C-box 9 perfmon local box status MSR.
 *
 */
#define MSR_C9_PMON_BOX_STATUS 0xfc1

/**
 * @brief Uncore C-box 9 perfmon local box overflow control MSR.
 *
 */
#define MSR_C9_PMON_BOX_OVF_CTRL 0xfc2

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL0 0xfd0

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR0 0xfd1

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL1 0xfd2

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR1 0xfd3

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL2 0xfd4

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR2 0xfd5

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL3 0xfd6

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR3 0xfd7

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL4 0xfd8

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR4 0xfd9

/**
 * @brief Uncore C-box 9 perfmon event select MSR.
 *
 */
#define MSR_C9_PMON_EVNT_SEL5 0xfda

/**
 * @brief Uncore C-box 9 perfmon counter MSR.
 *
 */
#define MSR_C9_PMON_CTR5 0xfdb

/**
 * @brief GBUSQ Event Control and Counter  Register (R/W) See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor MP with Up to 8-MByte L3 Cache.
 *
 */
#define MSR_EMON_L3_CTR_CTL0 0x107cc

/**
 * @brief IFSB BUSQ Event Control and Counter  Register (R/W) See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor  MP with Up to 8-MByte L3 Cache.
 *
 */
#define MSR_IFSB_BUSQ0 0x107cc

/**
 * @brief GBUSQ Event Control/Counter Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_CTR_CTL1 0x107cd

/**
 * @brief IFSB BUSQ Event Control and Counter Register (R/W)
 *
 */
#define MSR_IFSB_BUSQ1 0x107cd

/**
 * @brief GSNPQ Event Control and Counter  Register (R/W)  See Section 18.17, Performance 
 * Monitoring on 64-bit Intel Xeon Processor MP with Up to 8-MByte L3 Cache.
 *
 */
#define MSR_EMON_L3_CTR_CTL2 0x107ce

/**
 * @brief IFSB SNPQ Event Control and Counter  Register (R/W)  See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor  MP with Up to 8-MByte L3 Cache.
 *
 */
#define MSR_IFSB_SNPQ0 0x107ce

/**
 * @brief GSNPQ Event Control/Counter Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_CTR_CTL3 0x107cf

/**
 * @brief IFSB SNPQ Event Control and Counter  Register (R/W)
 *
 */
#define MSR_IFSB_SNPQ1 0x107cf

/**
 * @brief EFSB DRDY Event Control and Counter Register (R/W)  See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor MP with Up to 8-MByte L3 Cache  for  details.
 *
 */
#define MSR_EFSB_DRDY0 0x107d0

/**
 * @brief FSB Event Control and Counter Register (R/W)  See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor MP with Up to 8-MByte L3 Cache  for  details.
 *
 */
#define MSR_EMON_L3_CTR_CTL4 0x107d0

/**
 * @brief EFSB DRDY Event Control and Counter  Register (R/W)
 *
 */
#define MSR_EFSB_DRDY1 0x107d1

/**
 * @brief FSB Event Control/Counter Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_CTR_CTL5 0x107d1

/**
 * @brief FSB Event Control/Counter Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_CTR_CTL6 0x107d2

/**
 * @brief IFSB Latency Event Control Register  (R/W) See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor MP with Up to 8-MByte L3 Cache  for  details.
 *
 */
#define MSR_IFSB_CTL6 0x107d2

/**
 * @brief FSB Event Control/Counter Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_CTR_CTL7 0x107d3

/**
 * @brief IFSB Latency Event Counter Register  (R/W)  See Section 18.17, Performance  
 * Monitoring on 64-bit Intel Xeon Processor  MP with Up to 8-MByte L3 Cache.
 *
 */
#define MSR_IFSB_CNTR7 0x107d3

/**
 * @brief L3/FSB Common Control Register (R/W) Apply to Intel Xeon processor 7400 series 
 * (processor signature  06_1D) only. See Section 17.2.2
 *
 */
#define MSR_EMON_L3_GL_CTL 0x107d8

/**
 * @brief If (  CPUID.80000001.EDX.[bit  20] or  CPUID.80000001.EDX.[bit 29])
 *
 */
#define IA32_EFER 0xc0000080

/**
 * @brief System Call Target Address (R/W)  See Table 35-2.
 *
 */
#define IA32_STAR 0xc0000081

/**
 * @brief IA-32e Mode System Call Target Address (R/W)  See Table 35-2.
 *
 */
#define IA32_LSTAR 0xc0000082

/**
 * @brief System Call Target Address the compatibility mode.
 *
 */
#define IA32_CSTAR 0xc0000083

/**
 * @brief System Call Flag Mask (R/W)  See Table 35-2.
 *
 */
#define IA32_FMASK 0xc0000084

/**
 * @brief Map of BASE Address of FS (R/W)  See Table 35-2.
 *
 */
#define IA32_FS_BASE 0xc0000100

/**
 * @brief Map of BASE Address of GS (R/W)  See Table 35-2.
 *
 */
#define IA32_GS_BASE 0xc0000101

/**
 * @brief Swap Target of BASE Address of GS (R/W) See Table 35-2.
 *
 */
#define IA32_KERNEL_GSBASE 0xc0000102

/**
 * @brief AUXILIARY TSC Signature. (R/W) See Table 35-2 and Section  17.13.2, 
 * IA32_TSC_AUX Register and RDTSCP Support.
 *
 */
#define IA32_TSC_AUX 0xc0000103
