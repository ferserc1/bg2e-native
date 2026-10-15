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
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    float altitude = max(n.y, 0.0);
    vec3 sky = mix(vec3(0.55, 0.65, 0.8), vec3(0.06, 0.22, 0.6), sqrt(altitude));
    vec3 ground = vec3(0.04, 0.035, 0.025);
    vec3 color = mix(ground, sky, smoothstep(-0.03, 0.03, n.y));
    vec3 sun = normalize(vec3(0.4, 0.7, 0.2));
    color += vec3(8.0, 6.5, 4.0) * pow(max(dot(n, sun), 0.0), 512.0);
    return vec4(color, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
