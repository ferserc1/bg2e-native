#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outNormal;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
} sceneData;

void main() {
    mat4 view = mat4(mat3(sceneData.view));
    gl_Position = sceneData.proj * view * vec4(inPosition, 1.0f);
    outNormal = normalize(inPosition);
}