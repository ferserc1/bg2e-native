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
#include <bg2e/app/CommandTransaction.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>

namespace bg2e_composer::command::component {

class EditMembership final : public bg2e::app::Command {
  public:
    EditMembership(std::shared_ptr<StageScene> stage,
        std::shared_ptr<bg2e::manipulation::SelectionManager> selection,
        std::weak_ptr<bg2e::scene::Node> node, std::shared_ptr<bg2e::scene::Component> original,
        std::shared_ptr<bg2e::scene::Component> replacement);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::manipulation::SelectionManager> _selection;
    std::weak_ptr<bg2e::scene::Node> _node;
    std::shared_ptr<command::SceneRetention> _original, _replacement;
    std::vector<std::weak_ptr<bg2e::scene::Component>> _before, _after;
    StageScene::EditingState _beforeState, _afterState;
    bg2e::manipulation::SelectionState _beforeSelection, _afterSelection;
    bool _captured = false;
    bg2e::app::CommandInfo _info{"Edit component membership", "component.membership"};

    void publish(const std::vector<std::weak_ptr<bg2e::scene::Component>>& value);
    bg2e::app::CommandResult restore(bool forward) noexcept;
};

enum class Action { Property, TransformIdentity, ResetProjection, ResetOrbit };
bool supportedProperty(bg2e::scene::Component& component, const bg2e::ui::ReflectionEditIntent& intent);

class EditProperty final : public bg2e::app::CommandTransaction {
  public:
    EditProperty(std::weak_ptr<StageScene> stage, std::weak_ptr<bg2e::scene::Component> component,
        bg2e::ui::ReflectionEditIntent intent, Action action = Action::Property,
        std::function<void()> effects = {});

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;

    bg2e::app::CommandResult begin() noexcept override;
    bg2e::app::CommandMemory memoryForUpdate(const std::any& value) const override;
    bg2e::app::CommandResult update(const std::any& value) noexcept override;
    bool changed() const noexcept override;

    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    struct Pose {
        glm::vec2 rotation;
        glm::vec3 center;
        float distance;
    };
    struct Value {
        std::any scalar;
        std::optional<glm::mat4> matrix;
        std::shared_ptr<bg2e::math::Projection> projection;
        std::optional<Pose> pose;
    };
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::scene::Component> _target;
    bg2e::ui::ReflectionEditIntent _intent;
    Action _action;
    Value _before, _after;
    StageScene::EditingState _beforeState, _afterState;
    bool _captured = false;
    bool _changed = false;
    std::function<void()> _effects;
    bg2e::app::CommandInfo _info{"Edit component property", "component.property"};

    static bool equal(const Value& a, const Value& b);
    Value capture(bg2e::scene::Component& component) const;
    void write(bg2e::scene::Component& component, const Value& value) const;
    void propose(bg2e::scene::Component& component) const;
    bg2e::app::CommandResult restore(bool forward) noexcept;
};

}
