#ifndef BG2E_DRAW_MATH_METAL
#define BG2E_DRAW_MATH_METAL
#include <metal_stdlib>
using namespace metal;
constant float DRAW_PI = 3.14159265358979323846f;
inline float drawRadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}
inline float2 drawHammersley(uint index, uint count)
{
    return float2(float(index) / float(max(count, 1u)), drawRadicalInverse(index));
}
inline float3 drawImportanceSampleGGX(float2 xi, float3 normal, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * DRAW_PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / max(1.0f + (a * a - 1.0f) * xi.y, 1e-7f));
    float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));
    float3 h = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3 up = abs(normal.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    return normalize(tangent * h.x + cross(normal, tangent) * h.y + normal * h.z);
}
#endif
