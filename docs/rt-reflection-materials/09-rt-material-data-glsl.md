# Step 9: Create rt_material_data.glsl Shader Include

## File Path

`shaders/src/lib/rt_material_data.glsl`

## Purpose

Define GLSL structs that match the C++ `RTMaterialData` and `geo::Vertex` (VertexPNUUT) layouts, for use in ray tracing shaders.

## Dependencies

The C++ structs that must be matched:

### RTMaterialData (from `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h`)

```cpp
struct RTMaterialData {
    base::Color albedo;        // 4 floats (r, g, b, a) = 16 bytes
    glm::vec2 albedoScale;     // 2 floats = 8 bytes
    uint32_t padding[2];       // 8 bytes
};                             // Total: 32 bytes
```

### base::Color (from `lib/include/bg2e/base/Color.hpp`)

```cpp
struct Color {
    float r, g, b, a;   // 4 floats, no alignment padding
};
```

### geo::Vertex / VertexPNUUT (from `lib/include/bg2e/geo/Vertex.hpp`)

```cpp
struct VertexPNUUT {
    glm::vec3 position;    // 12 bytes
    glm::vec3 normal;      // 12 bytes
    glm::vec2 texCoord0;   // 8 bytes
    glm::vec2 texCoord1;   // 8 bytes
    glm::vec3 tangent;     // 12 bytes
};                         // Total: 52 bytes
```

## GLSL Content

```glsl
#ifndef RT_MATERIAL_DATA_GLSL
#define RT_MATERIAL_DATA_GLSL

#include "lib/constants.glsl"

// Must match C++ RTMaterialData struct layout (32 bytes)
struct RTMaterialData {
    vec4 albedo;          // base::Color (rgba)
    vec2 albedoScale;
    uint padding0;
    uint padding1;
};

// Must match C++ geo::Vertex / VertexPNUUT struct layout (52 bytes)
struct RTVertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord0;
    vec2 texCoord1;
    vec3 tangent;
};

#endif
```

## Layout Verification

### RTMaterialData alignment

| Field | Type | Offset | Size |
|-------|------|--------|------|
| albedo | vec4 | 0 | 16 |
| albedoScale | vec2 | 16 | 8 |
| padding0 | uint | 24 | 4 |
| padding1 | uint | 28 | 4 |
| **Total** | | | **32** |

C++ and GLSL match. Both are 32 bytes with the same field offsets.

### RTVertex alignment

| Field | Type | Offset | Size |
|-------|------|--------|------|
| position | vec3 | 0 | 12 |
| normal | vec3 | 12 | 12 |
| texCoord0 | vec2 | 24 | 8 |
| texCoord1 | vec2 | 32 | 8 |
| tangent | vec3 | 40 | 12 |
| **Total** | | | **52** |

C++ `glm::vec3` is 12 bytes with 4-byte alignment. GLSL `vec3` is also 12 bytes. The layout matches.

**Note**: GLSL `vec3` has a base alignment of 16 bytes in std140 layout, but in SSBO with std430 layout, `vec3` has a base alignment of 4 bytes (same as `float`). Since we're using SSBOs, std430 rules apply and the layout matches C++. However, if a `vec3` is followed by a `float` or `vec2`, there could be alignment issues. In this case, `vec3 position` is followed by `vec3 normal` (both 4-byte aligned in std430), so there's no padding issue.

## Usage in Shaders

The shader uses array bindings with `GL_EXT_nonuniform_qualifier`:

```glsl
#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "lib/rt_material_data.glsl"

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
```

## Constants

The `MAX_OBJECTS` constant should be defined in `lib/constants.glsl`:

```glsl
#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL

#define MAX_OBJECTS 256

#endif
```