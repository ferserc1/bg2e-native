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

#include <bg2e/ui/Button.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

bool Button::button(const std::string & title, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::Button(title.c_str());
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

bool Button::checkBox(const std::string & title, bool * value, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::Checkbox(title.c_str(), value);
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

bool Button::radioButton(const std::string & label, int * value, int id, bool sameLine, bool disabled)
{
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
    if (sameLine)
    {
        ImGui::SameLine();
    }
    auto result = ImGui::RadioButton(label.c_str(), value, id);
    if (disabled)
    {
        ImGui::EndDisabled();
    }
    return result;
}

}
}
