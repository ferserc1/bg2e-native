#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec2 uv = pixel / float(params.size);
    float nv = clamp(uv.x, 0.0001, 0.9999);
    float roughness = clamp(uv.y, 0.0001, 0.9999);
    vec3 v = vec3(sqrt(1.0 - nv * nv), 0.0, nv);
    vec2 result = vec2(0.0);
    for (uint i = 0u; i < params.samples; ++i)
    {
        vec3 h = sampleGGX(hammersley(i, params.samples), vec3(0, 0, 1), roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);
        float nl = max(l.z, 0.0);
        float nh = max(h.z, 0.0);
        float vh = clamp(dot(v, h), 0.0, 1.0);
        if (nl > 0.0)
        {
            float g = geometryIBL(nv, roughness) * geometryIBL(nl, roughness);
            float visibility = g * vh / max(nh * nv, 0.000001);
            float fc = pow(1.0 - vh, 5.0);
            result += vec2((1.0 - fc) * visibility, fc * visibility);
        }
    }
    return vec4(result / float(params.samples), 0.0, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
