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

#include "tracing.glsl"
#include "blue_noise.glsl"
#include "rtgi_payload.glsl"
layout(set=0,binding=0,std140) uniform GIUniforms
{
    mat4 inverseViewProjection;
    vec4 parameters;
    uvec4 counts;
    uvec4 noise;
    uvec4 extent;
    DrawLightUniforms lights;
} dataBlock;
layout(set=0,binding=1) uniform texture2D normalImage;
layout(set=0,binding=2) uniform texture2D positionImage;
layout(set=0,binding=3) uniform texture2D coverageImage;
layout(set=0,binding=4) uniform sampler nearestSampler;
layout(set=0,binding=5,rgba16f) uniform image2D giOutput;
#if GI_BLUE
layout(set=0,binding=6) uniform texture2D noiseImage;
layout(set=0,binding=7) uniform sampler noiseSampler;
#endif
layout(set=0,binding=8) uniform textureCube environmentImage;
layout(set=0,binding=9) uniform sampler environmentSampler;
layout(location=0) rayPayloadEXT GIHit giPayload;
GIHit giTrace(vec3 origin, vec3 direction, float maximum)
{
    giPayload.found = 0u;
    traceRayEXT(traceScene,0u,255u,0u,0u,0u,origin,dataBlock.parameters.y,direction,maximum,0);
    return giPayload;
}
vec3 giEnvironment(vec3 direction)
{
    // Shared environment irradiance includes PI. Output is incoming radiance/pi.
    return textureLod(samplerCube(environmentImage,environmentSampler),direction,0.0).rgb/3.14159265359;
}
vec3 giHemisphere(vec3 normal, vec2 xi)
{
    vec3 reference = abs(normal.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent = normalize(cross(reference,normal));
    float radius = sqrt(xi.x), angle = 6.28318530718*xi.y;
    return normalize(tangent*(radius*cos(angle))+cross(normal,tangent)*(radius*sin(angle))+
        normal*sqrt(max(0.0,1.0-xi.x)));
}
void main()
{
    uvec2 pixel = gl_LaunchIDEXT.xy;
    if (any(greaterThanEqual(pixel,dataBlock.extent.zw)))
    {
        return;
    }
    ivec2 sourcePixel = min(ivec2((vec2(pixel)+0.5)/vec2(dataBlock.extent.zw)*vec2(dataBlock.extent.xy)),ivec2(dataBlock.extent.xy)-1);
    vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),sourcePixel,0).xyz;
    vec3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if GI_WORLD
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
        imageStore(giOutput,ivec2(pixel),vec4(0));
        return;
    }
    vec3 radiance = vec3(0.0);
    for (uint sampleIndex = 0u; sampleIndex < dataBlock.counts.x; ++sampleIndex)
    {
        vec3 throughput = vec3(1.0), point = position, bounceNormal = normal;
        for (uint bounce = 0u; bounce < dataBlock.counts.y; ++bounce)
        {
            uint sequence = sampleIndex*dataBlock.counts.y+bounce;
#if GI_BLUE
            vec2 xi = bg2eBlueNoise2(noiseImage,noiseSampler,dataBlock.noise,ivec2(pixel),dataBlock.counts.w,sequence);
#else
            vec2 xi = bg2eHashNoise2(ivec2(pixel),dataBlock.counts.w,sequence);
#endif
            vec3 direction = giHemisphere(bounceNormal,xi);
            vec3 origin = point+bounceNormal*dataBlock.parameters.y;
            GIHit hit = giTrace(origin,direction,dataBlock.parameters.x);
            if (hit.found == 0u)
            {
                radiance += throughput*giEnvironment(direction);
                break;
            }
            point = origin+direction*hit.distance;
            bounceNormal = traceGeometricNormal(hit.instance,hit.primitive,direction);
            uint materialIndex = traceInstances[hit.instance].ranges.w;
            DrawMaterialData material = traceMaterials[materialIndex].values;
            vec4 uv = traceUV(hit.instance,hit.primitive,hit.bary);
            vec3 albedo = max(material.albedo.rgb*traceSample(materialIndex,0u,uv).rgb,vec3(0.0));
            float emission = max(material.emissionAlpha.x*drawScalarSample(traceSample(materialIndex,5u,uv),material.channels.w,material.invert.z),0.0);
            vec3 outgoing = albedo*emission;
            for (uint lightIndex = 0u; lightIndex < dataBlock.lights.count.x; ++lightIndex)
            {
                DrawLightData light = dataBlock.lights.lights[lightIndex];
                // Draw LightData's existing participation bit is shared with reflections.
                if (light.flags.w == 0u)
                {
                    continue;
                }
                vec3 delta = light.positionIntensity.xyz-point;
                float d2 = max(dot(delta,delta),1e-4);
                vec3 toLight = light.flags.x == 4u ? -light.directionAngle.xyz : delta;
                float visibility = 0.0;
                uint shadows = uint(dataBlock.parameters.z);
                for (uint shadow = 0u; shadow < shadows; ++shadow)
                {
                    vec2 jitter = bg2eHashNoise2(ivec2(pixel),dataBlock.counts.w,
                        sequence*4099u+lightIndex*67u+shadow+100000u);
                    float z = 2.0*jitter.x-1.0, angle = 6.28318530718*jitter.y;
                    vec3 offset = vec3(sqrt(max(0.0,1.0-z*z))*cos(angle),sqrt(max(0.0,1.0-z*z))*sin(angle),z);
                    vec3 shadowVector = toLight+offset*max(light.parameters.y,0.0);
                    float distance = light.flags.x == 4u ? dataBlock.parameters.x : length(shadowVector);
                    vec3 shadowDirection = dot(shadowVector,shadowVector) > 1e-12 ? normalize(shadowVector) : bounceNormal;
                    if (dataBlock.parameters.w == 0.0 || light.flags.y == 0u || distance <= 2.0*dataBlock.parameters.y)
                    {
                        visibility += 1.0;
                    }
                    else
                    {
                        GIHit blocker = giTrace(point+bounceNormal*dataBlock.parameters.y,shadowDirection,
                            max(dataBlock.parameters.y*1.01,distance-dataBlock.parameters.y));
                        visibility += blocker.found == 0u ? 1.0 : 0.0;
                    }
                }
                vec3 l = dot(toLight,toLight) > 1e-12 ? normalize(toLight) : bounceNormal;
                float attenuation = light.flags.x == 4u ? 1.0 : 1.0/d2;
                if (light.flags.x == 1u)
                {
                    float outer = cos(light.directionAngle.w*3.14159265359/180.0);
                    float inner = cos(light.parameters.x*3.14159265359/180.0);
                    attenuation *= clamp((dot(l,-light.directionAngle.xyz)-outer)/max(inner-outer,1e-5),0.0,1.0);
                }
                outgoing += albedo*light.color.rgb*light.positionIntensity.w*attenuation*
                    max(dot(bounceNormal,l),0.0)*(visibility/float(shadows))/3.14159265359;
            }
            radiance += throughput*outgoing;
            throughput *= albedo;
            if (bounce+1u == dataBlock.counts.y)
            {
                // Terminal ambient closes the path once, never once per intermediate hit.
                radiance += throughput*giEnvironment(bounceNormal);
            }
        }
    }
    vec3 result = max(radiance/float(dataBlock.counts.x),vec3(0.0));
    bool invalid = any(isnan(result)) || any(isinf(result));
    imageStore(giOutput,ivec2(pixel),invalid ? vec4(0.0) : vec4(min(result,vec3(65504.0)),1.0));
}
