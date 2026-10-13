#include <metal_stdlib>
using namespace metal;

float lumaAt(texture2d<float, access::read> image, int2 pos)
{
    int2 maxPos = int2(int(image.get_width()) - 1, int(image.get_height()) - 1);
    uint2 clampedPos = uint2(clamp(pos, int2(0), maxPos));
    return dot(image.read(clampedPos).rgb, float3(0.299, 0.587, 0.114));
}

kernel void compMain(texture2d<float, access::read> inputImage [[texture(0)]],
                     texture2d<float, access::write> outputImage [[texture(1)]],
                     uint2 gid [[thread_position_in_grid]])
{
    uint w = inputImage.get_width();
    uint h = inputImage.get_height();
    if (gid.x >= w || gid.y >= h)
    {
        return;
    }

    int2 pos = int2(gid);

    // Sample 3x3 neighborhood luminances
    float tl = lumaAt(inputImage, pos + int2(-1, -1));
    float t = lumaAt(inputImage, pos + int2(0, -1));
    float tr = lumaAt(inputImage, pos + int2(1, -1));
    float l = lumaAt(inputImage, pos + int2(-1, 0));
    float r = lumaAt(inputImage, pos + int2(1, 0));
    float bl = lumaAt(inputImage, pos + int2(-1, 1));
    float b = lumaAt(inputImage, pos + int2(0, 1));
    float br = lumaAt(inputImage, pos + int2(1, 1));

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0f, 1.0f);

    outputImage.write(float4(float3(edge), 1.0), gid);
}
