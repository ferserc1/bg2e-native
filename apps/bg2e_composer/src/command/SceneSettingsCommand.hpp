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

#include <bg2e/app/CompositeCommand.hpp>

namespace bg2e_composer::command {

// One authored settings request, with typed child adapters and atomic recovery.
// The initial source/catalog guard is deliberately absent from history replay.
class SceneSettingsCommand final : public bg2e::app::Command {
  public:
    SceneSettingsCommand(std::vector<std::unique_ptr<bg2e::app::Command>> children, std::function<bool()> valid);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::function<bool()> _valid;
    std::unique_ptr<bg2e::app::CompositeCommand> _batch;
};

}
