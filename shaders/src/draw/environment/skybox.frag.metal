#include <metal_stdlib>
using namespace metal;
struct SkyboxPush {
    float4x4 inverseViewProjection;
    float4 parameters;
};
fragment float4 fragMain(float4 position [[position]], constant SkyboxPush& params [[buffer(0)]],
    texturecube<float> sky [[texture(0)]], sampler skySampler [[sampler(0)]])
{
    float2 uv = position.xy / params.parameters.xy;
    float4 world = params.inverseViewProjection * float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    float3 direction = normalize(world.xyz);
    return float4(sky.sample(skySampler, direction, level(params.parameters.z)).rgb * exp2(params.parameters.w), 1.0);
}
