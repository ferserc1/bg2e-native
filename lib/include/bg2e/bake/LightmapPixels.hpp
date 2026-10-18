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

#include <bg2e/bake/Types.hpp>
#include <cstddef>
#include <variant>
#include <vector>

namespace bg2e::bake {
// Tightly packed RGB, row-major, row zero is top (texture UV v=0), no padding/alpha.
// Linear indirect multiplier, never gamma-encoded or premultiplied. Presence/coverage
// is separate. RGB32F preserves finite nonnegative HDR values above one unchanged.
// RGB8 is linear UNORM: GI export applies 2^exposureEV then clamps/rounds; AO does not.
// No conversion or GPU readback occurs in this CPU value type.
struct BG2E_API LightmapPixels {
    uint32_t width = 0, height = 0;
    LightmapPixelFormat format = LightmapPixelFormat::RGB8;
    std::variant<std::vector<uint8_t>,std::vector<float>> rgb;

    size_t elementCount() const; // Checked width*height*3; requires nonzero dimensions.
    void validate() const; // Checks dimensions, variant, exact payload and float domain.
};
}
