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

#include "ImageDraw.hpp"
#include "ImageRegistry.hpp"
#include <imgui.h>

namespace bg2e::ui::detail {

bool imageValid(const std::shared_ptr<ImageEntry>& entry)
{
    return entry && !entry->registry.expired() && entry->image && entry->sampler;
}

uint64_t imageIdentifier(const std::shared_ptr<ImageEntry>& entry)
{
    if (imageValid(entry))
    {
        if (auto registry = entry->registry.lock())
        {
            return registry->identifier(*entry);
        }
    }
    return 0;
}

void drawImage(const std::shared_ptr<ImageEntry>& entry, float width, float height)
{
    if (imageValid(entry))
    {
        ImGui::Image(imageIdentifier(entry), ImVec2(width, height));
    }
}

bool imageButton(const std::shared_ptr<ImageEntry>& entry, const std::string& id,
    float width, float height)
{
    if (imageValid(entry))
    {
        return ImGui::ImageButton(id.c_str(), imageIdentifier(entry), ImVec2(width, height));
    }
    return false;
}

void drawImageAt(const std::shared_ptr<ImageEntry>& entry, float x, float y,
    float width, float height, const base::Color& tint, ImDrawList* drawList)
{
    if (imageValid(entry))
    {
        // GetColorU32 applies style alpha, including any enclosing disabled
        // scope. Do not apply DisabledAlpha a second time to the texture tint.
        auto* target = drawList ? drawList : ImGui::GetWindowDrawList();
        target->AddImage(imageIdentifier(entry), ImVec2(x, y),
            ImVec2(x + width, y + height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
            ImGui::GetColorU32(ImVec4(tint.r, tint.g, tint.b, tint.a)));
    }
}

}
