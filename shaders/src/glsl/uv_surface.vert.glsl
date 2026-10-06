#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv1;
layout(location = 3) in vec2 inUv2;

layout(push_constant) uniform DrawData {
    mat4 objectToWorld;
    uint submeshIndex;
} drawData;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) flat out uint submeshIndex;
layout(location = 3) out vec2 sourceUv1;

void main()
{
    vec4 position = drawData.objectToWorld * vec4(inPosition, 1.0);
    worldPosition = position.xyz;
    worldNormal = normalize(transpose(inverse(mat3(drawData.objectToWorld))) * inNormal);
    submeshIndex = drawData.submeshIndex;
    sourceUv1 = inUv1;

    // Direct UV2-to-clip mapping; the engine's negative-height Vulkan viewport
    // places v=1 at the top row, preserving the conventional image orientation.
    gl_Position = vec4(inUv2 * 2.0 - 1.0, 0.0, 1.0);
}
