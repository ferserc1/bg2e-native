#include <metal_stdlib>
using namespace metal;
fragment float4 fragMain(float4 position [[position]], texture2d<float> source [[texture(0)]],
    sampler sourceSampler [[sampler(0)]], constant float4& outputSize [[buffer(0)]])
{
    float2 size = float2(source.get_width(), source.get_height());
    float2 p = position.xy / outputSize.xy * size - 0.5;
    float2 b = floor(p);
    float2 w = p - b;
    float4 a = source.sample(sourceSampler, (b + 0.5) / size, level(0));
    float4 c = source.sample(sourceSampler, (b + float2(1.5, 0.5)) / size, level(0));
    float4 d = source.sample(sourceSampler, (b + float2(0.5, 1.5)) / size, level(0));
    float4 e = source.sample(sourceSampler, (b + 1.5) / size, level(0));
    return mix(mix(a, c, w.x), mix(d, e, w.x), w.y);
}
