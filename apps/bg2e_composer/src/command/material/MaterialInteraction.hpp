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
#include <bg2e/ui/DrawMaterialEditor.hpp>

namespace bg2e_composer::command::material {

// CPU values only. Texture loading, sampling and clipboard publication use
// EditMaterials instead. No native material snapshot is created per update.
std::unique_ptr<bg2e::app::CommandTransaction> materialInteraction(
    std::weak_ptr<StageScene> stage, bg2e::ui::MaterialEditTargets targets,
    std::function<void()> effects);

}
