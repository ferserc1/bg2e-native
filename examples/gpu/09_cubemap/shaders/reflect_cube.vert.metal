#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
    float3 localPos;
};

struct CameraUBO {
    float4x4 projection;
    float4x4 view;
    float4   cameraPos;
};

struct ModelUBO {
    float4x4 model;
};

vertex VertexOut vertMain(VertexIn in [[stage_in]],
                            constant CameraUBO& camera [[buffer(2)]],
                            constant ModelUBO&  model  [[buffer(3)]])
{
    VertexOut out;
    float4 worldPos = model.model * float4(in.position, 1.0);
    float3x3 normalMatrix = float3x3(
        model.model[0].xyz,
        model.model[1].xyz,
        model.model[2].xyz
    );

    out.position = camera.projection * camera.view * worldPos;
    out.worldPos = worldPos.xyz;
    out.normal   = normalize(normalMatrix * in.normal);
    out.localPos = in.position;

    return out;
}