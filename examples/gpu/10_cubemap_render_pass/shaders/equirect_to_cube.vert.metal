#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct CameraData {
    float4x4 projection;
    float4x4 view;
};

vertex VertexOut vertMain(VertexIn in [[stage_in]],
                          constant CameraData& camera [[buffer(1)]])
{
    VertexOut out;
    out.position = camera.projection * camera.view * float4(in.position, 1.0);
    out.uv       = in.texCoord;
    return out;
}
