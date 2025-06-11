#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/uniforms.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV0;
layout(location = 2) out vec2 outUV1;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;


layout (set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

void main() {
    mat3 normalMatrix = mat3(objectData.modelMatrix);
	gl_Position = sceneData.projMatrix * sceneData.viewMatrix * objectData.modelMatrix * vec4(inPosition, 1.0f);
    outNormal = normalMatrix * inNormal * 0.5 + 0.5;
    outTangent = normalMatrix * inTangent;
    outBitangent = cross(outNormal, outTangent);
	outUV0 = inUV0;
    outUV1 = inUV1;
}
