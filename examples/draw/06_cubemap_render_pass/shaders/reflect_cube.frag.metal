#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
};

struct PushConstants {
    packed_float3 cameraPos; // Match the host vec3 + float layout (16 bytes).
    float lod;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant PushConstants& push [[buffer(0)]],
                         texturecube<float> uCubeMap [[texture(0)]],
                         sampler uSampler [[sampler(0)]])
{
    float3 N = normalize(in.normal);
    float3 I = normalize(in.worldPos - float3(push.cameraPos));
    float3 R = reflect(I, N);
    return uCubeMap.sample(uSampler, R, level(push.lod));
}
