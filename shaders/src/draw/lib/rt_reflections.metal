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
struct ReflectionBlock {
    float4x4 inverseViewProjection;
    float4 parameters;
    uint4 counts;
    uint4 noise;
    uint4 extent;
    float4 cameraRoughness;
    float4 spread;
    DrawLightUniforms lights;
};
struct GIHit { uint found; uint instance; uint primitive; float distance; float2 bary; };
GIHit giTrace(float3 origin, float3 direction, float maximum, float bias, uint count,
    instance_acceleration_structure scene, thread const TraceData& data)
{
    ray queryRay;
    queryRay.origin = origin;
    queryRay.direction = direction;
    queryRay.min_distance = bias;
    queryRay.max_distance = maximum;
    intersection_params parameters;
    parameters.assume_geometry_type(geometry_type::triangle);
    parameters.accept_any_intersection(false);
    intersection_query<instancing,triangle_data> query(queryRay,scene,255u,parameters);
    while (query.next())
    {
        if (query.get_candidate_intersection_type() == intersection_type::triangle &&
            traceAccept(query.get_candidate_instance_id(),query.get_candidate_primitive_id(),
                query.get_candidate_triangle_barycentric_coord(),count,data))
        {
            query.commit_triangle_intersection();
        }
    }
    GIHit result{};
    if (query.get_committed_intersection_type() == intersection_type::triangle)
    {
        result.found = 1u;
        result.instance = query.get_committed_instance_id();
        result.primitive = query.get_committed_primitive_id();
        result.distance = query.get_committed_distance();
        result.bary = query.get_committed_triangle_barycentric_coord();
    }
    return result;
}
float3 reflectionNormal(uint instance, uint primitive, float2 bary, float3 direction, thread const TraceData& data)
{
    TraceInstance record = data.instances[instance];
    uint first = record.ranges.x+primitive*3u;
    TraceVertex a = data.vertices[record.ranges.y+data.indices[first]];
    TraceVertex b = data.vertices[record.ranges.y+data.indices[first+1u]];
    TraceVertex c = data.vertices[record.ranges.y+data.indices[first+2u]];
    float3 weights = float3(1.0-bary.x-bary.y,bary);
    float3 n = float3x3(record.normal[0].xyz,record.normal[1].xyz,record.normal[2].xyz)*(a.normal.xyz*weights.x+b.normal.xyz*weights.y+c.normal.xyz*weights.z);
    if (dot(n,n) <= 1e-12)
    {
        return traceGeometricNormal(instance,primitive,direction,data);
    }
    n = normalize(n);
    // Source tangents have XYZ only. The raster convention uses +1 object sign,
    // with determinant sign applied after the model transform.
    float3 t = float3x3(record.model[0].xyz,record.model[1].xyz,record.model[2].xyz)*(a.tangent.xyz*weights.x+b.tangent.xyz*weights.y+c.tangent.xyz*weights.z);
    t -= n*dot(n,t);
    if (dot(t,t) < 1e-8)
    {
        t = cross(abs(n.z) < 0.99 ? float3(0,0,1) : float3(0,1,0),n);
    }
    t = normalize(t);
    float4 uv = traceUV(instance,primitive,bary,data);
    float3 mapped = traceSample(record.ranges.w,3u,uv,data).xyz*2.0-1.0;
    float sign = record.identity.z != 0u ? -1.0 : 1.0;
    float3 candidate = t*mapped.x+cross(n,t)*sign*mapped.y+n*mapped.z;
    if (dot(candidate,candidate) > 1e-12)
    {
        n = normalize(candidate);
    }
    return dot(n,direction) > 0.0 ? -n : n;
}
float3 reflectionDirection(float2 xi, float3 direction, float roughness)
{
    float a = max(roughness*roughness,0.001);
    float phi = 6.28318530718*xi.x;
    float cosine = sqrt((1.0-xi.y)/max(1.0+(a*a-1.0)*xi.y,1e-8));
    float sine = sqrt(max(0.0,1.0-cosine*cosine));
    float3 up = abs(direction.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 tangent = normalize(cross(up,direction));
    return normalize(tangent*cos(phi)*sine+cross(direction,tangent)*sin(phi)*sine+direction*cosine);
}
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant ReflectionBlock& dataBlock [[buffer(1)]],
    const device TraceVertex* vertices [[buffer(2)]],
    const device uint* indices [[buffer(3)]],
    const device TraceInstance* instances [[buffer(4)]],
    const device TraceMaterial* materials [[buffer(5)]],
    instance_acceleration_structure scene [[buffer(6)]],
    constant TraceImageArgument* traceImages [[buffer(8)]],
    constant TraceSamplerArgument* traceSamplers [[buffer(9)]],
    texture2d<float> normalImage [[texture(0)]],
#if REF_WORLD
    texture2d<float> positionImage [[texture(1)]],
#else
    depth2d<float> positionImage [[texture(1)]],
#endif
    texture2d<float> coverageImage [[texture(2)]],
#if REF_BLUE
    texture2d<float> noiseImage [[texture(3)]],
#endif
    texture2d<float,access::write> reflectionOutput [[texture(4)]],
    texturecube<float> environmentImage [[texture(5)]],
    sampler environmentSampler [[sampler(11)]],
    texture2d<float> materialImage [[texture(6)]])
{
    if (any(pixel >= dataBlock.extent.zw))
    {
        return;
    }
    uint2 sourcePixel = min(uint2((float2(pixel)+0.5)/float2(dataBlock.extent.zw)*float2(dataBlock.extent.xy)),dataBlock.extent.xy-1u);
    float3 encoded = normalImage.read(sourcePixel).xyz;
    float3 position, normal;
    bool covered = dot(encoded,encoded) > 1e-8;
#if REF_WORLD
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
        reflectionOutput.write(float4(0),pixel);
        return;
    }
    const TraceData data{vertices,indices,instances,materials,traceImages,traceSamplers};
    float roughness = materialImage.read(sourcePixel).g;
    float3 viewRay = position-dataBlock.cameraRoughness.xyz;
    if ((as_type<uint>(roughness) & 0x7f800000u) == 0x7f800000u || roughness > dataBlock.cameraRoughness.w || dot(viewRay,viewRay) <= 1e-12)
    {
        reflectionOutput.write(float4(0),pixel);
        return;
    }
    roughness = clamp(roughness,0.0,1.0);
    float3 reflected = normalize(reflect(normalize(viewRay),normal));
    uint samples = dataBlock.counts.x;
    if (roughness <= 0.01)
    {
        samples = 1u;
    }
    else
    {
        float factor = smoothstep(0.0,1.0,clamp((roughness-0.01)/max(dataBlock.cameraRoughness.w-0.01,1e-4),0.0,1.0));
        samples = uint(ceil(mix(float(min(samples,2u)),float(samples),factor)));
    }
    float3 radiance = float3(0.0);
    uint hits = 0u;
    for (uint sampleIndex = 0u; sampleIndex < samples; ++sampleIndex)
    {
        uint sequence = sampleIndex;
#if REF_BLUE
        float2 xi = bg2eBlueNoise2(noiseImage,dataBlock.noise,int2(pixel),dataBlock.counts.w,sequence);
#else
        float2 xi = bg2eHashNoise2(int2(pixel),dataBlock.counts.w,sequence);
#endif
        float3 direction = reflected;
        if (roughness > 0.01)
        {
            direction = reflectionDirection(xi,reflected,roughness*dataBlock.spread.x);
            if (dot(direction,normal) <= 0.01)
            {
                direction = normalize(reflected+normal*0.1);
            }
        }
        GIHit hit = giTrace(position+normal*dataBlock.parameters.y,direction,dataBlock.parameters.x,dataBlock.parameters.y,dataBlock.counts.z,scene,data);
        if (hit.found == 0u)
        {
            continue;
        }
        float3 point = position+normal*dataBlock.parameters.y+direction*hit.distance;
        float3 bounceNormal = reflectionNormal(hit.instance,hit.primitive,hit.bary,direction,data);
        uint materialIndex = data.instances[hit.instance].ranges.w;
        DrawMaterialData material = data.materials[materialIndex].values;
        float4 uv = traceUV(hit.instance,hit.primitive,hit.bary,data);
        float3 albedo = max(material.albedo.rgb*traceSample(materialIndex,0u,uv,data).rgb,float3(0));
        float emission = max(material.emissionAlpha.x*drawScalarSample(traceSample(materialIndex,5u,uv,data),material.channels.w,material.invert.z),0.0);
        float3 outgoing = albedo*emission+albedo*(environmentImage.sample(environmentSampler,bounceNormal,level(0.0)).rgb/3.14159265359);
        for (uint lightIndex = 0u; lightIndex < dataBlock.lights.count.x; ++lightIndex)
        {
            DrawLightData light = dataBlock.lights.lights[lightIndex];
            // Draw LightData's existing participation bit is shared with reflections.
            if (light.flags.w == 0u)
            {
                continue;
            }
            float3 delta = light.positionIntensity.xyz-point;
            float d2 = max(dot(delta,delta),1e-4);
            float3 toLight = light.flags.x == 4u ? -light.directionAngle.xyz : delta;
            float visibility = 0.0;
            uint shadows = uint(dataBlock.parameters.z);
            for (uint shadow = 0u; shadow < shadows; ++shadow)
            {
                float2 jitter = bg2eHashNoise2(int2(pixel),dataBlock.counts.w,
                    sequence*4099u+lightIndex*67u+shadow+100000u);
                float z = 2.0*jitter.x-1.0, angle = 6.28318530718*jitter.y;
                float3 offset = float3(sqrt(max(0.0,1.0-z*z))*cos(angle),sqrt(max(0.0,1.0-z*z))*sin(angle),z);
                float3 shadowVector = toLight+offset*max(light.parameters.y,0.0);
                float distance = light.flags.x == 4u ? dataBlock.parameters.x : length(shadowVector);
                float3 shadowDirection = dot(shadowVector,shadowVector) > 1e-12 ? normalize(shadowVector) : bounceNormal;
                if (dataBlock.parameters.w == 0.0 || light.flags.y == 0u || distance <= 2.0*dataBlock.parameters.y)
                {
                    visibility += 1.0;
                }
                else
                {
                    GIHit blocker = giTrace(point+bounceNormal*dataBlock.parameters.y,shadowDirection,
                        max(dataBlock.parameters.y*1.01,distance-dataBlock.parameters.y),dataBlock.parameters.y,dataBlock.counts.z,scene,data);
                    visibility += blocker.found == 0u ? 1.0 : 0.0;
                }
            }
            float3 l = dot(toLight,toLight) > 1e-12 ? normalize(toLight) : bounceNormal;
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
        if (!any((as_type<uint3>(outgoing) & uint3(0x7f800000u)) == uint3(0x7f800000u)))
        {
            radiance += clamp(outgoing,float3(0),float3(65504));
            hits++;
        }
    }
    float3 result = hits > 0u ? radiance/float(hits) : float3(0);
    reflectionOutput.write(float4(result,float(hits)/float(samples)),pixel);
}
