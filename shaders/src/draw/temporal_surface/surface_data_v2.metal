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

#ifndef DRAW_TEMPORAL_SURFACE_DATA
#define DRAW_TEMPORAL_SURFACE_DATA
#include "../raster/vertex.metal"
struct TemporalSurfaceUniforms {
    uint4 identity;
    float4x4 previousModel;
    float4x4 currentViewProjection;
    float4x4 previousViewProjection;
    float4x4 previousNormalFromCurrent;
    float4 extent;
};
static_assert(sizeof(TemporalSurfaceUniforms) == 288, "Temporal surface v2 uniform size");
static_assert(__builtin_offsetof(TemporalSurfaceUniforms, previousModel) == 16, "Previous model offset");
static_assert(__builtin_offsetof(TemporalSurfaceUniforms, currentViewProjection) == 80, "Current camera offset");
static_assert(__builtin_offsetof(TemporalSurfaceUniforms, previousViewProjection) == 144, "Previous camera offset");
static_assert(__builtin_offsetof(TemporalSurfaceUniforms, previousNormalFromCurrent) == 208, "Normal transform offset");
static_assert(__builtin_offsetof(TemporalSurfaceUniforms, extent) == 272, "Extent offset");
struct TemporalSurfaceVaryings {
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
    float3 worldTangent;
    float2 uv0;
    float2 uv1;
    float tangentSign;
    float4 currentClip;
    float4 previousClip;
    float3 previousWorldPosition;
};
#endif
