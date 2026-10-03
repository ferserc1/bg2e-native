# Step 08 — Documentation update

## Goal

Update `doc/deferred_render_model.md` so the technical reference matches the
new behavior.

## File

| File | Action |
|---|---|
| `doc/deferred_render_model.md` | Modify |

## Changes

### 1. §5.1 G-buffer attachment table

Update the fresnel+flags row:

```markdown
| 3 | Fresnel + flags | `VK_FORMAT_R8G8B8A8_UNORM` | RGB = fresnel tint. A = material flags bitfield (bit 0 = unlit, bit 1 = alpha test, bit 2 = transparent) |
```

### 2. §5.3 G-buffer fill pass — fragment shader paragraph

Add after the channel packing description:

```markdown
The fragment shader also resolves the material's **alpha test**: fragments
whose final albedo alpha (`baseColorFactor.a * texture.a`) is below
`material.alphaCutoff` are discarded, writing neither color nor depth. The
test applies to both the opaque and the transparent layer. Opaque cutout
materials write albedo alpha 1.0 after passing the test so the composite
blends them fully; transparent cutout materials keep their sampled alpha.
`alphaCutoff == 0` disables the test (the `ALPHA_TEST` flag is not set).
```

### 3. §6.2/§6.3 composite shaders

Mention that `DeferredGBufferData` exposes `alphaTest`/`transparent` decoded
from the flags bitfield (no composite behavior change).

### 4. §7 Ray-traced shadows

Replace the "Hard shadows / Soft shadows" bullet intro with a note that all
shadow ray queries in the deferred path (RT composite and both hit shaders)
use alpha-tested candidate traversal:

```markdown
BLAS geometry is built **without** `VK_GEOMETRY_OPAQUE_BIT_KHR` (BLAS are
shared between materials). Shadow and occlusion ray queries are initialized
without `gl_RayFlagsOpaqueEXT` and iterate candidates manually: a candidate
triangle is confirmed only if its material's alpha test passes
(`texture(albedoTex, uv).a * albedo.a >= alphaCutoff`, evaluated by
`rtAlphaTestCandidate()` in `lib/rt_material_data.glsl`). The RT composite
binds `RTMaterialDataBinding` as descriptor set 5 for this purpose.
```

### 5. §9 RTGI and §10 RT reflections

Add to both sections:

```markdown
Primary rays are traced without `gl_RayFlagsOpaqueEXT`, and the pipeline
includes a shared any-hit shader (`rt_alpha_test.rahit.glsl`) that discards
triangle hits failing the material's alpha test via `ignoreIntersectionEXT`.
```

Also update the pipeline bullet in each section:
"`VK_KHR_ray_tracing` pipeline with raygen / miss / closest-hit **+ any-hit**
groups".

### 6. §8 RTAO

Add:

```markdown
The AO pass binds `RTMaterialDataBinding` as descriptor set 1 and uses
`queryAOCutout()`, so occlusion rays see through alpha-tested holes.
```

### 7. §16 Ray tracing data bindings

Update the `RTMaterialDataBinding` bullet:

- binding 0 struct: add `alphaCutoff` (the former padding slot).
- Add: "The layout is created once in `DeferredLayer::build()` with stages
  `CLOSEST_HIT | ANY_HIT | FRAGMENT | COMPUTE` and shared by the GI and
  reflection pipelines (set 1), the RT composite (set 5) and the AO compute
  pass (set 1)."

### 8. §17 Known limitations

- Remove/adjust the entry about the glTF importer not supporting MASK if
  mentioned; add instead:
  - "glTF `alphaMode: MASK` maps to `alphaCutoff`; MASK materials render in
    the opaque queue and are included in ray tracing."
  - "The alpha test also applies to BLEND materials (default cutoff 0.5):
    alpha gradients below the cutoff are clipped."
  - "Transparent materials are excluded from the TLAS, so RT alpha testing
    only covers opaque cutout materials."
  - "All RT ray queries run without the opaque ray flag and every BLAS hit is
    a candidate: scenes without alpha-tested materials pay one any-hit
    invocation / candidate confirmation per hit."

## Verification

- Read through the modified document checking that every `file:line`
  reference still points to the right place after the code changes.
