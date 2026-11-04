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

#include <bg2e/base/Color.hpp>
#include <cstdint>
#include <memory>
#include <string>

struct ImDrawList;

namespace bg2e::ui::detail {

struct ImageEntry;

// Dimensions are UI coordinates as supplied, with no implicit scale or aspect
// adjustment. Consumers apply their own presentation policy before drawing.
bool imageValid(const std::shared_ptr<ImageEntry>& entry);
uint64_t imageIdentifier(const std::shared_ptr<ImageEntry>& entry);
void drawImage(const std::shared_ptr<ImageEntry>& entry, float width, float height);
bool imageButton(const std::shared_ptr<ImageEntry>& entry, const std::string& id,
    float width, float height);

// Submit only image geometry in screen coordinates; no item, padding or scale.
// Used inside composite widgets after they have established their hit rectangle.
void drawImageAt(const std::shared_ptr<ImageEntry>& entry, float x, float y,
    float width, float height, const base::Color& tint, ImDrawList* drawList = nullptr);

}
