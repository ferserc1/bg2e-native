#version 460

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) flat in uint submeshIndex;
layout(location = 3) in vec2 sourceUv1;

layout(location = 0) out vec4 outWorldPosition;
layout(location = 1) out vec4 outWorldNormal;
layout(location = 2) out uint outSubmeshIndex;
layout(location = 3) out uint outValidTexel;

void main()
{
    // sourceUv1 remains interpolated for later per-material surface sampling;
    // this geometry-only pass intentionally does not alter coverage by UV1.
    outWorldPosition = vec4(worldPosition, 1.0);
    outWorldNormal = vec4(normalize(worldNormal), 0.0);
    outSubmeshIndex = submeshIndex;
    outValidTexel = 1u;
}
