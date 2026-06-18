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

#include "lib/deferred_utils.glsl"

// Input images (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_InputImage;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Depth;

// Output image (RGBA16F for HDR GI irradiance)
layout(set = 0, binding = 3, rgba16f) uniform image2D outImage;

layout(push_constant) uniform PushConstant {
    vec2        outputSize;
    int         kernelRadius;
    float       depthThreshold;
    float       normalThreshold;
    float       depthSigma;
    float       normalSigma;
    uint        padding;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main() {
    ivec2 imgSize = imageSize(outImage);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= imgSize.x || pixelCoord.y >= imgSize.y) {
        return;
    }

    vec2 uv = vec2(pixelCoord + 0.5) / pc.outputSize;

    vec3 centerNormal = texture(g_Normal, uv).xyz * 2.0 - 1.0;
    float centerDepth = texture(g_Depth, uv).r;

    if (centerDepth >= 1.0) {
        imageStore(outImage, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }

    vec3 sum = vec3(0.0);
    float weightSum = 0.0;
    int radius = pc.kernelRadius;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            vec2 neighborUV = uv + vec2(float(x), float(y)) / pc.outputSize;
            vec3 neighborInput = texture(g_InputImage, neighborUV).rgb;
            vec3 neighborNormal = texture(g_Normal, neighborUV).xyz * 2.0 - 1.0;
            float neighborDepth = texture(g_Depth, neighborUV).r;

            float spatialDist = float(x * x + y * y);
            float spatialWeight = exp(-spatialDist / (2.0 * float(radius * radius)));

            float depthDiff = abs(centerDepth - neighborDepth);
            float depthWeight = depthDiff < pc.depthThreshold ? 1.0
                : exp(-(depthDiff * depthDiff) / (2.0 * pc.depthSigma * pc.depthSigma));

            float normalDot = clamp(dot(centerNormal, neighborNormal), 0.0, 1.0);
            float normalWeight = normalDot > pc.normalThreshold ? 1.0
                : exp(-(1.0 - normalDot) / pc.normalSigma);

            float weight = spatialWeight * depthWeight * normalWeight;
            sum += neighborInput * weight;
            weightSum += weight;
        }
    }

    vec3 result = sum / max(weightSum, 0.0001);
    imageStore(outImage, pixelCoord, vec4(result, 1.0));
}
