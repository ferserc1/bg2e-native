#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 worldNormal;
    float3 albedo;
};

struct CameraUBO {
    float4x4 projectionView;
};

struct ObjectUBO {
    float4x4 model;
    float4   albedo;
};

// Buffer indices match ShaderBinding.metal:
//   set 0, binding 0 (camera) -> buffer(2)
//   set 1, binding 0 (object) -> buffer(3)
vertex VertexOut vertMain(VertexIn in [[stage_in]],
                          constant CameraUBO& camera [[buffer(2)]],
                          constant ObjectUBO& object [[buffer(3)]])
{
    VertexOut out;
    float4 worldPos = object.model * float4(in.position, 1.0);
    out.worldPos    = worldPos.xyz;
    out.worldNormal = (object.model * float4(in.normal, 0.0)).xyz;
    out.albedo      = object.albedo.rgb;
    out.position    = camera.projectionView * worldPos;
    return out;
}
