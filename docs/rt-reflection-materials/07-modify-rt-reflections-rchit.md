# Step 7: Modify rt_reflections.rchit.glsl

## File Path

`shaders/src/rt_reflections.rchit.glsl`

## Purpose

Modify the closest hit shader to:
1. Use `gl_InstanceCustomIndexEXT` as the material index
2. Read material data (albedo color, albedoScale) from the SSBO
3. Recover UV coordinates from vertex/index buffers using barycentric coordinates
4. Sample the albedo texture at the interpolated UV
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

#include "lib/rt_material_data.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

// Set 1: Material data binding
layout(set = 1, binding = 0) readonly buffer MaterialDataBuffer {
    RTMaterialData materials[];
};

// Per-material vertex/index/texture bindings
// MAX_MATERIALS = 64, each material uses 3 bindings (vertex SSBO, index SSBO, albedo CIS)
// Bindings: 1+3*i (vertex), 2+3*i (index), 3+3*i (texture)
layout(set = 1, binding =  1) readonly buffer VBuf0  { RTVertex v0[];  };
layout(set = 1, binding =  2) readonly buffer IBuf0  { uint      i0[];  };
layout(set = 1, binding =  3) uniform sampler2D      tex0;
layout(set = 1, binding =  4) readonly buffer VBuf1  { RTVertex v1[];  };
layout(set = 1, binding =  5) readonly buffer IBuf1  { uint      i1[];  };
layout(set = 1, binding =  6) uniform sampler2D      tex1;
// ... repeat for all 64 materials ...
// This is verbose but necessary without GL_EXT_nonuniform_qualifier

// Helper function to get vertex data for a material
// Uses switch-case because GLSL doesn't support dynamic buffer indexing
// without the GL_EXT_nonuniform_qualifier extension
void getTriangleVertices(uint matIdx, uint triIdx,
    out RTVertex v0, out RTVertex v1, out RTVertex v2)
{
    uint idx0, idx1, idx2;

    // Read indices and vertices based on material index
    // This will be implemented as a switch-case over matIdx
    // For now, shown as pseudocode:
    //
    // switch (matIdx) {
    //   case 0:
    //     idx0 = i0[triIdx * 3 + 0];
    //     idx1 = i0[triIdx * 3 + 1];
    //     idx2 = i0[triIdx * 3 + 2];
    //     v0 = v0[idx0]; v1 = v0[idx1]; v2 = v0[idx2];
    //     break;
    //   case 1:
    //     idx0 = i1[triIdx * 3 + 0];
    //     ...
    // }
}

// Interpolate vec2 attribute using barycentric coordinates
vec2 interpolateVec2(vec2 a, vec2 b, vec2 c, vec3 bary) {
    return a * bary.x + b * bary.y + c * bary.z;
}

void main() {
    uint matIdx = gl_InstanceCustomIndexEXT;

    vec3 bary = vec3(
        1.0 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );

    // Get primitive index (triangle index within the mesh)
    uint triIdx = gl_PrimitiveID;

    // TODO: Get triangle vertices using matIdx to select buffer
    // RTVertex vert0, vert1, vert2;
    // getTriangleVertices(matIdx, triIdx, vert0, vert1, vert2);

    // Interpolate UV coordinates
    // vec2 uv = interpolateVec2(vert0.texCoord0, vert1.texCoord0, vert2.texCoord0, bary);

    // Read material data
    // RTMaterialData mat = materials[matIdx];
    // vec2 scaledUV = uv * mat.albedoScale;

    // Sample albedo texture
    // vec3 texColor = texture(albedoTex[matIdx], scaledUV).rgb;

    // Final color = albedo color * texture color
    // payload.hitColor = mat.albedo.rgb * texColor;

    // Temporary: output barycentric coordinates (keep until buffers are verified)
    payload.hitColor = bary;
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}
```

## Implementation Strategy

The shader needs dynamic indexing of buffers based on `matIdx`. GLSL has two approaches:

### Option A: Switch-case (Recommended for now)

Generate a switch-case for each material slot. Verbose but works without extensions:

```glsl
void main() {
    uint matIdx = gl_InstanceCustomIndexEXT;
    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    uint triIdx = gl_PrimitiveID;

    vec2 uv;
    vec3 albedoColor;
    vec2 albedoScale;
    vec3 texColor;

    switch (matIdx) {
        case 0: {
            uint i0_0 = i0[triIdx * 3 + 0];
            uint i0_1 = i0[triIdx * 3 + 1];
            uint i0_2 = i0[triIdx * 3 + 2];
            uv = v0[i0_0].texCoord0 * bary.x
               + v0[i0_1].texCoord0 * bary.y
               + v0[i0_2].texCoord0 * bary.z;
            albedoColor = materials[0].albedo.rgb;
            albedoScale = materials[0].albedoScale;
            texColor = texture(tex0, uv * albedoScale).rgb;
            break;
        }
        case 1: {
            uint i1_0 = i1[triIdx * 3 + 0];
            // ... same pattern ...
            break;
        }
        // ... cases 2-63 ...
        default: {
            albedoColor = vec3(1.0, 0.0, 1.0); // Magenta = error
            texColor = vec3(1.0);
            break;
        }
    }

    payload.hitColor = albedoColor * texColor;
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}
```

### Option B: Non-uniform descriptor indexing (Cleaner)

Use `GL_EXT_nonuniform_qualifier` for direct indexing. Requires device support for `shaderStorageBufferArrayNonUniformIndexing` and `shaderSampledImageArrayNonUniformIndexing`.

```glsl
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding =  1) readonly buffer VBuf0  { RTVertex v[];  } vb[64];
layout(set = 1, binding =  2) readonly buffer IBuf0  { uint      i[];  } ib[64];
layout(set = 1, binding =  3) uniform sampler2D      tex[64];

void main() {
    uint matIdx = gl_InstanceCustomIndexEXT;
    uint nmatIdx = nonuniformEXT(matIdx);

    uint idx0 = ib[nmatIdx].i[triIdx * 3 + 0];
    uint idx1 = ib[nmatIdx].i[triIdx * 3 + 1];
    uint idx2 = ib[nmatIdx].i[triIdx * 3 + 2];

    vec2 uv = vb[nmatIdx].v[idx0].texCoord0 * bary.x
            + vb[nmatIdx].v[idx1].texCoord0 * bary.y
            + vb[nmatIdx].v[idx2].texCoord0 * bary.z;

    vec3 texColor = texture(tex[nmatIdx], uv * materials[matIdx].albedoScale).rgb;
    payload.hitColor = materials[matIdx].albedo.rgb * texColor;
}
```

**IMPORTANT**: Option B requires changes to the descriptor set layout creation in C++ (array bindings with `descriptorCount = MAX_MATERIALS` instead of individual bindings). This contradicts the current plan's "one binding per texture" approach. Stick with Option A (switch-case) unless performance is a concern.

## Recommended Approach

Use **Option A (switch-case)** for the initial implementation. It:
- Works without additional extensions
- Matches the descriptor set layout from the C++ plan (individual bindings)
- Is easier to debug
- Can be upgraded to Option B later if needed

The switch-case can be generated programmatically or written out for MAX_MATERIALS = 64. For a first implementation, start with a smaller MAX_MATERIALS (e.g., 16) and expand later.

## Incremental Implementation

To avoid a massive shader change, implement in stages:

1. **First**: Keep the barycentric output, just add the descriptor set bindings to verify layout matches
2. **Second**: Add the switch-case for a single material (case 0) to verify the full pipeline
3. **Third**: Expand to all 64 cases

## UV Recovery Algorithm

```glsl
// For a triangle with vertices v0, v1, v2 and barycentric coords (b0, b1, b2):
// UV = v0.texCoord0 * b0 + v1.texCoord0 * b1 + v2.texCoord0 * b2

// To get vertex indices from triangle index:
// idx0 = indexBuffer[triIdx * 3 + 0]
// idx1 = indexBuffer[triIdx * 3 + 1]
// idx2 = indexBuffer[triIdx * 3 + 2]

// To get vertex data from vertex index:
// vertex = vertexBuffer[idx]
// uv = vertex.texCoord0
```
