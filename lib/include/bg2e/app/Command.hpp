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

#include <bg2e/common.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bg2e::app {

enum class CommandStatus { Applied, NoOp, Rejected, Cancelled, Failed, Faulted };
enum class CommandState { Pending, Executing, Applied, Undone, Discarded, Faulted };
enum class CommandHistoryPolicy { Never, Always, UserPreference };
enum class CommandDocumentImpact { None, Document };
enum class CommandOrigin { Editor, PreparedResult, Navigation, Runtime, HistoryReplay };
enum class CommandCompatibility { Unspecified, Preserve, InvalidateRedo, ResetHistory };

struct CommandResult {
    CommandStatus status = CommandStatus::NoOp;
    // Diagnostics must have static lifetime. Returning results never allocates.
    std::string_view message;

    bool accepted() const noexcept
    {
        return status == CommandStatus::Applied || status == CommandStatus::NoOp;
    }
};

struct CommandInfo {
    std::string name;
    std::string category;
    CommandHistoryPolicy history = CommandHistoryPolicy::Always;
    CommandDocumentImpact impact = CommandDocumentImpact::Document;
    bool undoable = true;
};

struct CommandContext {
    uint64_t generation = 1;
    CommandOrigin origin = CommandOrigin::Editor;
    CommandCompatibility compatibility = CommandCompatibility::Unspecified;
};

// A shared identity represents one immutable retained resource VERSION. All
// users must declare the same size. The ticket also retains its owner/module.
struct CommandResource {
    std::shared_ptr<const void> identity;
    size_t bytes = 0;
};

struct CommandMemory {
    size_t localBytes = 0;
    std::vector<CommandResource> resources;
};

class BG2E_API Command {
  public:
    virtual ~Command();

    virtual const CommandInfo& info() const noexcept = 0;
    // Upper bound including preparation, rollback and redo data. May allocate;
    // checked at admission AND again before execution/begin. Do not grow
    // during mutation beyond the successfully checked reservation.
    virtual CommandMemory memory() const = 0;

    // Failed/Rejected/Cancelled/NoOp guarantee no model change. Implementations
    // recover their own partial mutations; failed recovery returns Faulted.
    // No exception may escape an operation. Redo uses the captured forward state.
    virtual CommandResult execute() noexcept = 0;
    virtual CommandResult undo() noexcept = 0;
    virtual CommandResult redo() noexcept = 0;
};

// Caller-supplied retirement boundary. release must not throw or reenter the
// history. Retain this sink and device/module owners through deferred release.
// Without a sink, destruction is immediate on the owner thread.
class BG2E_API CommandReleaseSink {
  public:
    virtual ~CommandReleaseSink() = default;

    // Retain BOTH the command and its accounting/resource tickets until safe.
    virtual void release(std::unique_ptr<Command> command, CommandMemory retained) noexcept = 0;
};

using CommandPreferenceResolver = std::function<std::optional<bool>(std::string_view)>;

}
