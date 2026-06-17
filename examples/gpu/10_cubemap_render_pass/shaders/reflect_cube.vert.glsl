#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
} camera;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    gl_Position  = camera.projection * camera.view * worldPos;
    fragWorldPos = worldPos.xyz;
    fragNormal   = mat3(object.model) * inNormal;
}
