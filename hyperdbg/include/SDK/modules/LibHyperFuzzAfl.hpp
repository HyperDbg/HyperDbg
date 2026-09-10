/**
 * @file LibHyperFuzzAfl.hpp
 * @author HyperDbg Dev Team
 * @brief Header-only C++ LibAFL / AFL++ Windows harness executor bridge for HyperDbg.
 * @details Provides zero-overhead in-process and out-of-process snapshot fuzzing
 *          execution, direct IOCTL mapping, and shared 64KB AFL bitmap telemetry.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#include <windows.h>
#include <winioctl.h>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#include "SDK/headers/BasicTypes.h"
#include "SDK/headers/SnapshotFuzzing.h"
#include "SDK/modules/HyperFuzz.h"

namespace hyperdbg {
namespace fuzzing {

/**
 * @brief Execution outcome enum for LibAFL compatibility
 */
enum class ExitKind {
    Ok,
    Crash,
    Timeout,
    Diff,
};

/**
 * @brief High-performance C++ LibAFL Executor Bridge
 */
class HyperDbgAflExecutor {
public:
    HyperDbgAflExecutor(const std::wstring& device_path = L"\\\\.\\HyperDbgDevice")
        : device_handle_(INVALID_HANDLE_VALUE), device_path_(device_path) {
        Connect();
    }

    ~HyperDbgAflExecutor() {
        Disconnect();
    }

    /**
     * @brief Connects to the HyperDbg kernel driver device
     */
    void Connect() {
        if (device_handle_ != INVALID_HANDLE_VALUE) {
            return;
        }

        device_handle_ = CreateFileW(
            device_path_.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (device_handle_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to connect to HyperDbg driver at " +
                                     std::string(device_path_.begin(), device_path_.end()) +
                                     " (Error: " + std::to_string(GetLastError()) + ")");
        }
    }

    /**
     * @brief Disconnects from the kernel driver
     */
    void Disconnect() {
        if (device_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(device_handle_);
            device_handle_ = INVALID_HANDLE_VALUE;
        }
    }

    /**
     * @brief Arms a baseline snapshot at current vCPU state
     */
    bool TakeSnapshot(uint32_t pid = 0, uint32_t mode = SNAPSHOT_MODE_USER_APPLICATION) {
        DEBUGGER_SNAPSHOT_TAKE_REQUEST req{};
        req.TargetProcessId = pid;
        req.ExecutionMode   = mode;

        DWORD returned = 0;
        BOOL status = DeviceIoControl(
            device_handle_,
            IOCTL_SNAPSHOT_TAKE,
            &req, sizeof(req),
            &req, sizeof(req),
            &returned,
            nullptr
        );

        return status && (req.KernelStatus == 0);
    }

    /**
     * @brief Restores execution state and dirty pages to baseline snapshot
     */
    uint32_t RestoreSnapshot() {
        DEBUGGER_SNAPSHOT_RESTORE_REQUEST req{};
        DWORD returned = 0;

        DeviceIoControl(
            device_handle_,
            IOCTL_SNAPSHOT_RESTORE,
            &req, sizeof(req),
            &req, sizeof(req),
            &returned,
            nullptr
        );

        return req.RestoredPageCount;
    }

    /**
     * @brief Clears snapshot frames and disarms dirty logging
     */
    bool ClearSnapshot() {
        uint32_t status_val = 0;
        DWORD returned = 0;

        BOOL status = DeviceIoControl(
            device_handle_,
            IOCTL_SNAPSHOT_CLEAR,
            &status_val, sizeof(status_val),
            &status_val, sizeof(status_val),
            &returned,
            nullptr
        );

        return status && (status_val == 0);
    }

    /**
     * @brief Executes a single mutated testcase iteration (LibAFL Target Execution)
     *
     * @param target_va Virtual address of target function entry
     * @param exit_va Virtual address of return/exit breakpoint
     * @param data Raw testcase payload bytes
     * @param size Length of payload in bytes
     * @param max_instructions Instruction timeout limit
     * @param[out] out_cycles CPU cycle duration of iteration
     * @return ExitKind Outcome for LibAFL observer triage
     */
    ExitKind RunTarget(uint64_t target_va,
                       uint64_t exit_va,
                       const uint8_t* data,
                       size_t size,
                       uint64_t max_instructions,
                       uint64_t* out_cycles = nullptr) {
        DEBUGGER_FUZZ_ITERATE_REQUEST req{};
        req.TargetVirtualAddress  = target_va;
        req.ExitBreakpointAddress = exit_va;
        req.MaxInstructionCount   = max_instructions;
        req.InputSize             = static_cast<uint32_t>(min(size, static_cast<size_t>(SNAPSHOT_MAX_INPUT_SIZE)));

        if (data != nullptr && req.InputSize > 0) {
            memcpy(req.InputBuffer, data, req.InputSize);
        }

        DWORD returned = 0;
        BOOL status = DeviceIoControl(
            device_handle_,
            IOCTL_FUZZ_ITERATE,
            &req, sizeof(req),
            &req, sizeof(req),
            &returned,
            nullptr
        );

        if (!status) {
            return ExitKind::Crash;
        }

        if (out_cycles != nullptr) {
            *out_cycles = req.ElapsedCycles;
        }

        switch (req.ExecutionStatus) {
        case FUZZ_STATUS_SUCCESS:
            return ExitKind::Ok;
        case FUZZ_STATUS_CRASH_EXCEPTION:
            return ExitKind::Crash;
        case FUZZ_STATUS_TIMEOUT_EXCEEDED:
            return ExitKind::Timeout;
        default:
            return ExitKind::Crash;
        }
    }

    /**
     * @brief Fetches the current 64KB AFL-compatible coverage bitmap
     */
    bool GetCoverageMap(FUZZ_AFL_COVERAGE_MAP* out_map) {
        if (!out_map) return false;
        DWORD returned = 0;

        return DeviceIoControl(
            device_handle_,
            IOCTL_FUZZ_MAP_COVERAGE,
            out_map, sizeof(FUZZ_AFL_COVERAGE_MAP),
            out_map, sizeof(FUZZ_AFL_COVERAGE_MAP),
            &returned,
            nullptr
        );
    }

    /**
     * @brief Fetches the last intercepted crash and LBR callstack report
     */
    bool GetCrashReport(FUZZ_CRASH_REPORT* out_report) {
        if (!out_report) return false;
        DWORD returned = 0;

        return DeviceIoControl(
            device_handle_,
            IOCTL_FUZZ_GET_CRASH_REPORT,
            out_report, sizeof(FUZZ_CRASH_REPORT),
            out_report, sizeof(FUZZ_CRASH_REPORT),
            &returned,
            nullptr
        );
    }

private:
    HANDLE        device_handle_;
    std::wstring  device_path_;
};

} // namespace fuzzing
} // namespace hyperdbg
