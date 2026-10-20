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

#include "tracing.metal"
#include "blue_noise.metal"
struct AOBlock {
    float4x4 inverseViewProjection;
    float4 parameters;
    uint4 counts;
    uint4 noise;
    uint4 extent;
};
float3 aoHemisphere(float3 normal, float2 xi)
{
    float3 reference = abs(normal.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 tangent = normalize(cross(reference,normal));
    float radius = sqrt(xi.x), angle = 6.28318530718*xi.y;
    return normalize(tangent*(radius*cos(angle))+cross(normal,tangent)*(radius*sin(angle))+
        normal*sqrt(max(0.0,1.0-xi.x)));
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant AOBlock& dataBlock [[buffer(1)]],
    const device TraceVertex* vertices [[buffer(2)]],
    const device uint* indices [[buffer(3)]],
    const device TraceInstance* instances [[buffer(4)]],
    const device TraceMaterial* materials [[buffer(5)]],
    instance_acceleration_structure scene [[buffer(6)]],
    constant TraceImageArgument* traceImages [[buffer(8)]],
    constant TraceSamplerArgument* traceSamplers [[buffer(9)]],
    texture2d<float> normalImage [[texture(0)]],
#if AO_WORLD
    texture2d<float> positionImage [[texture(1)]],
#else
    depth2d<float> positionImage [[texture(1)]],
#endif
    texture2d<float> coverageImage [[texture(2)]],
#if AO_BLUE
    texture2d<float> noiseImage [[texture(3)]],
#endif
    texture2d<float,access::write> aoOutput [[texture(4)]])
{
    if (any(pixel >= dataBlock.extent.zw))
    {
        return;
    }
    uint2 sourcePixel = min(uint2((float2(pixel)+0.5)/float2(dataBlock.extent.zw)*float2(dataBlock.extent.xy)),dataBlock.extent.xy-1u);
    float3 encoded = normalImage.read(sourcePixel).xyz;
    float3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if AO_WORLD
    covered = covered && coverageImage.read(sourcePixel).r > 0.0;
    position = positionImage.read(sourcePixel).xyz;
    normal = normalize(encoded);
#else
    float depth = positionImage.read(sourcePixel);
    covered = covered && depth < 1.0;
    float2 uv = (float2(sourcePixel)+0.5)/float2(dataBlock.extent.xy);
    float4 world = dataBlock.inverseViewProjection*float4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    covered = covered && abs(world.w) > 1e-8;
    position = world.xyz/(abs(world.w) > 1e-8 ? world.w : 1.0);
    normal = normalize(encoded*2.0-1.0);
#endif
    // Bit-level finite check remains meaningful under Metal fast math.
    if (!covered || any((as_type<uint3>(position) & uint3(0x7f800000u)) == uint3(0x7f800000u)) ||
        any((as_type<uint3>(normal) & uint3(0x7f800000u)) == uint3(0x7f800000u)))
    {
        aoOutput.write(float4(1),pixel);
        return;
    }
    const TraceData data{vertices,indices,instances,materials,traceImages,traceSamplers};
    float occlusion = 0.0;
    for (uint sampleIndex = 0u; sampleIndex < dataBlock.counts.x; ++sampleIndex)
    {
        float3 point = position, bounceNormal = normal;
        float contribution = 1.0;
        for (uint bounce = 0u; bounce < dataBlock.counts.y; ++bounce)
        {
            uint sequence = sampleIndex*dataBlock.counts.y+bounce;
#if AO_BLUE
            float2 xi = bg2eBlueNoise2(noiseImage,dataBlock.noise,int2(pixel),dataBlock.counts.w,sequence);
#else
            float2 xi = bg2eHashNoise2(int2(pixel),dataBlock.counts.w,sequence);
#endif
            float3 direction = aoHemisphere(bounceNormal,xi);
            float3 origin = point+bounceNormal*dataBlock.parameters.y;
            TraceHit hit = traceNearest(origin,direction,dataBlock.parameters.y,dataBlock.parameters.x,dataBlock.counts.z,scene,data);
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
    aoOutput.write(float4(visibility),pixel);
}
