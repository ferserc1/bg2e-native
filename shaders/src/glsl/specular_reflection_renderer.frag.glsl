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
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in flat int inCurrentMipLevel;
layout (location = 2) in flat int inTotalMipLevels;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube skyTexture;

float vanDerCorpus(int n, int base) {
    float invBase = 1.0 / float(base);
    float denom   = 1.0;
    float result  = 0.0;

    for(int i = 0; i < 8; ++i)
    {
        if(n > 0)
        {
            denom   = mod(float(n), 2.0);
            result += denom * invBase;
            invBase = invBase / 2.0;
            n       = int(float(n) / 2.0);
        }
    }

    return result;
}

vec2 hammersleyNoBitOps(int i, int N) {
    return vec2(float(i)/float(N), vanDerCorpus(i, 2));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness * roughness * roughness;
    
    float phi = 2.0 * 3.14159265359 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = sin(phi) * sinTheta;
    H.y = cos(phi) * sinTheta;
    H.z = cosTheta;
    
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
} 

void main()
{
    int sampleCount = 1024;
    vec3 maxRange = vec3(2.0);

    vec3 N = normalize(inNormal);
    vec3 R = N;
    vec3 V = R;

    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    float roughness = float(inCurrentMipLevel) / float(inTotalMipLevels - 1);
    if (roughness < 0.01)
    {
        prefilteredColor = clamp(texture(skyTexture, inNormal).rgb, vec3(0.0), maxRange);
    }
    else {
        for (int i = 0; i < sampleCount; ++i)
        {
            vec2 Xi = hammersleyNoBitOps(i, sampleCount);
            vec3 H = importanceSampleGGX(Xi, N, roughness);
            vec3 L = normalize(2.0 * dot(V, H) * H - V);

            float NdotL = max(dot(N,L), 0.0);
            if (NdotL > 0.0)
            {
                vec3 tex = texture(skyTexture, L).rgb;
                prefilteredColor += clamp(tex, vec3(0.0), maxRange) * NdotL;
                totalWeight += NdotL;
            }
        }
        prefilteredColor = prefilteredColor / totalWeight;
    }

    outFragColor = vec4(prefilteredColor, 1.0);
}
