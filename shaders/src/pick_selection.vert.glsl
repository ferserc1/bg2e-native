#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out flat uint outIdentifier;

layout (set = 0, binding = 0) uniform PickSelectionData {
    mat4 mvp;
    uint identifier;
} pickSelectionData;

void main()
{
    gl_Position = pickSelectionData.mvp * vec4(inPosition, 1.0f);
    outIdentifier = pickSelectionData.identifier;
}
