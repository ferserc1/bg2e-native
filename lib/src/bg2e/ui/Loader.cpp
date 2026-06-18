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

#include <bg2e/ui/Loader.hpp>

#include "imgui.h"

#include <algorithm>

namespace bg2e {
namespace ui {

void Loader::setMessage(const std::string& msg)
{
    std::lock_guard lock(_mutex);
    _message = msg;
}

std::string Loader::getMessage() const
{
    std::lock_guard lock(_mutex);
    return _message;
}

void Loader::setProgress(float progress)
{
    std::lock_guard lock(_mutex);
    _progress = std::clamp(progress, 0.f, 1.f);
}

float Loader::getProgress() const
{
    std::lock_guard lock(_mutex);
    return _progress;
}

void Loader::draw()
{
    std::lock_guard lock(_mutex);

    ImGuiIO& io = ImGui::GetIO();
    float winW = 420.f;
    float winH = 90.f;
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - winW) * 0.5f,
               (io.DisplaySize.y - winH) * 0.5f),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar   |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoScrollbar  |
        ImGuiWindowFlags_NoCollapse   |
        ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##loader", nullptr, flags))
    {
        ImGui::TextUnformatted(_message.c_str());
        ImGui::Spacing();
        ImGui::ProgressBar(_progress, ImVec2(-1.f, 0.f));
    }
    ImGui::End();
}

} // ui
} // bg2e
