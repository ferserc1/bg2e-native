#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;

layout (push_constant) uniform constants {
    int currentFace;
} pushConstants;

layout (set = 0, binding = 0) uniform ProjectionData {
    mat4 view[6];
    mat4 proj;
} projectionData;

void main() {
    mat4 view = mat4(mat3(projectionData.view[pushConstants.currentFace]));

    gl_Position = projectionData.proj * view * vec4(inPosition, 1.0f);
    outTexCoord = inTexCoord;
}
