#ifndef BG2E_DRAW_COLOR_METAL
#define BG2E_DRAW_COLOR_METAL
#include <metal_stdlib>
using namespace metal;
// Explicit output conversion: do not apply to an sRGB attachment a second time.
inline float3 drawLinearToGamma(float3 color, float gamma)
{
    return pow(max(color, float3(0.0f)), float3(1.0f / max(gamma, 1e-4f)));
}
inline float3 drawGammaToLinear(float3 color, float gamma)
{
    return pow(max(color, float3(0.0f)), float3(max(gamma, 1e-4f)));
}
inline float3 drawColorCorrection(float3 color, float exposure, float gamma, float brightness, float contrast)
{
    float3 mapped = float3(1.0f) - exp(-max(color, float3(0.0f)) * max(exposure, 0.0f));
    return (drawLinearToGamma(mapped, gamma) + brightness) * contrast + (1.0f - contrast) * 0.5f;
}
#endif
