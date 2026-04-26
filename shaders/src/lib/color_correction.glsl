/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

vec4 fragmentShaderOutput(vec4 inColor, float exposureAmount, float gamma, float brightness, float contrast)
{
    vec4 outColor = exposure(inColor, exposureAmount);
    outColor = lineal2SRGB(outColor, gamma);
    return brightnessContrast(outColor, brightness, contrast);
}

#endif
