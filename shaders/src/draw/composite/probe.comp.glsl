#version 450
#extension GL_GOOGLE_include_directive : require
#include "lib/color.glsl"
layout(local_size_x = 16, local_size_y = 16) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D outputImage;
void main()
{
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(position, imageSize(outputImage))))
    {
        return;
    }
    imageStore(outputImage, position, vec4(drawColorCorrection(vec3(0.5), 1.0, 2.2, 0.0, 1.0), 1.0));
}
