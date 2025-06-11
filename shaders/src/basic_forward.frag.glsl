#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 2, binding = 0) uniform samplerCube giTex;

layout(push_constant) uniform PushConstant
{
    float gamma;
} pushConstant;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

void main() {
    vec3 color = texture(colorTex, inUV0).rgb * objectData.material.albedo.rgb;
    vec3 lighting = texture(giTex, inNormal).rgb;
    
    outColor = lineal2SRGB(vec4(color, 1.0f) * vec4(lighting, 1.0), pushConstant.gamma);
}
