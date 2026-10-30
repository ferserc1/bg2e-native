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
#include "../SceneRetention.hpp"
#include <bg2e/draw/environment/EnvironmentGenerator.hpp>

namespace bg2e_composer::command::lighting {

class EditEnvironment final : public bg2e::app::Command {
  public:
    using Generator = bg2e::draw::environment::EnvironmentGenerator;
    EditEnvironment(std::weak_ptr<StageScene> stage,
        std::shared_ptr<bg2e::scene::EnvironmentComponent> target,
        std::shared_ptr<bg2e::scene::EnvironmentComponent> prepared,
        std::function<Generator*()> generator);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::scene::EnvironmentComponent> _target;
    std::shared_ptr<bg2e::scene::EnvironmentComponent> _before, _after, _alternate;
    std::function<Generator*()> _generator;
    Generator* _identity = nullptr;
    Generator::State _beforeResources, _afterResources;
    std::shared_ptr<const bg2e::draw::environment::EnvironmentResources> _expected;
    bg2e::app::CommandMemory _memory;
    bool _applied = false;
    bg2e::app::CommandInfo _info{"Edit environment", "environment.properties"};

    bg2e::app::CommandResult exchange(bool applied) noexcept;
};

// Parsed presets enter as prepared data. Removed components and the generated
// rig keep their identities; replay restores exact membership and stage state.
class AdoptEnvironmentPreset final : public bg2e::app::Command {
  public:
    using Generator = EditEnvironment::Generator;
    AdoptEnvironmentPreset(const std::shared_ptr<StageScene>& stage,
        const std::shared_ptr<bg2e::scene::Node>& preset, std::function<Generator*()> generator);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    using Node = bg2e::scene::Node;
    using Component = bg2e::scene::Component;
    struct Membership {
        std::weak_ptr<Node> node;
        std::vector<std::weak_ptr<Component>> before, after;
    };
    struct State {
        std::vector<std::weak_ptr<Node>> children;
        StageScene::EditingState editing;
        glm::mat4 floor{1}, fill{1};
        bool floorEnabled = false;
    };
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<Node> _root;
    std::shared_ptr<SceneRetention> _rig;
    std::vector<Membership> _memberships;
    std::vector<std::shared_ptr<Component>> _retained;
    State _before, _after;
    std::function<Generator*()> _generator;
    Generator* _identity = nullptr;
    Generator::State _beforeResources, _afterResources;
    std::shared_ptr<const bg2e::draw::environment::EnvironmentResources> _expected;
    bg2e::app::CommandMemory _memory;
    bool _applied = false;
    bg2e::app::CommandInfo _info{"Apply environment preset", "environment.preset"};

    void assign(bool after);
    bg2e::app::CommandResult exchange(bool applied) noexcept;
};

}
