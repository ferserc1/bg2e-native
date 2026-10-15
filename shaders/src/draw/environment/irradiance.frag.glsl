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
    vec3 sum = vec3(0.0);
    for (uint i = 0u; i < params.samples; ++i)
    {
        vec2 xi = hammersley(i, params.samples);
        float phi = 2.0 * PI * xi.x;
        vec3 local = vec3(cos(phi) * sqrt(xi.y), sin(phi) * sqrt(xi.y), sqrt(1.0 - xi.y));
        vec3 l = tangentToWorld(local, n);
        float lod = clamp(0.5 * log2(max(6.0 * float(params.sourceSize) * float(params.sourceSize) / float(params.samples), 1.0)), 0.0, params.sourceMaxLod);
        sum += textureLod(samplerCube(sourceImage, sourceSampler), l, lod).rgb;
    }
    return vec4(sum * (PI / float(params.samples)), 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
