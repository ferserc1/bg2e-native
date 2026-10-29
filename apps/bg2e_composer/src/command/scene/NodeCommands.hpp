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

namespace bg2e_composer::command::scene {

class RenameNode final : public bg2e::app::Command {
  public:
    RenameNode(std::weak_ptr<bg2e::scene::Node> target, std::string name);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;

    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<bg2e::scene::Node> _target;
    std::string _before;
    std::string _after;
    bool _captured = false;
    bg2e::app::CommandInfo _info{"Rename node", "scene.name"};

    bg2e::app::CommandResult assign(const std::string& name) noexcept;
};

class SetNodeEnabled final : public bg2e::app::Command {
  public:
    SetNodeEnabled(std::weak_ptr<StageScene> stage, std::weak_ptr<bg2e::scene::Node> target, bool enabled);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;

    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::scene::Node> _target;
    bool _before = false;
    bool _after;
    bool _captured = false;
    StageScene::EditingState _beforeState;
    StageScene::EditingState _afterState;
    bg2e::app::CommandInfo _info{"Set node enabled", "scene.enabled"};

    bg2e::app::CommandResult restore(bool enabled, const StageScene::EditingState& state) noexcept;
};

}
