#version 450
layout(set = 0, binding = 0) uniform texture2D depthImage;
layout(set = 0, binding = 1) uniform sampler depthSampler;
layout(push_constant) uniform Motion
{
    mat4 currentFromJittered;
    mat4 previousFromJittered;
} pc;
layout(location = 0) out vec2 outMotion;
void main()
{
    vec2 size = vec2(textureSize(sampler2D(depthImage, depthSampler), 0));
    vec2 uv = gl_FragCoord.xy / size;
    float depth = texelFetch(sampler2D(depthImage, depthSampler), ivec2(gl_FragCoord.xy), 0).r;
    // Vulkan uses the engine's negative-height viewport, matching Metal Y.
    vec4 clip = vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    vec4 current = pc.currentFromJittered * clip;
    vec4 previous = pc.previousFromJittered * clip;
    outMotion = vec2(0.0);
    if (abs(current.w) > 1e-6 && previous.w > 1e-6)
    {
        outMotion = (previous.xy / previous.w - current.xy / current.w) * vec2(0.5, -0.5) * size;
    }
}
