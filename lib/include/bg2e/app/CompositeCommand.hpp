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

#include <bg2e/app/Command.hpp>

namespace bg2e::app {

// Children are immutable after construction. A composite is one recorded edit;
// every child must be undoable and use the same category/policy as the parent.
class BG2E_API CompositeCommand final : public Command {
  public:
    CompositeCommand(CommandInfo info, std::vector<std::unique_ptr<Command>> children);
    ~CompositeCommand() override;

    const CommandInfo& info() const noexcept override;
    CommandMemory memory() const override;

    CommandResult execute() noexcept override;
    CommandResult undo() noexcept override;
    CommandResult redo() noexcept override;

  private:
    CommandInfo _info;
    std::vector<std::unique_ptr<Command>> _children;
    std::vector<unsigned char> _effective;
    bool _applied = false;

    CommandResult apply(bool replay) noexcept;
};

}
