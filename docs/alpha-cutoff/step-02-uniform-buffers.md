# Step 02 — Uniform buffers: PBRMaterialData and RTMaterialData

## Goal

Carry `alphaCutoff` (plus the material flags needed by the G-buffer shader)
from `MaterialAttributes` into the two GPU-facing material structs, reusing
the existing `uint32_t padding` slots so struct sizes and offsets do not
change.

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/render/uniforms/materials.hpp` | Modify |
| `shaders/src/glsl/lib/uniforms.glsl` | Modify |
| `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h` | Modify |
| `shaders/src/glsl/lib/rt_material_data.glsl` | Modify (struct only; the binding restructure + helpers are step 04) |
| `lib/src/bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.cpp` | Modify |

## 2.1 `PBRMaterialData` — `lib/include/bg2e/render/uniforms/materials.hpp`

### Flags enum — add two bits:

```cpp
enum Flags : uint32_t
{
    NONE = 0x0,
    UNLIT = 0x1u << 0,
    ALPHA_TEST = 0x1u << 1,
    TRANSPARENT = 0x1u << 2,
};
```

### Member — replace the trailing padding (line 75):

```cpp
    uint32_t lightEmissionUVSet;

    float alphaCutoff = 0.5f;   // was: uint32_t padding = 0;
```

`float` and `uint32_t` are both 4-byte aligned here; the struct size and all
other offsets are unchanged.

### `operator=` — fill the new fields:

```cpp
void operator=(const base::MaterialAttributes& att)
{
    ...
    lightEmissionUVSet = att.lightEmissionUVSet();

    alphaCutoff = att.alphaCutoff();

    flags = 0;
    flags |= att.isUnlit() ? UNLIT : 0u;
    // Alpha test applies to opaque AND transparent materials.
    flags |= att.alphaCutoff() > 0.0f ? ALPHA_TEST : 0u;
    flags |= att.isTransparent() ? TRANSPARENT : 0u;
}
```

The `TRANSPARENT` bit lets the G-buffer shader know whether the surviving
alpha must be forced to 1.0 (opaque cutout) or preserved (transparent cutout,
needed for the composite blending).

Upload path: `ObjectDataBinding` builds `ObjectUniforms { mat4; PBRMaterialData; }`
and copies with `uniforms.material = material->materialAttributes();`
(`lib/src/bg2e/scene/vk/ObjectDataBinding.cpp:64-69`) — no changes needed
there.

## 2.2 GLSL mirror — `shaders/src/glsl/lib/uniforms.glsl`

Struct (line 65):

```glsl
    // Light emission
    float lightEmission;
    vec2 lightEmissionScale;
    int lightEmissionChannel;
    int lightEmissionInvert;
    int lightEmissionUVSet;

    float alphaCutoff;   // was: uint padding;
};
```

Constants (after line 68):

```glsl
const uint MATERIAL_FLAG_UNLIT          = 1u << 0;
const uint MATERIAL_FLAG_ALPHA_TEST     = 1u << 1;
const uint MATERIAL_FLAG_TRANSPARENT    = 1u << 2;
```

Note: the C++ `flags` member is named `unlit` in the GLSL struct (historical).
Keep the name; it already carries the UNLIT bit and now carries the two new
bits. All readers use `mat.unlit & MATERIAL_FLAG_*`.

This struct is shared by the forward shaders; they ignore the new field and
bits — no behavior change there.

## 2.3 `RTMaterialData` — `lib/include/bg2e/render/vulkan/rt/RTMaterialData.h`

```cpp
struct RTMaterialData {
    base::Color albedo;
    glm::vec2 albedoScale;
    uint32_t indexOffset;   // firstIndex of the submesh in the shared index buffer
    float lightEmission;
    uint32_t lightEmissionChannel;
    uint32_t lightEmissionInvert;
    glm::vec2 lightEmissionScale;
    uint32_t lightEmissionUVSet;
    float alphaCutoff;      // was: uint32_t padding;
};
```

Still 56 bytes. The GLSL side is read with `layout(scalar)`, so the array
stride stays exactly 56 (see the comment in `rt_gi.rchit.glsl:41-43`).

## 2.4 GLSL mirror — `shaders/src/glsl/lib/rt_material_data.glsl`

```glsl
// Must match C++ RTMaterialData struct layout (56 bytes)
struct RTMaterialData {
    vec4 albedo;          // base::Color (rgba)
    vec2 albedoScale;
    uint indexOffset;     // firstIndex of the submesh in the shared index buffer
    float lightEmission;
    uint lightEmissionChannel;
    uint lightEmissionInvert;
    vec2 lightEmissionScale;
    uint lightEmissionUVSet;
    float alphaCutoff;    // was: uint padding1;
};
```

## 2.5 Population — `CollectRayTracingInstancesVisitor.cpp:82`

Replace `objInst.materialData.padding = 0;` with:

```cpp
objInst.materialData.alphaCutoff = renderMat->materialAttributes().alphaCutoff();
```

The visitor's filter at line 53
(`!drw->renderMaterial(i)->materialAttributes().isTransparent()`) stays:
transparent materials never enter the TLAS, so RT alpha testing only covers
opaque cutout materials.

## Integration points

- Step 03 reads `mat.alphaCutoff` and the flag bits in
  `deferred_gbuffer.frag.glsl` / `deferred_utils.glsl`.
- Steps 05–07 read `RTMaterialData.alphaCutoff` in the alpha-test ray helpers.
- `RTMaterialDataBinding.cpp` copies the structs verbatim into the SSBO — no
  changes needed there.

## Verification

- `static_assert(sizeof(RTMaterialData) == 56)` may be added in
  `RTMaterialData.h` to lock the layout.
- Renderdoc / debug visualization of `GBufferFresnelFlags` (alpha as gray,
  channel mode 4) shows the flag bits once step 03 packs them.
