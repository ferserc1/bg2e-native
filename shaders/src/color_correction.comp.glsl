//
//  color_correction.comp.glsl
#version 460
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler2D inputImage;
layout (rgba16f, set = 0, binding = 1) uniform image2D outputImage;

void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);
    vec2 inTexCoord = vec2(gl_GlobalInvocationID.x / size.x, gl_GlobalInvocationID.y);
    
    float gamma = 2.2; // Gamma value for conversion
    vec3 sampleColor = lineal2SRGB(texture(inputImage, inTexCoord).rgb, gamma);
    imageStore(outputImage, texelCoord, vec4(sampleColor, 1.0));
    
}


