#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vAlbedo;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionView;
} camera;

layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec4 albedo;
} object;

void main()
{
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    vWorldPos     = worldPos.xyz;
    // Uniform scale is used in the example, so the upper 3x3 is fine for normals.
    vWorldNormal  = mat3(object.model) * inNormal;
    vAlbedo       = object.albedo.rgb;

    gl_Position = camera.projectionView * worldPos;
}
