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

#include "../../../apps/bg2e_composer/src/Document.hpp"

#include <stdexcept>

namespace {
using namespace bg2e::app;
using bg2e_composer::Document;
using bg2e_composer::command::Dispatcher;

void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Recoverable document rejection contract failed");
    }
}

class Preview final : public CommandTransaction {
  public:
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
        return {};
    }

    CommandResult begin() noexcept override
    {
        return {};
    }

    CommandResult update(const std::any&) noexcept override
    {
        _changed = true;
        return {CommandStatus::Applied, {}};
    }

    bool changed() const noexcept override
    {
        return _changed;
    }

    CommandResult execute() noexcept override
    {
        return {CommandStatus::Rejected, "Preview requires adoption"};
    }

    CommandResult undo() noexcept override
    {
        _changed = false;
        return {CommandStatus::Applied, {}};
    }

    CommandResult redo() noexcept override
    {
        _changed = true;
        return {CommandStatus::Applied, {}};
    }

  private:
    CommandInfo _info{"Document rejection preview", "value"};
    bool _changed = false;
};

std::shared_ptr<Dispatcher> dispatcher()
{
    return std::make_shared<Dispatcher>(1, true,
        [](auto, auto*, auto run, auto) { run(); },
        [](const auto&) {}, [](auto) {});
}

void rejectedNotificationPreservesDirtyState()
{
    Document document;
    document.commands = dispatcher();
    auto& history = document.commands->history();
    require(history.beginTransaction(std::make_unique<Preview>()).accepted());
    // A legacy camera move must not crash even with an unchanged preview.
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Rejected);
    require(document.dirty && !history.dirty());
    document.synchronizeDirty();
    require(document.dirty && history.transactionActive());
    require(history.cancelTransaction().accepted());
    document.synchronizeDirty();
    require(document.dirty && history.dirty());
    require(document.pendingCompatibility == CommandCompatibility::Unspecified);
    require(history.undoCount() == 0 && history.redoCount() == 0);
    require(history.markSaved().accepted());
    document.synchronizeDirty();
    require(!document.dirty);
    require(document.markModified(CommandCompatibility::Unspecified).status == CommandStatus::Rejected);
    require(!document.dirty);
}

void pendingCompatibilityAndFaults()
{
    Document document;
    document.commands = dispatcher();
    auto& history = document.commands->history();
    require(history.beginTransaction(std::make_unique<Preview>()).accepted());
    require(history.updateTransaction(1).accepted());
    require(document.markModified(CommandCompatibility::ResetHistory).status == CommandStatus::Rejected);
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Rejected);
    require(history.commitTransaction().accepted());
    require(history.undoCount() == 1);
    document.synchronizeDirty();
    require(history.undoCount() == 0 && document.dirty);
    require(history.reportExternalFault().status == CommandStatus::Faulted);
    require(document.markModified(CommandCompatibility::Preserve).status == CommandStatus::Faulted);
    document.synchronizeDirty();
    require(document.dirty && history.faulted());
}

void rejectionCallbackIsIsolated()
{
    auto commands = dispatcher();
    int completed = 0;
    const auto result = commands->submit({}, bg2e_composer::command::Boundary::CpuValue, {},
        [] { return true; }, [&](auto rejection)
        {
            require(rejection.status == CommandStatus::Rejected);
            ++completed;
            throw std::runtime_error("Injected notification failure");
        });
    require(result.status == CommandStatus::Rejected && completed == 1);
    require(!commands->history().faulted() && commands->pending() == 0);
}
}

int main()
{
    rejectedNotificationPreservesDirtyState();
    pendingCompatibilityAndFaults();
    rejectionCallbackIsIsolated();
}
