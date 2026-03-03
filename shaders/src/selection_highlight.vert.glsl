#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform FrameData {
    mat4 mvp;
    vec4 color;
} frameData;

void main()
{
    gl_Position = frameData.mvp * vec4(inPosition, 1.0);
    outColor = frameData.color;
}