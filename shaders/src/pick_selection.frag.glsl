#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in flat uint inIdentifier;

void main()
{
    const float inv255 = 1.0 / 255.0;

    uvec4 bytes = uvec4(
        inIdentifier & 0xFFu,
        (inIdentifier >> 8)  & 0xFFu,
        (inIdentifier >> 16) & 0xFFu,
        (inIdentifier >> 24) & 0xFFu
    );

    outColor = vec4(bytes) * inv255;
}
