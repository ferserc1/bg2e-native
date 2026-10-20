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

#include <bg2e/base/CpuProfile.hpp>
#include <algorithm>
#include <atomic>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>

namespace bg2e::base {
namespace {
struct Output {
    std::mutex mutex;
    std::ofstream file;
    std::filesystem::path path;
    std::string sessionInfo;
    std::string error;
    CpuProfiler::ContextProvider contextProvider = nullptr;
    void* contextData = nullptr;
    std::atomic<bool> enabled{false};
    std::atomic<uint64_t> session{0};
};

Output& output()
{
    static Output value;
    return value;
}
}

bool CpuProfiler::start(const std::filesystem::path& path)
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    data.enabled = false;
    ++data.session;
    data.file.close();
    data.file.clear();
    data.path = path;
    const auto startedAt = std::chrono::system_clock::now();
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        startedAt.time_since_epoch()).count();
    const auto time = std::chrono::system_clock::to_time_t(startedAt);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream formattedTime;
    formattedTime << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    data.sessionInfo = "session=" + std::to_string(data.session.load()) +
        " started=" + formattedTime.str() + " started_unix_ms=" + std::to_string(timestamp);
    data.error.clear();
    data.file.open(path, std::ios::out | std::ios::app);
    if (!data.file)
    {
        data.error = "Cannot open CPU profile file: " + path.string();
        return false;
    }
    data.file << "\n=== CPU profiling session ===\n"
              << data.sessionInfo << '\n';
    data.file.flush();
    if (!data.file)
    {
        data.error = "Cannot write CPU profile file: " + path.string();
        data.file.close();
        return false;
    }
    data.enabled = true;
    return true;
}

void CpuProfiler::stop()
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    data.enabled = false;
    ++data.session;
    if (data.file.is_open())
    {
        data.file << "=== CPU profiling stopped ===\n";
        data.file.close();
    }
}

void CpuProfiler::setContextProvider(ContextProvider provider, void* contextData)
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    data.contextProvider = provider;
    data.contextData = contextData;
}

std::string CpuProfiler::context()
{
    ContextProvider provider;
    void* contextData = nullptr;
    {
        auto& data = output();
        std::lock_guard lock(data.mutex);
        provider = data.contextProvider;
        contextData = data.contextData;
    }
    return provider ? provider(contextData) : std::string{};
}

void CpuProfiler::count(const char* name, uint64_t value) noexcept
{
    if (!enabled() || !name)
    {
        return;
    }
    try
    {
        auto& state = CpuProfileScope::state();
        const auto currentSession = session();
        if (state.session != currentSession)
        {
            state = {};
            state.session = currentSession;
        }
        auto& count = state.counts[name];
        count.total += value;
        count.maximum = std::max(count.maximum, value);
    }
    catch (...)
    {
    }
}

bool CpuProfiler::enabled()
{
    return output().enabled.load();
}

uint64_t CpuProfiler::session()
{
    return output().session.load();
}

std::filesystem::path CpuProfiler::outputPath()
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    return data.path;
}

std::string CpuProfiler::sessionInfo()
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    return data.sessionInfo;
}

std::string CpuProfiler::error()
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    return data.error;
}

void CpuProfiler::write(uint64_t session, const std::string& report)
{
    auto& data = output();
    std::lock_guard lock(data.mutex);
    if (!data.enabled || data.session != session)
    {
        return;
    }
    data.file << report;
    data.file.flush();
    if (!data.file)
    {
        data.error = "Cannot write CPU profile file: " + data.path.string();
        data.enabled = false;
        ++data.session;
        data.file.close();
    }
}
}
