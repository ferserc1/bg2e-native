#include <metal_stdlib>
using namespace metal;
#include "common.metal"
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]], texturecube<float> sourceImage [[texture(0)]], sampler sourceSampler [[sampler(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    if (params.roughness <= 0.0001)
    {
        return float4(sourceImage.sample(sourceSampler, n, level(0.0)).rgb, 1.0);
    }
    float3 sum = float3(0.0);
    float weight = 0.0;
    float a = params.roughness * params.roughness;
    for (uint i = 0u; i < params.samples; ++i)
    {
        float3 h = sampleGGX(hammersley(i, params.samples), n, params.roughness);
        float3 l = normalize(2.0 * dot(n, h) * h - n);
        float nl = max(dot(n, l), 0.0);
        if (nl > 0.0)
        {
            float nh = max(dot(n, h), 0.0);
            float denom = nh * nh * (a * a - 1.0) + 1.0;
            float distribution = a * a / max(PI * denom * denom, 0.000001);
            float pdf = max(distribution * nh / max(4.0 * nh, 0.000001), 0.000001);
            float texelArea = 4.0 * PI / (6.0 * float(params.sourceSize) * float(params.sourceSize));
            float sampleArea = 1.0 / (float(params.samples) * pdf);
            float lod = clamp(0.5 * log2(sampleArea / texelArea), 0.0, params.sourceMaxLod);
            sum += sourceImage.sample(sourceSampler, l, level(lod)).rgb * nl;
            weight += nl;
        }
    }
    return float4(sum / max(weight, 0.000001), 1.0);
}
