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

#version 460
layout(local_size_x=16,local_size_y=16,local_size_z=1) in;
layout(set=0,binding=0,std140) uniform PresentUniforms
{
    mat4 inverseViewProjection, stableViewProjection;
    uvec4 extent;
    vec4 rejection, options;
} dataBlock;
layout(set=0,binding=1) uniform texture2D stableSignal;
layout(set=0,binding=2) uniform texture2D stablePosition;
layout(set=0,binding=3) uniform texture2D stableNormal;
layout(set=0,binding=4) uniform texture2D normalImage;
layout(set=0,binding=5) uniform texture2D depthImage;
layout(set=0,binding=6) uniform texture2D currentSignal;
layout(set=0,binding=7) uniform sampler nearestSampler;
layout(set=0,binding=8,rgba16f) uniform image2D outputImage;
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    vec2 uv = (vec2(pixel)+0.5)/vec2(dataBlock.extent.zw);
    ivec2 receiver = min(ivec2(uv*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
    vec2 receiverUV = (vec2(receiver)+0.5)/vec2(dataBlock.extent.xy);
    float depth = texelFetch(sampler2D(depthImage,nearestSampler),receiver,0).r;
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),receiver,0).xyz;
    vec3 decoded = encoded*2.0-1.0;
    vec4 world = dataBlock.inverseViewProjection*vec4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,depth,1.0);
    bool covered = (!any(isnan(world)) && !any(isinf(world))) && (!any(isnan(vec4(decoded,depth))) && !any(isinf(vec4(decoded,depth)))) &&
        world.w > 1e-8 && depth >= 0.0 && depth < 1.0 && dot(encoded,encoded) > 1e-8 && dot(decoded,decoded) > 1e-8;
    vec4 result = vec4(0);
    if (covered)
    {
        vec3 position = world.xyz/world.w;
        vec3 normal = normalize(decoded);
        vec4 clip = dataBlock.stableViewProjection*vec4(position,1);
        if (any(isnan(clip)) || any(isinf(clip)) || clip.w <= 1e-8)
        {
            imageStore(outputImage,ivec2(pixel),vec4(0));
            return;
        }
        vec2 stableUV = uv+(clip.xy/clip.w*vec2(0.5,-0.5)+0.5)-receiverUV;
        vec2 q = stableUV*vec2(dataBlock.extent.zw)-0.5;
        ivec2 base = ivec2(floor(q));
        vec2 fraction = q-vec2(base);
        bool nearest = dataBlock.rejection.w > 0.5;
        if (nearest)
        {
            base = ivec2(floor(q+0.5));
        }
        float tolerance = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.options.xyz);
        vec4 sum = vec4(0);
        float validWeight = 0.0;
        for (int tap = 0; tap < 4; ++tap)
        {
            if (nearest && tap != 0)
            {
                break;
            }
            ivec2 offset = ivec2(tap & 1,tap >> 1);
            ivec2 p = base+offset;
            float weight = nearest ? 1.0 : (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
                (offset.y == 0 ? 1.0-fraction.y : fraction.y);
            if (weight <= 0.0 || any(lessThan(p,ivec2(0))) || any(greaterThanEqual(p,ivec2(dataBlock.extent.zw))))
            {
                continue;
            }
            vec4 point = texelFetch(sampler2D(stablePosition,nearestSampler),p,0);
            vec4 direction = texelFetch(sampler2D(stableNormal,nearestSampler),p,0);
            vec4 value = texelFetch(sampler2D(stableSignal,nearestSampler),p,0);
            if ((any(isnan(point)) || any(isinf(point))) || (any(isnan(direction)) || any(isinf(direction))) || (any(isnan(value)) || any(isinf(value))) ||
                point.w <= 0.5 || direction.w < 1.0 ||
                length(point.xyz-position) > tolerance || dot(direction.xyz,normal) < dataBlock.rejection.z ||
                (dataBlock.options.w < 0.5 && value.a <= 0.5))
            {
                continue;
            }
            sum += value*weight;
            validWeight += weight;
        }
        if (validWeight > 1e-6)
        {
            result = sum/validWeight;
        }
        else
        {
            // A presentation hole may use this frame's raw GI, never its history.
            result = dataBlock.options.w < 0.5 ? texelFetch(sampler2D(currentSignal,nearestSampler),ivec2(pixel),0) : vec4(0.25);
        }
    }
    if (any(isnan(result)) || any(isinf(result)))
    {
        result = vec4(0);
    }
    imageStore(outputImage,ivec2(pixel),result);
}
