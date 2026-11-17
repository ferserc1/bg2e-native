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

#include <string_view>
#include <algorithm>
#include <cstring>
#include <bg2e/base/Log.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <thread>
#include <unordered_map>

#if BG2E_IS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif
#if BG2E_IS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace bg2e::base {
namespace {
constexpr size_t capacity = 12 * 1024 * 1024;
constexpr size_t trimBlock = 512 * 1024;
constexpr size_t payloadLimit = 8192;
std::atomic_bool enabled{false};
std::atomic<uint64_t> identities{1};

// Fixed storage also bounds user-defined stream formatting and native messages.
class BoundedBuffer : public std::streambuf {
  public:
    BoundedBuffer()
    {
        setp(_data.data(), _data.data() + _data.size());
    }

    std::string rawText() const
    {
        return std::string(pbase(), pptr());
    }

    std::string text() const
    {
        auto result = rawText();
        if (_truncated)
        {
            result += " [truncated]";
        }
        for (auto& character : result)
        {
            if (character == '\n' || character == '\r' || character == '\0')
            {
                character = ' ';
            }
        }
        return result;
    }

    bool truncated() const
    {
        return _truncated;
    }

  protected:
    std::streamsize xsputn(const char* data, std::streamsize count) override
    {
        const auto copied = std::min(count, static_cast<std::streamsize>(epptr() - pptr()));
        if (copied > 0)
        {
            std::memcpy(pptr(), data, static_cast<size_t>(copied));
            pbump(static_cast<int>(copied));
        }
        _truncated = _truncated || copied < count;
        return count;
    }

    int_type overflow(int_type character) override
    {
        _truncated = true;
        return traits_type::not_eof(character);
    }

  private:
    std::array<char, payloadLimit> _data;
    bool _truncated = false;
};

std::string_view boundedField(const char* value, size_t limit)
{
    size_t length = 0;
    if (value)
    {
        while (length < limit && value[length])
        {
            ++length;
        }
    }
    return {value ? value : "", length};
}

std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if BG2E_IS_WINDOWS
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    std::ostringstream result;
    result << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
        << (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000) << 'Z';
    return result.str();
}

std::filesystem::path executableDirectory()
{
    auto path = PlatformTools::applicationPath();
#if BG2E_IS_MAC
    if (path.extension() != ".app")
    {
        return path;
    }
    const auto name = path.string();
    auto url = CFURLCreateFromFileSystemRepresentation(nullptr,
        reinterpret_cast<const UInt8*>(name.data()), name.size(), true);
    auto bundle = url ? CFBundleCreate(nullptr, url) : nullptr;
    auto executable = bundle ? CFBundleCopyExecutableURL(bundle) : nullptr;
    std::array<UInt8, 32768> buffer{};
    const bool resolved = executable && CFURLGetFileSystemRepresentation(executable, true, buffer.data(), buffer.size());
    if (executable)
    {
        CFRelease(executable);
    }
    if (bundle)
    {
        CFRelease(bundle);
    }
    if (url)
    {
        CFRelease(url);
    }
    if (!resolved)
    {
        throw std::runtime_error("Deep debug: unable to resolve bundle executable metadata: " + name);
    }
    path = std::filesystem::path(reinterpret_cast<const char*>(buffer.data())).parent_path();
#endif
    return path;
}

struct Sink {
    std::mutex mutex;
    std::filesystem::path path;
    std::ofstream file;
    std::string header;
    size_t bytes = 0;
    uint64_t sequence = 0;
    std::unordered_map<std::string, uint64_t> repetitions;
#if BG2E_IS_WINDOWS
    HANDLE ownership = INVALID_HANDLE_VALUE;
#else
    int ownership = -1;
#endif

    void close() noexcept
    {
        enabled.store(false, std::memory_order_release);
        file.close();
        file.clear();
        repetitions.clear();
#if BG2E_IS_WINDOWS
        if (ownership != INVALID_HANDLE_VALUE)
        {
            CloseHandle(ownership);
            ownership = INVALID_HANDLE_VALUE;
        }
#else
        if (ownership >= 0)
        {
            ::close(ownership);
            ownership = -1;
        }
#endif
    }

    void compact(size_t incoming)
    {
        file.flush();
        if (!file)
        {
            throw std::runtime_error("flush failed");
        }
        // Remove at least one block, then skip the partial oldest record.
        std::ifstream source(path, std::ios::binary);
        const auto discard = std::max(trimBlock, bytes + incoming - capacity);
        source.seekg(static_cast<std::streamoff>(header.size() + discard));
        std::string partial;
        std::getline(source, partial);
        if (!source)
        {
            throw std::runtime_error("compaction read failed");
        }
        auto temporary = path;
        temporary += ".tmp";
        std::ofstream target(temporary, std::ios::binary | std::ios::trunc);
        target.write(header.data(), static_cast<std::streamsize>(header.size()));
        std::array<char, 65536> block;
        size_t retained = header.size();
        while (source)
        {
            source.read(block.data(), block.size());
            const auto count = source.gcount();
            target.write(block.data(), count);
            retained += static_cast<size_t>(count);
        }
        target.flush();
        if (source.bad() || !target || retained + incoming > capacity)
        {
            throw std::runtime_error("compaction copy failed");
        }
        target.close();
        if (!target)
        {
            throw std::runtime_error("compaction close failed");
        }
        source.close();
        file.close();
#if BG2E_IS_WINDOWS
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
        {
            throw std::runtime_error("compaction replacement failed");
        }
#else
        std::filesystem::rename(temporary, path);
#endif
        file.open(path, std::ios::binary | std::ios::app);
        bytes = retained;
        if (!file)
        {
            throw std::runtime_error("compaction reopen failed");
        }
    }
};

// Intentionally process-lived: late native callbacks can safely query the sink.
// Explicit shutdown releases files/ownership without destroying the mutex.
std::atomic<Sink*> existingSink{nullptr};

Sink& sink()
{
    static auto* value = []
    {
        auto* created = new Sink;
        existingSink.store(created, std::memory_order_release);
        return created;
    }();
    return *value;
}
}

struct Log::Capture {
    BoundedBuffer buffer;
    std::ostream stream{&buffer};
};

bool Log::isDebug()
{
#if NDEBUG
    return false;
#else
    return true;
#endif
}

bool Log::deepDebugEnabled() noexcept
{
    return enabled.load(std::memory_order_acquire);
}

uint64_t Log::nextDiagnosticIdentity() noexcept
{
    return identities.fetch_add(1, std::memory_order_relaxed);
}

void Log::initializeDeepDebug(const std::filesystem::path& directory)
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    if (enabled.load())
    {
        throw std::logic_error("Deep debug session already initialized");
    }
    state.close();
    state.path = (directory.empty() ? executableDirectory() : directory) / "bg2e-deep-debug.log";
    auto ownershipPath = state.path;
    ownershipPath += ".lock";
    try
    {
#if BG2E_IS_WINDOWS
        state.ownership = CreateFileW(ownershipPath.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (state.ownership == INVALID_HANDLE_VALUE)
#else
        state.ownership = ::open(ownershipPath.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (state.ownership < 0 || flock(state.ownership, LOCK_EX | LOCK_NB) != 0)
#endif
        {
            throw std::runtime_error("directory is not writable or another process owns the session");
        }
        state.file.open(state.path, std::ios::binary | std::ios::trunc);
#if BG2E_IS_WINDOWS
        const auto process = GetCurrentProcessId();
#else
        const auto process = getpid();
#endif
        state.header = "# bg2e deep debug session " + timestamp() + " pid=" + std::to_string(process) +
            " debugBuild=" + std::to_string(isDebug()) +
            " limit=12582912 record_payload_limit=8192; encode return does not imply completion\n";
        state.file << state.header;
        state.file.flush();
        if (!state.file)
        {
            throw std::runtime_error("unable to initialize persistent output");
        }
        state.bytes = state.header.size();
        state.sequence = 0;
        enabled.store(true, std::memory_order_release);
        std::cerr << "Deep debug active: " << state.path << '\n';
    }
    catch (const std::exception& error)
    {
        state.close();
        throw std::runtime_error("Deep debug " + state.path.string() + ": " + error.what());
    }
}

void Log::shutdownDeepDebug() noexcept
{
    auto* state = existingSink.load(std::memory_order_acquire);
    if (!state)
    {
        return;
    }
    trace("log", "session.shutdown", [](std::ostream&) {});
    std::lock_guard lock(state->mutex);
    state->close();
}

void Log::reportTraceFailure() noexcept
{
    // No logger recursion, and never throw into a driver callback/destructor.
    if (enabled.exchange(false))
    {
        std::fputs("Deep debug persistent logging failed; trace disabled. Check disk space and permissions.\n", stderr);
    }
}

void Log::writeTrace(const char* backend, const char* event,
    const std::function<void(std::ostream&)>& writer, bool repeated)
{
    BoundedBuffer buffer;
    std::ostream stream(&buffer);
    writer(stream); // No log lock across payload/native inspection.
    auto payload = buffer.text();
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    if (!enabled.load())
    {
        return;
    }
    if (repeated && !buffer.truncated())
    {
        const auto key = std::string(boundedField(backend, 64)) + ' ' + std::string(boundedField(event, 128)) + ' ' + buffer.rawText();
        if (state.repetitions.size() >= 128 && !state.repetitions.count(key))
        {
            state.repetitions.clear();
        }
        auto& count = state.repetitions[key];
        ++count;
        // First and powers of two: report storms with cumulative counts.
        if ((count & (count - 1)) != 0)
        {
            return;
        }
        payload += " occurrences=" + std::to_string(count);
    }
    std::ostringstream envelope;
    envelope << ++state.sequence << ' ' << timestamp() << " backend=" << boundedField(backend, 64)
        << " thread=" << std::this_thread::get_id() << " event=" << boundedField(event, 128) << ' ' << payload << '\n';
    const auto record = envelope.str();
    if (record.size() > capacity - state.header.size())
    {
        throw std::runtime_error("record exceeds capacity");
    }
    if (state.bytes + record.size() > capacity)
    {
        state.compact(record.size());
    }
    state.file.write(record.data(), static_cast<std::streamsize>(record.size()));
    state.file.flush(); // Preserve useful evidence without waiting for shutdown.
    if (!state.file)
    {
        throw std::runtime_error("persistent write failed");
    }
    state.bytes += record.size();
}

Log::Log(Level level) : _level(level)
{
}

Log::Log(const Log& other) : _level(other._level)
{
}

Log& Log::operator=(const Log& other)
{
    if (this != &other)
    {
        _capture.reset();
        _level = other._level;
    }
    return *this;
}

Log::~Log()
{
    if (_capture)
    {
        trace("log", _level == Level::Error ? "error" : "warning", [&](std::ostream& out)
        {
            out << _capture->buffer.text();
        });
    }
}

std::ostream* Log::captureStream() noexcept
{
    if (!deepDebugEnabled())
    {
        return nullptr;
    }
    try
    {
        if (!_capture)
        {
            _capture = std::make_unique<Capture>();
        }
        return &_capture->stream;
    }
    catch (...)
    {
        reportTraceFailure();
        return nullptr;
    }
}

}
