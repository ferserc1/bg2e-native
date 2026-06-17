#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
};

struct PushConstants {
    float3 cameraPos;
    float  lod;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant PushConstants& push [[buffer(0)]],
                         texturecube<float>      uCubeMap [[texture(0)]],
                         sampler                 uSampler [[sampler(0)]])
{
    float3 N = normalize(in.normal);
    float3 I = normalize(in.worldPos - push.cameraPos);
    float3 R = reflect(I, N);
    return uCubeMap.sample(uSampler, R, level(push.lod));
}
