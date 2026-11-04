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

#include <bg2e/ui/Icon.hpp>
#include <bg2e/ui/IconDrawOptions.hpp>
#include <filesystem>
#include <memory>
#include <string>

struct ImDrawList;

namespace bg2e::ui::detail {

// Measured in scaled UI coordinates. This is the single sizing policy for
// button rendering and later toolbar right-alignment calculations.
struct IconButtonMetrics {
    float width;
    float height;
    float contentWidth;
    float contentHeight;
    float iconSize;
    float textWidth;
    float textHeight;
    float gap;
};

std::shared_ptr<Icon> resolveIcon(const std::filesystem::path& path);
IconButtonMetrics measureIconContent(const std::string& label, bool hasIcon, float size);
IconButtonMetrics measureIconButton(const std::string& label, bool hasIcon, float size);

// Already-scaled screen coordinates; optional target supports parent menu
// headers after BeginMenu has entered the child popup. No item is submitted.
void paintIcon(const Icon& icon, float x, float y, float boxSize,
    const base::Color& tint, ImDrawList* drawList = nullptr);

void drawIconContent(const std::string& label, const std::shared_ptr<Icon>& icon,
    float size, const IconDrawOptions& options);
void drawIconImage(const std::shared_ptr<Icon>& icon, float size, const IconDrawOptions& options);
bool drawIconButton(const std::string& id, const std::string& label,
    const std::shared_ptr<Icon>& icon, float size, const IconDrawOptions& options);

}
