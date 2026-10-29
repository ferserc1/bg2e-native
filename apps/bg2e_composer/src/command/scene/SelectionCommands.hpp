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

#include "../../StageScene.hpp"
#include <bg2e/manipulation/SelectionManager.hpp>

namespace bg2e_composer::command::scene {

class SetSelection final : public bg2e::app::Command {
  public:
    SetSelection(std::weak_ptr<StageScene> stage,
        std::weak_ptr<bg2e::manipulation::SelectionManager> selection,
        std::weak_ptr<bg2e::scene::Node> node, bool additive,
        std::optional<bg2e::manipulation::SelectionItem> item = {}, bool clear = false);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::manipulation::SelectionManager> _selection;
    std::weak_ptr<bg2e::scene::Node> _node;
    bool _additive;
    bool _clear = false;
    std::optional<bg2e::manipulation::SelectionItem> _item;
    bg2e::manipulation::SelectionState _before, _after;
    bool _captured = false;
    bg2e::app::CommandInfo _info{"Select scene objects", "selection",
        bg2e::app::CommandHistoryPolicy::UserPreference, bg2e::app::CommandDocumentImpact::None};

    bg2e::app::CommandResult assign(const bg2e::manipulation::SelectionState& state) noexcept;
};

}
