#include <metal_stdlib>
using namespace metal;
struct Motion {
    float4x4 currentFromJittered;
    float4x4 previousFromJittered;
};
fragment float2 fragMain(float4 position [[position]], depth2d<float> depth [[texture(0)]],
    sampler depthSampler [[sampler(0)]], constant Motion& pc [[buffer(0)]])
{
    float2 size = float2(depth.get_width(), depth.get_height());
    float2 uv = position.xy / size;
    // Metal NDC Y is opposite framebuffer Y.
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth.read(uint2(position.xy)), 1.0);
    float4 current = pc.currentFromJittered * clip;
    float4 previous = pc.previousFromJittered * clip;
    if (abs(current.w) > 1e-6 && previous.w > 1e-6)
    {
        return (previous.xy / previous.w - current.xy / current.w) * float2(0.5, -0.5) * size;
    }
    return float2(0.0);
}
