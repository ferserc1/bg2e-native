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

#include <bg2e/draw/ShaderData.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::draw {
namespace {
glm::vec4 color(const base::Color& c)
{
    return {c.r, c.g, c.b, c.a};
}
void finite(const glm::vec4& value)
{
    for (int i = 0; i < 4; ++i)
    {
        if (!std::isfinite(value[i]))
        {
            throw std::invalid_argument("Shader data must contain finite values");
        }
    }
}
void invertible(const glm::mat4& value)
{
    for (int i = 0; i < 4; ++i)
    {
        finite(value[i]);
    }
    const float determinant = glm::determinant(value);
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f)
    {
        throw std::invalid_argument("Shader matrix must be finite and invertible");
    }
}
}
MaterialData packMaterial(const MaterialBase& material)
{
    const auto& a = material.materialAttributes();
    if (!a.isUpdated())
    {
        throw std::logic_error("Realize pending material texture changes before queue extraction");
    }
    MaterialData result;
    result.albedo = color(a.albedo());
    result.fresnelTint = color(a.fresnelTint());
    result.sheenColor = color(a.sheenColor());
    result.factors = {a.metalness(), a.roughness(), a.sheenIntensity(), a.refractionFactor()};
    result.emissionAlpha = {a.lightEmission(), a.alphaCutoff(), 0, 0};
    result.uvScale = {glm::vec4(a.albedoScale(), a.normalScale()), glm::vec4(a.metalnessScale(), a.roughnessScale()), glm::vec4(a.aoScale(), a.lightEmissionScale())};
    uint32_t flags = (a.isUnlit() ? MaterialUnlit : 0u) | (a.alphaCutoff() > 0 ? MaterialAlphaTest : 0u) |
        (a.isTransparent() ? MaterialTransparent : 0u) | (a.isSolid() ? MaterialSolid : 0u) |
        (a.aoTexture() ? MaterialHasAOTexture : 0u);
    uint32_t decodeMask = 0;
    uint32_t colorMask = 0;
    for (uint32_t i = 0; i < 6; ++i)
    {
        const auto texture = material.texture(TextureSlot(i));
        if (!texture || !texture->image() || !texture->sampler() || !texture->image()->isValid() || !texture->sampler()->isValid())
        {
            throw std::logic_error("Material requires all six realized images and samplers");
        }
        const auto format = texture->image()->pixelFormat();
        const bool nativeSRGB = format == gpu::PixelFormat::R8G8B8A8_SRGB || format == gpu::PixelFormat::B8G8R8A8_SRGB;
        if (texture->colorType() != base::Color::TypeLinear && texture->colorType() != base::Color::TypeSRGB)
        {
            throw std::invalid_argument("Unknown material texture color interpretation");
        }
        if (nativeSRGB && texture->colorType() != base::Color::TypeSRGB)
        {
            throw std::invalid_argument("Native sRGB image cannot be interpreted as linear encoded data without a compatible view");
        }
        if (texture->colorType() == base::Color::TypeSRGB)
        {
            colorMask |= 1u << i;
            decodeMask |= nativeSRGB ? 0u : 1u << i;
        }
    }
    result.uvSets = {glm::uvec4(a.albedoUVSet(), a.metalnessUVSet(), a.roughnessUVSet(), a.normalUVSet()),
        glm::uvec4(a.aoUVSet(), a.lightEmissionUVSet(), flags, decodeMask)};
    result.channels = {a.metalnessChannel(), a.roughnessChannel(), a.aoChannel(), a.lightEmissionChannel()};
    result.invert = {a.metalnessInvert() ? 1u : 0u, a.roughnessInvert() ? 1u : 0u, a.lightEmissionInvert() ? 1u : 0u, colorMask};
    for (const auto uv : {a.albedoUVSet(), a.normalUVSet(), a.metalnessUVSet(), a.roughnessUVSet(), a.aoUVSet(), a.lightEmissionUVSet()})
    {
        if (uv > 1)
        {
            throw std::invalid_argument("Standard draw geometry supports UV sets 0 and 1 only");
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        if (result.channels[i] > 3)
        {
            throw std::invalid_argument("Material channel must be in [0, 3]");
        }
    }
    for (const auto& value : {result.albedo, result.fresnelTint, result.sheenColor, result.factors, result.emissionAlpha,
        result.uvScale[0], result.uvScale[1], result.uvScale[2]})
    {
        finite(value);
    }
    if (a.alphaCutoff() < 0 || a.alphaCutoff() > 1)
    {
        throw std::invalid_argument("Alpha cutoff must be in [0, 1]");
    }
    return result;
}
FrameData packFrame(const FrameContext& frame, const glm::mat4& view, const glm::mat4& projection)
{
    invertible(view);
    invertible(projection);
    invertible(projection * view);
    if (frame.extent.width == 0 || frame.extent.height == 0 || !std::isfinite(frame.deltaSeconds) || !std::isfinite(frame.sceneDeltaSeconds))
    {
        throw std::invalid_argument("Frame requires a valid extent and finite timing");
    }
    FrameData result;
    result.view = view;
    result.projection = projection;
    result.inverseViewProjection = glm::inverse(projection * view);
    result.cameraPosition = glm::inverse(view)[3];
    result.extentAndDelta = {float(frame.extent.width), float(frame.extent.height), frame.deltaSeconds, frame.sceneDeltaSeconds};
    result.frameInfo = {uint32_t(frame.frameNumber), uint32_t(frame.frameNumber >> 32), uint32_t(frame.sceneFrameNumber), uint32_t(frame.sceneFrameNumber >> 32)};
    return result;
}
}
