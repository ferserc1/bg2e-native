#include <metal_stdlib>
using namespace metal;
#include "../lib/bindings.metal"
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 uv0 [[attribute(2)]];
    float2 uv1 [[attribute(3)]];
    float3 tangent [[attribute(4)]];
};
struct VertexOut {
    float4 position [[position]];
    float2 uv0;
    float2 uv1;
};
vertex VertexOut vertMain(VertexIn in [[stage_in]], DRAW_VERTEX_DATA_ARGUMENTS)
{
    VertexOut out;
    out.uv0 = in.uv0;
    out.uv1 = in.uv1;
    out.position = drawFrame.projection * drawFrame.view * drawObject.model * float4(in.position, 1.0);
    return out;
}
