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
#include <bg2e/app/CommandQueue.hpp>
#include <bg2e/app/CompositeCommand.hpp>

#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace bg2e::app;

void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Command/memento contract failed");
    }
}

void status(CommandResult result, CommandStatus expected)
{
    require(result.status == expected);
}

struct Behavior {
    bool failExecute = false;
    bool failUndo = false;
    bool failRedo = false;
    bool faultExecute = false;
    bool faultUndo = false;
    int executed = 0;
    int undone = 0;
    int redone = 0;
};

class SetValue final : public Command {
  public:
    SetValue(int& model, int value, CommandInfo info = {"Set value", "value"},
        std::shared_ptr<Behavior> behavior = std::make_shared<Behavior>(), CommandMemory memory = {})
        : _model(model), _after(value), _info(std::move(info)), _behavior(std::move(behavior)), _memory(std::move(memory))
    {
    }

    const CommandInfo& info() const noexcept override
    {
        return _info;
    }
    CommandMemory memory() const override
    {
        return _memory;
    }

    CommandResult execute() noexcept override
    {
        ++_behavior->executed;
        _before = _model;
        if (_behavior->faultExecute)
        {
            _model = _after; // Deliberately unrecoverable fake mutation.
            return {CommandStatus::Faulted, "Injected execution fault"};
        }
        if (_behavior->failExecute)
        {
            _model = _after;
            _model = _before; // A failed command recovers its own partial work.
            return {CommandStatus::Failed, "Injected recovered execution failure"};
        }
        return assign(_after);
    }
    CommandResult undo() noexcept override
    {
        ++_behavior->undone;
        if (_behavior->faultUndo)
        {
            return {CommandStatus::Faulted, "Injected undo fault"};
        }
        if (_behavior->failUndo)
        {
            return {CommandStatus::Failed, "Injected recovered undo failure"};
        }
        return assign(_before);
    }
    CommandResult redo() noexcept override
    {
        ++_behavior->redone;
        if (_behavior->failRedo)
        {
            return {CommandStatus::Failed, "Injected recovered redo failure"};
        }
        return assign(_after);
    }

  private:
    int& _model;
    int _before = 0;
    int _after;
    CommandInfo _info;
    std::shared_ptr<Behavior> _behavior;
    CommandMemory _memory;

    CommandResult assign(int value) noexcept
    {
        if (_model == value)
        {
            return {};
        }
        _model = value;
        return {CommandStatus::Applied, {}};
    }
};

struct PreviewBehavior {
    int begun = 0;
    int executed = 0;
    int updated = 0;
    bool failUpdate = false;
    bool failCancel = false;
    size_t estimate = 0;
};

class Preview final : public CommandTransaction {
  public:
    Preview(int& model, std::shared_ptr<PreviewBehavior> behavior,
        CommandInfo info = {"Preview", "value"})
        : _model(model), _behavior(std::move(behavior)), _info(std::move(info))
    {
    }

    const CommandInfo& info() const noexcept override
    {
        return _info;
    }
    CommandMemory memory() const override
    {
        return {};
    }
    CommandMemory memoryForUpdate(const std::any&) const override
    {
        return {_behavior->estimate, {}};
    }

    CommandResult begin() noexcept override
    {
        ++_behavior->begun;
        _before = _after = _model;
        return {};
    }
    CommandResult update(const std::any& value) noexcept override
    {
        ++_behavior->updated;
        const auto number = std::any_cast<int>(&value);
        if (!number)
        {
            return {CommandStatus::Rejected, "Invalid update payload"};
        }
        if (_behavior->failUpdate)
        {
            return {CommandStatus::Failed, "Recovered preview failure"};
        }
        _model = _after = *number;
        return {CommandStatus::Applied, {}};
    }
    bool changed() const noexcept override
    {
        return _before != _after;
    }
    CommandResult execute() noexcept override
    {
        ++_behavior->executed;
        _model = _after;
        return {CommandStatus::Applied, {}};
    }
    CommandResult undo() noexcept override
    {
        if (_behavior->failCancel)
        {
            return {CommandStatus::Failed, "Recovered cancellation failure"};
        }
        _model = _before;
        return {CommandStatus::Applied, {}};
    }
    CommandResult redo() noexcept override
    {
        _model = _after;
        return {CommandStatus::Applied, {}};
    }

  private:
    int& _model;
    int _before = 0;
    int _after = 0;
    std::shared_ptr<PreviewBehavior> _behavior;
    CommandInfo _info;
};

class DeferredRelease final : public CommandReleaseSink {
  public:
    // Fixed preallocated slots let the noexcept sink retain without allocation.
    std::unique_ptr<Command> retired[32];
    CommandMemory tickets[32];
    size_t count = 0;

    void release(std::unique_ptr<Command> command, CommandMemory memory) noexcept override
    {
        if (count < 32)
        {
            retired[count] = std::move(command);
            tickets[count] = std::move(memory);
            ++count;
        }
    }
};

void branchingAndIdentity()
{
    int model = 0;
    int selected = 0;
    CommandHistory history;
    const auto initial = history.documentState();
    status(history.execute(std::make_unique<SetValue>(model, 1)), CommandStatus::Applied);
    const auto first = history.documentState();
    status(history.markSaved(), CommandStatus::NoOp);
    require(!history.dirty());
    CommandInfo selection{"Select", "selection", CommandHistoryPolicy::Always, CommandDocumentImpact::None};
    status(history.execute(std::make_unique<SetValue>(selected, 7, selection)), CommandStatus::Applied);
    require(history.documentState() == first && !history.dirty());
    status(history.undo(), CommandStatus::Applied);
    require(!history.dirty() && selected == 0);
    status(history.undo(), CommandStatus::Applied);
    require(model == 0 && history.documentState() == initial && history.dirty());
    status(history.redo(), CommandStatus::Applied);
    require(!history.dirty());
    status(history.execute(std::make_unique<SetValue>(model, 2)), CommandStatus::Applied);
    require(history.redoCount() == 0 && history.documentState() != first);
    status(history.discardOldest(2), CommandStatus::NoOp);
    require(history.undoCount() == 0 && history.dirty()); // Saved state evicted.
    const auto branch = history.documentState();
    status(history.execute(std::make_unique<SetValue>(model, 3)), CommandStatus::Applied);
    status(history.undo(), CommandStatus::Applied);
    require(history.documentState() == branch && history.dirty());
    status(history.markSaved(), CommandStatus::NoOp);
    status(history.noteNonhistoricalChange(CommandCompatibility::Preserve), CommandStatus::Applied);
    require(history.redoCount() == 1 && history.dirty());
    status(history.redo(), CommandStatus::Applied);
    status(history.undo(), CommandStatus::Applied);
    require(history.dirty()); // Navigation revision survives replay.
    status(history.resetBaseline(2), CommandStatus::NoOp);
    require(!history.dirty() && history.undoCount() == 0 && history.redoCount() == 0);
}

void policyAndNoOps()
{
    int model = 0;
    bool preference = true;
    CommandHistory history({}, [&](std::string_view category) -> std::optional<bool>
    {
        return category == "selection" ? std::optional<bool>(preference) : std::nullopt;
    });
    CommandInfo optional{"Select", "selection", CommandHistoryPolicy::UserPreference, CommandDocumentImpact::None};
    auto admission = history.prepare(std::make_unique<SetValue>(model, 1, optional));
    require(bool(admission.reservation));
    preference = false;
    status(history.execute(std::move(admission.reservation)), CommandStatus::Applied);
    require(history.undoCount() == 1); // Frozen dispatch preference.
    status(history.undo(), CommandStatus::Applied);
    status(history.execute(std::make_unique<SetValue>(model, 4, optional),
        {1, CommandOrigin::Editor, CommandCompatibility::Preserve}), CommandStatus::Applied);
    require(history.redoCount() == 1 && !history.dirty());
    status(history.redo(), CommandStatus::Applied); // Existing entry remains active.
    require(model == 1);
    status(history.undo(), CommandStatus::Applied);
    const auto state = history.documentState();
    status(history.execute(std::make_unique<SetValue>(model, 0)), CommandStatus::NoOp);
    require(history.documentState() == state && history.redoCount() == 1);
    auto failure = std::make_shared<Behavior>();
    failure->failExecute = true;
    status(history.execute(std::make_unique<SetValue>(model, 9, CommandInfo{"Fail", "value"}, failure)), CommandStatus::Failed);
    require(model == 0 && history.documentState() == state && history.redoCount() == 1);
    status(history.execute(std::make_unique<SetValue>(model, 2), {1, CommandOrigin::Navigation}), CommandStatus::Rejected);
    status(history.execute(std::make_unique<SetValue>(model, 2), {1, CommandOrigin::Runtime}), CommandStatus::Rejected);
    status(history.execute(std::make_unique<SetValue>(model, 2), {1, CommandOrigin::HistoryReplay}), CommandStatus::Rejected);
    CommandInfo invalid{"Invalid", "value", CommandHistoryPolicy::Always, CommandDocumentImpact::Document, false};
    status(history.execute(std::make_unique<SetValue>(model, 2, invalid)), CommandStatus::Rejected);
    optional.category = "unknown";
    status(history.execute(std::make_unique<SetValue>(model, 2, optional)), CommandStatus::Rejected);
    require(model == 0);
}

void unrecordedCompatibility()
{
    int model = 0;
    int auxiliary = 0;
    CommandHistory history;
    status(history.execute(std::make_unique<SetValue>(model, 1)), CommandStatus::Applied);
    status(history.execute(std::make_unique<SetValue>(model, 2)), CommandStatus::Applied);
    status(history.undo(), CommandStatus::Applied);
    CommandInfo unrecorded{"Auxiliary", "auxiliary", CommandHistoryPolicy::Never, CommandDocumentImpact::None, false};
    status(history.execute(std::make_unique<SetValue>(auxiliary, 1, unrecorded)), CommandStatus::Rejected);
    status(history.execute(std::make_unique<SetValue>(auxiliary, 1, unrecorded),
        {1, CommandOrigin::Navigation, CommandCompatibility::Preserve}), CommandStatus::Applied);
    require(history.redoCount() == 1 && history.undoCount() == 1);
    status(history.execute(std::make_unique<SetValue>(auxiliary, 2, unrecorded),
        {1, CommandOrigin::Editor, CommandCompatibility::InvalidateRedo}), CommandStatus::Applied);
    require(history.redoCount() == 0 && history.undoCount() == 1);
    unrecorded.impact = CommandDocumentImpact::Document;
    const auto before = history.documentState();
    status(history.execute(std::make_unique<SetValue>(model, 7, unrecorded),
        {1, CommandOrigin::Editor, CommandCompatibility::ResetHistory}), CommandStatus::Applied);
    require(model == 7 && history.undoCount() == 0 && history.documentState() != before && history.dirty());
}

void replayFailureAndFaults()
{
    int model = 0;
    auto behavior = std::make_shared<Behavior>();
    CommandHistory history;
    status(history.execute(std::make_unique<SetValue>(model, 1, CommandInfo{"Set", "value"}, behavior)), CommandStatus::Applied);
    const auto applied = history.documentState();
    behavior->failUndo = true;
    status(history.undo(), CommandStatus::Failed);
    require(model == 1 && history.undoCount() == 1 && history.documentState() == applied);
    behavior->failUndo = false;
    status(history.undo(), CommandStatus::Applied);
    const auto undone = history.documentState();
    behavior->failRedo = true;
    status(history.redo(), CommandStatus::Failed);
    require(model == 0 && history.redoCount() == 1 && history.documentState() == undone);
    behavior->failRedo = false;
    status(history.redo(), CommandStatus::Applied);
    behavior->faultUndo = true;
    status(history.undo(), CommandStatus::Faulted);
    require(history.faulted() && history.dirty());
    status(history.redo(), CommandStatus::Faulted);
    status(history.execute(std::make_unique<SetValue>(model, 5)), CommandStatus::Faulted);
    status(history.markSaved(), CommandStatus::Faulted);
    status(history.resetBaseline(2), CommandStatus::NoOp);
    require(!history.faulted());
    auto stale = history.prepare(std::make_unique<SetValue>(model, 3), {2});
    status(history.resetBaseline(3), CommandStatus::NoOp);
    status(history.execute(std::move(stale.reservation)), CommandStatus::Cancelled);
    require(model == 1 && !history.dirty());
    status(history.reportExternalFault(), CommandStatus::Faulted);
    require(history.faulted() && history.dirty());
    status(history.undo(), CommandStatus::Faulted);
    status(history.resetBaseline(4), CommandStatus::NoOp);
}

std::unique_ptr<Command> composite(int& a, int& b, std::shared_ptr<Behavior> first, std::shared_ptr<Behavior> second)
{
    std::vector<std::unique_ptr<Command>> children;
    children.push_back(std::make_unique<SetValue>(a, 1, CommandInfo{"A", "value"}, first));
    children.push_back(std::make_unique<SetValue>(b, 2, CommandInfo{"B", "value"}, second));
    return std::make_unique<CompositeCommand>(CommandInfo{"Both", "value"}, std::move(children));
}

void compositeRecovery()
{
    int a = 0;
    int b = 0;
    CommandHistory history;
    auto first = std::make_shared<Behavior>();
    auto second = std::make_shared<Behavior>();
    second->failExecute = true;
    status(history.execute(composite(a, b, first, second)), CommandStatus::Failed);
    require(a == 0 && b == 0 && history.undoCount() == 0 && first->undone == 1);
    second->failExecute = false;
    status(history.execute(composite(a, b, first, second)), CommandStatus::Applied);
    first->failUndo = true;
    status(history.undo(), CommandStatus::Failed);
    require(a == 1 && b == 2 && history.undoCount() == 1 && second->redone == 1);
    first->failUndo = false;
    status(history.undo(), CommandStatus::Applied);
    second->failRedo = true;
    status(history.redo(), CommandStatus::Failed);
    require(a == 0 && b == 0 && history.redoCount() == 1);
    second->failRedo = false;
    status(history.redo(), CommandStatus::Applied);
    status(history.resetBaseline(2), CommandStatus::NoOp);
    a = b = 0;
    first->failUndo = true;
    second->failExecute = true;
    status(history.execute(composite(a, b, first, second), {2}), CommandStatus::Faulted);
    require(history.faulted() && history.retainedEntries() == 1);
}

void transactions()
{
    int model = 0;
    CommandHistory history;
    auto behavior = std::make_shared<PreviewBehavior>();
    status(history.beginTransaction(std::make_unique<Preview>(model, behavior)), CommandStatus::NoOp);
    require(history.transactionActive() && history.transactionId() != 0);
    status(history.updateTransaction(4), CommandStatus::Applied);
    require(model == 4 && history.dirty() && history.undoCount() == 0);
    status(history.undo(), CommandStatus::Rejected);
    status(history.markSaved(), CommandStatus::Rejected);
    status(history.beginTransaction(std::make_unique<Preview>(model, behavior)), CommandStatus::Rejected);
    behavior->failUpdate = true;
    status(history.updateTransaction(9), CommandStatus::Failed);
    require(model == 4);
    behavior->failUpdate = false;
    status(history.updateTransaction(5), CommandStatus::Applied);
    status(history.commitTransaction(), CommandStatus::Applied);
    require(model == 5 && history.undoCount() == 1 && behavior->begun == 1 && behavior->executed == 0);
    status(history.undo(), CommandStatus::Applied);
    require(model == 0 && !history.dirty());
    status(history.beginTransaction(std::make_unique<Preview>(model, behavior)), CommandStatus::NoOp);
    status(history.updateTransaction(8), CommandStatus::Applied);
    status(history.updateTransaction(0), CommandStatus::Applied);
    status(history.commitTransaction(), CommandStatus::NoOp);
    require(history.redoCount() == 1 && !history.dirty());
    status(history.beginTransaction(std::make_unique<Preview>(model, behavior)), CommandStatus::NoOp);
    status(history.updateTransaction(7), CommandStatus::Applied);
    behavior->failCancel = true;
    status(history.cancelTransaction(), CommandStatus::Failed);
    require(history.transactionActive() && model == 7);
    behavior->failCancel = false;
    status(history.cancelTransaction(), CommandStatus::Applied);
    require(model == 0 && history.redoCount() == 1 && !history.dirty());
    const auto previousBytes = history.retainedBytes();
    status(history.beginTransaction(std::make_unique<Preview>(model, behavior)), CommandStatus::NoOp);
    behavior->estimate = 128 * 1024 * 1024;
    status(history.updateTransaction(99), CommandStatus::Rejected);
    require(model == 0);
    status(history.cancelTransaction(), CommandStatus::NoOp);
    require(history.retainedBytes() == previousBytes);
}

void budgetsAndRelease()
{
    int model = 0;
    auto release = std::make_shared<DeferredRelease>();
    CommandHistory history({3, 1024 * 1024}, {}, release);
    auto resource = std::make_shared<int>(42);
    const auto weak = std::weak_ptr<int>(resource);
    CommandMemory memory{16, {{resource, 4096}}};
    status(history.execute(std::make_unique<SetValue>(model, 1, CommandInfo{"A", "value"}, std::make_shared<Behavior>(), memory)), CommandStatus::Applied);
    const auto firstBytes = history.retainedBytes();
    status(history.execute(std::make_unique<SetValue>(model, 2, CommandInfo{"B", "value"}, std::make_shared<Behavior>(), memory)), CommandStatus::Applied);
    require(history.retainedBytes() - firstBytes < 4096); // Shared ticket counted once.
    auto pending = history.prepare(std::make_unique<SetValue>(model, 3));
    require(bool(pending.reservation) && history.retainedEntries() == 3);
    status(history.execute(std::make_unique<SetValue>(model, 4)), CommandStatus::Rejected);
    require(model == 2 && history.undoCount() == 2);
    pending.reservation.reset();
    status(history.undo(), CommandStatus::Applied);
    const auto before = history.documentState();
    status(history.execute(std::make_unique<SetValue>(model, 9, CommandInfo{"Huge", "value"},
        std::make_shared<Behavior>(), CommandMemory{2 * 1024 * 1024, {}})), CommandStatus::Rejected);
    require(history.redoCount() == 1 && history.documentState() == before);
    status(history.setLimits({0, 0}), CommandStatus::Rejected);
    memory.resources.clear();
    resource.reset();
    status(history.discardRedo(), CommandStatus::NoOp);
    status(history.discardOldest(10), CommandStatus::NoOp);
    require(history.retainedEntries() == 0 && !weak.expired());
    for (size_t i = 0; i < release->count; ++i)
    {
        release->retired[i].reset();
        release->tickets[i] = {};
    }
    require(weak.expired());
}

void queueOrderingAndCompletion()
{
    int model = 0;
    CommandHistory history;
    std::vector<CommandQueue::Task> scheduled;
    std::vector<CommandQueue::Task> cancelled;
    std::vector<CommandStatus> results;
    CommandQueue queue(history, [&](const CommandInfo*, const CommandContext*, auto run, auto cancel)
    {
        scheduled.push_back(std::move(run));
        cancelled.push_back(std::move(cancel));
    });
    auto completion = [&](CommandResult result)
    {
        results.push_back(result.status);
    };
    status(queue.enqueue(std::make_unique<SetValue>(model, 1), {}, completion), CommandStatus::NoOp);
    status(queue.enqueue(std::make_unique<SetValue>(model, 2), {}, completion), CommandStatus::NoOp);
    require(model == 0 && queue.pending() == 2);
    queue.pump();
    queue.pump();
    require(scheduled.size() == 1);
    scheduled[0]();
    scheduled[0]();
    cancelled[0]();
    require(model == 1 && results.size() == 1 && queue.pending() == 1);
    queue.pump();
    require(scheduled.size() == 2);
    scheduled[1]();
    require(model == 2 && results.size() == 2 && queue.pending() == 0);
    queue.enqueueUndo(completion);
    queue.pump();
    cancelled[2]();
    scheduled[2]();
    require(model == 2 && results.back() == CommandStatus::Cancelled);
    queue.enqueueUndo([](CommandResult)
    {
        throw std::runtime_error("Notification error");
    });
    queue.pump();
    scheduled[3]();
    require(model == 1 && history.redoCount() == 1);
    queue.enqueueRedo(completion);
    queue.pump();
    queue.cancelPending();
    scheduled[4]();
    require(model == 1 && results.size() == 4);
    queue.enqueue(std::make_unique<SetValue>(model, 6), {}, completion);
    status(history.resetBaseline(2), CommandStatus::NoOp);
    queue.pump();
    scheduled[5]();
    require(model == 1 && results.back() == CommandStatus::Cancelled);
}

void ownerThreadAndForeignReservations()
{
    int model = 0;
    CommandHistory history;
    CommandHistory other;
    auto admission = history.prepare(std::make_unique<SetValue>(model, 5));
    status(other.execute(std::move(admission.reservation)), CommandStatus::Cancelled);
    require(model == 0 && history.retainedEntries() == 0);
    bool rejected = false;
    std::thread producer([&]
    {
        try
        {
            history.undo();
        }
        catch (const std::logic_error&)
        {
            rejected = true;
        }
    });
    producer.join();
    require(rejected && model == 0);
}

void queueCancellationAndBudgets()
{
    int model = 0;
    CommandHistory history;
    CommandQueue::Task late;
    int completed = 0;
    {
        CommandQueue queue(history, [&](const CommandInfo*, const CommandContext*, auto run, auto)
        {
            late = std::move(run);
        });
        queue.setPayloadLimit(2);
        status(queue.enqueueUpdate(1, 9, sizeof(int), [&](CommandResult result)
        {
            status(result, CommandStatus::Rejected);
            ++completed;
        }), CommandStatus::Rejected);
        queue.enqueue(std::make_unique<SetValue>(model, 8), {}, [&](CommandResult result)
        {
            status(result, CommandStatus::Cancelled);
            ++completed;
        });
        queue.pump();
        require(bool(late));
        status(queue.shutdown(), CommandStatus::NoOp);
    }
    late(); // Callback surviving queue destruction cannot access the history.
    require(model == 0 && completed == 2 && history.retainedEntries() == 0);

    CommandQueue throwing(history, [](const CommandInfo*, const CommandContext*, auto, auto)
    {
        throw std::runtime_error("Scheduling failed");
    });
    throwing.enqueue(std::make_unique<SetValue>(model, 3), {}, [&](CommandResult result)
    {
        status(result, CommandStatus::Cancelled);
        ++completed;
    });
    throwing.pump();
    require(model == 0 && completed == 3 && throwing.pending() == 0);
}

void compositeNoOpAndUndoFault()
{
    int a = 1;
    int b = 0;
    auto first = std::make_shared<Behavior>();
    auto second = std::make_shared<Behavior>();
    CommandHistory history;
    status(history.execute(composite(a, b, first, second)), CommandStatus::Applied);
    status(history.undo(), CommandStatus::Applied);
    require(a == 1 && b == 0 && first->undone == 0); // No-op child excluded from replay.
    status(history.redo(), CommandStatus::Applied);
    require(first->redone == 0 && b == 2);
    status(history.resetBaseline(2), CommandStatus::NoOp);
    a = b = 0;
    status(history.execute(composite(a, b, first, second), {2}), CommandStatus::Applied);
    first->failUndo = true;
    second->failRedo = true; // Recovery of the already undone suffix now fails.
    status(history.undo(), CommandStatus::Faulted);
    require(history.faulted() && history.undoCount() == 1);
}

void queueTransactionsAndFaults()
{
    int model = 0;
    CommandHistory history;
    CommandQueue queue(history, [](const CommandInfo*, const CommandContext*, auto run, auto)
    {
        run();
    });
    auto preview = std::make_shared<PreviewBehavior>();
    int superseded = 0;
    queue.enqueueBegin(std::make_unique<Preview>(model, preview), 1);
    queue.enqueueAbsoluteUpdate(1, "object/property", 3, sizeof(int), [&](CommandResult result)
    {
        status(result, CommandStatus::NoOp);
        ++superseded;
    });
    queue.enqueueAbsoluteUpdate(1, "object/property", 4, sizeof(int));
    queue.enqueueUpdate(1, 5, sizeof(int)); // Incremental/ordinary seam blocks coalescing.
    queue.enqueueAbsoluteUpdate(1, "object/property", 6, sizeof(int));
    queue.enqueueCommit(1);
    require(queue.pending() == 5 && superseded == 1);
    queue.pump();
    require(model == 6 && preview->updated == 3 && preview->executed == 0 && history.undoCount() == 1);
    queue.enqueueUndo();
    queue.pump();
    require(model == 0);
    queue.enqueueBegin(std::make_unique<Preview>(model, preview), 2);
    queue.enqueueUpdate(2, 9, sizeof(int));
    queue.pump();
    require(model == 9 && history.transactionActive());
    status(queue.shutdown(), CommandStatus::Applied);
    require(model == 0 && !history.transactionActive() && history.redoCount() == 1);

    CommandQueue failures(history, [](const CommandInfo*, const CommandContext*, auto run, auto)
    {
        run();
    });
    int completions = 0;
    auto fault = std::make_shared<Behavior>();
    fault->faultExecute = true;
    failures.enqueue(std::make_unique<SetValue>(model, 7, CommandInfo{"Fault", "value"}, fault));
    failures.enqueue(std::make_unique<SetValue>(model, 8), {}, [&](CommandResult result)
    {
        status(result, CommandStatus::Cancelled);
        ++completions;
    });
    failures.pump();
    require(model == 7 && history.faulted() && completions == 1 && failures.pending() == 0);
}
}

int main()
{
    branchingAndIdentity();
    policyAndNoOps();
    unrecordedCompatibility();
    replayFailureAndFaults();
    compositeRecovery();
    compositeNoOpAndUndoFault();
    transactions();
    budgetsAndRelease();
    queueOrderingAndCompletion();
    ownerThreadAndForeignReservations();
    queueCancellationAndBudgets();
    queueTransactionsAndFaults();
    return 0;
}
