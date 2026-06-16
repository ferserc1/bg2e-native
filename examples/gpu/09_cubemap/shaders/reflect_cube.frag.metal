#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
    float3 localPos;
};

struct PushConstants {
    float3 cameraPos;
};

struct CubeRenderSettings {
    uint mode;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                     constant PushConstants& push [[buffer(0)]],
                     constant CubeRenderSettings& settings [[buffer(3)]],
                     texturecube<float> uCubeMap  [[texture(0)]],
                     sampler            uSampler  [[sampler(0)]])
{
    if (settings.mode == 0) {
        float3 N = normalize(in.normal);
        float3 I = normalize(in.worldPos - push.cameraPos);
        float3 R = reflect(I, N);
        return uCubeMap.sample(uSampler, R);
    }
    else {
        float3 D = normalize(in.localPos);
        return uCubeMap.sample(uSampler, D);
    }
}
