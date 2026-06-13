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

struct PushConstants {
    float4 color;
};

vertex VertexOut triangle_vertex(VertexIn in [[stage_in]])
{
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.uv       = in.texCoord;
    return out;
}

fragment float4 triangle_fragment(VertexOut in [[stage_in]],
                                  constant PushConstants& pc [[buffer(0)]],
                                  texture2d<float> uTex      [[texture(0)]],
                                  sampler          uSampler  [[sampler(1)]])
{
    float4 tex = uTex.sample(uSampler, in.uv);
    return float4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}

kernel void gradient_compute(texture2d<float, access::write> outImage [[texture(0)]],
                             uint2 gid [[thread_position_in_grid]])
{
    uint w = outImage.get_width();
    uint h = outImage.get_height();
    if (gid.x >= w || gid.y >= h) return;
    float2 uv = float2(float(gid.x) / float(w), float(gid.y) / float(h));
    outImage.write(float4(uv.x, uv.y, 0.5, 1.0), gid);
}
