#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outDirection;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

layout (set = 1, binding = 0) uniform ObjectData {
    mat4 modelMatrix;
} objectData;

void main() {
	gl_Position = sceneData.projMatrix * sceneData.viewMatrix * objectData.modelMatrix * vec4(inPosition, 1.0f);
	outTexCoord = inTexCoord;
    outDirection = (objectData.modelMatrix * vec4(inPosition, 1.0)).rgb;
}
