#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord0;
layout(location = 3) in vec2 inTexCoord1;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord0;
layout(location = 2) out vec2 fragTexCoord1;
layout(location = 3) out vec3 fragTangent;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord0 = inTexCoord0;
    fragTexCoord1 = inTexCoord1;
    fragNormal = inNormal;
    fragTangent = inTangent;
}
