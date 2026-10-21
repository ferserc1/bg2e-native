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

#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/math/base.hpp>

#include <array>
#include <cstddef>

namespace bg2e::draw {
inline constexpr uint32_t MaxLights = 8;
// Slider values close to zero disable alpha testing; equality enables it.
inline constexpr float AlphaTestCutoffThreshold = 0.02f;
// Validates the authored cutoff without modifying it, then classifies for GPU use.
BG2E_API float effectiveAlphaTestCutoff(float authoredCutoff);
enum MaterialFlags : uint32_t
{
    MaterialUnlit = 1, MaterialAlphaTest = 2, MaterialTransparent = 4,
    MaterialSolid = 8, MaterialHasAOTexture = 16, MaterialBakedMultiplier = 32, MaterialBakedGI = 64
};
// Only 16-byte vectors/matrices: identical std140 and Metal layouts.
struct alignas(16) MaterialData {
    glm::vec4 albedo{0};
    glm::vec4 fresnelTint{0};
    glm::vec4 sheenColor{0};
    glm::vec4 factors{0}; // metalness, roughness, sheen intensity, refraction
    glm::vec4 emissionAlpha{0}; // emission, effective alpha cutoff (zero disables testing), reserved, reserved
    std::array<glm::vec4, 3> uvScale{glm::vec4{0}, glm::vec4{0}, glm::vec4{0}}; // albedo/normal, metalness/roughness, AO/emission
    std::array<glm::uvec4, 2> uvSets{glm::uvec4{0}, glm::uvec4{0}}; // albedo/metalness/roughness/normal; AO/emission/flags/sRGB mask
    glm::uvec4 channels{0}; // metalness, roughness, AO, emission
    glm::uvec4 invert{0}; // metalness, roughness, emission, requested sRGB mask
};
struct alignas(16) FrameData {
    glm::mat4 view{1};
    glm::mat4 projection{1};
    glm::mat4 inverseViewProjection{1};
    glm::vec4 cameraPosition{0};
    glm::vec4 extentAndDelta{0}; // width, height, presentation delta, scene delta
    glm::uvec4 frameInfo{0}; // presentation lo/hi, scene lo/hi
};
struct alignas(16) ObjectData {
    glm::mat4 model{1};
    glm::mat4 normal{1};
    MaterialData material;
    glm::uvec4 identity{0}; // one-based frame object ID, submesh, reserved, reserved
};
struct alignas(16) LightData {
    glm::vec4 positionIntensity{0};
    glm::vec4 color{0};
    glm::vec4 directionAngle{0}; // normalized local +Z transformed to world; spot angle
    glm::vec4 parameters{0}; // spot cutoff, source size, reserved, reserved
    glm::uvec4 flags{0}; // base::Light type, cast shadows, shadow samples, affects reflections
};
struct alignas(16) LightUniforms {
    glm::uvec4 count{0};
    std::array<LightData, MaxLights> lights{};
};
struct alignas(16) EnvironmentData {
    glm::vec4 parameters{0}; // maximum prefilter LOD, normalized sky blur, visible, reserved
};
static_assert(sizeof(MaterialData) == 192 && offsetof(MaterialData, uvScale) == 80 && offsetof(MaterialData, channels) == 160);
static_assert(sizeof(FrameData) == 240 && sizeof(ObjectData) == 336 && offsetof(ObjectData, identity) == 320);
static_assert(sizeof(LightData) == 80 && sizeof(LightUniforms) == 656 && sizeof(EnvironmentData) == 16);
BG2E_API MaterialData packMaterial(const MaterialBase& material);
BG2E_API MaterialData packMaterial(const MaterialBase& material, MaterialRevisions* revisions);
BG2E_API FrameData packFrame(const FrameContext& frame, const glm::mat4& view, const glm::mat4& projection);
}
