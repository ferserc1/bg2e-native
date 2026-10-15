#version 450
layout(set = 0, binding = 0) uniform textureCube sky;
layout(set = 0, binding = 1) uniform sampler skySampler;
layout(push_constant) uniform Push
{
    mat4 inverseViewProjection;
    vec4 parameters;
} params;
layout(location = 0) out vec4 outColor;
void main()
{
    vec2 uv = gl_FragCoord.xy / params.parameters.xy;
    vec4 world = params.inverseViewProjection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    vec3 direction = normalize(world.xyz);
    outColor = vec4(textureLod(samplerCube(sky, skySampler), direction, params.parameters.z).rgb * exp2(params.parameters.w), 1.0);
}
