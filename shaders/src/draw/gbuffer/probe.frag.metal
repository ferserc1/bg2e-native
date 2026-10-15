#include "lib/pbr.metal"
fragment float4 fragMain()
{
    return float4(drawDirectBRDF(float3(0, 0, 1), float3(0, 0, 1), float3(0, 0, 1), float3(0.5f), 0.0f, 0.5f), 1.0f);
}
