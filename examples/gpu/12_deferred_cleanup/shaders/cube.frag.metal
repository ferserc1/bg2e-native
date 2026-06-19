#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                     texture2d<float> uTex     [[texture(0)]],
                     sampler          uSampler [[sampler(1)]])
{
    return uTex.sample(uSampler, in.uv);
}
