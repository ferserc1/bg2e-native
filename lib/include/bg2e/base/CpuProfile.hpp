/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/GpuProfile.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace bg2e::base {
// All timestamps are steady-clock nanoseconds, not Unix timestamps.
struct CpuProfileSample {
    enum class Kind { Scope, Counter, Frame };
    Kind kind = Kind::Scope;
    uint64_t epoch = 0, recording = 0, thread = 0;
    uint64_t id = 0, parent = 0, presentationFrame = 0, sceneFrame = 0, presentationNumber = 0;
    uint64_t begin = 0, end = 0, calls = 1;
    double inclusiveMs = 0, exclusiveMs = 0, maximumMs = 0;
    uint64_t value = 0, maximumValue = 0;
    bool complete = true, sceneRendered = false;
    std::string label;
    std::string unit;
};

struct CpuProfileSnapshot {
    std::vector<CpuProfileSample> samples;
    std::vector<GpuProfileSample> gpuSamples;
    std::vector<GpuProfileQueueStatus> gpuQueues;
    uint64_t gpuPending = 0, gpuDropped = 0, gpuFailed = 0, gpuUnsupported = 0, gpuEvicted = 0, gpuLate = 0;
    std::vector<CpuProfileSample> totals; // Per thread/label/unit; no frame normalization.
    uint64_t presentationFrames = 0, sceneFrames = 0, incompleteFrames = 0;
    uint64_t begin = 0, end = 0;
    uint64_t evicted = 0, dropped = 0, writerDropped = 0, partial = 0;
};

// Coherent recording state for controls, including completed-session duration.
struct CpuProfileRecordingStatus {
    bool active = false;
    uint64_t session = 0, begin = 0, end = 0;
    std::filesystem::path path;
    std::string error;
};

// An independent lease for each live consumer. Destruction never stops recording.
class BG2E_API CpuProfileCollection {
  public:
    CpuProfileCollection();
    ~CpuProfileCollection();
    CpuProfileCollection(const CpuProfileCollection&) = delete;
    CpuProfileCollection& operator=(const CpuProfileCollection&) = delete;
};

class BG2E_API CpuProfiler {
  public:
    using ContextProvider = std::string (*)(void*);
    using Collection = CpuProfileCollection;

    // Existing files are rejected unless overwrite was explicitly accepted.
    static bool start(const std::filesystem::path& path, bool overwrite = false);
    static void stop();
    // Replacement/detachment waits for an executing callback. Callbacks must
    // not replace themselves. Owners detach before destroying borrowed data.
    static void setContextProvider(ContextProvider provider, void* context);
    static std::string context();

    static bool enabled(); // Any live lease OR an active recording.
    static bool recording();
    static uint64_t session(); // Collection epoch; independent of recording ID.
    static uint64_t recordingSession();
    // A reader-local cutoff; this never clears shared history or recordings.
    static CpuProfileSnapshot snapshot(uint64_t since = 0);
    static CpuProfileRecordingStatus recordingStatus();
    // An adapter outside gpu owns publication. Stop counts pending request tokens
    // without consulting native queues; late completions retain their identities.
    static bool gpuTimingEnabled() noexcept;
    static bool gpuPassTimingEnabled() noexcept;
    static void setGpuPassTimingEnabled(bool enabled) noexcept;
    static bool gpuDetailedTimingEnabled() noexcept;
    static void setGpuDetailedTimingEnabled(bool enabled) noexcept;
    static void setGpuTimingEnabled(bool enabled) noexcept;
    static GpuProfileSample beginGpu(uint64_t presentation, uint64_t scene, GpuProfileWorkload workload) noexcept;
    static void completeGpu(const GpuProfileSample& sample) noexcept;
    static void gpuQueueStatus(const GpuProfileQueueStatus& status) noexcept;
    static void publish() noexcept; // Flush this thread's non-frame work.
    static void count(const char* name, uint64_t value, const char* unit = "count") noexcept;
    static std::filesystem::path outputPath();
    static std::string sessionInfo();
    static std::string error();
};

// Labels and units have static lifetime. No GPU synchronization is introduced.
class BG2E_API CpuProfileScope {
  public:
    explicit CpuProfileScope(const char* label, bool reportBoundary = false) noexcept;
    ~CpuProfileScope() noexcept;
    CpuProfileScope(const CpuProfileScope&) = delete;
    CpuProfileScope& operator=(const CpuProfileScope&) = delete;

    void next(const char* label) noexcept;
    // Call only after a successful presentation. Omission marks an aborted frame.
    void frameCompleted(uint64_t presentation, uint64_t scene, bool sceneRendered) noexcept;
    static bool enabled();
    static void count(const char* name, uint64_t value, const char* unit = "count") noexcept;

  private:
    friend class CpuProfiler;
    using Clock = std::chrono::steady_clock;
    void finish() noexcept;
    void begin() noexcept;

    static thread_local CpuProfileScope* _current;
    const char* _label;
    bool _boundary = false, _active = false, _complete = false, _sceneRendered = false;
    CpuProfileScope* _parent = nullptr;
    Clock::time_point _start;
    double _children = 0;
    uint64_t _epoch = 0, _recording = 0, _id = 0, _frame = 0;
    uint64_t _presentation = 0, _scene = 0;
};
}
