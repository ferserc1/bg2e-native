#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D colorTexture;

void main() {
    float gamma = 1.5;
    outColor = gammaCorrection(texture(colorTexture, inTexCoord), gamma);
}
