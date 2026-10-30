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

#include "Dispatcher.hpp"

#include <limits>
#include <bg2e/plugin/PluginManager.hpp>
#include <stdexcept>
#include <utility>

namespace bg2e_composer::command {
namespace {
using namespace bg2e::app;

// Allocated before queue admission. Its intrusive retirement link avoids
// allocating inside the release sink's noexcept ownership transfer.
class Envelope final : public Command {
  public:
    Envelope(std::unique_ptr<Command> command, Effects effects,
        Dispatcher::EffectHandler handler, std::function<bool()> valid)
        : _command(std::move(command)), _effects(effects), _handler(std::move(handler)), _valid(std::move(valid))
    {
        auto& plugins = bg2e::plugin::PluginManager::get();
        if (!plugins.plugins().empty())
        {
            // Hold registries/modules stable while pending/history callbacks and
            // weak control blocks exist. Unload (and new loading) returns Busy.
            _registryLease = plugins.registryReadLease();
        }
    }

    const CommandInfo& info() const noexcept override
    {
        return _command->info();
    }

    CommandMemory memory() const override
    {
        auto result = _command->memory();
        constexpr size_t overhead = sizeof(*this) + 256;
        if (result.localBytes > std::numeric_limits<size_t>::max() - overhead)
        {
            throw std::length_error("Composer command memory overflow");
        }
        result.localBytes += overhead;
        return result;
    }

    CommandResult execute() noexcept override
    {
        return apply(0);
    }

    CommandResult undo() noexcept override
    {
        return apply(1);
    }

    CommandResult redo() noexcept override
    {
        return apply(2);
    }

    std::unique_ptr<Command> retiredNext;
    CommandMemory retiredMemory;

  private:
    // Declared first so command-owned objects/callbacks die before the lease.
    std::shared_ptr<void> _registryLease;
    std::unique_ptr<Command> _command;
    Effects _effects;
    Dispatcher::EffectHandler _handler;
    std::function<bool()> _valid;

    CommandResult apply(int operation) noexcept
    {
        try
        {
            if (!_valid())
            {
                return {CommandStatus::Rejected, "Composer command target/document is stale"};
            }
        }
        catch (...)
        {
            return {CommandStatus::Rejected, "Composer target validation failed"};
        }
        const auto result = operation == 0 ? _command->execute() :
            operation == 1 ? _command->undo() : _command->redo();
        if (result.status != CommandStatus::Applied)
        {
            return result;
        }
        try
        {
            _handler(_effects);
            return result;
        }
        catch (...)
        {
            const auto recovery = operation == 1 ? _command->redo() : _command->undo();
            if (recovery.status != CommandStatus::Applied)
            {
                return {CommandStatus::Faulted, "Composer effects failed and model recovery failed"};
            }
            try
            {
                _handler(_effects);
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Composer effects recovery failed"};
            }
            return {CommandStatus::Failed, "Composer effects failed; edit restored"};
        }
    }
};
}

struct Dispatcher::Retirement final : bg2e::app::CommandReleaseSink {
    std::unique_ptr<bg2e::app::Command> head;
    bool resourcesPending = false;

    void release(std::unique_ptr<bg2e::app::Command> command, bg2e::app::CommandMemory memory) noexcept override
    {
        auto* wrapped = dynamic_cast<Envelope*>(command.get());
        if (!wrapped)
        {
            // The mutable ledger is for document bookkeeping. Unwrapped CPU
            // interactions must already be at their caller-owned safe boundary.
            return;
        }
        resourcesPending = resourcesPending || !memory.resources.empty();
        auto& envelope = *wrapped;
        envelope.retiredMemory = std::move(memory);
        envelope.retiredNext = std::move(head);
        head = std::move(command);
    }

    void drain() noexcept
    {
        resourcesPending = false;
        while (head)
        {
            auto current = std::move(head);
            head = std::move(static_cast<Envelope&>(*current).retiredNext);
        }
    }

    ~Retirement() override
    {
        drain();
    }
};

Dispatcher::Dispatcher(uint64_t generation, bool saved, Scheduler scheduler,
    EffectHandler effects, Completion completion)
    : _retirement(std::make_shared<Retirement>()), _scheduler(std::move(scheduler)),
      _effects(std::move(effects)), _completion(std::move(completion))
{
    _history = std::make_unique<bg2e::app::CommandHistory>(bg2e::app::CommandHistoryLimits{},
        [this](std::string_view category) -> std::optional<bool>
        {
            return category == "selection" ? std::optional<bool>(_selectionHistory) : std::nullopt;
        }, _retirement);
    if (generation > 1)
    {
        const auto result = _history->resetBaseline(generation, saved);
        if (!result.accepted())
        {
            throw std::logic_error("Cannot initialize Composer document history");
        }
    }
    if (!saved)
    {
        _history->noteNonhistoricalChange(bg2e::app::CommandCompatibility::Preserve);
    }
    _queue = std::make_unique<bg2e::app::CommandQueue>(*_history,
        [this](const auto* info, const auto* context, auto run, auto cancel)
        {
            // Replay/control operations always use a conservative boundary.
            const auto category = info ? _boundaries.find(info->category) : _boundaries.end();
            const auto boundary = category != _boundaries.end() ? category->second : Boundary::Conservative;
            _scheduler(boundary, context, std::move(run), std::move(cancel));
        });
}

Dispatcher::~Dispatcher()
{
    _queue.reset();
    _history.reset();
    _retirement->drain();
}

bg2e::app::CommandResult Dispatcher::submit(std::unique_ptr<bg2e::app::Command> command,
    Boundary boundary, Effects effects, std::function<bool()> valid, Completion completion, bg2e::app::CommandOrigin origin)
{
    const auto reject = [&](std::string_view message)
    {
        const bg2e::app::CommandResult result{bg2e::app::CommandStatus::Rejected, message};
        try
        {
            if (_completion)
            {
                _completion(result);
            }
        }
        catch (...)
        {
        }
        if (completion)
        {
            completion(result);
        }
        return result;
    };
    if (!command)
    {
        return reject("Missing Composer command");
    }
    const auto [category, inserted] = _boundaries.try_emplace(command->info().category, boundary);
    if (!inserted && category->second != boundary)
    {
        return reject("Command category has conflicting execution boundaries");
    }
    const auto compatibility = command->info().impact == bg2e::app::CommandDocumentImpact::None
        ? bg2e::app::CommandCompatibility::Preserve : bg2e::app::CommandCompatibility::Unspecified;
    auto wrapped = std::make_unique<Envelope>(std::move(command), effects, _effects, std::move(valid));
    return _queue->enqueue(std::move(wrapped), {_history->documentState().generation, origin, compatibility},
        [notify = _completion, completion = std::move(completion)](auto result)
        {
            try
            {
                if (notify)
                {
                    notify(result);
                }
            }
            catch (...)
            {
            }
            if (completion)
            {
                completion(result);
            }
        });
}

bg2e::app::CommandResult Dispatcher::beginCpuInteraction(std::unique_ptr<bg2e::app::CommandTransaction> command)
{
    if (_queue->pending())
    {
        return {bg2e::app::CommandStatus::Rejected, "Wait for pending commands before manipulating"};
    }
    return _history->beginTransaction(std::move(command), {_history->documentState().generation});
}


void Dispatcher::setSelectionHistoryEnabled(bool enabled)
{
    _selectionHistory = enabled;
}


bool Dispatcher::selectionHistoryEnabled() const
{
    return _selectionHistory;
}


void Dispatcher::undo()
{
    _queue->enqueueUndo(_completion);
}

void Dispatcher::redo()
{
    _queue->enqueueRedo(_completion);
}

void Dispatcher::pump()
{
    // CPU interactions are supervised outside the transport queue, at input
    // boundaries. Keep admitted commands ordered and reserved until adoption or
    // cancellation, rather than executing against an in-progress preview.
    if (_history->transactionActive() && !_history->faulted())
    {
        return;
    }
    _queue->pump();
}

size_t Dispatcher::pending() const
{
    return _queue->pending();
}

bg2e::app::CommandResult Dispatcher::settle(bool commit)
{
    if (_queue->pending())
    {
        return {bg2e::app::CommandStatus::Rejected, "Flush commands before settling document history"};
    }
    if (_history->transactionActive())
    {
        return commit ? _history->commitTransaction() : _history->cancelTransaction();
    }
    return _history->faulted() ? bg2e::app::CommandResult{bg2e::app::CommandStatus::Faulted,
        "Document history requires repair or replacement"} : bg2e::app::CommandResult{};
}

bg2e::app::CommandResult Dispatcher::shutdown()
{
    return _queue->shutdown();
}

bool Dispatcher::retiredResourcesPending() const
{
    return _retirement->resourcesPending;
}


void Dispatcher::releaseRetired()
{
    _retirement->drain();
}

bg2e::app::CommandHistory& Dispatcher::history()
{
    return *_history;
}

const bg2e::app::CommandHistory& Dispatcher::history() const
{
    return *_history;
}

}
