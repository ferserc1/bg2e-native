#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct PushConstants {
    float4 color;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                     constant PushConstants& pc [[buffer(0)]],
                     texture2d<float> uTex      [[texture(0)]],
                     sampler          uSampler  [[sampler(1)]])
{
    float4 tex = uTex.sample(uSampler, in.uv);
    return float4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}
