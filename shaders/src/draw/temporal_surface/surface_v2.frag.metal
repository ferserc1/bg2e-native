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

#include "surface_data_v2.metal"
struct TemporalSurfaceOut {
    uint token [[color(0)]];
    float2 motion [[color(1)]];
    float4 previousPosition [[color(2)]];
    float4 previousNormal [[color(3)]];
};
fragment TemporalSurfaceOut fragMain(TemporalSurfaceVaryings in [[stage_in]], constant DrawObjectData& drawObject [[buffer(7)]],
    texture2d<float> drawAlbedo [[texture(0)]], sampler drawAlbedoSampler [[sampler(0)]],
    texture2d<float> drawNormal [[texture(3)]], sampler drawNormalSampler [[sampler(3)]],
    constant TemporalSurfaceUniforms& temporalSurface [[buffer(8)]])
{
    DrawMaterialData m = drawObject.material;
    float4 a = drawSampleMaterial(drawAlbedo, drawAlbedoSampler, m, 0u, in.uv0, in.uv1) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard_fragment();
    }
    float3 n = normalize(in.worldNormal);
    float3 t = normalize(in.worldTangent - n * dot(n, in.worldTangent));
    float3 mapped = drawSampleMaterial(drawNormal, drawNormalSampler, m, 3u, in.uv0, in.uv1).xyz * 2.0 - 1.0;
    mapped = t * mapped.x + cross(n, t) * mapped.y * in.tangentSign + n * mapped.z;
    n = dot(mapped, mapped) > 1e-8 ? normalize(mapped) : n;
    float3 previousNormal = (temporalSurface.previousNormalFromCurrent * float4(n, 0.0)).xyz;
    float normalLength = length(previousNormal);
    TemporalSurfaceOut out;
    out.token = 0u;
    out.motion = float2(0.0);
    out.previousPosition = float4(0.0);
    out.previousNormal = float4(0.0);
    if (temporalSurface.identity.y == 0u || !all(isfinite(in.currentClip)) || !all(isfinite(in.previousClip)) ||
        in.currentClip.w <= 1e-6 || in.previousClip.w <= 1e-6 ||
        !all(isfinite(float4(in.previousWorldPosition, normalLength))) || normalLength <= 1e-6)
    {
        return out;
    }
    float2 motion = (in.previousClip.xy / in.previousClip.w - in.currentClip.xy / in.currentClip.w) *
        float2(0.5, -0.5) * temporalSurface.extent.xy;
    if (!all(isfinite(motion)) || !all(isfinite(previousNormal)) || any(abs(motion) > float2(65504.0)))
    {
        return out;
    }
    out.token = temporalSurface.identity.x;
    out.motion = motion;
    out.previousPosition = float4(in.previousWorldPosition, 1.0);
    out.previousNormal = float4(previousNormal / normalLength, 1.0);
    return out;
}
