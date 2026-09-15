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

#include <bg2e/ui/Layout.hpp>

#include "imgui.h"

namespace bg2e {
namespace ui {

void Layout::sameLine(int32_t xPos)
{
    if (xPos < 0)
    {
        xPos = std::abs(xPos);
        auto winWidth = ImGui::GetWindowWidth();
        ImGui::SameLine(winWidth - static_cast<float>(xPos));
    }
    else {
        ImGui::SameLine(static_cast<float>(xPos));
    }
}

void Layout::spacing(int32_t spacing)
{
    ImGui::Dummy(ImVec2(0, static_cast<float>(spacing)));
}

uint32_t Layout::calcTextWidth(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    return static_cast<uint32_t>(textSize.x);
}

uint32_t Layout::calcTextHeight(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    return static_cast<uint32_t>(textSize.y);
}

uint32_t Layout::calcButtonWidth(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    return static_cast<uint32_t>(textSize.x + padding.x * 2.0f);
}

uint32_t Layout::calcButtonHeight(const std::string & title)
{
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    return static_cast<uint32_t>(textSize.y + padding.y * 2.0f);
}

uint32_t Layout::getItemHorizontalSpacing()
{
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    return static_cast<uint32_t>(spacing.x);
}

uint32_t Layout::getItemVerticalSpacing()
{
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    return static_cast<uint32_t>(spacing.y);
}

void Layout::padding(uint32_t width, uint32_t height)
{
    ImGui::Dummy(ImVec2(static_cast<float>(width), static_cast<float>(height)));
}

float Layout::getContentRegionAvailWidth()
{
    return ImGui::GetContentRegionAvail().x;
}

float Layout::getContentRegionAvailHeight()
{
    return ImGui::GetContentRegionAvail().y;
}

void Layout::beginChild(const std::string & id, float width, float height, bool border)
{
    ImGui::BeginChild(id.c_str(), ImVec2(width, height), border);
}

void Layout::endChild()
{
    ImGui::EndChild();
}

}
}
