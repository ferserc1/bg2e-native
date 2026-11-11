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

// New samples are filtered once into the stable grid. This helper never reads history.
vec4 stableGICurrent(ivec2 pixel, vec3 centerPosition, vec3 centerNormal)
{
    if (all(equal(dataBlock.stableJitter.xy,vec2(0))))
    {
        return texelFetch(sampler2D(currentImage,nearestSampler),pixel,0);
    }
    vec2 currentUV = (vec2(pixel)+0.5)/vec2(dataBlock.extent.zw)+dataBlock.stableJitter.xy;
    ivec2 centerReceiver = clamp(ivec2(currentUV*vec2(dataBlock.extent.xy)),ivec2(0),ivec2(dataBlock.extent.xy)-1);
#if TEMPORAL_METADATA
    uint centerToken = texelFetch(usampler2D(tokenImage,nearestSampler),centerReceiver,0).r;
#endif
    vec2 q = currentUV*vec2(dataBlock.extent.zw)-0.5;
    ivec2 base = ivec2(floor(q));
    vec2 fraction = q-vec2(base);
    vec4 sum = vec4(0);
    float weightSum = 0.0;
    float tolerance = dataBlock.rejection.x+dataBlock.rejection.y*length(centerPosition-dataBlock.cameraRoughness.xyz);
    for (int tap = 0; tap < 4; ++tap)
    {
        ivec2 offset = ivec2(tap & 1,tap >> 1);
        ivec2 candidate = base+offset;
        float weight = (offset.x == 0 ? 1.0-fraction.x : fraction.x)*
            (offset.y == 0 ? 1.0-fraction.y : fraction.y);
        if (weight <= 0.0 || any(lessThan(candidate,ivec2(0))) || any(greaterThanEqual(candidate,ivec2(dataBlock.extent.zw))))
        {
            continue;
        }
        vec2 uv = (vec2(candidate)+0.5)/vec2(dataBlock.extent.zw);
        ivec2 receiver = min(ivec2(uv*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
#if TEMPORAL_METADATA
        if (dataBlock.flags.y != 0u && (centerToken == 0u || texelFetch(usampler2D(tokenImage,nearestSampler),receiver,0).r != centerToken))
        {
            continue;
        }
#endif
        float depth = texelFetch(sampler2D(positionImage,nearestSampler),receiver,0).r;
        vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),receiver,0).xyz;
        vec3 decoded = encoded*2.0-1.0;
        float normalLength = dot(decoded,decoded);
        vec2 receiverUV = (vec2(receiver)+0.5)/vec2(dataBlock.extent.xy);
        vec4 world = dataBlock.inverseViewProjection*vec4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,depth,1.0);
        vec4 value = texelFetch(sampler2D(currentImage,nearestSampler),candidate,0);
        if ((any(isnan(value)) || any(isinf(value))) || (any(isnan(world)) || any(isinf(world))) || (any(isnan(vec4(decoded,depth))) || any(isinf(vec4(decoded,depth)))) ||
            depth < 0.0 || depth >= 1.0 || dot(encoded,encoded) <= 1e-8 ||
            normalLength <= 1e-8 || world.w <= 1e-8 || value.a <= 0.5)
        {
            continue;
        }
        vec3 position = world.xyz/world.w;
        if (length(position-centerPosition) > tolerance ||
            dot(decoded/sqrt(normalLength),centerNormal) < dataBlock.rejection.z)
        {
            continue;
        }
        sum += value*weight;
        weightSum += weight;
    }
    return weightSum > 1e-6 ? vec4(sum.rgb/weightSum,1) : vec4(0);
}
