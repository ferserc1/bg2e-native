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
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <utility>

namespace bg2e::gpu::detail {

// One immutable submission, independent of the lifetime/reuse of its wrapper.
class BG2E_API CompletionRecord {
public:
    CompletionRecord(std::function<bool()> poll, std::function<void()> wait)
        : _poll(std::move(poll)), _wait(std::move(wait)) {}

    bool completed() {
        std::lock_guard lock(_mutex);
        if (!_terminal) {
            try { if (_poll()) finish(); }
            catch (...) { fail(std::current_exception()); }
        }
        if (_error) std::rethrow_exception(_error);
        return _terminal;
    }

    void wait() {
        std::lock_guard lock(_mutex);
        if (!_terminal) {
            try { _wait(); finish(); }
            catch (...) { fail(std::current_exception()); }
        }
        if (_error) std::rethrow_exception(_error);
    }

    // Submission failed before GPU admission; never wait on its native fence.
    void cancel(std::exception_ptr error) {
        std::lock_guard lock(_mutex);
        fail(error);
    }

    uint64_t id = 0;

private:
    void finish() { _terminal = true; _poll = {}; _wait = {}; }
    void fail(std::exception_ptr error) { _error = error; finish(); }
    std::mutex _mutex;
    bool _terminal = false;
    std::exception_ptr _error;
    std::function<bool()> _poll;
    std::function<void()> _wait;
};

class BG2E_API SubmissionState {
public:
    // Shared by all queues, including wrappers aliasing one native queue.
    // The guard covers each native submit transaction, not GPU execution.
    std::unique_lock<std::mutex> admit() {
        std::unique_lock lock(_mutex);
        _changed.wait(lock, [&] { return !_draining; });
        if (_closed) throw std::logic_error("GPU device is closed to submissions");
        retire();
        return lock;
    }

    // Caller holds admit() through registration and native submit.
    void track(const std::shared_ptr<CompletionRecord>& record) {
        record->id = ++_nextId;
        _pending.push_back(record);
    }

    void cancel(const std::shared_ptr<CompletionRecord>& record, std::exception_ptr error) {
        record->cancel(error);
        for (auto it = _pending.begin(); it != _pending.end(); ++it) {
            if (*it == record) { _pending.erase(it); break; }
        }
    }

    std::vector<std::shared_ptr<CompletionRecord>> snapshot() {
        std::unique_lock lock(_mutex);
        _changed.wait(lock, [&] { return !_draining; });
        retire();
        return _pending;
    }

    void drain(std::function<void()> nativeWait = {}, bool close = false) {
        std::vector<std::shared_ptr<CompletionRecord>> pending;
        {
            std::unique_lock lock(_mutex);
            _changed.wait(lock, [&] { return !_draining; });
            _draining = true;
            if (close) _closed = true;
            pending.swap(_pending);
        }
        std::exception_ptr error;
        try { if (nativeWait) nativeWait(); }
        catch (...) { error = std::current_exception(); }
        // Drain every record, even if one reports an error.
        for (const auto& record : pending) {
            try { record->wait(); }
            catch (...) { if (!error) error = std::current_exception(); }
        }
        {
            std::lock_guard lock(_mutex);
            if (error) _closed = true;
            _draining = false;
        }
        _changed.notify_all();
        if (error) std::rethrow_exception(error);
    }

private:
    void retire() {
        for (auto it = _pending.begin(); it != _pending.end();) {
            if ((*it)->completed()) it = _pending.erase(it);
            else ++it;
        }
    }
    std::mutex _mutex;
    std::condition_variable _changed;
    bool _draining = false;
    bool _closed = false;
    uint64_t _nextId = 0;
    std::vector<std::shared_ptr<CompletionRecord>> _pending;
};

}
