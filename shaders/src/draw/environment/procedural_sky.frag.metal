#include <metal_stdlib>
using namespace metal;
#include "common.metal"
fragment float4 fragMain(float4 position [[position]], constant GenerationPush& params [[buffer(0)]])
{
    float2 pixel = position.xy;
    float3 n = cubeDirection(params.face, pixel / float(params.size));
    float altitude = max(n.y, 0.0);
    float3 sky = mix(float3(0.55, 0.65, 0.8), float3(0.06, 0.22, 0.6), sqrt(altitude));
    float3 ground = float3(0.04, 0.035, 0.025);
    float3 color = mix(ground, sky, smoothstep(-0.03, 0.03, n.y));
    float3 sun = normalize(float3(0.4, 0.7, 0.2));
    color += float3(8.0, 6.5, 4.0) * pow(max(dot(n, sun), 0.0), 512.0);
    return float4(color, 1.0);
}
