#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 dir;
};

struct CameraData {
    float4x4 projection;
    float4x4 view;
};

vertex VertexOut vertMain(VertexIn in [[stage_in]],
                          constant CameraData& camera [[buffer(1)]])
{
    VertexOut out;
    out.dir      = in.position;
    out.position = camera.projection * camera.view * float4(in.position, 1.0);
    return out;
}
