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

#include <bg2e/base/Light.hpp>
#include <bg2e/math/base.hpp>

namespace bg2e::base {
// Runtime solar snapshot; size-dependent values are resolved in shaders.
// Never written back to Light or scene JSON.
struct SolarState {
    bool enabled = false;
    glm::vec3 direction{0.0f, 1.0f, 0.0f}; // Observer to sun, world space.
    float size = 1.0f; // Solar radius multiplier; resolved in shaders.
    float distanceMillionKm = 149.6f;
    bool atmosphere = false;
    glm::vec3 color{1.0f};
    float intensity = 0.0f; // Distance-scaled; shaders apply horizon visibility.
    float skyIntensity = 0.0f;

    bool operator==(const SolarState&) const = default;
};

// rayDirection follows local +Z transformed into world space. Atmosphere uses
// a fixed terrestrial profile at sea level and world +Y as the horizon normal.
BG2E_API SolarState evaluateSolarLight(const Light& light, const glm::vec3& rayDirection, bool atmosphere = true);
}
