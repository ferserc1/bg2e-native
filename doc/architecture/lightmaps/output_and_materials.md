# Output files and current material integration

## LightmapOutputWriter and ImageFormat

`ImageFormat` identifies PNG, JPEG, BMP and TGA. Extension helpers accept dotted
or undotted names case-insensitively; JPEG aliases resolve to canonical `.jpg`.
There is no HDR file export. `LightmapOutputWriter` owns a private `State` through
a unique pointer; it is movable and non-copyable. The state holds target plans,
identity lookup, reserved artifacts, committed sidecars and overwrite/preflight flags.

`preflight()` is a one-time operation planning a complete batch before baking.
It accepts `Target` records (identity, source `.bg2`, node), optional model-copy
output and explicit protected input paths. `OutputPaths` contains image path and
optional model path. All targets must be attached Drawables under one root.
Their model sources, loaded scene texture/environment inputs and caller-provided
context/prefab/other asset paths are protected against output aliasing.
Comparison uses weak canonicalization and Windows case folding. Existing paths,
including symlinks, and reservations for other outputs/texture sidecars are
checked; overwrite permission never permits replacing protected inputs.

Filename stems retain ASCII letters/digits, `_` and `-`; separators become `__`
and other characters become `_`. Empty stems use `target`. Long names are
truncated and gain a stable 64-bit FNV-1a hash; sanitized collisions gain the
identity hash as well. A remaining reservation collision throws. Sidecars may
share an output only when they correspond to the same source.

Sources: [writer declaration](../../../lib/include/bg2e/db/LightmapOutputWriter.hpp),
[writer State/naming/preflight](../../../lib/src/bg2e/db/LightmapOutputWriter.cpp),
[image API](../../../lib/include/bg2e/db/image.hpp).

## Image/model write sequence and failure limits

`write(pixels, identity)` requires successful preflight and one known, unwritten
target. It validates dimensions, variant and element count. RGB8 is copied;
RGB32F must be finite, is clamped to `[0,1]` and rounded to bytes without exposure.
The output directory is created at write time. The pending image uses a sibling
filename preserving the extension so `saveImage()` selects the correct encoder.

For model copies, a staging directory receives the image and a cloned Drawable.
The clone's materials are assigned the staged image using:

```cpp
material.setAoTexture(lightmapTexture);
material.setAoScale(glm::vec2(1.0f, 1.0f));
material.setAoUVSet(1);
material.setAoChannel(0);
```

Source: [LightmapOutputWriter::write](../../../lib/src/bg2e/db/LightmapOutputWriter.cpp).
`storeDrawableBg2()` serializes the clone with the generated UV2 and stages its
required texture sidecars. Input Drawable material attributes are not changed.
Commit renames sidecars, then model, then image; already committed shared sidecars
are tracked to avoid duplicating their output. On failure the writer removes
artifacts newly committed by that call and its staging files.

These are staged writes with cleanup, not an atomic multi-file transaction.
When overwrite is enabled, `replaceWith()` removes the existing output before
rename. The old output is not backed up/restored if rename or a later commit
fails. Even one replacement has a remove/rename gap. Successful earlier target
writes remain if a later target fails. This is the actual implementation contract.

## Material AO and the pending RGB illumination channel

A separate RGB illumination/lightmap material channel has not been added. This
remains future work. The current public/serialized carrier is the AO texture
property, with a UV-set selector, scale and scalar channel selector. Its ordinary
AO sampling contract selects one component and the standard material G-buffer
stores scalar AO in an 8-bit UNORM channel. That scalar path alone cannot express
independent colored illumination.

Sources: [MaterialAttributes AO fields](../../../lib/include/bg2e/base/MaterialAttributes.hpp),
[material serialization](../../../lib/src/bg2e/utils/MaterialSerializer.cpp),
[scalar AO sample](../../../shaders/src/glsl/lib/uniforms.glsl),
[material G-buffer](../../../shaders/src/glsl/deferred_gbuffer.frag.glsl).

The source already includes a transitional RGB deferred path reusing that same
AO texture; documenting the code requires distinguishing it from a dedicated
material channel:

- `MaterialUniforms` sets `HAS_BAKED_LIGHTMAP` from explicit AO texture presence.
  It is an internal flag, not an independently serialized material property.
- The default `GBufferManager` includes a separate RGBA8 baked multiplier
  attachment. The deferred G-buffer shader samples AO texture `.rgb` into it
  when the flag is set, otherwise white. This is distinct from the baker's
  four-attachment depthless UV surface profile and from scalar material AO.
- The ordinary and RT deferred composite paths use the baked multiplier on
  ambient/indirect lighting, leaving direct light and emission separate. The
  RT composite selects its ambient branch for baked materials rather than
  compounding the live indirect result with the bake.

```glsl
// Existing deferred composition, not a new public material channel.
vec3 bakedMultiplier = texture(g_BakedLightmap, vTexcoord).rgb;
outColor = compositeFinalColor(ambient * bakedMultiplier + emissionColor,
    Lo, gbuf.inputColor, gbuf.albedo.a,
    pushConstant.exposure, pushConstant.gamma,
    pushConstant.brightness, pushConstant.contrast);
```

Sources: [material flag](../../../lib/include/bg2e/render/uniforms/materials.hpp),
[G-buffer profile](../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp),
[G-buffer shader](../../../shaders/src/glsl/deferred_gbuffer.frag.glsl),
[deferred composite](../../../shaders/src/glsl/deferred_composite.frag.glsl),
[RT composite](../../../shaders/src/glsl/deferred_composite_rt.frag.glsl).

The presence-based flag also means a supplied AO texture is currently treated
as baked by that deferred path; it does not independently distinguish authored
scalar AO from a colored bake. The exported `.bg2` references AO, not a separate
RGB illumination slot. GPU RGBA32F bake output and CPU RGB32F access therefore
do not establish HDR or dedicated RGB material support, and the deferred baked
attachment itself is 8-bit. Future material-channel work must address the
independent representation/application of RGB illumination. No such API is
assumed or specified here.
