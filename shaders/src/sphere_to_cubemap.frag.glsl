#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform sampler2D colorTexture;

layout (push_constant) uniform constants {
    int currentFace;
    int currentMipLevel;
    int totalMipLevels;
    float gamma;
} pushConstants;

void main() {
    float gamma = pushConstants.gamma;
    outColor = lineal2SRGB(texture(colorTexture, inTexCoord), gamma);
}
