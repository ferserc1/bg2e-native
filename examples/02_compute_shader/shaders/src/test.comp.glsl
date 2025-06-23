#version 450

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0, rgba8) uniform writeonly image2D outImg;

layout (push_constant) uniform PushConstants {
    int width;
    int height;
} pushConstants;

void main()
{
    imageStore(outImg, ivec2(gl_GlobalInvocationID.xy), vec4(gl_GlobalInvocationID.x / float(pushConstants.width), gl_GlobalInvocationID.y / float(pushConstants.height), 0.5, 1.0));
}
