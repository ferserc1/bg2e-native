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

#include <bg2e/ui/Text.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void Text::text(const std::string & text, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::Text("%s", text.c_str());
}

void Text::separator(const std::string & title, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::SeparatorText(title.c_str());
}

void Text::listItem(const std::string & label, bool sameLine)
{
    if (sameLine)
    {
        ImGui::SameLine();
    }
    ImGui::BulletText("%s", label.c_str());
}

void Text::tooltip(const std::string & text)
{
    if (!text.empty() && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text.c_str());
    }
}

}
}
