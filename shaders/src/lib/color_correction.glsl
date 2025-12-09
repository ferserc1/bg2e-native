#ifndef COLOR_CORRECTION_GLSL
#define COLOR_CORRECTION_GLSL

// Convert lineal color to SRGB for shader output
vec4 lineal2SRGB(vec4 color, float gamma)
{
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec3 lineal2SRGB(vec3 color, float gamma)
{
    return pow(color.rgb, vec3(1.0 / gamma));
}

// Convert SRGB textures to lineal color
vec4 SRGB2Lineal(vec4 color, float gamma)
{
    return vec4(pow(color.rgb, vec3(gamma)), color.a);
}

vec3 SRGB2Lineal(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}

vec4 brightnessContrast(vec4 color, float brightness, float contrast)
{
    mat4 brightnessMat = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        brightness, brightness, brightness, 1.0
    );
    float t = (1.0 - contrast) / 2.0;
    mat4 contrastMat = mat4(
        contrast, 0.0, 0.0, 0.0,
        0.0, contrast, 0.0, 0.0,
        0.0, 0.0, contrast, 0.0,
        t, t, t, 1.0
    );
    return contrastMat * brightnessMat * color;
}

vec3 brightnessContrast(vec3 color, float brightness, float contrast)
{
    mat4 brightnessMat = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        brightness, brightness, brightness, 1.0
    );
    float t = (1.0 - contrast) / 2.0;
    mat4 contrastMat = mat4(
        contrast, 0.0, 0.0, 0.0,
        0.0, contrast, 0.0, 0.0,
        0.0, 0.0, contrast, 0.0,
        t, t, t, 1.0
    );
    return (contrastMat * brightnessMat * vec4(color, 1.0)).rgb;
}

vec4 exposure(vec4 color, float exposure)
{
    return vec4(vec3(1.0) - exp(-color.rgb * exposure), 1.0);
}

vec3 exposure(vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

#endif
