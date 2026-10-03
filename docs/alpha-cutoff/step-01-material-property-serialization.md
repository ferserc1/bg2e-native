# Step 01 — Material property, serialization, loaders and UI

## Goal

Add `alphaCutoff` (float, 0..1, default 0.5) to `bg2e::base::MaterialAttributes`,
serialize/deserialize it with the exact JSON name `"alphaCutoff"`, map glTF
`alphaMode: MASK` into it, and expose it in the model_edit material editor.

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/base/MaterialAttributes.hpp` | Modify |
| `lib/src/bg2e/utils/MaterialSerializer.cpp` | Modify |
| `lib/src/bg2e/db/scene_gltf.cpp` | Modify |
| `lib/src/bg2e/ui/MaterialEditor.cpp` | Modify |

## 1.1 `MaterialAttributes.hpp`

Add the property next to `refractionFactor` (both relate to transparency).
Follow the existing getter/setter convention (numeric scalar: `alphaCutoff()`
getter, `setAlphaCutoff(float)` setter).

Declaration, after the `refractionFactor` accessors (~line 51):

```cpp
inline float refractionFactor() const { return _refractionFactor; }
inline void setRefractionFactor(float f) { _refractionFactor = f; }

// Alpha test threshold. Fragments with final albedo alpha < alphaCutoff are
// discarded (alpha cutout). Applies to opaque and transparent materials.
inline float alphaCutoff() const { return _alphaCutoff; }
inline void setAlphaCutoff(float c) { _alphaCutoff = c; }
```

Member, after `_refractionFactor` (~line 198):

```cpp
float _refractionFactor = 0.017f;
float _alphaCutoff = 0.5f;
```

The default is 0.5 to match the glTF specification and bg2-scene's
`Bg2Material::alphaCutoff = 0.5f`
(`lib/third_party/bg2-scene/bg2-material.hpp:57`).

## 1.2 `MaterialSerializer.cpp`

### Parse — `parseMaterial()`, after the `refractionFactor` block (~line 80):

```cpp
if (mat["alphaCutoff"] && mat["alphaCutoff"]->isNumber())
{
    result.setAlphaCutoff(mat["alphaCutoff"]->numberValue(0.5f));
}
```

Old files without the field keep the class default (0.5) — same behavior as
bg2-scene deserializing legacy materials.

### Serialize — `MaterialSerializer::serializeMaterial()`, base `JsonObject`
block (lines 244-254). Add after `"refractionFactor"`:

```cpp
auto matJson = JSON(JsonObject{
    { "name", JSON(mat.name()) },
    { "groupName", JSON(mat.groupName()) },
    { "type", JSON("pbr") },
    { "class", JSON("PBRMaterial") },
    { "isTransparent", JSON(mat.isTransparent()) },
    { "refractionFactor", JSON(mat.refractionFactor()) },
    { "alphaCutoff", JSON(mat.alphaCutoff()) },
    { "isSolid", JSON(mat.isSolid() )},
    ...
});
```

The `.bg2` path (`lib/src/bg2e/db/mesh_bg2.cpp:64-88`) funnels materials
through `MaterialSerializer::deserializeMaterialArray`, so it is covered
automatically.

## 1.3 `scene_gltf.cpp` — glTF MASK support

Replace the "MASK not supported" comment block at lines 289-301:

```cpp
if (material)
{
    result.setRefractionFactor(0.001f);

    // glTF defines transparency through alphaMode. BLEND requires ordinary
    // alpha compositing; OPAQUE ignores alpha; MASK enables the alpha test
    // with the material's cutoff (cgltf defaults it to 0.5 when absent).
    if (material->alpha_mode == cgltf_alpha_mode_blend)
    {
        result.setIsTransparent(true);
        result.setIsSolid(false);
    }
    else if (material->alpha_mode == cgltf_alpha_mode_mask)
    {
        result.setAlphaCutoff(material->alpha_cutoff);
    }
    ...
}
```

Notes:
- MASK materials stay opaque (`isTransparent == false`) → they render in the
  opaque render queue and enter the RT acceleration structures. This is the
  correct glTF semantic (MASK ≠ BLEND).
- cgltf already parses `alpha_cutoff` with default 0.5
  (`lib/third_party/cgltf/cgltf.h:4763,4817-4820`), so no guard for "field
  absent" is needed.
- BLEND materials keep the class default cutoff (0.5) and are therefore also
  alpha-tested in the G-buffer pass (per the agreed decision that the test
  applies to transparent materials too).

## 1.4 `MaterialEditor.cpp` — editor slider

Insert immediately after the `refractionFactor` slider block
(`lib/src/bg2e/ui/MaterialEditor.cpp:183-191`), following the identical
multi-edit pattern:

```cpp
auto alphaCutoff = _material->materialAttributes().alphaCutoff();
if (Numeric::sliderFloat("Alpha Cutoff##alphaCutoff", &alphaCutoff, 0.0f, 1.0f))
{
    for (auto & mat : _editMaterialList)
    {
        mat->materialAttributes().setAlphaCutoff(alphaCutoff);
    }
    notifyOnChange();
}
```

## Integration points

- Downstream consumers (`PBRMaterialData::operator=`, RT visitor) read the
  property via `materialAttributes().alphaCutoff()` — wired in step 02.
- No changes to `RenderQueue` classification: cutout materials remain in the
  opaque queue; only `isTransparent`/`isSolid` drive queue selection
  (`lib/src/bg2e/render/RenderQueue.cpp:35-63`).

## Verification

- Round-trip: serialize a material with `setAlphaCutoff(0.3)`, deserialize,
  expect 0.3. Deserialize JSON without the field, expect 0.5.
- Load a glTF with `alphaMode: MASK` and check `alphaCutoff()` matches the
  file's `alphaCutoff` (0.5 if absent).
