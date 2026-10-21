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
#include <cstdint>
#include <vector>

namespace bg2e::gpu {
using SubmissionTimingRequest = base::GpuProfileSample;
using SubmissionTimingResult = base::GpuProfileSample;
using SubmissionTimingStatus = base::GpuProfileStatus;
using SubmissionTimingBackend = base::GpuProfileBackend;
using SubmissionTimingWorkload = base::GpuProfileWorkload;

struct SubmissionTimingCapabilities {
    bool supported = false;
    uint32_t maximumPending = 128;
    uint32_t timestampValidBits = 0;
    double timestampPeriodNs = 0;
    uint64_t device = 0, queue = 0;
    SubmissionTimingBackend backend = SubmissionTimingBackend::Unknown;
    const char* description = "Submission timing is unsupported by this backend";
};

struct SubmissionTimingBatch {
    std::vector<SubmissionTimingResult> results;
    uint64_t pending = 0, dropped = 0, failed = 0, unsupported = 0;
};

namespace detail {
BG2E_API uint64_t nextTimingIdentity() noexcept;
}
}
