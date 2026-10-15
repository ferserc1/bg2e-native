#include <metal_stdlib>
using namespace metal;
#include "common.metal"
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]], texture2d<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    float2 uv = float2(atan2(n.z, n.x) / (2.0 * PI) + 0.5, acos(clamp(n.y, -1.0, 1.0)) / PI);
    float lod = clamp(log2(max(float(params.sourceSize) / (4.0 * float(params.size)), 1.0)), 0.0, params.sourceMaxLod);
    return float4(sourceImage.sample(sourceSampler, uv, level(lod)).rgb, 1.0);
}
