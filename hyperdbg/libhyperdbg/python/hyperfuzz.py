#!/usr/bin/env python3
"""
HyperDbg Hypervisor-Assisted Snapshot Fuzzing Python SDK & Harness Bridge

Provides a Python ctypes interface and LibAFL/AFL++ compatible bridge to HyperDbg's
VMX root mode snapshot rollback and Intel PT / EPT coverage engine.
"""

import ctypes
import os
import sys
import time
import random
from ctypes import wintypes

#
# Constants & Enums (mirrored from SnapshotFuzzing.h)
#
SNAPSHOT_AFL_MAP_SIZE    = 65536
SNAPSHOT_MAX_DIRTY_PAGES = 16384
SNAPSHOT_MAX_LBR_DEPTH   = 32
SNAPSHOT_MAX_INPUT_SIZE  = 1024 * 1024

FUZZ_STATUS_SUCCESS         = 0x00000000
FUZZ_STATUS_CRASH_EXCEPTION = 0x00000001
FUZZ_STATUS_TIMEOUT_EXCEEDED= 0x00000002
FUZZ_STATUS_MAX_BRANCHES_HIT= 0x00000003
FUZZ_STATUS_INVALID_STATE   = 0x00000004
FUZZ_STATUS_RESTORE_FAILED  = 0x00000005

SNAPSHOT_MODE_KERNEL_SUPERVISOR = 0x00000001
SNAPSHOT_MODE_USER_APPLICATION  = 0x00000002
SNAPSHOT_MODE_HYBRID_TRANSITION = 0x00000003

#
# IOCTL Codes
#
def CTL_CODE(device_type, function, method, access):
    return (device_type << 16) | (access << 14) | (function << 2) | method

FILE_DEVICE_UNKNOWN = 0x00000022
METHOD_BUFFERED     = 0
FILE_ANY_ACCESS     = 0
IOCTL_FUZZER_BASE   = 0x800 + 0x400

IOCTL_SNAPSHOT_TAKE         = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_SNAPSHOT_RESTORE      = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_SNAPSHOT_CLEAR        = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_ITERATE          = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_MAP_COVERAGE     = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_GET_CRASH_REPORT   = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x06, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_RUN_BATCH           = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x07, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_CLEAR_CRASH_REPORT = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_GET_PT_STREAM       = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x09, METHOD_BUFFERED, FILE_ANY_ACCESS)

EVASION_MODE_NONE                     = 0x00000000
EVASION_MODE_FIXED_DELTA              = 0x00000001
EVASION_MODE_INSTRUCTION_DILATION     = 0x00000002
EVASION_MODE_MSR_LATENCY_COMPENSATION = 0x00000004
EVASION_MODE_EPT_TIMING_SMOOTHING     = 0x00000008
EVASION_MODE_KTRAP_FRAME_CLEANSE      = 0x00000010

#
# ctypes Structures
#
class LBR_ENTRY(ctypes.Structure):
    _fields_ = [
        ("From", ctypes.c_ulonglong),
        ("To",   ctypes.c_ulonglong),
    ]

class SNAPSHOT_SEGMENT_DESCRIPTOR(ctypes.Structure):
    _fields_ = [
        ("Selector",   ctypes.c_ushort),
        ("Attributes", ctypes.c_ushort),
        ("Limit",      ctypes.c_uint),
        ("Base",       ctypes.c_ulonglong),
    ]

class SNAPSHOT_DESCRIPTOR_TABLE(ctypes.Structure):
    _fields_ = [
        ("Limit", ctypes.c_ushort),
        ("Base",  ctypes.c_ulonglong),
    ]

class SNAPSHOT_VCPU_CONTEXT(ctypes.Structure):
    _fields_ = [
        ("Rax", ctypes.c_ulonglong), ("Rcx", ctypes.c_ulonglong),
        ("Rdx", ctypes.c_ulonglong), ("Rbx", ctypes.c_ulonglong),
        ("Rsp", ctypes.c_ulonglong), ("Rbp", ctypes.c_ulonglong),
        ("Rsi", ctypes.c_ulonglong), ("Rdi", ctypes.c_ulonglong),
        ("R8",  ctypes.c_ulonglong), ("R9",  ctypes.c_ulonglong),
        ("R10", ctypes.c_ulonglong), ("R11", ctypes.c_ulonglong),
        ("R12", ctypes.c_ulonglong), ("R13", ctypes.c_ulonglong),
        ("R14", ctypes.c_ulonglong), ("R15", ctypes.c_ulonglong),
        ("Rip", ctypes.c_ulonglong), ("Rflags", ctypes.c_ulonglong),
        ("Cr0", ctypes.c_ulonglong), ("Cr2", ctypes.c_ulonglong),
        ("Cr3", ctypes.c_ulonglong), ("Cr4", ctypes.c_ulonglong),
        ("Cr8", ctypes.c_ulonglong),
        ("Dr0", ctypes.c_ulonglong), ("Dr1", ctypes.c_ulonglong),
        ("Dr2", ctypes.c_ulonglong), ("Dr3", ctypes.c_ulonglong),
        ("Dr6", ctypes.c_ulonglong), ("Dr7", ctypes.c_ulonglong),
        ("Cs", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Ss", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Ds", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Es", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Fs", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Gs", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Tr", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Ldtr", SNAPSHOT_SEGMENT_DESCRIPTOR),
        ("Gdtr", SNAPSHOT_DESCRIPTOR_TABLE),
        ("Idtr", SNAPSHOT_DESCRIPTOR_TABLE),
        ("FsBase", ctypes.c_ulonglong),
        ("GsBase", ctypes.c_ulonglong),
        ("KernelGsBase", ctypes.c_ulonglong),
        ("Efer", ctypes.c_ulonglong),
        ("SysenterCs", ctypes.c_ulonglong),
        ("SysenterEsp", ctypes.c_ulonglong),
        ("SysenterEip", ctypes.c_ulonglong),
        ("Star", ctypes.c_ulonglong),
        ("Lstar", ctypes.c_ulonglong),
        ("Cstar", ctypes.c_ulonglong),
        ("Sfmask", ctypes.c_ulonglong),
        ("XsaveArea", ctypes.c_ubyte * 4096),
    ]

class FUZZ_AFL_COVERAGE_MAP(ctypes.Structure):
    _fields_ = [
        ("TraceBits", ctypes.c_ubyte * SNAPSHOT_AFL_MAP_SIZE),
        ("PreviousIp", ctypes.c_ulonglong),
        ("TotalEdgesCovered", ctypes.c_ulonglong),
        ("TotalExecutions", ctypes.c_ulonglong),
    ]

class FUZZ_CRASH_REPORT(ctypes.Structure):
    _fields_ = [
        ("ExceptionVector", ctypes.c_uint),
        ("HardwareErrorCode", ctypes.c_uint),
        ("FaultingAddress", ctypes.c_ulonglong),
        ("FaultingRip", ctypes.c_ulonglong),
        ("RegistersAtCrash", SNAPSHOT_VCPU_CONTEXT),
        ("LbrEntryCount", ctypes.c_uint),
        ("LbrStack", LBR_ENTRY * SNAPSHOT_MAX_LBR_DEPTH),
        ("CrashHash", ctypes.c_ulonglong),
    ]

class DEBUGGER_SNAPSHOT_TAKE_REQUEST(ctypes.Structure):
    _fields_ = [
        ("TargetProcessId", ctypes.c_uint),
        ("ExecutionMode",   ctypes.c_uint),
        ("SnapshotRip",     ctypes.c_ulonglong),
        ("KernelStatus",    ctypes.c_uint),
    ]

class DEBUGGER_SNAPSHOT_RESTORE_REQUEST(ctypes.Structure):
    _fields_ = [
        ("RestoredPageCount", ctypes.c_uint),
        ("RestoredRip",       ctypes.c_ulonglong),
        ("KernelStatus",      ctypes.c_uint),
    ]

class DEBUGGER_FUZZ_ITERATE_REQUEST(ctypes.Structure):
    _fields_ = [
        ("TargetVirtualAddress",  ctypes.c_ulonglong),
        ("ExitBreakpointAddress", ctypes.c_ulonglong),
        ("InputBuffer",           ctypes.c_ubyte * SNAPSHOT_MAX_INPUT_SIZE),
        ("InputSize",             ctypes.c_uint),
        ("MaxInstructionCount",   ctypes.c_ulonglong),
        ("ExecutionStatus",       ctypes.c_uint),
        ("ElapsedCycles",         ctypes.c_ulonglong),
    ]

class DEBUGGER_FUZZ_RUN_BATCH_REQUEST(ctypes.Structure):
    _fields_ = [
        ("IterationCount",        ctypes.c_uint),
        ("TargetProcessId",       ctypes.c_uint),
        ("TargetVirtualAddress",  ctypes.c_ulonglong),
        ("InputSize",             ctypes.c_uint),
        ("InputBuffer",           ctypes.c_ubyte * SNAPSHOT_MAX_INPUT_SIZE),
        ("MutationStrategyFlags", ctypes.c_uint),
        ("ExecutedCount",         ctypes.c_uint),
        ("ExecutionStatus",       ctypes.c_uint),
        ("ElapsedCycles",         ctypes.c_ulonglong),
        ("CrashReport",           FUZZ_CRASH_REPORT),
        ("KernelStatus",          ctypes.c_uint),
    ]

class DEBUGGER_FUZZ_GET_PT_STREAM_REQUEST(ctypes.Structure):
    _fields_ = [
        ("BufferSize",       ctypes.c_uint),
        ("TransferredBytes", ctypes.c_uint),
        ("PacketBuffer",     ctypes.c_ubyte * SNAPSHOT_MAX_INPUT_SIZE),
        ("KernelStatus",     ctypes.c_uint),
    ]



class HyperDbgSnapshotFuzzer:
    """
    High-performance snapshot fuzzer harness controller for HyperDbg.
    """
    def __init__(self, device_path=r"\\.\HyperDbgDevice"):
        self.device_path = device_path
        self.handle = None
        self._open_device()

    def _open_device(self):
        GENERIC_READ_WRITE = 0xC0000000
        OPEN_EXISTING = 3
        FILE_ATTRIBUTE_NORMAL = 0x80

        kernel32 = ctypes.windll.kernel32
        self.handle = kernel32.CreateFileW(
            self.device_path,
            GENERIC_READ_WRITE,
            0,
            None,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            None
        )
        if self.handle == -1 or self.handle == 0:
            err = kernel32.GetLastError()
            raise RuntimeError(f"Could not connect to HyperDbg device {self.device_path} (Win32 Error: {err})")

    def _send_ioctl(self, ioctl_code, in_struct, out_struct):
        kernel32 = ctypes.windll.kernel32
        returned_bytes = wintypes.DWORD(0)

        in_buf = ctypes.byref(in_struct) if in_struct else None
        in_size = ctypes.sizeof(in_struct) if in_struct else 0
        out_buf = ctypes.byref(out_struct) if out_struct else None
        out_size = ctypes.sizeof(out_struct) if out_struct else 0

        success = kernel32.DeviceIoControl(
            self.handle,
            ioctl_code,
            in_buf,
            in_size,
            out_buf,
            out_size,
            ctypes.byref(returned_bytes),
            None
        )
        if not success:
            err = kernel32.GetLastError()
            raise RuntimeError(f"DeviceIoControl (0x{ioctl_code:x}) failed with error: {err}")
        return True

    def take_snapshot(self, pid=0, mode=SNAPSHOT_MODE_USER_APPLICATION):
        """
        Freezes execution and captures pristine hardware vCPU state and memory snapshot.
        """
        req = DEBUGGER_SNAPSHOT_TAKE_REQUEST()
        req.TargetProcessId = pid
        req.ExecutionMode = mode
        req.SnapshotRip = 0
        self._send_ioctl(IOCTL_SNAPSHOT_TAKE, req, req)
        return req.KernelStatus == 0

    def restore_snapshot(self):
        """
        Rolls back registers and dirty EPT pages to the pristine baseline.
        """
        req = DEBUGGER_SNAPSHOT_RESTORE_REQUEST()
        self._send_ioctl(IOCTL_SNAPSHOT_RESTORE, req, req)
        return req.RestoredPageCount

    def clear_snapshot(self):
        """
        Deallocates shadow frames and disarms dirty logging.
        """
        status = ctypes.c_uint(0)
        self._send_ioctl(IOCTL_SNAPSHOT_CLEAR, status, status)
        return status.value == 0

    def run_iteration(self, target_va, exit_va, input_data: bytes, max_instructions=1_000_000):
        """
        Injects input_data into target_va, resumes execution until exit_va, and rolls back state.
        Returns: (status_code, elapsed_cycles)
        """
        req = DEBUGGER_FUZZ_ITERATE_REQUEST()
        req.TargetVirtualAddress = target_va
        req.ExitBreakpointAddress = exit_va
        req.MaxInstructionCount = max_instructions
        req.InputSize = min(len(input_data), SNAPSHOT_MAX_INPUT_SIZE)
        ctypes.memmove(req.InputBuffer, input_data, req.InputSize)

        self._send_ioctl(IOCTL_FUZZ_ITERATE, req, req)
        return req.ExecutionStatus, req.ElapsedCycles

    def run_batch(self, iteration_count: int, target_va: int = 0, input_data: bytes = b""):
        """
        Executes an autonomous batch of snapshot fuzzing iterations in-kernel.
        Returns: (executed_count, execution_status, elapsed_cycles, crash_report_dict or None)
        """
        req = DEBUGGER_FUZZ_RUN_BATCH_REQUEST()
        req.IterationCount = iteration_count
        req.TargetVirtualAddress = target_va
        req.InputSize = min(len(input_data), SNAPSHOT_MAX_INPUT_SIZE)
        if req.InputSize > 0:
            ctypes.memmove(req.InputBuffer, input_data, req.InputSize)

        self._send_ioctl(IOCTL_FUZZ_RUN_BATCH, req, req)

        crash_info = None
        if req.ExecutionStatus == FUZZ_STATUS_CRASH_EXCEPTION:
            crash_info = {
                "exception_vector": req.CrashReport.ExceptionVector,
                "faulting_rip": hex(req.CrashReport.FaultingRip),
                "faulting_address": hex(req.CrashReport.FaultingAddress),
                "crash_hash": hex(req.CrashReport.CrashHash),
            }

        return req.ExecutedCount, req.ExecutionStatus, req.ElapsedCycles, crash_info


    def get_coverage_bitmap(self):
        """
        Returns the 64KB AFL-compatible branch coverage bitmap.
        """
        cov_map = FUZZ_AFL_COVERAGE_MAP()
        self._send_ioctl(IOCTL_FUZZ_MAP_COVERAGE, cov_map, cov_map)
        return bytearray(cov_map.TraceBits), cov_map.TotalEdgesCovered, cov_map.TotalExecutions

    def get_crash_report(self):
        """
        Retrieves the last intercepted hardware crash report including LBR callstack.
        """
        report = FUZZ_CRASH_REPORT()
        self._send_ioctl(IOCTL_FUZZ_GET_CRASH_REPORT, report, report)

        lbr_entries = []
        for i in range(min(report.LbrEntryCount, SNAPSHOT_MAX_LBR_DEPTH)):
            lbr_entries.append((hex(report.LbrStack[i].From), hex(report.LbrStack[i].To)))

        return {
            "exception_vector": report.ExceptionVector,
            "hardware_error_code": report.HardwareErrorCode,
            "faulting_rip": hex(report.FaultingRip),
            "faulting_address": hex(report.FaultingAddress),
            "crash_hash": hex(report.CrashHash),
            "lbr_trace": lbr_entries,
            "registers": {
                "rax": hex(report.RegistersAtCrash.Rax),
                "rbx": hex(report.RegistersAtCrash.Rbx),
                "rcx": hex(report.RegistersAtCrash.Rcx),
                "rdx": hex(report.RegistersAtCrash.Rdx),
                "rsp": hex(report.RegistersAtCrash.Rsp),
                "rbp": hex(report.RegistersAtCrash.Rbp),
                "rflags": hex(report.RegistersAtCrash.Rflags),
                "cr2": hex(report.RegistersAtCrash.Cr2),
                "cr3": hex(report.RegistersAtCrash.Cr3),
            }
        }

    def clear_crash_report(self):
        """
        Resets the last intercepted hardware crash report in the kernel.
        """
        dummy = ctypes.c_ulong()
        self._send_ioctl(IOCTL_FUZZ_CLEAR_CRASH_REPORT, dummy, dummy)

    def get_pt_stream(self) -> bytes:
        """
        Retrieves the raw binary Intel PT ToPA packet buffer from the hypervisor.
        """
        req = DEBUGGER_FUZZ_GET_PT_STREAM_REQUEST()
        req.BufferSize = SNAPSHOT_MAX_INPUT_SIZE
        self._send_ioctl(IOCTL_FUZZ_GET_PT_STREAM, req, req)
        if req.TransferredBytes > 0:
            return bytes(req.PacketBuffer[:req.TransferredBytes])
        return b""

    def run_batch(self, iterations: int, target_va: int, seed: bytes = b"") -> tuple:
        """
        Runs an autonomous batch of fuzz iterations directly in kernel mode.
        """
        req = DEBUGGER_FUZZ_RUN_BATCH_REQUEST()
        req.IterationCount = iterations
        req.TargetVirtualAddress = target_va
        req.InputSize = min(len(seed), SNAPSHOT_MAX_INPUT_SIZE)
        if seed:
            for i, b in enumerate(seed[:req.InputSize]):
                req.InputBuffer[i] = b

        self._send_ioctl(IOCTL_FUZZ_RUN_BATCH, req, req)
        return req.ExecutedCount, req.ExecutionStatus, req.ElapsedCycles

    def close(self):
        if self.handle and self.handle != -1:
            ctypes.windll.kernel32.CloseHandle(self.handle)
            self.handle = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


def default_mutate(data: bytearray) -> bytearray:
    """Radamsa-style fast bit/byte mutator."""
    if not data:
        return bytearray(b"FUZZ")
    mutated = bytearray(data)
    op = random.randint(0, 3)
    idx = random.randint(0, len(mutated) - 1)
    if op == 0:  # Bit flip
        mutated[idx] ^= (1 << random.randint(0, 7))
    elif op == 1:  # Byte replace
        mutated[idx] = random.randint(0, 255)
    elif op == 2:  # Arithmetic
        mutated[idx] = (mutated[idx] + random.choice([-1, 1])) & 0xFF
    elif op == 3:  # Interesting value
        mutated[idx] = random.choice([0x00, 0x01, 0x7F, 0x80, 0xFF, 0xFE])
    return mutated


def main():
    import argparse
    parser = argparse.ArgumentParser(description="HyperDbg Hypervisor Snapshot Fuzzing Bridge")
    parser.add_argument("--target", type=lambda x: int(x, 0), help="Target function entry virtual address")
    parser.add_argument("--exit", type=lambda x: int(x, 0), help="Target exit breakpoint virtual address")
    parser.add_argument("--iterations", type=int, default=1000, help="Number of fuzzing iterations")
    parser.add_argument("--pid", type=lambda x: int(x, 0), default=0, help="Target process ID (0 for kernel)")
    args = parser.parse_args()

    print("[*] HyperDbg Hypervisor Snapshot Fuzzer Python Harness Bridge")
    if not args.target or not args.exit:
        print("[!] Specify --target <VA> and --exit <VA> to run a fuzzing batch.")
        sys.exit(0)

    try:
        with HyperDbgSnapshotFuzzer() as fuzzer:
            print(f"[*] Arming baseline snapshot for PID {args.pid}...")
            fuzzer.take_snapshot(pid=args.pid)

            corpus = bytearray(b"INITIAL_SEED_INPUT_FOR_TESTING")
            total_cycles = 0
            start_time = time.time()

            print(f"[*] Starting {args.iterations} fuzzing iterations...")
            for i in range(1, args.iterations + 1):
                mutated_input = default_mutate(corpus)
                status, cycles = fuzzer.run_iteration(args.target, args.exit, bytes(mutated_input))
                total_cycles += cycles

                if status == FUZZ_STATUS_CRASH_EXCEPTION:
                    print(f"\n[!] CRASH DETECTED at iteration {i}!")
                    report = fuzzer.get_crash_report()
                    print(f"    Exception: {report['exception_vector']} at RIP {report['faulting_rip']}")
                    print(f"    Callstack Hash: {report['crash_hash']}")
                    break

                if i % 100 == 0:
                    _, edges, total = fuzzer.get_coverage_bitmap()
                    print(f" [{i}/{args.iterations}] Unique Edges: {edges} | Total Execs: {total}")

            elapsed = time.time() - start_time
            exec_per_sec = args.iterations / elapsed if elapsed > 0 else 0
            print(f"\n[+] Completed {args.iterations} executions in {elapsed:.2f}s ({exec_per_sec:.1f} exec/sec)")
            fuzzer.clear_snapshot()

    except Exception as e:
        print(f"[-] Fuzzer failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
