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
#include <bg2e/ui/DrawMaterialEditor.hpp>

namespace bg2e_composer::command::material {

struct PreparedMaterial {
    bg2e::manipulation::SelectionItem target;
    std::shared_ptr<bg2e::draw::MaterialBase> material;
    bg2e::base::MaterialAttributes definition;
};

// Prepare at a stopped graphics boundary, before admission. Preparation touches
// only private materials. Publication/replay exchange retained states and never
// read a source file or allocate a replacement realization.
class EditMaterials final : public bg2e::app::Command {
  public:
    EditMaterials(std::weak_ptr<StageScene> stage,
        const bg2e::ui::MaterialEditTargets& targets,
        const bg2e::ui::MaterialEditIntent& intent,
        const std::shared_ptr<bg2e::draw::MaterialBase>& paste = {});

    EditMaterials(std::weak_ptr<StageScene> stage, const std::vector<PreparedMaterial>& prepared,
        std::function<bool()> sourceValid);

    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;

  private:
    struct Target {
        bg2e::manipulation::SelectionItem item;
        std::weak_ptr<bg2e::draw::MaterialBase> live;
        std::shared_ptr<bg2e::draw::MaterialBase> alternate;
        bg2e::base::MaterialAttributes definition;
        uint64_t expectedRevision = 0;
        uint64_t expectedDefinition = 0;
    };
    std::weak_ptr<StageScene> _stage;
    std::vector<Target> _targets;
    bg2e::app::CommandMemory _memory;
    std::function<bool()> _sourceValid;
    bool _applied = false;
    bool _effective = false;
    bg2e::app::CommandInfo _info{"Edit materials", "material.edit"};

    bg2e::app::CommandResult exchange(bool applied) noexcept;
};

}
