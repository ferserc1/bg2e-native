#version 460

layout(set = 0, binding = 0) uniform sampler2D composedSample;
layout(set = 0, binding = 1) uniform sampler2D previousHistory;
layout(set = 0, binding = 2) uniform usampler2D validTexelMask;
layout(set = 0, binding = 3, rgba32f) uniform image2D nextHistory;

layout(push_constant) uniform PushConstant {
    uint previousSamples;
} pc;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 extent = imageSize(nextHistory);
    if (pixel.x >= extent.x || pixel.y >= extent.y) return;

    if (texelFetch(validTexelMask, pixel, 0).r == 0u)
    {
        imageStore(nextHistory, pixel, vec4(1.0, 1.0, 1.0, 0.0));
        return;
    }

    vec3 sampleValue = texelFetch(composedSample, pixel, 0).rgb;
    vec3 value = sampleValue;
    if (pc.previousSamples != 0u)
    {
        vec3 previous = texelFetch(previousHistory, pixel, 0).rgb;
        value = previous + (sampleValue - previous) / float(pc.previousSamples + 1u);
    }
    imageStore(nextHistory, pixel, vec4(value, 1.0));
}
