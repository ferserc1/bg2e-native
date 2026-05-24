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

#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_query : require

#include "lib/deferred_utils.glsl"
#include "lib/ray_tracing.glsl"

// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Normal;
layout(set = 0, binding = 1) uniform sampler2D g_Depth;

// TLAS (set=0)
layout(set = 0, binding = 2) uniform accelerationStructureEXT tlas;

// AO output (set=0)
layout(set = 0, binding = 3, r8) uniform image2D aoOutput;

// Push constants
layout(push_constant) uniform PushConstant {
    mat4 inverseViewProjection;
    int sampleCount;
    int bounceCount;
    float radius;
    float bias;
    float falloff;
    float bounceAttenuation;
    uint frameIndex;

    int padding0;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main() {
    ivec2 imageSize = imageSize(aoOutput);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    vec2 uv = vec2(pixelCoord + 0.5) / vec2(imageSize);

    float depth = texelFetch(g_Depth, pixelCoord, 0).r;
    if (depth >= 1.0) {
        imageStore(aoOutput, pixelCoord, vec4(1.0, 0.0, 0.0, 0.0));
        return;
    }

    vec3 worldPos = reconstructWorldPosition(uv, depth, pc.inverseViewProjection);
    vec3 normal = normalize(texelFetch(g_Normal, pixelCoord, 0).xyz * 2.0 - 1.0);

    float occlusion = 0.0;
    uint seed =
        uint(pixelCoord.x) * 1973u ^
        uint(pixelCoord.y) * 9277u ^
        uint(pc.frameIndex) * 26699u;

    for (int i = 0; i < pc.sampleCount; ++i)
    {
        vec3 origin = worldPos;
        vec3 bounceNormal = normal;
        float contribution = 1.0;

        for (int j = 0; j < pc.bounceCount; ++j)
        {
            // Generate a random direction in the hemisphere of the normal
            vec3 rayDir = randomHemisphereDirection(bounceNormal, seed);

            // Cast a ray from worldPos in the random direction. If hit, add occlusion
            float hitDistance;
            if (queryAO(tlas, origin, normal, rayDir, pc.radius, pc.bias, hitDistance))
            {
                vec3 hitPosition = origin + rayDir * hitDistance;
                occlusion += contribution;
                origin = hitPosition;
            }
        }
    }

    float ao = 1.0 - (occlusion / float(pc.sampleCount));

    imageStore(aoOutput, pixelCoord, vec4(ao, 0.0, 0.0, 0.0));
}