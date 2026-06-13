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

// Buffer indices must match metal::PipelineLayout::metalBufferIndex():
//   set 0, binding 0 (camera) -> 8 + 0*8 + 0 = 8
//   set 1, binding 0 (model)  -> 8 + 1*8 + 0 = 16
vertex VertexOut cube_vertex(VertexIn in [[stage_in]],
                             constant CameraUBO& camera [[buffer(8)]],
                             constant ModelUBO&  model  [[buffer(16)]])
{
    VertexOut out;
    out.position = camera.projectionView * model.model * float4(in.position, 1.0);
    out.uv       = in.texCoord;
    return out;
}

fragment float4 cube_fragment(VertexOut in [[stage_in]],
                              texture2d<float> uTex     [[texture(0)]],
                              sampler          uSampler [[sampler(1)]])
{
    return uTex.sample(uSampler, in.uv);
}
