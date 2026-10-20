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

#include <algorithm>
#include <bg2e/common.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace bg2e::base {
// Process-wide control and file output. Callers supply application identity;
// the base profiler does not depend on the application or UI namespaces.
class BG2E_API CpuProfiler {
  public:
    using ContextProvider = std::string (*)(void*);
    static bool start(const std::filesystem::path& path);
    static void stop();
    static void setContextProvider(ContextProvider provider, void* context);
    static std::string context();
    static void count(const char* name, uint64_t value) noexcept;

    static bool enabled();
    static uint64_t session();
    static std::filesystem::path outputPath();
    static std::string sessionInfo();
    static std::string error();

    static void write(uint64_t session, const std::string& report);
};

// Opt-in CPU wall-clock diagnostics, including driver calls and blocking waits.
// Labels must have static lifetime. No GPU synchronization is introduced.
class CpuProfileScope {
  public:
    explicit CpuProfileScope(const char* label, bool reportBoundary = false)
        : _label(label), _boundary(reportBoundary), _active(enabled())
    {
        if (_active)
        {
            _session = CpuProfiler::session();
            _frameBoundary = reportBoundary;
            _parent = _current;
            _current = this;
            _start = Clock::now();
        }
    }

    ~CpuProfileScope() noexcept
    {
        if (_active)
        {
            finish();
            _current = _parent;
            if (_boundary && enabled() && _session == CpuProfiler::session())
            {
                report(_frameBoundary);
            }
        }
    }

    CpuProfileScope(const CpuProfileScope&) = delete;
    CpuProfileScope& operator=(const CpuProfileScope&) = delete;

    // End one sequential stage and start the next without changing its parent.
    void next(const char* label) noexcept
    {
        if (_active)
        {
            finish();
            _label = label;
            _children = 0;
            _start = Clock::now();
        }
    }

    static bool enabled()
    {
        return CpuProfiler::enabled();
    }

    static void count(const char* name, uint64_t value) noexcept
    {
        CpuProfiler::count(name, value);
    }

  private:
    friend class CpuProfiler;

    using Clock = std::chrono::steady_clock;
    struct Sample {
        uint64_t calls = 0;
        double total = 0, self = 0, maximum = 0;
    };
    struct Count {
        uint64_t total = 0;
        uint64_t maximum = 0;
    };
    struct State {
        std::map<std::string, Sample> samples;
        std::map<std::string, Count> counts;
        Clock::time_point last = Clock::now();
        uint64_t frames = 0;
        uint64_t session = 0;
    };

    static State& state()
    {
        static thread_local State value;
        return value;
    }

    void finish() noexcept
    {
        const double ms = std::chrono::duration<double, std::milli>(Clock::now() - _start).count();
        if (_parent)
        {
            _parent->_children += ms;
        }
        try
        {
            if (!enabled() || _session != CpuProfiler::session())
            {
                return;
            }
            auto& data = state();
            if (data.session != _session)
            {
                data = State{};
                data.session = _session;
            }
            auto& sample = data.samples[_label];
            ++sample.calls;
            sample.total += ms;
            sample.self += std::max(0.0, ms - _children);
            sample.maximum = std::max(sample.maximum, ms);
        }
        catch (...)
        {
            // Diagnostics must never interrupt rendering or exception unwinding.
        }
    }

    static void report(bool frameBoundary) noexcept
    {
        try
        {
            auto& data = state();
            ++data.frames;
            const auto now = Clock::now();
            const double seconds = std::chrono::duration<double>(now - data.last).count();
            if (frameBoundary && seconds < 2.0)
            {
                return;
            }
            std::vector<std::pair<std::string, Sample>> sorted(data.samples.begin(), data.samples.end());
            std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b)
            {
                return a.second.self > b.second.self;
            });
            std::ostringstream text;
            text.setf(std::ios::fixed);
            text.precision(3);
            text << "\n[CPU profile] " << data.frames << " frame calls / " << seconds
                 << " s; wall time (includes waits), sorted by self time\n"
                 << "self ms/frame | total ms/frame | avg ms/call | max ms/call | calls | stage\n";
            const auto context = CpuProfiler::context();
            if (!context.empty())
            {
                text << "context: " << context << '\n';
            }
            for (const auto& [name, sample] : sorted)
            {
                text << sample.self / data.frames << " | " << sample.total / data.frames << " | "
                     << sample.total / sample.calls << " | " << sample.maximum << " | "
                     << sample.calls << " | " << name << '\n';
            }
            for (const auto& [name, count] : data.counts)
            {
                text << "count " << name << " | total=" << count.total
                     << " | max/call=" << count.maximum << '\n';
            }
            CpuProfiler::write(data.session, text.str());
            data.samples.clear();
            data.counts.clear();
            data.frames = 0;
            data.last = Clock::now();
        }
        catch (...)
        {
        }
    }

    inline static thread_local CpuProfileScope* _current = nullptr;
    const char* _label;
    bool _boundary, _active;
    bool _frameBoundary = false;
    CpuProfileScope* _parent = nullptr;
    Clock::time_point _start;
    double _children = 0;
    uint64_t _session = 0;
};
}
