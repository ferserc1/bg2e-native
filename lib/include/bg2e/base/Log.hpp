/*
 *    business grade graphic engine (bg2 engine)
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
#include <utility>
#include <iostream>
#include <functional>
#include <memory>
#include <ostream>

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>

namespace bg2e {
namespace base {

class BG2E_API Log {
  public:
    enum class Level
    {
        Info = 0,
        Warning = 1,
        Error = 2,
        Debug = 3
    };

    static bool isDebug();

    // Starts a new exclusive session. Throws on initialization failure.
    // An empty path resolves the executable directory using PlatformTools.
    static void initializeDeepDebug(const std::filesystem::path& directory = {});
    static void shutdownDeepDebug() noexcept;
    static bool deepDebugEnabled() noexcept;
    static uint64_t nextDiagnosticIdentity() noexcept;

    template <typename Writer>
    static void trace(const char* backend, const char* event, Writer&& writer, bool repeated = false) noexcept
    {
        if (deepDebugEnabled())
        {
            try
            {
                writeTrace(backend, event, std::forward<Writer>(writer), repeated);
            }
            catch (...)
            {
                reportTraceFailure();
            }
        }
    }

    Log(Level l);
    ~Log();
    Log(const Log& other);
    Log& operator=(const Log& other);

    template <typename T>
    Log& operator<<(const T& value)
    {
        auto& out_fn = (_level == Level::Error || _level == Level::Warning) ? std::cerr : std::cout;
        if ((_level == Level::Debug && isDebug()) || _level != Level::Debug)
        {
            out_fn << value;
        }
        if (_level == Level::Warning || _level == Level::Error)
        {
            if (auto* capture = captureStream())
            {
                try
                {
                    *capture << value;
                }
                catch (...)
                {
                    reportTraceFailure();
                }
            }
        }
        return *this;
    }

  private:
    struct Capture;
    std::unique_ptr<Capture> _capture;
    std::ostream* captureStream() noexcept;
    static void writeTrace(const char* backend, const char* event,
        const std::function<void(std::ostream&)>& writer, bool repeated);
    static void reportTraceFailure() noexcept;

  protected:
    Level _level;
};

}
}

#define bg2e_log_info bg2e::base::Log(bg2e::base::Log::Level::Info)
#define bg2e_log_warning bg2e::base::Log(bg2e::base::Log::Level::Warning)
#define bg2e_log_error bg2e::base::Log(bg2e::base::Log::Level::Error)
#define bg2e_log_debug bg2e::base::Log(bg2e::base::Log::Level::Debug)
#define bg2e_log_end "\n"

// Payloads are evaluated only while the persistent diagnostic session is active.
#define bg2e_deep_trace(backend, event, payload) \
    ::bg2e::base::Log::trace(backend, event, [&](std::ostream& trace) { trace << payload; })
