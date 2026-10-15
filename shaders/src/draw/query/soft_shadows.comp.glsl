#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_query : require
#include "../lib/bindings.glsl"
struct ShadowInstanceData {
    uvec4 ranges;
    vec4 alpha;
    uvec4 options;
};
struct ShadowTriangleUV {
    vec4 a;
    vec4 b;
    vec4 c;
};
struct ShadowUniforms {
    mat4 inverseViewProjection;
    vec4 parameters;
    uvec4 counts;
    DrawLightUniforms lights;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(std140, set = 0, binding = 0) uniform ShadowBlock { ShadowUniforms data; } queryData;
layout(std430, set = 0, binding = 1) readonly buffer InstanceBlock { ShadowInstanceData values[]; } instanceData;
layout(std430, set = 0, binding = 2) readonly buffer UVBlock { ShadowTriangleUV values[]; } uvData;
layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevel;
layout(set = 0, binding = 4) uniform texture2D normalImage;
layout(set = 0, binding = 5) uniform texture2D depthImage;
layout(set = 0, binding = 6) uniform sampler nearestSampler;
layout(rgba16f, set = 0, binding = 7) uniform writeonly image2D shadow0;
layout(rgba16f, set = 0, binding = 8) uniform writeonly image2D shadow1;
layout(set = 0, binding = 16) uniform texture2D alpha0;
layout(set = 0, binding = 24) uniform sampler alphaSampler0;
layout(set = 0, binding = 17) uniform texture2D alpha1;
layout(set = 0, binding = 25) uniform sampler alphaSampler1;
layout(set = 0, binding = 18) uniform texture2D alpha2;
layout(set = 0, binding = 26) uniform sampler alphaSampler2;
layout(set = 0, binding = 19) uniform texture2D alpha3;
layout(set = 0, binding = 27) uniform sampler alphaSampler3;
layout(set = 0, binding = 20) uniform texture2D alpha4;
layout(set = 0, binding = 28) uniform sampler alphaSampler4;
layout(set = 0, binding = 21) uniform texture2D alpha5;
layout(set = 0, binding = 29) uniform sampler alphaSampler5;
layout(set = 0, binding = 22) uniform texture2D alpha6;
layout(set = 0, binding = 30) uniform sampler alphaSampler6;
layout(set = 0, binding = 23) uniform texture2D alpha7;
layout(set = 0, binding = 31) uniform sampler alphaSampler7;
float sampleAlpha(uint index, vec2 uv)
{
    if (index == 0u)
    {
        return textureLod(sampler2D(alpha0, alphaSampler0), uv, 0.0).a;
    }
    if (index == 1u)
    {
        return textureLod(sampler2D(alpha1, alphaSampler1), uv, 0.0).a;
    }
    if (index == 2u)
    {
        return textureLod(sampler2D(alpha2, alphaSampler2), uv, 0.0).a;
    }
    if (index == 3u)
    {
        return textureLod(sampler2D(alpha3, alphaSampler3), uv, 0.0).a;
    }
    if (index == 4u)
    {
        return textureLod(sampler2D(alpha4, alphaSampler4), uv, 0.0).a;
    }
    if (index == 5u)
    {
        return textureLod(sampler2D(alpha5, alphaSampler5), uv, 0.0).a;
    }
    if (index == 6u)
    {
        return textureLod(sampler2D(alpha6, alphaSampler6), uv, 0.0).a;
    }
    if (index == 7u)
    {
        return textureLod(sampler2D(alpha7, alphaSampler7), uv, 0.0).a;
    }
    return 0.0;
}
bool acceptsCandidate(uint instance, uint primitive, vec2 bary)
{
    if (instance >= queryData.data.counts.y)
    {
        return false;
    }
    ShadowInstanceData data = instanceData.values[instance];
    if (data.ranges.z == 0xffffffffu)
    {
        return true;
    }
    if (primitive >= data.ranges.y)
    {
        return false;
    }
    ShadowTriangleUV triangle = uvData.values[data.ranges.x + primitive];
    vec4 interpolated = triangle.a * (1.0 - bary.x - bary.y) + triangle.b * bary.x + triangle.c * bary.y;
    vec2 uv = (data.options.x == 0u ? interpolated.xy : interpolated.zw) * data.alpha.xy;
    return sampleAlpha(data.ranges.z, uv) * data.alpha.w >= data.alpha.z;
}
bool visible(vec3 origin, vec3 direction, float distance)
{
    rayQueryEXT query;
    rayQueryInitializeEXT(query, topLevel, gl_RayFlagsTerminateOnFirstHitEXT, 0xffu, origin,
        queryData.data.parameters.x, direction, distance);
    while (rayQueryProceedEXT(query))
    {
        if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT &&
            acceptsCandidate(uint(rayQueryGetIntersectionInstanceIdEXT(query, false)),
                uint(rayQueryGetIntersectionPrimitiveIndexEXT(query, false)), rayQueryGetIntersectionBarycentricsEXT(query, false)))
        {
            rayQueryConfirmIntersectionEXT(query);
        }
    }
    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT;
}
uint shadowHash(uint v)
{
    v ^= v >> 16u;
    v *= 0x7feb352du;
    v ^= v >> 15u;
    v *= 0x846ca68bu;
    return v ^ (v >> 16u);
}
vec3 sampleLightDirection(vec3 direction, float angle, uint index, uint count, uint seed)
{
    vec3 up = abs(direction.z) < 0.99 ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 t = normalize(cross(up, direction));
    vec3 b = cross(direction, t);
    float radius = sqrt((float(index) + 0.5) / float(count)) * tan(min(angle, 1.4));
    float rotation = float(shadowHash(seed)) * (6.28318530718 / 4294967296.0);
    float phase = float(index) * 2.39996322973 + rotation;
    return normalize(direction + radius * (cos(phase) * t + sin(phase) * b));
}
void main()
{
    uvec2 pixel = gl_GlobalInvocationID.xy;
    uvec2 extent = uvec2(imageSize(shadow0));
    if (pixel.x >= extent.x || pixel.y >= extent.y)
    {
        return;
    }
    vec4 first = vec4(1.0);
    vec4 second = vec4(1.0);
    vec3 encoded = texelFetch(sampler2D(normalImage, nearestSampler), ivec2(pixel), 0).xyz;
    if (dot(encoded, encoded) > 1e-8)
    {
        vec3 normal = normalize(encoded * 2.0 - 1.0);
        float depth = texelFetch(sampler2D(depthImage, nearestSampler), ivec2(pixel), 0).r;
        vec2 uv = (vec2(pixel) + 0.5) / vec2(extent);
        vec4 world = queryData.data.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
        vec3 position = world.xyz / world.w;
        float bias = queryData.data.parameters.x;
        vec3 origin = position + normal * bias;
        for (uint i = 0u; i < queryData.data.counts.x; ++i)
        {
            DrawLightData light = queryData.data.lights.lights[i];
            float visibility = 1.0;
            if (light.flags.y != 0u)
            {
                vec3 delta = light.flags.x == 4u ? -light.directionAngle.xyz : light.positionIntensity.xyz - origin;
                float distance = light.flags.x == 4u ? queryData.data.parameters.y : length(delta) - bias;
                if (distance > bias && dot(delta, delta) > 1e-12)
                {
                    vec3 direction = normalize(delta);
                    float angle = light.flags.x == 4u ? light.parameters.y * 0.01745329251994 : atan(light.parameters.y / max(distance, bias));
                    uint count = min(max(light.flags.z, 1u), queryData.data.counts.w);
                    visibility = 0.0;
                    uint seed = pixel.x + pixel.y * extent.x + i * 104729u;
                    for (uint sampleIndex = 0u; sampleIndex < count; ++sampleIndex)
                    {
                        vec3 sampleDirection = count == 1u ? direction : sampleLightDirection(direction, angle, sampleIndex, count, seed);
                        visibility += visible(origin, sampleDirection, distance) ? 1.0 : 0.0;
                    }
                    visibility /= float(count);
                }
            }
            if (i < 4u)
            {
                first[i] = visibility;
            }
            else
            {
                second[i - 4u] = visibility;
            }
        }
    }
    imageStore(shadow0, ivec2(pixel), first);
    imageStore(shadow1, ivec2(pixel), second);
}
