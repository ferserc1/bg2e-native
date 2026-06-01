# Step 4: Modify RTMaterialData.h - Add RTObjectInstance

## Purpose

Add the `RTObjectInstance` struct that combines material data, vertex buffer, index buffer, and albedo texture into a single struct. All indices share the same object index, so we don't need separate indices for each component.

## Files to Modify

- `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h`

## Changes to RTMaterialData.h

### Add include

```cpp
#include <bg2e/render/Texture.hpp>
```

(This may already be included, but ensure it's present.)

### Add MAX_OBJECTS constant

```cpp
static constexpr uint32_t MAX_OBJECTS = 256;
```

### Add RTObjectInstance struct

Add this struct after `RTMaterialInstance`:

```cpp
// Represents a single ray-tracing visible object.
// All indices (material, vertex buffer, index buffer, texture) share the same object index.
// This struct is stored sequentially by CollectRayTracingInstancesVisitor and indexed
// by gl_InstanceCustomIndexEXT in the closest hit shader.
struct RTObjectInstance {
    RTMaterialData materialData;    // albedo color + albedoScale
    const Buffer*  vertexBuffer = nullptr;  // vertex buffer for UV recovery
    const Buffer*  indexBuffer = nullptr;   // index buffer for triangle lookup
    render::Texture* albedoTexture = nullptr; // albedo texture (fallback white if null)
};
```

## Full File Content

```cpp
/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/Texture.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

static constexpr uint32_t MAX_OBJECTS = 256;

struct RTMaterialData {
    base::Color albedo;
    glm::vec2 albedoScale;
    uint32_t padding[2];
};

struct RTMaterialInstance {
    RTMaterialData data;
    const Buffer* vertexBuffer = nullptr;
    const Buffer* indexBuffer = nullptr;
    render::Texture* albedoTexture = nullptr;
};

// Represents a single ray-tracing visible object.
// All indices (material, vertex buffer, index buffer, texture) share the same object index.
// This struct is stored sequentially by CollectRayTracingInstancesVisitor and indexed
// by gl_InstanceCustomIndexEXT in the closest hit shader.
struct RTObjectInstance {
    RTMaterialData materialData;    // albedo color + albedoScale
    const Buffer*  vertexBuffer = nullptr;  // vertex buffer for UV recovery
    const Buffer*  indexBuffer = nullptr;   // index buffer for triangle lookup
    render::Texture* albedoTexture = nullptr; // albedo texture (fallback white if null)
};

}
}
}
}
```

## Notes

- `RTMaterialInstance` is kept for backward compatibility (used by the old visitor pattern)
- `RTObjectInstance` is the new unified struct used by the ray tracing system
- Both structs have the same fields, but `RTObjectInstance` is the canonical one going forward
- The `albedoTexture` field is always valid because `MaterialBase` returns a white texture fallback if no albedo is set
- The `vertexBuffer` and `indexBuffer` are always valid for RT-visible drawable objects