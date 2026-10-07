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

#include <bg2e/common.hpp>

#include <cstdint>

namespace bg2e {
namespace render {

// The composed lightmap is an RGB light multiplier: RTAO is vec3(AO visibility);
// RTGI is componentwise GI irradiance / unoccluded environment irradiance
// (zero reference channels produce zero). The factor modulates indirect light;
// direct lighting and material emission remain independent.
enum class LightmapMode { RTAO, RTGI };
enum class LightmapPixelFormat { RGB8, RGB32F };

struct BG2E_API LightmapSettings {
    uint32_t resolution = 512;
    LightmapMode mode = LightmapMode::RTAO;
    uint32_t accumulationFrames = 16;
    uint32_t samplesPerPixel = 8;
    uint32_t giBounces = 2;
    // RTGI ray range. RTAO lightmaps use a fixed 0.1 m occlusion radius.
    float maxRayDistance = 50.0f;
    LightmapPixelFormat cpuFormat = LightmapPixelFormat::RGB8;
};

}
}
