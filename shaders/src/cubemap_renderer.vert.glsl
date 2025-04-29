// This is the basic shader for CubeMapRenderer class
#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out flat int outCurrentMipLevel;
layout (location = 2) out flat int outTotalMipLevels;
layout (location = 3) out flat int outFaceIndex;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 view[6]; // One view matrix for each cube face
    mat4 proj;
} sceneData;

layout (push_constant) uniform Constants {
    int currentFace;
    int currentMipLevel;
    int totalMipLevels;
} SkyPushConstants;

void main() {
    mat4 view = mat4(mat3(sceneData.view[SkyPushConstants.currentFace]));
    gl_Position = sceneData.proj * view * vec4(inPosition, 1.0f);
    outNormal = normalize(inPosition);
    outCurrentMipLevel = SkyPushConstants.currentMipLevel;
    outTotalMipLevels = SkyPushConstants.totalMipLevels;
    outFaceIndex = SkyPushConstants.currentFace;
}