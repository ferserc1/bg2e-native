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

#version 450
#extension GL_GOOGLE_include_directive : require
#include "../lib/bindings.glsl"
layout(std140, set = 1, binding = 1) uniform DrawObjectBlock
{
    DrawObjectData data;
} drawObject;
layout(set = 1, binding = 2) uniform texture2D drawAlbedo;
layout(set = 1, binding = 8) uniform sampler drawAlbedoSampler;
layout(std140, set = 2, binding = 1) uniform TemporalSurfaceBlock
{
    layout(offset = 0) uvec4 identity;
    layout(offset = 16) mat4 previousModel;
    layout(offset = 80) mat4 currentViewProjection;
    layout(offset = 144) mat4 previousViewProjection;
    layout(offset = 208) mat4 previousNormalFromCurrent;
    layout(offset = 272) vec4 extent;
} temporalSurface;
layout(set = 1, binding = 5) uniform texture2D drawNormal;
layout(set = 1, binding = 11) uniform sampler drawNormalSampler;
layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 worldTangent;
layout(location = 5) in float tangentSign;
layout(location = 6) in vec4 currentClip;
layout(location = 7) in vec4 previousClip;
layout(location = 8) in vec3 previousWorldPosition;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 0) out uint surfaceToken;
layout(location = 1) out vec2 surfaceMotion;
layout(location = 2) out vec4 surfacePreviousPosition;
layout(location = 3) out vec4 surfacePreviousNormal;
bool finite4(vec4 value)
{
    return !any(isnan(value)) && !any(isinf(value));
}
void main()
{
    DrawMaterialData m = drawObject.data.material;
    vec4 a = drawDecodeSample(m, 0u, texture(sampler2D(drawAlbedo, drawAlbedoSampler), drawSelectUV(m, 0u, uv0, uv1))) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard;
    }
    // Match GBuffer normal mapping, including selected/scaled UV and tangent handedness.
    vec3 n = normalize(worldNormal);
    vec3 t = normalize(worldTangent - n * dot(n, worldTangent));
    vec3 mapped = drawDecodeSample(m, 3u, texture(sampler2D(drawNormal, drawNormalSampler), drawSelectUV(m, 3u, uv0, uv1))).xyz * 2.0 - 1.0;
    mapped = t * mapped.x + cross(n, t) * mapped.y * tangentSign + n * mapped.z;
    n = dot(mapped, mapped) > 1e-8 ? normalize(mapped) : n;
    vec3 previousNormal = mat3(temporalSurface.previousNormalFromCurrent) * n;
    float normalLength = length(previousNormal);
    surfaceToken = 0u;
    surfaceMotion = vec2(0.0);
    surfacePreviousPosition = vec4(0.0);
    surfacePreviousNormal = vec4(0.0);
    if (temporalSurface.identity.y == 0u || !finite4(currentClip) || !finite4(previousClip) ||
        currentClip.w <= 1e-6 || previousClip.w <= 1e-6 ||
        !finite4(vec4(previousWorldPosition, normalLength)) || normalLength <= 1e-6)
    {
        return;
    }
    // Perspective divide after interpolation; motion excludes projection jitter.
    vec2 motion = (previousClip.xy / previousClip.w - currentClip.xy / currentClip.w) *
        vec2(0.5, -0.5) * temporalSurface.extent.xy;
    if (!finite4(vec4(motion, previousNormal.xy)) || !finite4(vec4(previousNormal, 1.0)) ||
        any(greaterThan(abs(motion), vec2(65504.0))))
    {
        return; // RG16F overflow is invalid correspondence, never valid zero motion.
    }
    surfaceToken = temporalSurface.identity.x;
    surfaceMotion = motion;
    surfacePreviousPosition = vec4(previousWorldPosition, 1.0);
    surfacePreviousNormal = vec4(previousNormal / normalLength, 1.0);
}
