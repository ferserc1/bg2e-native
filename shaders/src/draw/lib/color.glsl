#ifndef BG2E_DRAW_COLOR_GLSL
#define BG2E_DRAW_COLOR_GLSL
// Explicit output conversion: do not apply to an sRGB attachment a second time.
vec3 drawLinearToGamma(vec3 color, float gamma)
{
    return pow(max(color, vec3(0.0)), vec3(1.0 / max(gamma, 1e-4)));
}
vec3 drawGammaToLinear(vec3 color, float gamma)
{
    return pow(max(color, vec3(0.0)), vec3(max(gamma, 1e-4)));
}
vec3 drawColorCorrection(vec3 color, float exposure, float gamma, float brightness, float contrast)
{
    vec3 mapped = vec3(1.0) - exp(-max(color, vec3(0.0)) * max(exposure, 0.0));
    return (drawLinearToGamma(mapped, gamma) + brightness) * contrast + (1.0 - contrast) * 0.5;
}
#endif
