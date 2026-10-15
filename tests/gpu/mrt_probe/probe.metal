#include <metal_stdlib>
using namespace metal;
struct VertexOutput {
    float4 position [[position]];
};
struct FragmentOutput {
    float4 color0 [[color(0)]];
    float4 color1 [[color(1)]];
    float4 color2 [[color(2)]];
    float4 color3 [[color(3)]];
    float4 color4 [[color(4)]];
    float4 color5 [[color(5)]];
};
vertex VertexOutput probeVertex(uint id [[vertex_id]], constant float& depth [[buffer(1)]])
{
    const float2 positions[3] = {float2(-1, -1), float2(3, -1), float2(-1, 3)};
    return {float4(positions[id], depth, 1)};
}
fragment FragmentOutput probeFragment(constant float& multiplier [[buffer(0)]])
{
    float m = multiplier;
    return {float4(m, 0, 0, 1), float4(0, m, 0, 1), float4(0, 0, m, 1),
            float4(m, m, 0, 1), float4(m, 0, m, 1), float4(0, m, m, 1)};
}
