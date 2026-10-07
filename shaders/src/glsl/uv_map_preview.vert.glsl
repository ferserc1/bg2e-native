#version 460

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec3 inBarycentric;
layout(location = 2) in uint inSubmesh;

layout(location = 0) out vec3 barycentric;
layout(location = 1) flat out uint submeshIndex;

void main()
{
    barycentric = inBarycentric;
    submeshIndex = inSubmesh;

    // Same convention as the UV surface pass: the engine's negative-height
    // viewport maps v=1 to the top image row.
    vec2 uvInv = vec2(inUv.x, 1.0 - inUv.y);
    gl_Position = vec4(uvInv * 2.0 - 1.0, 0.0, 1.0);
}
