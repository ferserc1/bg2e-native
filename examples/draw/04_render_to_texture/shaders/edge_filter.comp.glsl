#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputImage;
layout(set = 0, binding = 1, rgba8) uniform image2D outputImage;

float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float lumaAt(ivec2 pos)
{
    ivec2 clampedPos = clamp(pos, ivec2(0), imageSize(inputImage) - ivec2(1));
    return luma(imageLoad(inputImage, clampedPos).rgb);
}

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (pos.x >= size.x || pos.y >= size.y)
    {
        return;
    }

    // Sample 3x3 neighborhood luminances
    float tl = lumaAt(pos + ivec2(-1, -1));
    float t = lumaAt(pos + ivec2(0, -1));
    float tr = lumaAt(pos + ivec2(1, -1));
    float l = lumaAt(pos + ivec2(-1, 0));
    float r = lumaAt(pos + ivec2(1, 0));
    float bl = lumaAt(pos + ivec2(-1, 1));
    float b = lumaAt(pos + ivec2(0, 1));
    float br = lumaAt(pos + ivec2(1, 1));

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0, 1.0);

    imageStore(outputImage, pos, vec4(vec3(edge), 1.0));
}
