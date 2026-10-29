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

#include <bg2e/app/CommandHistory.hpp>

namespace bg2e::app {

// The executor schedules run/cancel at a caller-owned safe boundary. Exactly one
// head is in flight. Invoke one callback once on the owner thread; late/duplicate
// callbacks are harmless. The history must outlive this queue and its shutdown.
class BG2E_API CommandQueue {
  public:
    using Completion = std::function<void(CommandResult)>;
    using Task = std::function<void()>;
    using Executor = std::function<void(const CommandInfo*, const CommandContext*, Task run, Task cancel)>;

    explicit CommandQueue(CommandHistory& history, Executor executor);
    ~CommandQueue();
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

    // Admission returns NoOp on acceptance; completion reports application.
    // Failed admission also completes exactly once. Notification exceptions are
    // contained and never change history. pump explicitly schedules the head.
    CommandResult enqueue(std::unique_ptr<Command> command, CommandContext context = {}, Completion completion = {});
    CommandResult enqueueUndo(Completion completion = {});
    CommandResult enqueueRedo(Completion completion = {});
    CommandResult enqueueBegin(std::unique_ptr<CommandTransaction> command,
        uint64_t interaction, CommandContext context = {}, Completion completion = {});
    // Ordinary updates retain FIFO order, including across commit/cancel.
    // Payloads are immutable CPU values; memory is bounded by
    // payloadBytes (caller upper bound) and the queue's configured budget.
    CommandResult enqueueUpdate(uint64_t interaction, std::any value,
        size_t payloadBytes, Completion completion = {});
    // Only adjacent, unscheduled absolute updates with the same interaction,
    // generation and exact target/property key may replace each other. The
    // replaced update completes NoOp (superseded), exactly once.
    CommandResult enqueueAbsoluteUpdate(uint64_t interaction, std::string targetProperty,
        std::any value, size_t payloadBytes, Completion completion = {});
    CommandResult enqueueCommit(uint64_t interaction, Completion completion = {});
    CommandResult enqueueCancel(uint64_t interaction, Completion completion = {});

    // Call each owner tick; asynchronous completion makes the next head ready
    // for the next pump. Scheduling is bounded and never recursively pumps.
    // At most 1024 pending jobs are accepted (in addition to payload limits).
    void pump();
    void cancelPending();
    // Cancels pending work and restores an active queue-owned preview. Must be
    // called at a safe boundary before destroying history/device resources.
    CommandResult shutdown();
    size_t pending() const;
    void setPayloadLimit(size_t bytes);

  private:
    struct Impl;
    std::shared_ptr<Impl> _impl;
};

}
