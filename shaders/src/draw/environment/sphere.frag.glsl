#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(set = 0, binding = 0) uniform texture2D sourceImage;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    vec2 uv = vec2(atan(n.z, n.x) / (2.0 * PI) + 0.5, acos(clamp(n.y, -1.0, 1.0)) / PI);
    float lod = clamp(log2(max(float(params.sourceSize) / (4.0 * float(params.size)), 1.0)), 0.0, params.sourceMaxLod);
    return vec4(textureLod(sampler2D(sourceImage, sourceSampler), uv, lod).rgb, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
