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

#include <bg2e/app/CommandTransaction.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e_composer::command::scene {

// Supervised direct manipulation: update adopts an already applied matrix and
// commit never executes it again. Identity and before/after values are frozen.
class TransformInteraction final : public bg2e::app::CommandTransaction {
  public:
    TransformInteraction(std::weak_ptr<bg2e::scene::Node> target,
        std::function<bool()> valid, std::function<void()> effects);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandMemory memoryForUpdate(const std::any&) const override;

    bg2e::app::CommandResult begin() noexcept override;
    bg2e::app::CommandResult update(const std::any&) noexcept override;
    bool changed() const noexcept override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<bg2e::scene::Node> _target;
    std::weak_ptr<bg2e::scene::TransformComponent> _transform;
    std::function<bool()> _valid;
    std::function<void()> _effects;
    glm::mat4 _before{1.0f};
    glm::mat4 _after{1.0f};
    bg2e::app::CommandInfo _info{"Transform node", "scene.transform"};

    bg2e::app::CommandResult assign(const glm::mat4& matrix) noexcept;
};

}
