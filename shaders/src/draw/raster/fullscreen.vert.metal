#include <metal_stdlib>
using namespace metal;
vertex float4 vertMain(uint index [[vertex_id]])
{
    float2 p = float2(float((index << 1) & 2), float(index & 2));
    return float4(p * 2.0 - 1.0, 0.0, 1.0);
}
