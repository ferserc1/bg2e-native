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
        throw std::runtime_error("Workspace transition history contract failed");
    }
}

class SetValue final : public Command {
  public:
    SetValue(int& value, int next) : _value(value), _previous(value), _next(next)
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

    CommandResult execute() noexcept override
    {
        return redo();
    }

    CommandResult undo() noexcept override
    {
        _value = _previous;
        return {CommandStatus::Applied, {}};
    }

    CommandResult redo() noexcept override
    {
        _value = _next;
        return {CommandStatus::Applied, {}};
    }

  private:
    CommandInfo _info{"Set value", "workspace test"};
    int& _value;
    int _previous;
    int _next;
};

void exerciseEviction(bool dirty)
{
    int value = 0;
    Document doc;
    doc.generation = 7;
    doc.commands = std::make_shared<Dispatcher>(doc.generation, true,
        Dispatcher::Scheduler{}, Dispatcher::EffectHandler{}, Dispatcher::Completion{});
    auto& commands = *doc.commands;
    auto& history = commands.history();
    const CommandContext context{.generation = doc.generation};
    require(history.execute(std::make_unique<SetValue>(value, 1), context).accepted());
    if (!dirty)
    {
        require(history.markSaved().accepted());
    }
    require(history.execute(std::make_unique<SetValue>(value, 2), context).accepted());
    require(history.undo().accepted());
    doc.synchronizeDirty();
    require(doc.dirty == dirty && history.undoCount() == 1 && history.redoCount() == 1);

    require(commands.reserveCpuInteraction());
    require(!commands.discardWorkspaceHistory().accepted());
    require(history.undoCount() == 1 && history.redoCount() == 1);
    commands.releaseCpuInteraction();

    const auto state = history.documentState();
    const auto saved = history.savedState();
    require(commands.discardWorkspaceHistory().accepted());
    doc.synchronizeDirty();
    require(history.undoCount() == 0 && history.redoCount() == 0 && value == 1);
    require(history.documentState() == state && history.savedState() == saved && doc.dirty == dirty);

    require(history.execute(std::make_unique<SetValue>(value, 3), context).accepted());
    require(history.undoCount() == 1 && commands.discardWorkspaceHistory().accepted());
    require(history.undoCount() == 0 && history.redoCount() == 0 && value == 3);
}

}

int main()
{
    exerciseEviction(false);
    exerciseEviction(true);
}
