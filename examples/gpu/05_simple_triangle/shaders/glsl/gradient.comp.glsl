#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform image2D outImage;

void main()
{
    ivec2 size = imageSize(outImage);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= size.x || pos.y >= size.y) return;
    vec2 uv = vec2(float(pos.x) / float(size.x), float(pos.y) / float(size.y));
    imageStore(outImage, pos, vec4(uv.x, uv.y, 0.5, 1.0));
}
