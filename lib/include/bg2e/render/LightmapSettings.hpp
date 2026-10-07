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
    LightmapMode mode = LightmapMode::RTGI;
    uint32_t accumulationFrames = 16;
    uint32_t samplesPerPixel = 8;
    uint32_t giBounces = 2;
    // RTGI ray range. RTAO lightmaps use a fixed 0.1 m occlusion radius.
    float maxRayDistance = 50.0f;
    // RTGI ray origin offset in metres. Smaller values reduce detached contact
    // illumination; values that are too small can cause self-intersections.
    float giRayBias = 0.0005f;
    // RGB8 RTGI export only: multiply the floating-point result by 2^exposureEV
    // before clamping to [0, 1]. RGB32F and the GPU image remain unchanged.
    float exposureEV = 0.0f;
    LightmapPixelFormat cpuFormat = LightmapPixelFormat::RGB8;
};

}
}
