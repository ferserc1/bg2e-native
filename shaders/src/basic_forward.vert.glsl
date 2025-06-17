#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/color_correction.glsl"


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV0;
layout(location = 2) out vec2 outUV1;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out vec3 outFragPos;
layout(location = 5) out flat int outLightCount;
layout(location = 6) out mat3 outTBN;
layout(location = 9) out flat Light[LIGHT_COUNT] outLights;

layout (set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

layout (set = 3, binding = 0) uniform LightData {
    Light lights[LIGHT_COUNT];
    int lightCount;
    vec3 padding;
} lightData;

void main() {
	gl_Position = sceneData.projMatrix * sceneData.viewMatrix * objectData.modelMatrix * vec4(inPosition, 1.0f);
	outUV0 = inUV0;
    outUV1 = inUV1;

    outTBN = TBNMatrix(objectData.modelMatrix, inNormal, inTangent);
    
    outNormal = mat3(objectData.modelMatrix) * inNormal;

    outViewPos = (inverse(sceneData.viewMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    outLightCount = lightData.lightCount;
    outFragPos = (objectData.modelMatrix * vec4(inPosition, 1.0f)).xyz;

    for (int i = 0; i < lightData.lightCount; ++i)
    {
        outLights[i].position = lightData.lights[i].position;
        outLights[i].direction = lightData.lights[i].direction;
        outLights[i].intensity = lightData.lights[i].intensity;
        outLights[i].color = lightData.lights[i].color;
        outLights[i].type = lightData.lights[i].type;
    }
}

