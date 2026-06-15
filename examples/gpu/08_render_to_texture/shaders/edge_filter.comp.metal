#include <metal_stdlib>
using namespace metal;

kernel void compMain(texture2d<float, access::read>  inputImage  [[texture(0)]],
                     texture2d<float, access::write> outputImage [[texture(1)]],
                     uint2 gid [[thread_position_in_grid]])
{
    uint w = inputImage.get_width();
    uint h = inputImage.get_height();
    if (gid.x >= w || gid.y >= h) return;

    int2 pos = int2(gid);

    // Sample 3x3 neighborhood luminances
    float tl = dot(inputImage.read(uint2(pos + int2(-1, -1))).rgb, float3(0.299, 0.587, 0.114));
    float t  = dot(inputImage.read(uint2(pos + int2( 0, -1))).rgb, float3(0.299, 0.587, 0.114));
    float tr = dot(inputImage.read(uint2(pos + int2( 1, -1))).rgb, float3(0.299, 0.587, 0.114));
    float l  = dot(inputImage.read(uint2(pos + int2(-1,  0))).rgb, float3(0.299, 0.587, 0.114));
    float r  = dot(inputImage.read(uint2(pos + int2( 1,  0))).rgb, float3(0.299, 0.587, 0.114));
    float bl = dot(inputImage.read(uint2(pos + int2(-1,  1))).rgb, float3(0.299, 0.587, 0.114));
    float b  = dot(inputImage.read(uint2(pos + int2( 0,  1))).rgb, float3(0.299, 0.587, 0.114));
    float br = dot(inputImage.read(uint2(pos + int2( 1,  1))).rgb, float3(0.299, 0.587, 0.114));

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0f, 1.0f);

    outputImage.write(float4(float3(edge), 1.0), gid);
}
