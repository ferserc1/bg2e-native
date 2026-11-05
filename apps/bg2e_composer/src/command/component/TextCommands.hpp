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

#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/scene/TextFontComponent.hpp>
#include <bg2e/app/CommandTransaction.hpp>
#include <variant>
#include <optional>
#include "../../StageScene.hpp"

namespace bg2e_composer::command::component {

struct TextValues {
    bg2e::base::Text::Parameters paragraph;
    glm::vec2 size{2, 1}, pivot{0.5f};
    bool preserveAspect = true;
    bool operator==(const TextValues&) const = default;
};
using FontDeclarations = std::vector<bg2e::scene::TextFontComponent::Declaration>;
using TextAuthoringValue = std::variant<TextValues, FontDeclarations>;

// CPU authored state only. Never snapshots/replaces a material, generator, mesh
// or registry. Normal stopped frame preparation realizes the latest preview.
class EditText final : public bg2e::app::CommandTransaction {
  public:
    EditText(std::weak_ptr<StageScene> stage, std::weak_ptr<bg2e::scene::Component> target,
        std::function<void()> effects);

    static TextAuthoringValue capture(bg2e::scene::Component& target);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandMemory memoryForUpdate(const std::any& value) const override;

    bg2e::app::CommandResult begin() noexcept override;
    bg2e::app::CommandResult update(const std::any& value) noexcept override;
    bool changed() const noexcept override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    std::weak_ptr<StageScene> _stage;
    std::weak_ptr<bg2e::scene::Component> _target;
    std::function<void()> _effects;
    TextAuthoringValue _before, _after;
    bool _captured = false, _changed = false;
    bg2e::app::CommandInfo _info{"Edit text/font properties", "component.text"};

    bool valid() const;
    static bool write(bg2e::scene::Component& target, const TextAuthoringValue& value);
    bg2e::app::CommandResult restore(const TextAuthoringValue& value) noexcept;
};

}
