#include <metal_stdlib>
using namespace metal;
#include "common.metal"
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]], texturecube<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    float3 sum = float3(0.0);
    for (uint i = 0u; i < params.samples; ++i)
    {
        float2 xi = hammersley(i, params.samples);
        float phi = 2.0 * PI * xi.x;
        float3 local = float3(cos(phi) * sqrt(xi.y), sin(phi) * sqrt(xi.y), sqrt(1.0 - xi.y));
        float3 l = tangentToWorld(local, n);
        float lod = clamp(0.5 * log2(max(6.0 * float(params.sourceSize) * float(params.sourceSize) / float(params.samples), 1.0)), 0.0, params.sourceMaxLod);
        sum += sourceImage.sample(sourceSampler, l, level(lod)).rgb;
    }
    return float4(sum * (PI / float(params.samples)), 1.0);
}
