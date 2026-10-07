#version 460

layout(location = 0) in vec3 barycentric;
layout(location = 1) flat in uint submeshIndex;

layout(push_constant) uniform PreviewParams {
    float lineWidth;
    uint mode; // 0 = UV triangles, 1 = atlas boundary lines
} params;

layout(location = 0) out vec4 outColor;

vec3 submeshTint(uint index)
{
    uint h = index * 2654435761u + 1013904223u;
    float r = float(h & 0xffu) / 255.0;
    float g = float((h >> 8u) & 0xffu) / 255.0;
    float b = float((h >> 16u) & 0xffu) / 255.0;
    return mix(vec3(0.35), vec3(0.95), vec3(r, g, b));
}

void main()
{
    if (params.mode == 1u)
    {
        // Atlas boundary
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    // Dim per-submesh fill shows the valid-texel coverage; the barycentric
    // edge factor draws the wireframe without polygon-mode line fill.
    float minBarycentric = min(barycentric.x, min(barycentric.y, barycentric.z));
    float edge = 1.0 - smoothstep(0.0, params.lineWidth, minBarycentric);
    vec3 fill = submeshTint(submeshIndex) * 0.35;
    outColor = vec4(mix(fill, vec3(1.0), edge), 1.0);
}
