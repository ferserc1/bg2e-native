#include <metal_stdlib>
using namespace metal;

kernel void compMain(texture2d<float, access::write> outImage [[texture(0)]],
                 uint2 gid [[thread_position_in_grid]])
{
    uint w = outImage.get_width();
    uint h = outImage.get_height();
    if (gid.x >= w || gid.y >= h) return;
    float2 uv = float2(float(gid.x) / float(w), float(gid.y) / float(h));
    outImage.write(float4(uv.x, uv.y, 0.5, 1.0), gid);
}
