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
layout(set = 0, binding = 0) uniform sampler2D g_CurrentAO;
layout(set = 0, binding = 1) uniform sampler2D g_HistoryAO;
layout(set = 0, binding = 2) uniform sampler2D g_Depth;
layout(set = 0, binding = 3) uniform sampler2D g_Normal;

// Output image (set=0)
layout(set = 0, binding = 4, rgba16f) uniform image2D outAccumulated;

// Previous frame g-buffer images (set=0)
layout(set = 0, binding = 5) uniform sampler2D g_HistoryDepth;
layout(set = 0, binding = 6) uniform sampler2D g_HistoryNormal;

// Push constants
layout(push_constant) uniform PushConstant {
    mat4  currentInverseViewProjection;
    mat4  previousViewProjection;
    vec2  outputSize;
    float historyWeight;
    uint  accumulatedFrameCount;
    uint  useProgressiveMode;
    uint  hasHistory;
    float depthThreshold;
    float normalThreshold;
    uint  isHDR;
    uint  padding0;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main() {
    ivec2 imageSize = imageSize(outAccumulated);
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    vec2 uv = vec2(pixelCoord + 0.5) / pc.outputSize;

    float currentDepth = texture(g_Depth, uv).r;

    if (currentDepth >= 1.0) {
        if (pc.isHDR == 0u) {
            float currentAO = texture(g_CurrentAO, uv).r;
            imageStore(outAccumulated, pixelCoord, vec4(currentAO, 0.0, 0.0, 0.0));
        } else {
            imageStore(outAccumulated, pixelCoord, vec4(0.0));
        }
        return;
    }

    vec3 currentNormal = normalize(texture(g_Normal, uv).xyz * 2.0 - 1.0);
    vec3 worldPos = reconstructWorldPosition(
        uv,
        currentDepth,
        pc.currentInverseViewProjection
    );

    vec4 previousClip = pc.previousViewProjection * vec4(worldPos, 1.0);
    vec2 previousNDC = previousClip.xy / previousClip.w;
    vec2 previousUV = previousNDC * 0.5 + 0.5;
    previousUV.y = 1.0 - previousUV.y;

    bool validHistory = pc.hasHistory == 1u;

    if (validHistory &&
        previousClip.w > 0.0 &&
        (previousUV.x < 0.0 || previousUV.x > 1.0 ||
        previousUV.y < 0.0 || previousUV.y > 1.0)
    ) {
        validHistory = false;
    }

    if (validHistory) {
        vec2 historyUV = previousUV;
        float historyDepth = texture(g_HistoryDepth, historyUV).r;
        vec3 historyNormal = normalize(texture(g_HistoryNormal, historyUV).xyz * 2.0 - 1.0);

        float depthDiff = abs(currentDepth - historyDepth);
        if (depthDiff >= pc.depthThreshold) {
            validHistory = false;
        }

        float normalDot = dot(currentNormal, historyNormal);
        if (normalDot <= pc.normalThreshold) {
            validHistory = false;
        }
    }

    // Scalar AO mode
    if (pc.isHDR == 0u) {
        float currentAO = texture(g_CurrentAO, uv).r;
        float result;

        if (validHistory) {
            float previousAO = texture(g_HistoryAO, previousUV).r;

            if (pc.useProgressiveMode == 1u) {
                float weight = 1.0 / float(pc.accumulatedFrameCount + 1);
                result = mix(previousAO, currentAO, weight);
            } else {
                result = mix(previousAO, currentAO, 1.0 - pc.historyWeight);
            }
        } else {
            result = currentAO;
        }

        imageStore(outAccumulated, pixelCoord, vec4(result, 0.0, 0.0, 0.0));
        return;
    }

    // HDR RGBA reflection mode
    vec4 current = texture(g_CurrentAO, uv);

    // If the current frame has no valid reflection, do not preserve old reflection data.
    // This avoids ghost reflections when the current pixel should fall back to cubemap.
    if (current.a < 0.01) {
        imageStore(outAccumulated, pixelCoord, vec4(0.0));
        return;
    }

    if (!validHistory) {
        imageStore(outAccumulated, pixelCoord, current);
        return;
    }

    vec4 history = texture(g_HistoryAO, previousUV);

    if (history.a < 0.01) {
        imageStore(outAccumulated, pixelCoord, current);
        return;
    }

    float blendWeight;
    if (pc.useProgressiveMode == 1u) {
        blendWeight = 1.0 / float(pc.accumulatedFrameCount + 1);
    } else {
        blendWeight = 1.0 - pc.historyWeight;
    }

    vec3 resultRGB = mix(history.rgb, current.rgb, blendWeight);
    imageStore(outAccumulated, pixelCoord, vec4(resultRGB, 1.0));
}