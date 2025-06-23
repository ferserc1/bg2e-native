#version 450

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0, rgba8) uniform writeonly image2D outImg;

void main()
{
    imageStore(outImg, ivec2(gl_GlobalInvocationID.xy), vec4(0.3, 0.1, 0.56, 1.0));
}
