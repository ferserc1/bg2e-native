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

#include <bg2e/ui/Group.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Group::beginTree(const std::string & label)
{
    return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
}

void Group::endTree()
{
    ImGui::TreePop();
}

bool Group::collapsingHeader(const std::string & title, bool visible, bool allowOverlap)
{
    ImGuiTreeNodeFlags flags = visible ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (allowOverlap)
    {
        flags |= ImGuiTreeNodeFlags_AllowOverlap;
    }
    return ImGui::CollapsingHeader(title.c_str(), flags);
}

void Group::beginDisabled(bool disabled)
{
    ImGui::BeginDisabled(disabled);
}

void Group::endDisabled()
{
    ImGui::EndDisabled();
}

void Group::pushId(int id)
{
    ImGui::PushID(id);
}

void Group::popId()
{
    ImGui::PopID();
}

}
}
