#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionView;
} camera;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
