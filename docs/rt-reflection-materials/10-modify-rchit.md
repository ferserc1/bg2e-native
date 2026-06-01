# Step 10: Modify rt_reflections.rchit.glsl

## File Path

`shaders/src/rt_reflections.rchit.glsl`

## Purpose

Modify the closest hit shader to:
1. Use `gl_InstanceCustomIndexEXT` as the object/material index
2. Read material data (albedo color, albedoScale) from the SSBO array
3. Recover UV coordinates from vertex/index buffer arrays using barycentric coordinates
4. Sample the albedo texture array at the interpolated UV
5. Output the final albedo color (albedo color * texture color)

## Current Shader

The current shader is a stub that outputs barycentric coordinates as color:

```glsl
#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.didHit = 1u;
    payload.hitDistance = gl_HitTEXT;

    vec3 bary = vec3(
        1.0 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );

    float edge = min(min(bary.x, bary.y), bary.z);
    float line = smoothstep(0.0, 0.02, edge);

    payload.hitColor = mix(vec3(1.0), bary, line);
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}
```

## New Shader

```glsl
#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require

#include "lib/rt_material_data.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

// Set 1: Material data binding with array descriptors
layout(set = 1, binding = 0) readonly buffer MaterialDataBuffer {
    RTMaterialData materials[];
};

layout(set = 1, binding = 1) readonly buffer VertexBuffer {
    RTVertex vertices[];
} vb[MAX_OBJECTS];

layout(set = 1, binding = 2) readonly buffer IndexBuffer {
    uint indices[];
} ib[MAX_OBJECTS];

layout(set = 1, binding = 3) uniform sampler2D albedoTex[MAX_OBJECTS];

void main() {
    uint matIdx = gl_InstanceCustomIndexEXT;
    uint nmatIdx = nonuniformEXT(matIdx);

    vec3 bary = vec3(
        1.0 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );

    // Get primitive index (triangle index within the mesh)
    uint triIdx = gl_PrimitiveID;

    // Get triangle vertex indices from index buffer array
    uint idx0 = ib[nmatIdx].indices[triIdx * 3 + 0];
    uint idx1 = ib[nmatIdx].indices[triIdx * 3 + 1];
    uint idx2 = ib[nmatIdx].indices[triIdx * 3 + 2];

    // Get vertex data from vertex buffer array
    RTVertex vert0 = vb[nmatIdx].vertices[idx0];
    RTVertex vert1 = vb[nmatIdx].vertices[idx1];
    RTVertex vert2 = vb[nmatIdx].vertices[idx2];

    // Interpolate UV coordinates using barycentric weights
    vec2 uv = vert0.texCoord0 * bary.x
            + vert1.texCoord0 * bary.y
            + vert2.texCoord0 * bary.z;

    // Read material data
    RTMaterialData mat = materials[matIdx];
    vec2 scaledUV = uv * mat.albedoScale;

    // Sample albedo texture from texture array
    vec3 texColor = texture(albedoTex[nmatIdx], scaledUV).rgb;

    // Final color = albedo color * texture color
    payload.hitColor = mat.albedo.rgb * texColor;
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}
```

## Key Changes from Old Plan

1. **Array bindings instead of switch-case**: Instead of generating 256 switch cases (one per material), we use array descriptor bindings with `GL_EXT_nonuniform_qualifier`. This makes the shader much cleaner and shorter.

2. **`nonuniformEXT()` qualifier**: We wrap `matIdx` with `nonuniformEXT()` when indexing into the buffer/texture arrays. This tells the Vulkan driver that the index is dynamic and the descriptor array must be prepared for non-uniform indexing.

3. **Single uniform block per binding**: Instead of 3*256 = 768 individual bindings, we have 4 array bindings:
   - `vb[MAX_OBJECTS]` - vertex buffer array
   - `ib[MAX_OBJECTS]` - index buffer array
   - `albedoTex[MAX_OBJECTS]` - albedo texture array
   - `materials[]` - material data array (count=1, no array indexing needed)

4. **No UV recovery algorithm complexity**: The old plan had a complex pseudocode section for UV recovery. Now it's straightforward: read indices from `ib[nmatIdx]`, read vertices from `vb[nmatIdx]`, interpolate UV with barycentric coords.

## Implementation Strategy

The shader is a single clean implementation. No need for incremental stages:

1. **First**: Write the complete shader with array bindings
2. **Second**: Verify the descriptor layout in C++ matches the shader bindings
3. **Third**: Test with a simple scene with a few RT-visible objects

## Verification Checklist

- [ ] `GL_EXT_nonuniform_qualifier` extension is enabled
- [ ] `vb`, `ib`, and `albedoTex` are declared as arrays with `MAX_OBJECTS` size
- [ ] `nonuniformEXT(matIdx)` is used when indexing into `vb`, `ib`, and `albedoTex`
- [ ] `materials[matIdx]` is indexed directly (no nonuniformEXT needed, count=1)
- [ ] UV interpolation uses barycentric coordinates correctly
- [ ] Final color = `mat.albedo.rgb * texColor`
- [ ] `payload.hitDistance` and `payload.didHit` are set correctly

## Notes

- The `nonuniformEXT()` qualifier is required by Vulkan for dynamic indexing into descriptor arrays. Without it, the shader may fail to compile or produce undefined behavior.
- The vertex buffer array stores all vertices from all objects interleaved. Each object's vertices are accessed via `vb[nmatIdx].vertices[idx]` where `idx` is an index into that object's vertex buffer.
- The index buffer array stores all indices from all objects interleaved. Each object's indices are accessed via `ib[nmatIdx].indices[triIdx * 3 + i]`.
- The albedo texture array stores all albedo textures. Each object's texture is accessed via `albedoTex[nmatIdx]`.
- If an object has no albedo texture, the white texture fallback is used (bound by `RTMaterialDataBinding`).
- The material data array has count=1, so `materials[matIdx]` works directly without `nonuniformEXT()`. However, if the descriptor set layout is created with `descriptorCount = MAX_OBJECTS` for the material SSBO as well, then `nonuniformEXT()` would be needed there too.