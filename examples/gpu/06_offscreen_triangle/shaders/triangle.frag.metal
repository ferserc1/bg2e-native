#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 color;
};

struct PushConstants {
    float4 color;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                     constant PushConstants& pc [[buffer(0)]])
{
    return float4(in.color * pc.color.rgb, pc.color.a);
}
