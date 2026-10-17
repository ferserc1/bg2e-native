/*
 *    business grade graphic engine (bg2e engine)
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

#define DRAW_TRACE_QUERIES 1
#include "tracing.glsl"
#include "blue_noise.glsl"
layout(local_size_x=16,local_size_y=16) in;
layout(set=0,binding=0,std140) uniform AOBlock
{
    mat4 inverseViewProjection;
    vec4 parameters;
    uvec4 counts;
    uvec4 noise;
    uvec4 extent;
} dataBlock;
layout(set=0,binding=1) uniform texture2D normalImage;
layout(set=0,binding=2) uniform texture2D positionImage;
layout(set=0,binding=3) uniform texture2D coverageImage;
layout(set=0,binding=4) uniform sampler nearestSampler;
layout(set=0,binding=5,r16f) uniform image2D aoOutput;
#if AO_BLUE
layout(set=0,binding=6) uniform texture2D noiseImage;
layout(set=0,binding=7) uniform sampler noiseSampler;
#endif
vec3 aoHemisphere(vec3 normal, vec2 xi)
{
    vec3 reference = abs(normal.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent = normalize(cross(reference,normal));
    float radius = sqrt(xi.x), angle = 6.28318530718*xi.y;
    return normalize(tangent*(radius*cos(angle))+cross(normal,tangent)*(radius*sin(angle))+
        normal*sqrt(max(0.0,1.0-xi.x)));
}
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    ivec2 sourcePixel = min(ivec2((vec2(pixel)+0.5)/vec2(dataBlock.extent.zw)*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),sourcePixel,0).xyz;
    vec3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if AO_WORLD
    covered = covered && texelFetch(sampler2D(coverageImage,nearestSampler),sourcePixel,0).r > 0.0;
    position = texelFetch(sampler2D(positionImage,nearestSampler),sourcePixel,0).xyz;
    normal = normalize(encoded);
#else
    float depth = texelFetch(sampler2D(positionImage,nearestSampler),sourcePixel,0).r;
    covered = covered && depth < 1.0;
    vec2 uv = (vec2(sourcePixel)+0.5)/vec2(dataBlock.extent.xy);
    vec4 world = dataBlock.inverseViewProjection*vec4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
#endif
    if (!covered || any(isnan(position)) || any(isinf(position)) || any(isnan(normal)) || any(isinf(normal)))
    {
        imageStore(aoOutput,ivec2(pixel),vec4(1));
        return;
    }
    float occlusion = 0.0;
    for (uint sampleIndex = 0u; sampleIndex < dataBlock.counts.x; ++sampleIndex)
    {
        vec3 point = position, bounceNormal = normal;
        float contribution = 1.0;
        for (uint bounce = 0u; bounce < dataBlock.counts.y; ++bounce)
        {
            uint sequence = sampleIndex*dataBlock.counts.y+bounce;
#if AO_BLUE
            vec2 xi = bg2eBlueNoise2(noiseImage,noiseSampler,dataBlock.noise,ivec2(pixel),dataBlock.counts.w,sequence);
#else
            vec2 xi = bg2eHashNoise2(ivec2(pixel),dataBlock.counts.w,sequence);
#endif
            vec3 direction = aoHemisphere(bounceNormal,xi);
            vec3 origin = point+bounceNormal*dataBlock.parameters.y;
            TraceHit hit = traceNearest(origin,direction,dataBlock.parameters.y,dataBlock.parameters.x,dataBlock.counts.z);
            if (!hit.hit)
            {
                break;
            }
            occlusion += contribution*pow(max(0.0,1.0-hit.distance/dataBlock.parameters.x),dataBlock.parameters.z);
            contribution *= dataBlock.parameters.w;
            point = origin+direction*hit.distance;
            bounceNormal = hit.normal;
        }
    }
    float visibility = 1.0-clamp(occlusion/float(dataBlock.counts.x),0.0,1.0);
    imageStore(aoOutput,ivec2(pixel),vec4(visibility));
}
