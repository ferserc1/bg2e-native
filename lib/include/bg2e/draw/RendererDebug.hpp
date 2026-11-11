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

namespace bg2e::draw {
enum class RendererDebugView : uint32_t
{
    Final, Albedo, Normals, Roughness, Metallic, Emission, Depth, Baked,
    Direct, Indirect, GlobalIllumination, AmbientOcclusion, Reflections,
    ReflectionConfidence, ShadowVisibility, Irradiance, Prefiltered,
    TemporalAcceptance, TemporalMotion, TemporalCurrentWeight, TemporalCount,
    TemporalReactivity, TemporalVariance, BeforeUpscale, AfterUpscale, Material, FresnelFlags, SheenRefraction
};
enum class RendererDebugStage : uint32_t
{
    Original, Accumulated, Filtered
};
// Session-only inspection. Not part of the graphics settings/persistence catalog.
struct RendererDebugOptions {
    bool enabled = false;
    bool disableTransparent = false;
    RendererDebugView view = RendererDebugView::Final;
    RendererDebugStage stage = RendererDebugStage::Original;
    uint32_t channel = 0; // RGB, R, G, B, A.
    uint32_t shadowIndex = 0;
    uint32_t temporalSignal = 1; // Visibility, diffuse radiance, reflected radiance.
    uint64_t layer = 0; // 0 opaque, 1 shared transparent receiver layer.
    float exposure = 0;
    float zoom = 1;
    float depthMinimum = 0;
    float depthMaximum = 10;
};
}
