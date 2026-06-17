#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec3 fragDir;

layout(push_constant) uniform CameraData {
    mat4 projection;
    mat4 view;
} camera;

void main()
{
    // The cube local position is used as the sampling direction for the face.
    fragDir = inPosition;
    gl_Position = camera.projection * camera.view * vec4(inPosition, 1.0);
}
