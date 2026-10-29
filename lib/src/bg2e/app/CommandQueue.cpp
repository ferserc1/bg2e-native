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

#include <bg2e/app/CommandQueue.hpp>

#include <algorithm>
#include <list>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>

namespace bg2e::app {
namespace {
void notify(CommandQueue::Completion& completion, CommandResult result) noexcept
{
    if (completion)
    {
        try
        {
            completion(result);
        }
        catch (...)
        {
            // Completion is a notification, never part of model publication.
        }
    }
}
}

struct CommandQueue::Impl : std::enable_shared_from_this<CommandQueue::Impl> {
    enum class Kind { Execute, Undo, Redo, Begin, Update, Commit, Cancel };
    struct Job {
        Kind kind = Kind::Execute;
        std::unique_ptr<CommandReservation> reservation;
        std::any value;
        Completion completion;
        uint64_t generation = 0;
        uint64_t interaction = 0;
        size_t payload = 0;
        std::string absoluteKey;
        bool done = false;
    };

    CommandHistory* history;
    Executor executor;
    std::thread::id thread = std::this_thread::get_id();
    std::list<std::shared_ptr<Job>> jobs;
    std::shared_ptr<Job> inFlight;
    size_t payloadBytes = 0;
    size_t payloadLimit = 1024 * 1024;
    uint64_t interaction = 0;
    uint64_t lastInteraction = 0;
    uint64_t historyInteraction = 0;
    bool pumping = false;
    bool executing = false;
    bool stopping = false;

    void checkThread() const
    {
        if (thread != std::this_thread::get_id())
        {
            throw std::logic_error("CommandQueue is owner-thread only");
        }
    }

    CommandResult add(Kind kind, std::unique_ptr<CommandReservation> reservation,
        uint64_t id, std::any value, size_t payload, Completion completion)
    {
        checkThread();
        if (stopping || history->faulted() || jobs.size() >= 1024 ||
            payload > payloadLimit || payloadBytes > payloadLimit - payload)
        {
            const auto result = CommandResult{CommandStatus::Rejected, "Queue closed, faulted or over budget"};
            notify(completion, result);
            return result;
        }
        try
        {
            auto job = std::make_shared<Job>();
            job->kind = kind;
            job->reservation = std::move(reservation);
            job->value = std::move(value);
            job->generation = history->documentState().generation;
            job->interaction = id;
            job->payload = payload;
            jobs.push_back(job);
            job->completion = std::move(completion);
            payloadBytes += payload;
            return {};
        }
        catch (...)
        {
            const auto result = CommandResult{CommandStatus::Rejected, "Queue storage reservation failed"};
            notify(completion, result);
            return result;
        }
    }

    void finish(const std::shared_ptr<Job>& job, CommandResult result)
    {
        if (job->done)
        {
            return;
        }
        job->done = true;
        if (inFlight == job)
        {
            inFlight.reset();
        }
        payloadBytes -= job->payload;
        jobs.remove(job);
        // Release pending ownership before invoking arbitrary completion code.
        job->reservation.reset();
        job->value.reset();
        notify(job->completion, result);
    }

    void run(const std::shared_ptr<Job>& job)
    {
        checkThread();
        if (job->done || inFlight != job || executing)
        {
            return;
        }
        executing = true;
        CommandResult result;
        if (job->generation != history->documentState().generation)
        {
            result = {CommandStatus::Cancelled, "Queue item belongs to a stale document"};
        }
        else if (history->faulted())
        {
            result = {CommandStatus::Cancelled, "History fault cancelled dependent work"};
        }
        else
        {
            switch (job->kind)
            {
            case Kind::Execute:
                result = history->execute(std::move(job->reservation));
                break;
            case Kind::Undo:
                result = history->undo();
                break;
            case Kind::Redo:
                result = history->redo();
                break;
            case Kind::Begin:
                result = history->beginTransaction(std::move(job->reservation));
                if (result.accepted() && history->transactionActive())
                {
                    interaction = job->interaction;
                    historyInteraction = history->transactionId();
                }
                break;
            case Kind::Update:
            case Kind::Commit:
            case Kind::Cancel:
                if (!interaction || interaction != job->interaction ||
                    historyInteraction != history->transactionId() || !history->transactionActive())
                {
                    result = {CommandStatus::Rejected, "Missing or mismatched queue interaction"};
                }
                else if (job->kind == Kind::Update)
                {
                    result = history->updateTransaction(job->value);
                }
                else
                {
                    result = job->kind == Kind::Commit ? history->commitTransaction() : history->cancelTransaction();
                    if (!history->transactionActive())
                    {
                        interaction = 0;
                    }
                }
                break;
            }
        }
        const bool fault = history->faulted();
        executing = false;
        finish(job, result);
        if (fault)
        {
            cancelAll();
        }
    }

    void cancelAll()
    {
        // Detach first: completion may enqueue, but cannot recursively cancel
        // this batch. Work enqueued by notifications remains for the next pump.
        auto batch = std::move(jobs);
        jobs.clear();
        inFlight.reset();
        for (auto& job : batch)
        {
            finish(job, {CommandStatus::Cancelled, "Pending command cancelled"});
        }
    }
};

CommandQueue::CommandQueue(CommandHistory& history, Executor executor) : _impl(std::make_shared<Impl>())
{
    if (!executor)
    {
        throw std::invalid_argument("CommandQueue requires a safe-boundary executor");
    }
    _impl->history = &history;
    _impl->executor = std::move(executor);
}

CommandQueue::~CommandQueue()
{
    shutdown();
}

CommandResult CommandQueue::enqueue(std::unique_ptr<Command> command, CommandContext context, Completion completion)
{
    _impl->checkThread();
    auto admission = _impl->history->prepare(std::move(command), context);
    if (!admission.reservation)
    {
        notify(completion, admission.result);
        return admission.result;
    }
    return _impl->add(Impl::Kind::Execute, std::move(admission.reservation), 0, {}, 0, std::move(completion));
}

CommandResult CommandQueue::enqueueUndo(Completion completion)
{
    return _impl->add(Impl::Kind::Undo, {}, 0, {}, 0, std::move(completion));
}

CommandResult CommandQueue::enqueueRedo(Completion completion)
{
    return _impl->add(Impl::Kind::Redo, {}, 0, {}, 0, std::move(completion));
}

CommandResult CommandQueue::enqueueBegin(std::unique_ptr<CommandTransaction> command,
    uint64_t interaction, CommandContext context, Completion completion)
{
    _impl->checkThread();
    // Strictly increasing IDs prevent late updates from binding to a new edit.
    if (interaction == 0 || interaction <= _impl->lastInteraction)
    {
        auto result = CommandResult{CommandStatus::Rejected, "Interaction IDs must increase"};
        // Consume rejected commands through history's release boundary as well.
        auto discarded = _impl->history->prepare(std::move(command), context);
        discarded.reservation.reset();
        notify(completion, result);
        return result;
    }
    auto admission = _impl->history->prepare(std::move(command), context);
    if (!admission.reservation)
    {
        notify(completion, admission.result);
        return admission.result;
    }
    auto result = _impl->add(Impl::Kind::Begin, std::move(admission.reservation), interaction, {}, 0, std::move(completion));
    if (result.accepted())
    {
        _impl->lastInteraction = interaction;
    }
    return result;
}

CommandResult CommandQueue::enqueueUpdate(uint64_t interaction, std::any value,
    size_t payloadBytes, Completion completion)
{
    return _impl->add(Impl::Kind::Update, {}, interaction, std::move(value), payloadBytes, std::move(completion));
}

CommandResult CommandQueue::enqueueAbsoluteUpdate(uint64_t interaction, std::string targetProperty,
    std::any value, size_t payloadBytes, Completion completion)
{
    auto core = _impl;
    core->checkThread();
    if (targetProperty.capacity() > std::numeric_limits<size_t>::max() - payloadBytes)
    {
        const auto result = CommandResult{CommandStatus::Rejected, "Absolute update size overflow"};
        notify(completion, result);
        return result;
    }
    payloadBytes += targetProperty.capacity();
    if (targetProperty.empty())
    {
        const auto result = CommandResult{CommandStatus::Rejected, "Absolute updates require target/property identity"};
        notify(completion, result);
        return result;
    }
    if (!core->stopping && !core->history->faulted() && !core->jobs.empty())
    {
        auto job = core->jobs.back();
        if (job != core->inFlight && job->kind == Impl::Kind::Update &&
            job->interaction == interaction && job->absoluteKey == targetProperty &&
            job->generation == core->history->documentState().generation)
        {
            if (payloadBytes > core->payloadLimit ||
                core->payloadBytes - job->payload > core->payloadLimit - payloadBytes)
            {
                const auto result = CommandResult{CommandStatus::Rejected, "Queue payload budget exceeded"};
                notify(completion, result);
                return result;
            }
            auto replaced = std::move(job->completion);
            core->payloadBytes = core->payloadBytes - job->payload + payloadBytes;
            job->payload = payloadBytes;
            job->value = std::move(value);
            job->completion = std::move(completion);
            notify(replaced, {CommandStatus::NoOp, "Superseded absolute transaction update"});
            return {};
        }
    }
    auto result = core->add(Impl::Kind::Update, {}, interaction, std::move(value), payloadBytes, std::move(completion));
    if (result.accepted())
    {
        core->jobs.back()->absoluteKey = std::move(targetProperty);
    }
    return result;
}

CommandResult CommandQueue::enqueueCommit(uint64_t interaction, Completion completion)
{
    return _impl->add(Impl::Kind::Commit, {}, interaction, {}, 0, std::move(completion));
}

CommandResult CommandQueue::enqueueCancel(uint64_t interaction, Completion completion)
{
    return _impl->add(Impl::Kind::Cancel, {}, interaction, {}, 0, std::move(completion));
}

void CommandQueue::pump()
{
    auto core = _impl; // Completion may release the queue object itself.
    core->checkThread();
    if (core->stopping || core->pumping || core->executing || core->inFlight)
    {
        return;
    }
    core->pumping = true;
    // Bound synchronous work even if completions continually enqueue commands.
    size_t remaining = core->jobs.size();
    while (!core->stopping && !core->inFlight && !core->jobs.empty() && remaining--)
    {
        if (core->history->faulted())
        {
            core->cancelAll();
            break;
        }
        auto job = core->jobs.front();
        core->inFlight = job;
        const auto weak = std::weak_ptr<Impl>(core);
        const auto weakJob = std::weak_ptr<Impl::Job>(job);
        try
        {
            core->executor(job->reservation ? &job->reservation->info() : nullptr,
                job->reservation ? &job->reservation->context() : nullptr,
                [weak, weakJob]
                {
                    if (auto owner = weak.lock())
                    {
                        if (auto pending = weakJob.lock())
                        {
                            owner->run(pending);
                        }
                    }
                },
                [weak, weakJob]
                {
                    if (auto owner = weak.lock())
                    {
                        owner->checkThread();
                        if (auto pending = weakJob.lock(); pending && !owner->executing)
                        {
                            owner->finish(pending, {CommandStatus::Cancelled, "Executor cancelled stale work"});
                        }
                    }
                });
        }
        catch (...)
        {
            core->finish(job, {CommandStatus::Cancelled, "Safe-boundary scheduling failed"});
        }
    }
    core->pumping = false;
}

void CommandQueue::cancelPending()
{
    auto core = _impl;
    core->checkThread();
    if (core->executing)
    {
        throw std::logic_error("Cannot cancel the queue during model execution");
    }
    core->cancelAll();
}

CommandResult CommandQueue::shutdown()
{
    auto core = _impl;
    core->checkThread();
    if (core->executing)
    {
        return {CommandStatus::Rejected, "Cannot shut down during model execution"};
    }
    core->stopping = true;
    core->cancelAll();
    if (core->interaction && core->historyInteraction == core->history->transactionId() &&
        core->history->transactionActive())
    {
        auto result = core->history->cancelTransaction();
        if (!core->history->transactionActive())
        {
            core->interaction = 0;
        }
        return result;
    }
    return {};
}

size_t CommandQueue::pending() const
{
    _impl->checkThread();
    return _impl->jobs.size();
}

void CommandQueue::setPayloadLimit(size_t bytes)
{
    _impl->checkThread();
    if (bytes < _impl->payloadBytes)
    {
        throw std::invalid_argument("Pending payload exceeds new queue limit");
    }
    _impl->payloadLimit = bytes;
}

}
