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

struct CameraUBO {
    float4x4 projectionView;
};

struct ModelUBO {
    float4x4 model;
};

// Buffer indices match ShaderBinding.metal:
//   set 0, binding 0 (camera) -> metal 8
//   set 1, binding 0 (model)  -> metal 16
vertex VertexOut vertMain(VertexIn in [[stage_in]],
                      constant CameraUBO& camera [[buffer(2)]],
                      constant ModelUBO&  model  [[buffer(3)]])
{
    VertexOut out;
    out.position = camera.projectionView * model.model * float4(in.position, 1.0);
    out.uv       = in.texCoord;
    return out;
}
