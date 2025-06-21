#ifndef COLOR_CORRECTION_GLSL
#define COLOR_CORRECTION_GLSL

// Convert lineal color to SRGB for shader output
vec4 lineal2SRGB(vec4 color, float gamma)
{
    color = color / (color + vec4(1.0));
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec3 lineal2SRGB(vec3 color, float gamma)
{
    color = color / (color + vec3(1.0));
    return pow(color.rgb, vec3(1.0 / gamma));
}

// Convert SRGB textures to lineal color
vec4 SRGB2Lineal(vec4 color, float gamma)
{
    return vec4(pow(color.rgb, vec3(gamma)), color.a);
}

vec3 SRGB2lLineal(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}

#endif
