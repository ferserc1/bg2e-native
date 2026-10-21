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

#include <cstdint>
#include <array>

namespace bg2e::base {
// Plain telemetry transport; base has no dependency on GPU/native/UI types.
enum class GpuProfileStatus { Pending, Available, Unsupported, Dropped, Failed, Aborted, Unavailable };
enum class GpuProfileBackend { Unknown, Vulkan, Metal };
enum class GpuProfileWorkload { Unassociated, Scene, Ui, SceneAndUi, Auxiliary };

enum class GpuProfileGranularity { None, PipelineBoundary, EncoderEnvelope };
struct GpuProfileInterval {
    std::array<char, 96> label{};
    uint64_t context = 0;
    uint32_t id = 0, parent = 0;
    double durationMs = 0;
    GpuProfileStatus status = GpuProfileStatus::Pending;
    GpuProfileGranularity granularity = GpuProfileGranularity::None;
    bool ended = false;
};

struct GpuProfileSample {
    static constexpr uint32_t maximumIntervals = 32;
    std::array<GpuProfileInterval, maximumIntervals> intervals{};
    uint32_t intervalCount = 0, intervalsDropped = 0;
    bool passTiming = true, detailed = false;
    GpuProfileGranularity intervalCapability = GpuProfileGranularity::None;
    uint64_t token = 0, epoch = 0, recording = 0, thread = 0;
    uint64_t presentationFrame = 0, presentationNumber = 0, sceneFrame = 0;
    uint64_t device = 0, queue = 0, command = 0, submission = 0;
    // CPU monotonic correlation only. Never subtract these from GPU clocks.
    uint64_t requested = 0, submitted = 0, resolved = 0;
    double durationMs = 0;
    GpuProfileStatus status = GpuProfileStatus::Pending;
    GpuProfileBackend backend = GpuProfileBackend::Unknown;
    GpuProfileWorkload workload = GpuProfileWorkload::Unassociated;
};
static_assert(sizeof(GpuProfileSample) <= 8192, "GPU telemetry must remain bounded");

struct GpuProfileQueueStatus {
    uint64_t device = 0, queue = 0, pending = 0, dropped = 0, failed = 0, unsupported = 0;
    bool supported = false;
    GpuProfileBackend backend = GpuProfileBackend::Unknown;
};
}
