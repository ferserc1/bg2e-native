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

#include <bg2e/app/CommandTransaction.hpp>
#include <limits>

namespace bg2e::app {

struct CommandHistoryLimits {
    size_t entries = 256;
    size_t bytes = 64 * 1024 * 1024;
};

struct CommandDocumentState {
    uint64_t generation = 1;
    uint64_t state = 1;
    uint64_t nonhistoricalRevision = 0;

    bool operator==(const CommandDocumentState&) const = default;
};

class CommandQueue;

// Move-only admission token: owns the pending instance and reserved recording
// storage. Dropping it discards pending work through the release sink.
class BG2E_API CommandReservation {
  public:
    ~CommandReservation();
    CommandReservation(CommandReservation&&) noexcept;
    CommandReservation& operator=(CommandReservation&&) noexcept;
    CommandReservation(const CommandReservation&) = delete;
    CommandReservation& operator=(const CommandReservation&) = delete;

    const CommandInfo& info() const noexcept;
    const CommandContext& context() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> _impl;

    explicit CommandReservation(std::unique_ptr<Impl> impl);
    friend class CommandHistory;
};

struct CommandAdmission {
    CommandResult result;
    std::unique_ptr<CommandReservation> reservation;
};

// All calls, destruction, executor callbacks and completion callbacks belong to
// the constructing thread. Other producers must marshal to it. No reentrancy.
class BG2E_API CommandHistory {
  public:
    explicit CommandHistory(CommandHistoryLimits limits = {},
        CommandPreferenceResolver preferences = {},
        std::shared_ptr<CommandReleaseSink> release = {});
    ~CommandHistory();
    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    CommandAdmission prepare(std::unique_ptr<Command> command, CommandContext context = {});
    CommandResult execute(std::unique_ptr<CommandReservation> reservation);
    CommandResult execute(std::unique_ptr<Command> command, CommandContext context = {});
    CommandResult undo();
    CommandResult redo();

    // Begin consumes a reservation for a CommandTransaction. Generation and
    // policy are frozen at prepare; adoption is only possible for this token.
    CommandResult beginTransaction(std::unique_ptr<CommandReservation> reservation);
    CommandResult beginTransaction(std::unique_ptr<CommandTransaction> command, CommandContext context = {});
    CommandResult updateTransaction(const std::any& value);
    CommandResult commitTransaction();
    CommandResult cancelTransaction();
    bool transactionActive() const;
    uint64_t transactionId() const;

    CommandDocumentState documentState() const;
    CommandDocumentState savedState() const;
    bool dirty() const;
    // Call only after successful IO, with no active preview.
    CommandResult markSaved();
    CommandResult noteNonhistoricalChange(CommandCompatibility compatibility);
    // Explicit repair/document replacement. Caller must cancel producers first.
    // Requires a new, greater generation; stale pending tokens cannot execute.
    CommandResult resetBaseline(uint64_t generation, bool saved = true);

    // An application integration failed to restore an external publication.
    // Preserve recovery ownership and block unsafe replay until resetBaseline.
    CommandResult reportExternalFault();
    bool faulted() const;
    size_t undoCount() const;
    size_t redoCount() const;
    size_t retainedBytes() const;
    size_t retainedEntries() const;
    // Stable-boundary explicit eviction, never triggered by a failed/no-op edit.
    CommandResult discardOldest(size_t count);
    CommandResult discardRedo();
    CommandResult setLimits(CommandHistoryLimits limits);

  private:
    struct Impl;
    std::shared_ptr<Impl> _impl;

    friend class CommandQueue;
    friend class CommandReservation;
};

}
