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
#include <array>
#include <cmath>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <deque>
#include <cstdio>
#include <sstream>
#include <tuple>
#include <memory>
#include <map>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <utility>

namespace bg2e::base {
namespace {
constexpr size_t maxThreads = 64;
constexpr size_t batchSize = 128;
constexpr size_t historyBatches = 512;
constexpr size_t writerBatches = 256;
constexpr uint64_t historyNs = 30'000'000'000;

uint64_t nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string bounded(const char* text)
{
    if (!text)
    {
        return {};
    }
    size_t length = 0;
    while (length < 127 && text[length])
    {
        ++length;
    }
    return std::string(text, length);
}

// Formatting is called exclusively by the file writer.
void quoted(std::ostream& out, const std::string& value)
{
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (unsigned char c : value)
    {
        if (c == '"' || c == '\\')
        {
            out << '\\' << char(c);
        }
        else if (c < 32 || c >= 127)
        {
            out << "\\u00" << hex[c >> 4] << hex[c & 15];
        }
        else
        {
            out << char(c);
        }
    }
    out << '"';
}

struct Batch {
    std::vector<CpuProfileSample> samples;
    std::vector<GpuProfileSample> gpu;
};

struct ThreadState {
    std::mutex mutex;
    std::vector<CpuProfileSample> samples;
    uint64_t id = 0, lastPublication = 0;
};

struct Service {
    std::mutex mutex, control, providerMutex;
    std::condition_variable ready;
    std::atomic<bool> enabled{false}, recording{false}, gpuEnabled{true};
    bool recordingGpuEnabled = true;
    std::atomic<uint64_t> epoch{0}, recordingId{0}, identity{0};
    std::atomic<uint64_t> dropped{0}, partial{0};
    size_t consumers = 0;
    uint64_t openScopes = 0, recordingScopes = 0;
    std::vector<std::weak_ptr<ThreadState>> threads;
    std::deque<std::shared_ptr<const Batch>> history;
    std::deque<Batch> queue;
    uint64_t evicted = 0, writerDropped = 0, startTime = 0, endTime = 0;
    uint64_t sessionDropped = 0, sessionPartial = 0;
    uint64_t endDropped = 0, endPartial = 0;
    std::array<GpuProfileSample, 256> gpuPending{};
    std::array<GpuProfileQueueStatus, 64> gpuQueues{};
    size_t gpuQueueCount = 0;
    std::deque<GpuProfileSample> gpuHistory;
    uint64_t gpuDropped = 0, gpuFailed = 0, gpuUnsupported = 0, gpuEvicted = 0, gpuLate = 0;
    uint64_t sessionGpuAvailable = 0, sessionGpuFailed = 0, sessionGpuUnsupported = 0, sessionGpuDropped = 0;
    uint64_t unresolvedGpu = 0;
    bool closing = false;
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{nullptr, &std::fclose};
    std::thread writer;
    std::filesystem::path path;
    std::string error, metadata;
    CpuProfiler::ContextProvider provider = nullptr;
    void* providerData = nullptr;

    ~Service();

    void updateEnabled()
    {
        const bool next = consumers != 0 || recording.load();
        if (next && !enabled)
        {
            ++epoch;
        }
        enabled = next;
    }
};

Service& service()
{
    static Service instance;
    return instance;
}

void acceptLocked(Service& data, std::vector<CpuProfileSample> samples) noexcept
{
    if (samples.empty())
    {
        return;
    }
    // Live and recording destinations own independent copies.
    if (data.recording)
    {
        const auto eligible = std::count_if(samples.begin(), samples.end(), [&](const auto& sample)
        {
            return sample.recording == data.recordingId && sample.begin >= data.startTime;
        });
        if (eligible)
        {
            try
            {
                if (data.queue.size() == writerBatches)
                {
                    data.writerDropped += eligible;
                }
                else
                {
                    Batch recorded;
                    recorded.samples.reserve(eligible);
                    for (const auto& sample : samples)
                    {
                        if (sample.recording == data.recordingId && sample.begin >= data.startTime)
                        {
                            recorded.samples.push_back(sample);
                        }
                    }
                    data.queue.push_back(std::move(recorded));
                    data.ready.notify_one();
                }
            }
            catch (...)
            {
                data.writerDropped += eligible;
            }
        }
    }
    const auto sampleCount = samples.size();
    try
    {
        data.history.push_back(std::make_shared<const Batch>(Batch{std::move(samples)}));
    }
    catch (...)
    {
        data.dropped += sampleCount;
    }
    const auto now = nowNs();
    while (!data.history.empty() && (data.history.size() > historyBatches ||
        now - data.history.front()->samples.back().end > historyNs))
    {
        data.evicted += data.history.front()->samples.size();
        data.history.pop_front();
    }
}

void gpuAcceptLocked(Service& data, const GpuProfileSample& sample) noexcept
{
    const bool fileEligible = data.recording && sample.recording == data.recordingId &&
        sample.requested >= data.startTime;
    if (sample.status == GpuProfileStatus::Unsupported)
    {
        ++data.gpuUnsupported;
        data.sessionGpuUnsupported += fileEligible ? 1 : 0;
    }
    else if (sample.status == GpuProfileStatus::Dropped)
    {
        ++data.gpuDropped;
        data.sessionGpuDropped += fileEligible ? 1 : 0;
    }
    else if (sample.status != GpuProfileStatus::Available)
    {
        ++data.gpuFailed;
        data.sessionGpuFailed += fileEligible ? 1 : 0;
    }
    else
    {
        data.sessionGpuAvailable += fileEligible ? 1 : 0;
    }
    if (fileEligible)
    {
        try
        {
            if (data.queue.size() == writerBatches)
            {
                ++data.writerDropped;
            }
            else
            {
                Batch batch;
                batch.gpu.push_back(sample);
                data.queue.push_back(std::move(batch));
                data.ready.notify_one();
            }
        }
        catch (...)
        {
            ++data.writerDropped;
        }
    }
    try
    {
        data.gpuHistory.push_back(sample);
    }
    catch (...)
    {
        ++data.gpuDropped;
    }
    const auto now = nowNs();
    while (!data.gpuHistory.empty() && (data.gpuHistory.size() > 512 ||
        now - data.gpuHistory.front().resolved > historyNs))
    {
        data.gpuHistory.pop_front();
        ++data.gpuEvicted;
    }
}

void expireGpuLocked(Service& data)
{
    const auto now = nowNs();
    for (auto& pending : data.gpuPending)
    {
        if (pending.token && now - pending.requested > historyNs)
        {
            auto expired = pending;
            pending.token = 0;
            expired.status = GpuProfileStatus::Dropped;
            expired.resolved = now;
            gpuAcceptLocked(data, expired);
        }
    }
}

void flush(const std::shared_ptr<ThreadState>& state)
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    std::lock_guard threadLock(state->mutex);
    acceptLocked(data, std::move(state->samples));
    state->samples.clear();
    state->lastPublication = nowNs();
}

struct Local {
    std::shared_ptr<ThreadState> state;

    ~Local()
    {
        try
        {
            if (state)
            {
                flush(state);
            }
        }
        catch (...)
        {
            ++service().dropped;
        }
    }
};

std::shared_ptr<ThreadState> threadState()
{
    thread_local Local local;
    if (!local.state)
    {
        auto& data = service();
        std::lock_guard lock(data.mutex);
        std::erase_if(data.threads, [](const auto& weak)
        {
            return weak.expired();
        });
        if (data.threads.size() == maxThreads)
        {
            return {};
        }
        local.state = std::make_shared<ThreadState>();
        local.state->id = ++data.identity;
        data.threads.push_back(local.state);
    }
    return local.state;
}

void append(CpuProfileSample sample, bool* retired = nullptr, uint64_t scopeRecording = 0)
{
    auto state = threadState();
    if (!state)
    {
        ++service().dropped;
        return;
    }
    auto& data = service();
    std::unique_lock publication(data.mutex);
    // Admission and stop's TLS drain share this lock, so accepted records are
    // either in the writer queue or visible to stop before its end boundary.
    if (sample.recording && (!data.recording || sample.recording != data.recordingId ||
        sample.begin < data.startTime))
    {
        sample.recording = 0;
    }
    if (retired)
    {
        --data.openScopes;
        if (scopeRecording && data.recording && scopeRecording == data.recordingId)
        {
            --data.recordingScopes;
        }
        *retired = true;
    }
    sample.thread = state->id;
    bool full;
    {
        std::lock_guard lock(state->mutex);
        state->samples.push_back(std::move(sample));
        full = state->samples.size() >= batchSize || nowNs() - state->lastPublication >= 100'000'000;
    }
    if (full)
    {
        std::lock_guard lock(state->mutex);
        acceptLocked(data, std::move(state->samples));
        state->samples.clear();
        state->lastPublication = nowNs();
    }
}

void writeSample(std::ostream& out, const CpuProfileSample& s)
{
    out << "{\"type\":\"" << (s.kind == CpuProfileSample::Kind::Scope ? "scope" :
        s.kind == CpuProfileSample::Kind::Counter ? "counter" : "frame")
        << "\",\"session\":" << s.recording << ",\"epoch\":" << s.epoch
        << ",\"thread\":" << s.thread << ",\"id\":" << s.id << ",\"parent\":" << s.parent
        << ",\"presentation_number\":" << s.presentationNumber
        << ",\"frame\":" << s.presentationFrame << ",\"scene_frame\":" << s.sceneFrame
        << ",\"begin_ns\":" << s.begin << ",\"end_ns\":" << s.end
        << ",\"inclusive_ms\":" << s.inclusiveMs << ",\"exclusive_ms\":" << s.exclusiveMs
        << ",\"maximum_ms\":" << s.maximumMs << ",\"calls\":" << s.calls
        << ",\"value\":" << s.value << ",\"maximum_value\":" << s.maximumValue
        << ",\"complete\":" << (s.complete ? "true" : "false")
        << ",\"scene_rendered\":" << (s.sceneRendered ? "true" : "false") << ",\"label\":";
    quoted(out, s.label);
    out << ",\"unit\":";
    quoted(out, s.unit);
    out << "}\n";
}

const char* gpuStatusName(GpuProfileStatus status)
{
    switch (status)
    {
    case GpuProfileStatus::Available: return "available";
    case GpuProfileStatus::Unsupported: return "unsupported";
    case GpuProfileStatus::Dropped: return "dropped";
    case GpuProfileStatus::Failed: return "failed";
    case GpuProfileStatus::Aborted: return "aborted";
    case GpuProfileStatus::Unavailable: return "unavailable";
    default: return "pending";
    }
}

void writeGpu(std::ostream& out, const GpuProfileSample& sample)
{
    out << "{\"type\":\"gpu_submission\",\"session\":" << sample.recording
        << ",\"epoch\":" << sample.epoch << ",\"token\":" << sample.token << ",\"thread\":" << sample.thread
        << ",\"frame\":" << sample.presentationFrame << ",\"presentation_number\":" << sample.presentationNumber
        << ",\"scene_frame\":" << sample.sceneFrame << ",\"device\":" << sample.device << ",\"queue\":" << sample.queue
        << ",\"command\":" << sample.command << ",\"submission\":" << sample.submission
        << ",\"requested_ns\":" << sample.requested << ",\"submitted_ns\":" << sample.submitted
        << ",\"resolved_ns\":" << sample.resolved << ",\"status\":\"" << gpuStatusName(sample.status)
        << "\",\"backend\":\"" << (sample.backend == GpuProfileBackend::Vulkan ? "Vulkan" :
            sample.backend == GpuProfileBackend::Metal ? "Metal" : "Unknown")
        << "\",\"workload\":" << int(sample.workload) << ",\"gpu_duration_ms\":";
    if (sample.status == GpuProfileStatus::Available)
    {
        out << sample.durationMs;
    }
    else
    {
        out << "null";
    }
    out << ",\"pass_gpu\":" << (sample.passTiming ? "true" : "false") << ",\"detailed_gpu\":" << (sample.detailed ? "true" : "false")
        << ",\"interval_capability\":" << int(sample.intervalCapability)
        << ",\"intervals_dropped\":" << sample.intervalsDropped << ",\"intervals\":[";
    for (uint32_t i = 0; i < sample.intervalCount; ++i)
    {
        const auto& interval = sample.intervals[i];
        if (i)
        {
            out << ',';
        }
        out << "{\"id\":" << interval.id << ",\"parent\":" << interval.parent << ",\"context\":" << interval.context
            << ",\"label\":";
        quoted(out, interval.label.data());
        out << ",\"status\":\"" << gpuStatusName(interval.status) << "\",\"granularity\":" << int(interval.granularity)
            << ",\"gpu_duration_ms\":";
        if (interval.status == GpuProfileStatus::Available)
        {
            out << interval.durationMs;
        }
        else
        {
            out << "null";
        }
        out << '}';
    }
    out << "]}\n";
}

void writeText(Service& data, const std::string& text)
{
    if (std::fwrite(text.data(), 1, text.size(), data.file.get()) != text.size())
    {
        throw std::runtime_error("CPU profile write failed");
    }
}

void writerMain(Service& data) noexcept
{
    try
    {
        std::ostringstream header;
        header << "{\"type\":\"session\",\"version\":3,\"session\":" << data.recordingId
            << ",\"begin_ns\":" << data.startTime << ",\"gpu_submission_timing\":" << (data.recordingGpuEnabled ? "true" : "false")
            << ",\"clock\":\"steady_ns\",\"context\":";
        quoted(header, data.metadata);
        header << "}\n";
        writeText(data, header.str());
        for (;;)
        {
            Batch batch;
            {
                std::unique_lock lock(data.mutex);
                data.ready.wait(lock, [&]
                {
                    return data.closing || !data.queue.empty();
                });
                if (data.queue.empty())
                {
                    break;
                }
                batch = std::move(data.queue.front());
                data.queue.pop_front();
            }
            std::ostringstream formatted;
            formatted.precision(17);
            for (const auto& sample : batch.samples)
            {
                writeSample(formatted, sample);
            }
            for (const auto& sample : batch.gpu)
            {
                writeGpu(formatted, sample);
            }
            writeText(data, formatted.str());
        }
        std::ostringstream summary;
        {
            std::lock_guard lock(data.mutex);
            summary << "{\"type\":\"summary\",\"session\":" << data.recordingId
                << ",\"end_ns\":" << data.endTime << ",\"dropped\":" << data.endDropped - data.sessionDropped
                << ",\"writer_dropped\":" << data.writerDropped
                << ",\"partial\":" << data.endPartial - data.sessionPartial
                << ",\"unresolved_gpu\":" << data.unresolvedGpu
                << ",\"gpu_available\":" << data.sessionGpuAvailable
                << ",\"gpu_failed\":" << data.sessionGpuFailed
                << ",\"gpu_unsupported\":" << data.sessionGpuUnsupported
                << ",\"gpu_dropped\":" << data.sessionGpuDropped << "}\n";
        }
        writeText(data, summary.str());
        if (std::fclose(data.file.release()) != 0)
        {
            throw std::runtime_error("CPU profile close failed");
        }
    }
    catch (...)
    {
        std::lock_guard lock(data.mutex);
        data.error = "Cannot write CPU profile file";
        if (!data.endTime)
        {
            data.endTime = nowNs();
        }
        data.recording = false;
        data.updateEnabled();
        data.queue.clear();
        data.file.reset();
    }
}

void stopLocked(Service& data)
{
    {
        std::lock_guard lock(data.mutex);
        if (data.recording)
        {
            // No producer can publish across this boundary while we drain TLS.
            data.endTime = nowNs();
            data.unresolvedGpu = 0;
            for (const auto& pending : data.gpuPending)
            {
                data.unresolvedGpu += pending.token && pending.recording == data.recordingId ? 1 : 0;
            }
            data.partial += data.recordingScopes;
            data.recordingScopes = 0;
            for (const auto& weak : data.threads)
            {
                if (auto state = weak.lock())
                {
                    std::lock_guard threadLock(state->mutex);
                    acceptLocked(data, std::move(state->samples));
                    state->samples.clear();
                }
            }
            data.endDropped = data.dropped;
            data.endPartial = data.partial;
            data.recording = false;
            data.updateEnabled();
        }
        data.closing = true;
        data.ready.notify_one();
    }
    if (data.writer.joinable())
    {
        data.writer.join();
    }
}

Service::~Service()
{
    stopLocked(*this);
}
}

CpuProfileCollection::CpuProfileCollection()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    ++data.consumers;
    data.updateEnabled();
}

CpuProfileCollection::~CpuProfileCollection()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    --data.consumers;
    data.updateEnabled();
}

bool CpuProfiler::start(const std::filesystem::path& path, bool overwrite)
{
    auto& data = service();
    // Borrowed callbacks run before either publication or lifecycle locks.
    std::string metadata;
    try
    {
        metadata = context();
    }
    catch (...)
    {
        std::lock_guard lock(data.mutex);
        data.error = "Cannot collect CPU profile context";
        return false;
    }
    std::lock_guard control(data.control);
    if (data.recording)
    {
        std::lock_guard lock(data.mutex);
        data.error = "A CPU recording is already active";
        return false;
    }
    stopLocked(data);
    try
    {
        if (!overwrite && std::filesystem::exists(path))
        {
            std::lock_guard lock(data.mutex);
            data.error = "CPU profile destination exists; explicit overwrite required";
            return false;
        }
#ifdef _WIN32
        auto* opened = _wfopen(path.c_str(), overwrite ? L"wb" : L"wbx");
#else
        auto* opened = std::fopen(path.c_str(), overwrite ? "wb" : "wbx");
#endif
        std::unique_ptr<std::FILE, decltype(&std::fclose)> file(opened, &std::fclose);
        if (!file)
        {
            throw std::runtime_error("open failed");
        }
        std::lock_guard lock(data.mutex);
        data.file = std::move(file);
        data.path = path;
        data.error.clear();
        data.metadata = metadata.substr(0, 4096);
        data.queue.clear();
        data.writerDropped = 0;
        data.sessionDropped = data.dropped;
        data.sessionPartial = data.partial;
        data.partial += data.openScopes;
        data.recordingScopes = 0;
        data.sessionGpuAvailable = data.sessionGpuFailed = data.sessionGpuUnsupported = data.sessionGpuDropped = 0;
        data.unresolvedGpu = 0;
        data.recordingGpuEnabled = data.gpuEnabled;
        data.closing = false;
        ++data.recordingId;
        data.startTime = nowNs();
        data.endTime = 0;
        data.recording = true;
        data.updateEnabled();
        try
        {
            data.writer = std::thread(writerMain, std::ref(data));
        }
        catch (...)
        {
            data.recording = false;
            data.updateEnabled();
            data.file.reset();
            throw;
        }
        return true;
    }
    catch (...)
    {
        std::lock_guard lock(data.mutex);
        data.error = "Cannot start CPU profile recording";
        return false;
    }
}

void CpuProfiler::stop()
{
    auto& data = service();
    std::lock_guard control(data.control);
    stopLocked(data);
}

void CpuProfiler::setContextProvider(ContextProvider provider, void* contextData)
{
    auto& data = service();
    std::lock_guard lock(data.providerMutex);
    data.provider = provider;
    data.providerData = contextData;
}

std::string CpuProfiler::context()
{
    auto& data = service();
    std::lock_guard lock(data.providerMutex);
    return data.provider ? data.provider(data.providerData) : std::string{};
}

bool CpuProfiler::enabled()
{
    return service().enabled.load(std::memory_order_relaxed);
}

bool CpuProfiler::recording()
{
    return service().recording.load();
}

uint64_t CpuProfiler::session()
{
    return service().epoch.load();
}

uint64_t CpuProfiler::recordingSession()
{
    return recording() ? service().recordingId.load() : 0;
}

namespace {
std::atomic<bool> detailedGpuTiming{false};
std::atomic<bool> passGpuTiming{true};
}

bool CpuProfiler::gpuPassTimingEnabled() noexcept
{
    return passGpuTiming.load(std::memory_order_relaxed);
}

void CpuProfiler::setGpuPassTimingEnabled(bool enabled) noexcept
{
    passGpuTiming.store(enabled, std::memory_order_relaxed);
    CpuProfileScope::count("profiling.gpu.passChanges", enabled ? 1 : 0, "enable_events");
}

bool CpuProfiler::gpuDetailedTimingEnabled() noexcept
{
    return detailedGpuTiming.load(std::memory_order_relaxed);
}

void CpuProfiler::setGpuDetailedTimingEnabled(bool enabled) noexcept
{
    detailedGpuTiming.store(enabled, std::memory_order_relaxed);
    CpuProfileScope::count("profiling.gpu.detailChanges", enabled ? 1 : 0, "enable_events");
}

bool CpuProfiler::gpuTimingEnabled() noexcept
{
    return service().gpuEnabled.load(std::memory_order_relaxed);
}

void CpuProfiler::setGpuTimingEnabled(bool enabled) noexcept
{
    if (service().gpuEnabled.exchange(enabled) != enabled)
    {
        // Raw events preserve the mode transition; their additive total counts
        // enable transitions, not the current state of the switch.
        count("profiling.gpu.enabledChanges", enabled ? 1 : 0, "enable_events");
    }
}

GpuProfileSample CpuProfiler::beginGpu(uint64_t presentation, uint64_t scene, GpuProfileWorkload workload) noexcept
{
    if (!enabled() || !gpuTimingEnabled())
    {
        return {};
    }
    try
    {
        auto thread = threadState();
        if (!thread)
        {
            auto& data = service();
            std::lock_guard lock(data.mutex);
            ++data.gpuDropped;
            data.sessionGpuDropped += data.recording ? 1 : 0;
            return {};
        }
        auto& data = service();
        std::lock_guard lock(data.mutex);
        expireGpuLocked(data);
        if (!data.enabled || !data.gpuEnabled)
        {
            return {};
        }
        for (auto& pending : data.gpuPending)
        {
            if (!pending.token)
            {
                GpuProfileSample sample;
                sample.passTiming = gpuPassTimingEnabled();
                sample.detailed = gpuDetailedTimingEnabled();
                sample.token = ++data.identity;
                sample.epoch = data.epoch;
                sample.recording = data.recording ? data.recordingId.load() : 0;
                sample.thread = thread->id;
                sample.requested = nowNs();
                sample.presentationNumber = presentation;
                sample.sceneFrame = scene;
                sample.workload = workload;
                if (const auto* scope = CpuProfileScope::_current)
                {
                    sample.presentationFrame = scope->_frame;
                }
                pending = sample;
                return sample;
            }
        }
        ++data.gpuDropped;
        data.sessionGpuDropped += data.recording ? 1 : 0;
    }
    catch (...)
    {
        auto& data = service();
        std::lock_guard lock(data.mutex);
        ++data.gpuDropped;
        data.sessionGpuDropped += data.recording ? 1 : 0;
    }
    return {};
}

void CpuProfiler::completeGpu(const GpuProfileSample& sample) noexcept
{
    if (!sample.token)
    {
        return;
    }
    try
    {
        auto& data = service();
        std::lock_guard lock(data.mutex);
        for (auto& pending : data.gpuPending)
        {
            if (pending.token == sample.token)
            {
                // Epoch/session/frame/request identity was reserved by this
                // service. A late backend cannot relabel it into a new capture.
                auto completed = sample;
                completed.epoch = pending.epoch;
                completed.recording = pending.recording;
                completed.thread = pending.thread;
                completed.presentationFrame = pending.presentationFrame;
                completed.presentationNumber = pending.presentationNumber;
                completed.requested = pending.requested;
                completed.resolved = nowNs();
                if (completed.status == GpuProfileStatus::Pending ||
                    (completed.status == GpuProfileStatus::Available &&
                        (!completed.device || !completed.queue || !completed.command || !completed.submission ||
                            completed.submitted < completed.requested || completed.submitted > completed.resolved ||
                            !std::isfinite(completed.durationMs) || completed.durationMs < 0)))
                {
                    completed.status = GpuProfileStatus::Unavailable;
                }
                completed.intervalCount = std::min(completed.intervalCount, completed.maximumIntervals);
                for (uint32_t i = 0; i < completed.intervalCount; ++i)
                {
                    auto& interval = completed.intervals[i];
                    interval.label.back() = 0;
                    interval.id = i + 1;
                    if (interval.parent >= interval.id)
                    {
                        interval.parent = 0;
                    }
                    if (!interval.ended || interval.status == GpuProfileStatus::Pending ||
                        (interval.status == GpuProfileStatus::Available &&
                            (!std::isfinite(interval.durationMs) || interval.durationMs < 0 ||
                                completed.status != GpuProfileStatus::Available)))
                    {
                        interval.status = GpuProfileStatus::Unavailable;
                    }
                }
                pending.token = 0;
                gpuAcceptLocked(data, completed);
                return;
            }
        }
        ++data.gpuLate;
    }
    catch (...)
    {
        ++service().dropped;
    }
}

void CpuProfiler::gpuQueueStatus(const GpuProfileQueueStatus& status) noexcept
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    for (size_t i = 0; i < data.gpuQueueCount; ++i)
    {
        if (data.gpuQueues[i].device == status.device && data.gpuQueues[i].queue == status.queue)
        {
            data.gpuQueues[i] = status;
            return;
        }
    }
    if (data.gpuQueueCount < data.gpuQueues.size())
    {
        data.gpuQueues[data.gpuQueueCount++] = status;
    }
    else
    {
        ++data.gpuDropped;
    }
}

CpuProfileRecordingStatus CpuProfiler::recordingStatus()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    CpuProfileRecordingStatus result;
    result.active = data.recording;
    result.session = data.recordingId;
    result.begin = data.startTime;
    result.end = data.endTime;
    result.path = data.path;
    result.error = data.error;
    return result;
}

CpuProfileSnapshot CpuProfiler::snapshot(uint64_t since)
{
    auto& data = service();
    CpuProfileSnapshot result;
    std::deque<std::shared_ptr<const Batch>> history;
    {
        std::lock_guard lock(data.mutex);
        history = data.history;
        result.evicted = data.evicted;
        result.dropped = data.dropped;
        result.writerDropped = data.writerDropped;
        result.partial = data.partial;
        result.gpuDropped = data.gpuDropped;
        result.gpuFailed = data.gpuFailed;
        result.gpuUnsupported = data.gpuUnsupported;
        result.gpuEvicted = data.gpuEvicted;
        result.gpuLate = data.gpuLate;
        for (const auto& sample : data.gpuHistory)
        {
            if (sample.requested >= since && nowNs() - sample.resolved <= historyNs)
            {
                result.gpuSamples.push_back(sample);
            }
        }
        for (const auto& pending : data.gpuPending)
        {
            result.gpuPending += pending.token ? 1 : 0;
        }
        result.gpuQueues.assign(data.gpuQueues.begin(), data.gpuQueues.begin() + data.gpuQueueCount);
    }
    const auto now = nowNs();
    for (const auto& batch : history)
    {
        for (const auto& sample : batch->samples)
        {
            if (now - sample.end <= historyNs && sample.begin >= since)
            {
                result.samples.push_back(sample);
                result.begin = result.begin ? std::min(result.begin, sample.begin) : sample.begin;
                result.end = std::max(result.end, sample.end);
            }
        }
    }
    std::map<std::tuple<uint64_t, uint64_t, std::string, std::string, CpuProfileSample::Kind>, size_t> groups;
    for (const auto& sample : result.samples)
    {
        if (sample.kind == CpuProfileSample::Kind::Frame)
        {
            if (sample.complete)
            {
                ++result.presentationFrames;
                result.sceneFrames += sample.sceneRendered ? 1 : 0;
            }
            else
            {
                ++result.incompleteFrames;
            }
            continue;
        }
        const auto key = std::make_tuple(sample.epoch, sample.thread, sample.label, sample.unit, sample.kind);
        auto [it, inserted] = groups.emplace(key, result.totals.size());
        if (inserted)
        {
            auto total = sample;
            total.id = total.parent = total.presentationFrame = total.sceneFrame = 0;
            total.recording = total.presentationNumber = 0;
            result.totals.push_back(std::move(total));
        }
        else
        {
            auto& total = result.totals[it->second];
            total.begin = std::min(total.begin, sample.begin);
            total.end = std::max(total.end, sample.end);
            total.calls += sample.calls;
            total.inclusiveMs += sample.inclusiveMs;
            total.exclusiveMs += sample.exclusiveMs;
            total.maximumMs = std::max(total.maximumMs, sample.maximumMs);
            total.value += sample.value;
            total.maximumValue = std::max(total.maximumValue, sample.maximumValue);
        }
    }
    return result;
}

void CpuProfiler::publish() noexcept
{
    try
    {
        if (auto state = threadState())
        {
            flush(state);
        }
    }
    catch (...)
    {
        ++service().dropped;
    }
}

void CpuProfiler::count(const char* name, uint64_t value, const char* unit) noexcept
{
    if (!enabled() || !name)
    {
        return;
    }
    try
    {
        CpuProfileSample sample;
        sample.kind = CpuProfileSample::Kind::Counter;
        sample.epoch = session();
        sample.recording = recordingSession();
        sample.begin = sample.end = nowNs();
        sample.label = bounded(name);
        sample.unit = bounded(unit);
        sample.value = sample.maximumValue = value;
        if (const auto* scope = CpuProfileScope::_current)
        {
            sample.parent = scope->_id;
            sample.presentationFrame = scope->_frame;
        }
        append(std::move(sample));
    }
    catch (...)
    {
        ++service().dropped;
    }
}

std::filesystem::path CpuProfiler::outputPath()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    return data.path;
}

std::string CpuProfiler::sessionInfo()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    return data.recordingId ? "session=" + std::to_string(data.recordingId) +
        " started_monotonic_ns=" + std::to_string(data.startTime) : std::string{};
}

std::string CpuProfiler::error()
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    return data.error;
}

thread_local CpuProfileScope* CpuProfileScope::_current = nullptr;

CpuProfileScope::CpuProfileScope(const char* label, bool reportBoundary) noexcept
    : _label(label), _boundary(reportBoundary), _active(enabled())
{
    if (_active)
    {
        _parent = _current;
        _current = this;
        begin();
        _frame = _boundary ? _id : (_parent ? _parent->_frame : 0);
    }
}

CpuProfileScope::~CpuProfileScope() noexcept
{
    if (_active)
    {
        finish();
        _current = _parent;
        if (_boundary && enabled() && _epoch == CpuProfiler::session())
        {
            try
            {
                CpuProfileSample sample;
                sample.kind = CpuProfileSample::Kind::Frame;
                sample.epoch = _epoch;
                sample.recording = _recording == CpuProfiler::recordingSession() ? _recording : 0;
                sample.presentationFrame = _frame;
                sample.sceneFrame = _scene;
                sample.id = _frame;
                sample.presentationNumber = _presentation;
                sample.begin = std::chrono::duration_cast<std::chrono::nanoseconds>(_start.time_since_epoch()).count();
                sample.end = nowNs();
                sample.complete = _complete;
                sample.sceneRendered = _sceneRendered;
                append(std::move(sample));
            }
            catch (...)
            {
                ++service().dropped;
            }
        }
        if (!_parent || _boundary)
        {
            CpuProfiler::publish();
        }
    }
}

void CpuProfileScope::begin() noexcept
{
    auto& data = service();
    std::lock_guard lock(data.mutex);
    _epoch = data.epoch;
    _recording = data.recording ? data.recordingId.load() : 0;
    ++data.openScopes;
    if (_recording)
    {
        ++data.recordingScopes;
    }
    _id = ++service().identity;
    _children = 0;
    _start = Clock::now();
}

void CpuProfileScope::finish() noexcept
{
    const auto end = Clock::now();
    const double ms = std::chrono::duration<double, std::milli>(end - _start).count();
    if (_parent)
    {
        _parent->_children += ms;
    }
    struct FinishGuard {
        uint64_t recording;
        bool retired = false;

        ~FinishGuard()
        {
            if (retired)
            {
                return;
            }
            auto& data = service();
            std::lock_guard lock(data.mutex);
            --data.openScopes;
            if (recording && data.recording && recording == data.recordingId)
            {
                --data.recordingScopes;
            }
        }
    } guard{_recording};
    try
    {
        if (_epoch != CpuProfiler::session() || !enabled())
        {
            return;
        }
        CpuProfileSample sample;
        sample.epoch = _epoch;
        sample.recording = _recording;
        sample.id = _id;
        sample.parent = _parent ? _parent->_id : 0;
        sample.presentationFrame = _frame;
        sample.begin = std::chrono::duration_cast<std::chrono::nanoseconds>(_start.time_since_epoch()).count();
        sample.end = std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch()).count();
        sample.inclusiveMs = sample.maximumMs = ms;
        sample.exclusiveMs = std::max(0.0, ms - _children);
        sample.label = bounded(_label);
        sample.unit = "ms";
        sample.complete = !_boundary || _complete;
        if (_recording != CpuProfiler::recordingSession())
        {
            sample.recording = 0;
        }
        append(std::move(sample), &guard.retired, _recording);
    }
    catch (...)
    {
        ++service().dropped;
    }
}

void CpuProfileScope::next(const char* label) noexcept
{
    if (_active)
    {
        finish();
        _label = label;
        begin();
    }
}

void CpuProfileScope::frameCompleted(uint64_t presentation, uint64_t scene, bool sceneRendered) noexcept
{
    _presentation = presentation;
    _scene = scene;
    _sceneRendered = sceneRendered;
    _complete = true;
}

bool CpuProfileScope::enabled()
{
    return CpuProfiler::enabled();
}

void CpuProfileScope::count(const char* name, uint64_t value, const char* unit) noexcept
{
    CpuProfiler::count(name, value, unit);
}
}
