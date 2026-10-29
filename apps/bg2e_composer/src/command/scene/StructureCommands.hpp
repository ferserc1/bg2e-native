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

enum class NodeEdit { Insert, Remove, Reparent };

class EditNodes final : public bg2e::app::Command {
  public:
    EditNodes(std::shared_ptr<StageScene> stage,
        std::shared_ptr<bg2e::manipulation::SelectionManager> selection, NodeEdit operation,
        const std::vector<std::shared_ptr<bg2e::scene::Node>>& targets,
        std::weak_ptr<bg2e::scene::Node> destination = {}, std::string label = {});

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    struct Placement {
        std::weak_ptr<bg2e::scene::Node> parent;
        std::vector<std::weak_ptr<bg2e::scene::Node>> children;
    };
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::manipulation::SelectionManager> _selection;
    NodeEdit _operation;
    std::weak_ptr<bg2e::scene::Node> _destination;
    std::vector<std::shared_ptr<command::SceneRetention>> _targets;
    std::vector<Placement> _before, _after;
    StageScene::EditingState _beforeState, _afterState;
    bg2e::manipulation::SelectionState _beforeSelection, _afterSelection;
    bool _captured = false;
    bg2e::app::CommandInfo _info;

    void publish(const std::vector<Placement>& placements);
    bg2e::app::CommandResult restore(bool forward) noexcept;
};

}
