#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 color;
};

struct PushConstants {
    float4 color;
};

vertex VertexOut triangle_vertex(uint vertexID [[vertex_id]])
{
    const float2 positions[3] = {
        float2( 0.0, -0.5),
        float2( 0.5,  0.5),
        float2(-0.5,  0.5)
    };

    const float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VertexOut out;
    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.color = colors[vertexID];
    return out;
}

fragment float4 triangle_fragment(VertexOut in [[stage_in]],
                                 constant PushConstants& pc [[buffer(0)]])
{
    return float4(in.color * pc.color.rgb, pc.color.a);
}

kernel void noop_compute(uint gid [[thread_position_in_grid]])
{
}
