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
    CompletionRecord(std::function<bool()> poll, std::function<void()> wait,
        std::function<void(bool)> terminal = {})
        : _poll(std::move(poll)), _wait(std::move(wait)), _onTerminal(std::move(terminal))
    {
    }

    bool completed()
    {
        std::lock_guard lock(_mutex);
        return pollLocked();
    }

    // Profiling must not block behind a normal wait() holding this mutex.
    bool pollNonblocking()
    {
        std::unique_lock lock(_mutex, std::try_to_lock);
        return lock.owns_lock() && pollLocked();
    }

    void wait()
    {
        std::lock_guard lock(_mutex);
        if (!_terminal)
        {
            try
            {
                _wait();
                finish();
            }
            catch (...)
            {
                fail(std::current_exception());
            }
        }
        if (_error)
        {
            std::rethrow_exception(_error);
        }
    }

    // Submission failed before GPU admission; never wait on its native fence.
    void cancel(std::exception_ptr error)
    {
        std::lock_guard lock(_mutex);
        fail(error);
    }

    uint64_t id = 0;

  private:
    bool pollLocked()
    {
        if (!_terminal)
        {
            try
            {
                if (_poll())
                {
                    finish();
                }
            }
            catch (...)
            {
                fail(std::current_exception());
            }
        }
        if (_error)
        {
            std::rethrow_exception(_error);
        }
        return _terminal;
    }

    void finish(bool success = true)
    {
        _terminal = true;
        if (_onTerminal)
        {
            try
            {
                _onTerminal(success);
            }
            catch (...)
            {
                // Optional diagnostics must not change completion semantics.
            }
            _onTerminal = {};
        }
        _poll = {};
        _wait = {};
    }
    void fail(std::exception_ptr error)
    {
        _error = error;
        finish(false);
    }
    std::mutex _mutex;
    bool _terminal = false;
    std::exception_ptr _error;
    std::function<bool()> _poll;
    std::function<void()> _wait;
    std::function<void(bool)> _onTerminal;
};

// A recording is not a submission. Abandoned/reset recordings never become
// ready; only a successful send supplies a completion record. Polling never waits.
class BG2E_API RecordingCompletion {
  public:
    bool completed() const
    {
        std::shared_ptr<CompletionRecord> record;
        {
            std::lock_guard lock(_mutex);
            record = _submission;
        }
        return record && record->completed();
    }

    bool wasSubmitted() const
    {
        std::lock_guard lock(_mutex);
        return bool(_submission);
    }

    // Abandoned recordings are safe to recycle, but are never completed builds.
    bool retired() const
    {
        std::shared_ptr<CompletionRecord> record;
        {
            std::lock_guard lock(_mutex);
            record = _submission;
            if (!record)
            {
                return _abandoned;
            }
        }
        return record->completed();
    }

    void abandon()
    {
        std::lock_guard lock(_mutex);
        _abandoned = true;
    }

    void submitted(const std::shared_ptr<CompletionRecord>& record)
    {
        std::lock_guard lock(_mutex);
        _submission = record;
    }

  private:
    mutable std::mutex _mutex;
    std::shared_ptr<CompletionRecord> _submission;
    bool _abandoned = false;
};

class BG2E_API SubmissionState {
  public:
    // Shared by all queues, including wrappers aliasing one native queue.
    // The guard covers each native submit transaction, not GPU execution.
    std::unique_lock<std::mutex> admit()
    {
        std::unique_lock lock(_mutex);
        _changed.wait(lock, [&]
                      {
                          return !_draining;
                      });
        if (_closed)
        {
            throw std::logic_error("GPU device is closed to submissions");
        }
        retire();
        return lock;
    }

    // Caller holds admit() through registration and native submit.
    void track(const std::shared_ptr<CompletionRecord>& record)
    {
        record->id = ++_nextId;
        _pending.push_back(record);
    }

    void cancel(const std::shared_ptr<CompletionRecord>& record, std::exception_ptr error)
    {
        record->cancel(error);
        for (auto it = _pending.begin(); it != _pending.end(); ++it)
        {
            if (*it == record)
            {
                _pending.erase(it);
                break;
            }
        }
    }

    std::vector<std::shared_ptr<CompletionRecord>> snapshot()
    {
        std::unique_lock lock(_mutex);
        _changed.wait(lock, [&]
                      {
                          return !_draining;
                      });
        retire();
        return _pending;
    }

    void pollNonblocking() noexcept
    {
        std::unique_lock lock(_mutex, std::try_to_lock);
        if (!lock.owns_lock() || _draining)
        {
            return;
        }
        for (auto it = _pending.begin(); it != _pending.end();)
        {
            try
            {
                if ((*it)->pollNonblocking())
                {
                    it = _pending.erase(it);
                    continue;
                }
            }
            catch (...)
            {
                // Preserve failed records for normal admission/drain error reporting.
            }
            ++it;
        }
    }

    void drain(std::function<void()> nativeWait = {}, bool close = false)
    {
        std::vector<std::shared_ptr<CompletionRecord>> pending;
        {
            std::unique_lock lock(_mutex);
            _changed.wait(lock, [&]
                          {
                              return !_draining;
                          });
            _draining = true;
            if (close)
            {
                _closed = true;
            }
            pending.swap(_pending);
        }
        std::exception_ptr error;
        try
        {
            if (nativeWait)
            {
                nativeWait();
            }
        }
        catch (...)
        {
            error = std::current_exception();
        }
        // Drain every record, even if one reports an error.
        for (const auto& record : pending)
        {
            try
            {
                record->wait();
            }
            catch (...)
            {
                if (!error)
                {
                    error = std::current_exception();
                }
            }
        }
        {
            std::lock_guard lock(_mutex);
            if (error)
            {
                _closed = true;
            }
            _draining = false;
        }
        _changed.notify_all();
        if (error)
        {
            std::rethrow_exception(error);
        }
    }

  private:
    void retire()
    {
        for (auto it = _pending.begin(); it != _pending.end();)
        {
            if ((*it)->completed())
            {
                it = _pending.erase(it);
            }
            else
            {
                ++it;
            }
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
