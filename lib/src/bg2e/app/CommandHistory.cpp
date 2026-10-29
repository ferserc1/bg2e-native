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

#include <bg2e/app/CommandHistory.hpp>

#include <algorithm>
#include <list>
#include <stdexcept>
#include <thread>
#include <utility>

namespace bg2e::app {
namespace {
constexpr auto exhausted = CommandResult{CommandStatus::Rejected, "Command history budget exceeded"};
constexpr auto blocked = CommandResult{CommandStatus::Rejected, "History is busy or has an active transaction"};
constexpr auto broken = CommandResult{CommandStatus::Faulted, "History requires explicit repair or document replacement"};

struct BusyGuard {
    bool& busy;

    explicit BusyGuard(bool& value) : busy(value)
    {
        busy = true;
    }

    ~BusyGuard()
    {
        busy = false;
    }
};
}

struct CommandHistory::Impl {
    struct Entry {
        Impl* owner;
        std::unique_ptr<Command> command;
        CommandInfo info;
        CommandContext context;
        CommandMemory memory;
        bool record = false;
        bool registered = false;
        CommandState state = CommandState::Pending;
        uint64_t before = 0;
        uint64_t after = 0;
        std::list<Entry*>::iterator registration;

        explicit Entry(Impl* storage) : owner(storage)
        {
        }

        ~Entry()
        {
            if (registered)
            {
                owner->live.erase(registration);
            }
            if (command && owner->release)
            {
                owner->release->release(std::move(command), std::move(memory));
            }
        }
    };

    using Entries = std::list<std::unique_ptr<Entry>>;
    CommandHistoryLimits limits;
    CommandPreferenceResolver preferences;
    std::shared_ptr<CommandReleaseSink> release;
    std::thread::id thread = std::this_thread::get_id();
    std::list<Entry*> live;
    Entries entries;
    Entries active;
    Entries faults;
    size_t cursor = 0;
    CommandDocumentState current;
    CommandDocumentState saved;
    uint64_t nextState = 2;
    bool busy = false;
    bool fault = false;
    bool closed = false;

    void checkThread() const
    {
        if (thread != std::this_thread::get_id())
        {
            throw std::logic_error("CommandHistory is owner-thread only");
        }
    }

    bool available() const noexcept
    {
        return !busy && !fault && !closed && active.empty();
    }

    // No allocation: also used after swapping a proposed transaction estimate.
    std::optional<size_t> bytes() const noexcept
    {
        size_t total = 0;
        const auto add = [&total](size_t bytes)
        {
            if (bytes > std::numeric_limits<size_t>::max() - total)
            {
                return false;
            }
            total += bytes;
            return true;
        };
        for (auto entry : live)
        {
            if (!add(sizeof(Entry)) || !add(entry->info.name.capacity()) ||
                !add(entry->info.category.capacity()) || !add(entry->memory.localBytes) ||
                entry->memory.resources.capacity() > std::numeric_limits<size_t>::max() / sizeof(CommandResource) ||
                !add(entry->memory.resources.capacity() * sizeof(CommandResource)))
            {
                return {};
            }
            for (size_t i = 0; i < entry->memory.resources.size(); ++i)
            {
                const auto& resource = entry->memory.resources[i];
                if (!resource.identity)
                {
                    return {};
                }
                bool seen = false;
                for (auto previous : live)
                {
                    const auto count = previous == entry ? i : previous->memory.resources.size();
                    for (size_t j = 0; j < count; ++j)
                    {
                        const auto& other = previous->memory.resources[j];
                        if (other.identity.get() == resource.identity.get())
                        {
                            if (other.bytes != resource.bytes)
                            {
                                return {};
                            }
                            seen = true;
                        }
                    }
                    if (previous == entry)
                    {
                        break;
                    }
                }
                if (!seen && !add(resource.bytes))
                {
                    return {};
                }
            }
        }
        return total;
    }

    bool fits() const noexcept
    {
        const auto used = bytes();
        return used && *used <= limits.bytes && live.size() <= limits.entries;
    }

    bool refreshMemory(Entry& entry) noexcept
    {
        try
        {
            auto estimate = entry.command->memory();
            std::swap(entry.memory, estimate);
            if (!fits())
            {
                std::swap(entry.memory, estimate);
                return false;
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    auto at(size_t index)
    {
        auto it = entries.begin();
        std::advance(it, index);
        return it;
    }

    void eraseRedo() noexcept
    {
        entries.erase(at(cursor), entries.end());
    }

    void fail(Entries& source) noexcept
    {
        fault = true;
        source.front()->state = CommandState::Faulted;
        faults.splice(faults.end(), source);
    }

    // All fallible storage was reserved at admission. List splice and token
    // publication cannot allocate. Failed/no-op actions never reach this path.
    void publish(Entries& source) noexcept
    {
        auto& entry = *source.front();
        if (entry.record)
        {
            eraseRedo();
            entry.before = current.state;
            if (entry.info.impact == CommandDocumentImpact::Document)
            {
                current.state = entry.after;
            }
            else
            {
                entry.after = current.state;
            }
            entry.state = CommandState::Applied;
            entries.splice(entries.end(), source);
            ++cursor;
        }
        else
        {
            if (entry.context.compatibility == CommandCompatibility::ResetHistory)
            {
                entries.clear();
                cursor = 0;
                current.state = entry.after;
            }
            else if (entry.context.compatibility == CommandCompatibility::InvalidateRedo)
            {
                eraseRedo();
            }
            if (entry.info.impact == CommandDocumentImpact::Document)
            {
                ++current.nonhistoricalRevision;
            }
            entry.state = CommandState::Discarded;
            source.clear();
        }
    }
};

struct CommandReservation::Impl {
    // Entries must die before their storage/release adapter.
    std::shared_ptr<CommandHistory::Impl> owner;
    CommandHistory::Impl::Entries entries;
};

CommandReservation::CommandReservation(std::unique_ptr<Impl> impl) : _impl(std::move(impl))
{
}

CommandReservation::~CommandReservation() = default;

CommandReservation::CommandReservation(CommandReservation&&) noexcept = default;

CommandReservation& CommandReservation::operator=(CommandReservation&&) noexcept = default;

const CommandInfo& CommandReservation::info() const noexcept
{
    return _impl->entries.front()->info;
}

const CommandContext& CommandReservation::context() const noexcept
{
    return _impl->entries.front()->context;
}

CommandHistory::CommandHistory(CommandHistoryLimits limits, CommandPreferenceResolver preferences,
    std::shared_ptr<CommandReleaseSink> release) : _impl(std::make_shared<Impl>())
{
    _impl->limits = limits;
    _impl->preferences = std::move(preferences);
    _impl->release = std::move(release);
}

CommandHistory::~CommandHistory()
{
    // Destruction is an owner-safe boundary, just like explicit shutdown.
    _impl->checkThread();
    if (!_impl->active.empty() && !_impl->fault)
    {
        cancelTransaction();
    }
    _impl->closed = true;
    BusyGuard guard(_impl->busy);
    _impl->active.clear();
    _impl->entries.clear();
    _impl->faults.clear();
}

CommandAdmission CommandHistory::prepare(std::unique_ptr<Command> command, CommandContext context)
{
    auto& core = *_impl;
    core.checkThread();
    // Own and retire even rejected commands via the caller's release adapter.
    std::unique_ptr<Impl::Entry> entry;
    try
    {
        entry = std::make_unique<Impl::Entry>(&core);
    }
    catch (...)
    {
        if (command && core.release)
        {
            core.release->release(std::move(command), {});
        }
        return {{CommandStatus::Rejected, "Command ownership reservation failed"}, {}};
    }
    entry->command = std::move(command);
    if (core.busy || core.fault || core.closed)
    {
        return {core.fault ? broken : blocked, {}};
    }
    BusyGuard guard(core.busy);
    if (!entry->command || context.generation != core.current.generation)
    {
        return {{CommandStatus::Rejected, "Missing command or stale document generation"}, {}};
    }
    try
    {
        entry->info = entry->command->info();
        entry->context = context;
        const auto& info = entry->info;
        if (context.origin == CommandOrigin::HistoryReplay)
        {
            return {{CommandStatus::Rejected, "Replay cannot be dispatched as a new command"}, {}};
        }
        if (info.history == CommandHistoryPolicy::Always)
        {
            entry->record = true;
        }
        else if (info.history == CommandHistoryPolicy::UserPreference)
        {
            auto preference = core.preferences ? core.preferences(info.category) : std::nullopt;
            if (!preference)
            {
                return {{CommandStatus::Rejected, "Unresolved command category preference"}, {}};
            }
            entry->record = *preference;
        }
        if ((entry->record && (!info.undoable || context.origin == CommandOrigin::Navigation ||
                context.origin == CommandOrigin::Runtime)) ||
            (!entry->record && context.compatibility == CommandCompatibility::Unspecified))
        {
            return {{CommandStatus::Rejected, "Invalid recording, origin or compatibility policy"}, {}};
        }
        if (core.nextState == std::numeric_limits<uint64_t>::max() ||
            core.current.nonhistoricalRevision == std::numeric_limits<uint64_t>::max())
        {
            return {{CommandStatus::Rejected, "Document identity exhausted"}, {}};
        }
        entry->after = core.nextState++;
        entry->memory = entry->command->memory();
        auto pending = std::make_unique<CommandReservation::Impl>();
        pending->owner = _impl;
        pending->entries.push_back(std::move(entry));
        auto& stored = *pending->entries.front();
        core.live.push_back(&stored);
        stored.registration = std::prev(core.live.end());
        stored.registered = true;
        if (!core.fits())
        {
            return {exhausted, {}};
        }
        return {{CommandStatus::NoOp, {}},
            std::unique_ptr<CommandReservation>(new CommandReservation(std::move(pending)))};
    }
    catch (...)
    {
        return {{CommandStatus::Rejected, "Command admission/preparation failed"}, {}};
    }
}

CommandResult CommandHistory::execute(std::unique_ptr<CommandReservation> reservation)
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    if (!reservation || !reservation->_impl || reservation->_impl->owner != _impl ||
        reservation->context().generation != core.current.generation)
    {
        return {CommandStatus::Cancelled, "Missing, foreign or stale command reservation"};
    }
    BusyGuard guard(core.busy);
    auto& pending = reservation->_impl->entries;
    auto& entry = *pending.front();
    if (!entry.record && entry.info.impact == CommandDocumentImpact::Document &&
        core.current.nonhistoricalRevision == std::numeric_limits<uint64_t>::max())
    {
        return {CommandStatus::Rejected, "Nonhistorical revision exhausted"};
    }
    if (!core.refreshMemory(entry))
    {
        return exhausted;
    }
    entry.state = CommandState::Executing;
    auto result = entry.command->execute();
    if (result.status == CommandStatus::Applied)
    {
        core.publish(pending);
    }
    else if (result.status == CommandStatus::Faulted)
    {
        core.fail(pending);
    }
    else
    {
        entry.state = CommandState::Discarded;
    }
    return result;
}

CommandResult CommandHistory::execute(std::unique_ptr<Command> command, CommandContext context)
{
    auto admitted = prepare(std::move(command), context);
    return admitted.reservation ? execute(std::move(admitted.reservation)) : admitted.result;
}

CommandResult CommandHistory::undo()
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    if (core.cursor == 0)
    {
        return {};
    }
    BusyGuard guard(core.busy);
    auto& entry = **core.at(core.cursor - 1);
    entry.state = CommandState::Executing;
    auto result = entry.command->undo();
    if (result.status == CommandStatus::Applied)
    {
        --core.cursor;
        core.current.state = entry.before;
        entry.state = CommandState::Undone;
    }
    else
    {
        core.fault = result.status == CommandStatus::Faulted;
        entry.state = core.fault ? CommandState::Faulted : CommandState::Applied;
    }
    return result;
}

CommandResult CommandHistory::redo()
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    if (core.cursor == core.entries.size())
    {
        return {};
    }
    BusyGuard guard(core.busy);
    auto& entry = **core.at(core.cursor);
    entry.state = CommandState::Executing;
    auto result = entry.command->redo();
    if (result.status == CommandStatus::Applied)
    {
        ++core.cursor;
        core.current.state = entry.after;
        entry.state = CommandState::Applied;
    }
    else
    {
        core.fault = result.status == CommandStatus::Faulted;
        entry.state = core.fault ? CommandState::Faulted : CommandState::Undone;
    }
    return result;
}

CommandResult CommandHistory::beginTransaction(std::unique_ptr<CommandReservation> reservation)
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    if (!reservation || !reservation->_impl || reservation->_impl->owner != _impl ||
        reservation->context().generation != core.current.generation)
    {
        return {CommandStatus::Cancelled, "Missing, foreign or stale interaction reservation"};
    }
    auto& pending = reservation->_impl->entries;
    auto& entry = *pending.front();
    auto transaction = dynamic_cast<CommandTransaction*>(entry.command.get());
    if (!transaction || !entry.info.undoable)
    {
        return {CommandStatus::Rejected, "Interaction requires an undoable transaction command"};
    }
    BusyGuard guard(core.busy);
    if (!core.refreshMemory(entry))
    {
        return exhausted;
    }
    auto result = transaction->begin();
    if (result.status == CommandStatus::NoOp)
    {
        core.active.splice(core.active.end(), pending);
    }
    else if (result.status == CommandStatus::Faulted || result.status == CommandStatus::Applied)
    {
        // begin promises no mutation; Applied violates its contract.
        core.fail(pending);
        return {CommandStatus::Faulted, "Transaction begin failed to preserve its baseline"};
    }
    return result;
}

CommandResult CommandHistory::beginTransaction(std::unique_ptr<CommandTransaction> command, CommandContext context)
{
    auto admitted = prepare(std::move(command), context);
    return admitted.reservation ? beginTransaction(std::move(admitted.reservation)) : admitted.result;
}

CommandResult CommandHistory::updateTransaction(const std::any& value)
{
    auto& core = *_impl;
    core.checkThread();
    if (core.fault)
    {
        return broken;
    }
    if (core.busy || core.active.empty())
    {
        return blocked;
    }
    BusyGuard guard(core.busy);
    auto& entry = *core.active.front();
    auto& transaction = static_cast<CommandTransaction&>(*entry.command);
    try
    {
        auto estimate = transaction.memoryForUpdate(value);
        std::swap(entry.memory, estimate);
        if (!core.fits())
        {
            std::swap(entry.memory, estimate);
            return exhausted;
        }
        entry.state = CommandState::Executing;
        auto result = transaction.update(value);
        if (result.status == CommandStatus::Faulted)
        {
            core.fail(core.active);
        }
        else
        {
            entry.state = CommandState::Applied;
            if (!result.accepted())
            {
                std::swap(entry.memory, estimate);
            }
        }
        return result;
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Transaction memory preparation failed"};
    }
}

CommandResult CommandHistory::commitTransaction()
{
    auto& core = *_impl;
    core.checkThread();
    if (core.fault)
    {
        return broken;
    }
    if (core.busy || core.active.empty())
    {
        return blocked;
    }
    BusyGuard guard(core.busy);
    auto& entry = *core.active.front();
    if (!static_cast<CommandTransaction&>(*entry.command).changed())
    {
        core.active.clear();
        return {};
    }
    // Already-applied adoption: never call execute a second time.
    core.publish(core.active);
    return {CommandStatus::Applied, {}};
}

CommandResult CommandHistory::cancelTransaction()
{
    auto& core = *_impl;
    core.checkThread();
    if (core.fault)
    {
        return broken;
    }
    if (core.busy || core.active.empty())
    {
        return blocked;
    }
    BusyGuard guard(core.busy);
    auto& transaction = static_cast<CommandTransaction&>(*core.active.front()->command);
    if (!transaction.changed())
    {
        core.active.clear();
        return {};
    }
    auto result = transaction.undo();
    if (result.status == CommandStatus::Applied)
    {
        core.active.clear();
    }
    else
    {
        // Retain the preview for an explicit retry after recovered failure.
        if (result.status == CommandStatus::Faulted)
        {
            core.fail(core.active);
        }
        else if (result.status == CommandStatus::NoOp)
        {
            core.fail(core.active);
            return {CommandStatus::Faulted, "Changed preview could not restore its baseline"};
        }
    }
    return result;
}

bool CommandHistory::transactionActive() const
{
    _impl->checkThread();
    return !_impl->active.empty();
}

uint64_t CommandHistory::transactionId() const
{
    _impl->checkThread();
    return _impl->active.empty() ? 0 : _impl->active.front()->after;
}

CommandDocumentState CommandHistory::documentState() const
{
    _impl->checkThread();
    return _impl->current;
}

CommandDocumentState CommandHistory::savedState() const
{
    _impl->checkThread();
    return _impl->saved;
}

bool CommandHistory::dirty() const
{
    _impl->checkThread();
    return _impl->fault || _impl->current != _impl->saved ||
        (!_impl->active.empty() && _impl->active.front()->info.impact == CommandDocumentImpact::Document &&
            static_cast<CommandTransaction&>(*_impl->active.front()->command).changed());
}

CommandResult CommandHistory::markSaved()
{
    _impl->checkThread();
    if (!_impl->available())
    {
        return _impl->fault ? broken : blocked;
    }
    _impl->saved = _impl->current;
    return {};
}

CommandResult CommandHistory::noteNonhistoricalChange(CommandCompatibility compatibility)
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    if (compatibility == CommandCompatibility::Unspecified ||
        core.nextState == std::numeric_limits<uint64_t>::max() ||
        core.current.nonhistoricalRevision == std::numeric_limits<uint64_t>::max())
    {
        return {CommandStatus::Rejected, "Invalid compatibility or exhausted document identity"};
    }
    BusyGuard guard(core.busy);
    if (compatibility == CommandCompatibility::ResetHistory)
    {
        core.entries.clear();
        core.cursor = 0;
        core.current.state = core.nextState++;
    }
    else if (compatibility == CommandCompatibility::InvalidateRedo)
    {
        core.eraseRedo();
    }
    ++core.current.nonhistoricalRevision;
    return {CommandStatus::Applied, {}};
}

CommandResult CommandHistory::resetBaseline(uint64_t generation, bool saved)
{
    auto& core = *_impl;
    core.checkThread();
    if (core.busy || generation <= core.current.generation || !core.active.empty() ||
        core.nextState == std::numeric_limits<uint64_t>::max())
    {
        return {CommandStatus::Rejected, "Cancel preview and supply a new generation before resetting"};
    }
    BusyGuard guard(core.busy);
    core.entries.clear();
    core.faults.clear();
    core.cursor = 0;
    core.fault = false;
    core.current = {generation, core.nextState++, 0};
    if (saved)
    {
        core.saved = core.current;
    }
    return {};
}

CommandResult CommandHistory::reportExternalFault()
{
    auto& core = *_impl;
    core.checkThread();
    if (core.busy)
    {
        return blocked;
    }
    if (!core.active.empty())
    {
        core.fail(core.active);
    }
    core.fault = true;
    return {CommandStatus::Faulted, "External publication recovery failed"};
}

bool CommandHistory::faulted() const
{
    _impl->checkThread();
    return _impl->fault;
}

size_t CommandHistory::undoCount() const
{
    _impl->checkThread();
    return _impl->cursor;
}

size_t CommandHistory::redoCount() const
{
    _impl->checkThread();
    return _impl->entries.size() - _impl->cursor;
}

size_t CommandHistory::retainedBytes() const
{
    _impl->checkThread();
    return _impl->bytes().value_or(std::numeric_limits<size_t>::max());
}

size_t CommandHistory::retainedEntries() const
{
    _impl->checkThread();
    return _impl->live.size();
}

CommandResult CommandHistory::discardOldest(size_t count)
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    BusyGuard guard(core.busy);
    count = std::min(count, core.cursor);
    core.entries.erase(core.entries.begin(), core.at(count));
    core.cursor -= count;
    return {};
}

CommandResult CommandHistory::discardRedo()
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    BusyGuard guard(core.busy);
    core.eraseRedo();
    return {};
}

CommandResult CommandHistory::setLimits(CommandHistoryLimits limits)
{
    auto& core = *_impl;
    core.checkThread();
    if (!core.available())
    {
        return core.fault ? broken : blocked;
    }
    const auto used = core.bytes();
    if (!used || *used > limits.bytes || core.live.size() > limits.entries)
    {
        return exhausted;
    }
    core.limits = limits;
    return {};
}

}
