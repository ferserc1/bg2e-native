#include <metal_stdlib>
using namespace metal;
vertex float4 vertMain(uint index [[vertex_id]])
{
    float2 position = float2(float((index << 1) & 2), float(index & 2));
    return float4(position * 2.0f - 1.0f, 0.0f, 1.0f);
}
