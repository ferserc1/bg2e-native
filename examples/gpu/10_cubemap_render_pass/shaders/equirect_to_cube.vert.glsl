#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform CameraData {
    mat4 projection;
    mat4 view;
} camera;

void main()
{
    gl_Position = camera.projection * camera.view * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
