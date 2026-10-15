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

#ifndef DRAW_RASTER_VERTEX_METAL
#define DRAW_RASTER_VERTEX_METAL
#include "../lib/bindings.metal"
struct RasterVertex {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord0 [[attribute(2)]];
    float2 texcoord1 [[attribute(3)]];
    float3 tangent [[attribute(4)]];
};
struct RasterVaryings {
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
    float3 worldTangent;
    float2 uv0;
    float2 uv1;
    float tangentSign;
};
#endif
