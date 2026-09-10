/**
 * @file SnapshotFuzzing.h
 * @author HyperDbg Dev Team
 * @brief Header for in-kernel hypervisor snapshot fuzzing and coverage engine.
 * @details Declarations for vCPU snapshotting, hardware EPT Access/Dirty
 *          tracking, Intel PT edge coverage, and automated crash triage.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

IMPORT_EXPORT_VMM BOOLEAN
SnapshotInitialize();

IMPORT_EXPORT_VMM VOID
SnapshotUninitialize();

IMPORT_EXPORT_VMM NTSTATUS
SnapshotTake(PDEBUGGER_SNAPSHOT_TAKE_REQUEST Request);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotRestore(PDEBUGGER_SNAPSHOT_RESTORE_REQUEST Request);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotClear(PUINT32 Status);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotFuzzIterate(PDEBUGGER_FUZZ_ITERATE_REQUEST Request);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotMapCoverage(PFUZZ_AFL_COVERAGE_MAP DestinationBuffer);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotGetCrashReport(PFUZZ_CRASH_REPORT Report);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotClearCrashReport(VOID);

IMPORT_EXPORT_VMM NTSTATUS
SnapshotRunBatch(PDEBUGGER_FUZZ_RUN_BATCH_REQUEST Request);



VOID
SnapshotSaveVcpuContext(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_VCPU_CONTEXT Context);

VOID
SnapshotRestoreVcpuContext(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_VCPU_CONTEXT Context);

BOOLEAN
SnapshotInitializeMemoryTracker(PSNAPSHOT_MEMORY_TRACKER Tracker);

BOOLEAN
SnapshotRecordPristinePage(PSNAPSHOT_MEMORY_TRACKER Tracker, UINT64 PhysicalAddress);

BOOLEAN
SnapshotRestoreDirtyPages(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_MEMORY_TRACKER Tracker);

VOID
SnapshotParsePtCoverage(UINT8 * PtBuffer, SIZE_T PtSize, PFUZZ_AFL_COVERAGE_MAP AflMap);

VOID
SnapshotFreezeTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 FrozenTsc);

BOOLEAN
SnapshotIsActive(VOID);

UINT64
SnapshotGetVirtualizedTsc(VIRTUAL_MACHINE_STATE * VCpu);

VOID
SnapshotCaptureCrash(VIRTUAL_MACHINE_STATE * VCpu, UINT32 ExceptionVector, PFUZZ_CRASH_REPORT Report);

VOID
SnapshotSetEvasionMode(UINT32 Mode);

NTSTATUS
SnapshotRunAutonomousBatch(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IterationCount, PFUZZ_CRASH_REPORT CrashReport);

