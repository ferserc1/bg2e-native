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
#include "common.glsl"
layout(set = 0, binding = 0) uniform textureCube sourceImage;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    if (params.roughness <= 0.0001)
    {
        return vec4(textureLod(samplerCube(sourceImage, sourceSampler), n, 0.0).rgb, 1.0);
    }
    vec3 sum = vec3(0.0);
    float weight = 0.0;
    float a = params.roughness * params.roughness;
    for (uint i = 0u; i < params.samples; ++i)
    {
        vec3 h = sampleGGX(hammersley(i, params.samples), n, params.roughness);
        vec3 l = normalize(2.0 * dot(n, h) * h - n);
        float nl = max(dot(n, l), 0.0);
        if (nl > 0.0)
        {
            float nh = max(dot(n, h), 0.0);
            float denom = nh * nh * (a * a - 1.0) + 1.0;
            float distribution = a * a / max(PI * denom * denom, 0.000001);
            float pdf = max(distribution * nh / max(4.0 * nh, 0.000001), 0.000001);
            float texelArea = 4.0 * PI / (6.0 * float(params.sourceSize) * float(params.sourceSize));
            float sampleArea = 1.0 / (float(params.samples) * pdf);
            float lod = clamp(0.5 * log2(sampleArea / texelArea), 0.0, params.sourceMaxLod);
            sum += textureLod(samplerCube(sourceImage, sourceSampler), l, lod).rgb * nl;
            weight += nl;
        }
    }
    return vec4(sum / max(weight, 0.000001), 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
