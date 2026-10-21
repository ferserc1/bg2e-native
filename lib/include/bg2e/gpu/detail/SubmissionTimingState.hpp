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

#include <bg2e/gpu/SubmissionTiming.hpp>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>

namespace bg2e::gpu::detail {
inline uint64_t timingCpuNow() noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Modulo subtraction handles one wrap. A CPU upper bound rejects ambiguous
// multiple-wrap intervals instead of returning a plausible but incorrect zero.
inline bool timestampDuration(uint64_t first, uint64_t last, uint32_t bits,
    double periodNs, uint64_t upperBoundNs, double& milliseconds) noexcept
{
    if (!bits || bits > 64 || !std::isfinite(periodNs) || periodNs <= 0)
    {
        return false;
    }
    const long double wrapNs = std::ldexp(static_cast<long double>(periodNs), int(bits));
    if (static_cast<long double>(upperBoundNs) >= wrapNs)
    {
        return false;
    }
    const uint64_t mask = bits == 64 ? UINT64_MAX : (uint64_t(1) << bits) - 1;
    const auto ticks = (last - first) & mask;
    milliseconds = double(static_cast<long double>(ticks) * periodNs / 1e6L);
    return std::isfinite(milliseconds) && milliseconds >= 0 &&
        static_cast<long double>(ticks) * periodNs <= static_cast<long double>(upperBoundNs);
}

class SubmissionTimingState;
struct SubmissionTimingTicket {
    std::weak_ptr<SubmissionTimingState> owner;
    SubmissionTimingResult sample;
    size_t slot = 0;
    bool submitted = false, terminal = false;
    std::shared_ptr<void> resource;
    std::function<SubmissionTimingStatus(SubmissionTimingResult&)> resolve;

    void markSubmitted(uint64_t submission) noexcept;
    void context(uint64_t scene, SubmissionTimingWorkload workload) noexcept;
    void complete(bool success) noexcept;
    void completeNative(SubmissionTimingStatus status, double durationMs) noexcept;
    void abandon() noexcept;
};

// One fixed-capacity state per logical queue. Native completion callbacks publish
// plain values here, never invoke an application callback or touch a profiler.
class SubmissionTimingState : public std::enable_shared_from_this<SubmissionTimingState> {
  public:
    static constexpr size_t capacity = 128;
    SubmissionTimingCapabilities capabilities;

    std::shared_ptr<SubmissionTimingTicket> reserve(const SubmissionTimingRequest& request) noexcept
    {
        try
        {
            std::lock_guard lock(_mutex);
            if (_closed || !capabilities.supported)
            {
                ++_unsupported;
                return {};
            }
            for (size_t i = 0; i < capacity; ++i)
            {
                if (!_active[i])
                {
                    auto ticket = std::make_shared<SubmissionTimingTicket>();
                    ticket->owner = shared_from_this();
                    ticket->slot = i;
                    ticket->sample = request;
                    ticket->sample.intervalCount = 0;
                    ticket->sample.intervalsDropped = 0;
                    ticket->sample.intervals = {};
                    ticket->sample.device = capabilities.device;
                    ticket->sample.queue = capabilities.queue;
                    ticket->sample.backend = capabilities.backend;
                    ticket->sample.command = nextTimingIdentity();
                    _active[i] = ticket;
                    return ticket;
                }
            }
            ++_dropped;
        }
        catch (...)
        {
            std::lock_guard lock(_mutex);
            ++_dropped;
        }
        return {};
    }

    void markSubmitted(SubmissionTimingTicket& ticket, uint64_t submission) noexcept
    {
        std::lock_guard lock(_mutex);
        if (!ticket.terminal)
        {
            ticket.submitted = true;
            ticket.sample.submission = submission;
            ticket.sample.submitted = timingCpuNow();
        }
    }

    void context(SubmissionTimingTicket& ticket, uint64_t scene, SubmissionTimingWorkload workload) noexcept
    {
        std::lock_guard lock(_mutex);
        if (!ticket.terminal && !ticket.submitted)
        {
            ticket.sample.sceneFrame = scene;
            ticket.sample.workload = workload;
        }
    }

    void complete(SubmissionTimingTicket& ticket, bool success) noexcept
    {
        std::lock_guard lock(_mutex);
        if (ticket.terminal)
        {
            return;
        }
        auto status = success ? SubmissionTimingStatus::Unavailable : SubmissionTimingStatus::Failed;
        try
        {
            if (success && ticket.resolve)
            {
                status = ticket.resolve(ticket.sample);
            }
        }
        catch (...)
        {
            status = SubmissionTimingStatus::Failed;
        }
        finish(ticket, status, ticket.sample.durationMs);
    }

    void completeNative(SubmissionTimingTicket& ticket, SubmissionTimingStatus status, double milliseconds) noexcept
    {
        std::lock_guard lock(_mutex);
        if (!ticket.terminal)
        {
            if (status == SubmissionTimingStatus::Available && ticket.resolve)
            {
                try
                {
                    ticket.resolve(ticket.sample);
                }
                catch (...)
                {
                    // Preserve valid total timing if optional counters fail.
                }
            }
            finish(ticket, status, milliseconds);
        }
    }

    void abandon(SubmissionTimingTicket& ticket) noexcept
    {
        std::lock_guard lock(_mutex);
        if (!ticket.terminal && !ticket.submitted)
        {
            finish(ticket, SubmissionTimingStatus::Aborted, 0);
        }
    }

    SubmissionTimingBatch take() noexcept
    {
        SubmissionTimingBatch batch;
        try
        {
            std::lock_guard lock(_mutex);
            batch.results.reserve(_count);
            for (size_t i = 0; i < _count; ++i)
            {
                batch.results.push_back(_ready[(_head + i) % capacity]);
            }
            _head = _count = 0;
            for (const auto& item : _active)
            {
                batch.pending += item ? 1 : 0;
            }
            batch.dropped = _dropped;
            batch.failed = _failed;
            batch.unsupported = _unsupported;
        }
        catch (...)
        {
            // Keep completed results for the next poll if allocation fails.
            batch.pending = 1; // Preserve caller polling even after collection stops.
        }
        return batch;
    }

    // Device owner calls only after its existing submission drain, before native
    // device destruction. Unsubmitted recordings need no GPU wait to abandon.
    void close() noexcept
    {
        std::lock_guard lock(_mutex);
        _closed = true;
        capabilities.supported = false;
        capabilities.description = "Submission timing queue has been retired";
        for (size_t i = 0; i < capacity; ++i)
        {
            if (auto ticket = _active[i])
            {
                finish(*ticket, ticket->submitted ? SubmissionTimingStatus::Unavailable : SubmissionTimingStatus::Aborted, 0);
            }
        }
    }

  private:
    void finish(SubmissionTimingTicket& ticket, SubmissionTimingStatus status, double milliseconds) noexcept
    {
        if (status == SubmissionTimingStatus::Available && (!std::isfinite(milliseconds) || milliseconds < 0))
        {
            status = SubmissionTimingStatus::Unavailable;
        }
        for (uint32_t i = 0; i < ticket.sample.intervalCount; ++i)
        {
            auto& interval = ticket.sample.intervals[i];
            if (!interval.ended)
            {
                interval.status = SubmissionTimingStatus::Aborted;
            }
            else if (status != SubmissionTimingStatus::Available && interval.status == SubmissionTimingStatus::Available)
            {
                interval.status = status;
            }
            else if (interval.status == SubmissionTimingStatus::Pending)
            {
                interval.status = SubmissionTimingStatus::Unavailable;
            }
        }
        ticket.terminal = true;
        ticket.sample.status = status;
        ticket.sample.durationMs = status == SubmissionTimingStatus::Available ? milliseconds : 0;
        ticket.sample.resolved = timingCpuNow();
        ticket.resolve = {};
        ticket.resource.reset();
        if (status == SubmissionTimingStatus::Failed || status == SubmissionTimingStatus::Unavailable)
        {
            ++_failed;
        }
        if (_count == capacity)
        {
            ++_dropped;
        }
        else
        {
            _ready[(_head + _count++) % capacity] = ticket.sample;
        }
        _active[ticket.slot].reset();
    }

    std::mutex _mutex;
    std::array<std::shared_ptr<SubmissionTimingTicket>, capacity> _active{};
    std::array<SubmissionTimingResult, capacity> _ready{};
    size_t _head = 0, _count = 0;
    uint64_t _dropped = 0, _failed = 0, _unsupported = 0;
    bool _closed = false;
};

inline void SubmissionTimingTicket::markSubmitted(uint64_t submission) noexcept
{
    if (auto state = owner.lock())
    {
        state->markSubmitted(*this, submission);
    }
}

inline void SubmissionTimingTicket::context(uint64_t scene, SubmissionTimingWorkload workload) noexcept
{
    if (auto state = owner.lock())
    {
        state->context(*this, scene, workload);
    }
}

inline void SubmissionTimingTicket::complete(bool success) noexcept
{
    if (auto state = owner.lock())
    {
        state->complete(*this, success);
    }
}

inline void SubmissionTimingTicket::completeNative(SubmissionTimingStatus status, double milliseconds) noexcept
{
    if (auto state = owner.lock())
    {
        state->completeNative(*this, status, milliseconds);
    }
}

inline void SubmissionTimingTicket::abandon() noexcept
{
    if (auto state = owner.lock())
    {
        state->abandon(*this);
    }
}
}
