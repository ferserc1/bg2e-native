# MaterialAttributes

**Header:** `<bg2e/base/MaterialAttributes.hpp>`  
**Namespace:** `bg2e::base`

`MaterialAttributes` stores CPU material parameters, texture descriptions, and
mesh-related metadata. GPU realization is managed by higher-level consumers.

## Scalar and color attributes

Every property below has a getter with the listed name and a `set`-prefixed
setter (for example, `albedo()` / `setAlbedo(...)`).

| Property | Default / meaning |
|----------|-------------------|
| `isTransparent` | false; transparency selection. Setter: `setIsTransparent`. |
| `isSolid` | true. Setter: `setIsSolid`. |
| `isUnlit` | false. Setter: `setIsUnlit`. |
| `doubleSided` | false. `bool doubleSided() const` / `void setDoubleSided(bool)`. |
| `refractionFactor` | 0.017; screen-space refraction strength for transparent materials. |
| `alphaCutoff` | 0; discard below final albedo alpha threshold. |
| `albedo` | White; setters accept `Color` or float arrays of length 3/4. |
| `metalness` | 0. |
| `roughness` | 0.9. |
| `fresnelTint` | White. |
| `sheenIntensity`, `sheenColor` | 0 and White. |
| `lightEmission` | 0. |
| `name`, `groupName`, `visible` | Empty names and true; metadata retained for mesh serialization. |

## Texture slots

Double sided is generic material state, not a text feature: it selects no-cull
raster/picking variants and allows traced backfaces. The authored tangent frame
is normal-mapped first, then the resulting normal is reversed once on a double-sided
backface. Single-sided ray visibility rejects backfaces before alpha testing,
including mirrored transforms. Materials previously relying on invisible backfaces
as ray blockers must explicitly enable it. Clones, material JSON, glTF's
doubleSided import and Composer undo/redo retain the value.

Alpha cutoff tests the sampled albedo alpha times the material scalar alpha.
The public draw helper `float effectiveAlphaTestCutoff(float authoredCutoff)` in
`<bg2e/draw/ShaderData.hpp>` validates finite `[0,1]` input, then returns zero for
values below `AlphaTestCutoffThreshold = 0.02f` (equality enables the test).
Invalid generic material cutoffs throw at packing; TextComponent contains those
lower-level failures and omits the text. Use 0.1 as the text/tutorial default,
not merely an arbitrarily small positive cutoff. Packing derives the public
`MaterialDoubleSided = 128` flag from the material value; applications should
edit the material rather than patching packed GPU flags.
Text's generated coverage therefore cuts out real glyph silhouettes in raster,
shadows and traced visibility; picking intentionally retains its whole rectangle.
See [TextComponent](../scene/TextComponent.md).

Slots are `albedoTexture`, `metalnessTexture`, `roughnessTexture`,
`normalTexture`, `aoTexture`, and `lightEmissionTexture`. Each getter returns
`shared_ptr<Texture>`; setters accept shared pointers or raw pointers that
transfer ownership. All slots initially have no texture.

Each slot has a scale and UV-set pair using its prefix, for example
`albedoScale()` / `setAlbedoScale(...)` and `albedoUVSet()` /
`setAlbedoUVSet(...)`. Scales default to `(1, 1)` and accept GLM vectors or
float arrays; UV-set indices default to zero. Metalness, roughness, AO, and
emission expose channel selection (default 0). Metalness, roughness, and
emission also expose inversion (default false).

## Indirect texture interpretation

`indirectTextureMode()` / `setIndirectTextureMode(...)` selects
`IndirectTextureMode::AmbientOcclusion`, `BakedAmbientOcclusion`, or
`BakedGlobalIllumination`. The default is `AmbientOcclusion`; unsupported values
throw `std::invalid_argument`.

| Mode | Sampled components | Lighting interpretation |
|------|--------------------|-------------------------|
| `AmbientOcclusion` | One component selected by `aoChannel()` | Scalar AO, clamped to [0,1]. |
| `BakedAmbientOcclusion` | RGB | Linear multiplier for all indirect IBL. |
| `BakedGlobalIllumination` | RGB | Linear multiplier for diffuse IBL, including sheen; specular IBL/reflections are preserved. |

All three modes use the same `aoTexture()`, `aoScale()` and `aoUVSet()` properties.
There is no additional lightmap slot: ordinary AO and a baked map cannot occupy
this slot simultaneously. Baked modes bypass scalar AO and read RGB regardless
of the stored `aoChannel()` value. Direct lighting and emission remain separate.

The mode belongs to the material, not to individual texels or the image alpha
channel. The renderer propagates material flags to the G-buffer fragments drawn
with that material; lighting uses those flags to select baked composition and
bypass live AO/GI. An image, its UV set or its file extension never enables baked
interpretation automatically.

Component selection is independent of numeric precision: scalar AO does not
necessarily mean one byte, and baked RGB does not require HDR. RGB8 baked GI is
supported and is the default bake CPU output for browser/WebGL delivery using
ordinary image files. RGB32F is optional for preserving HDR multipliers.

Draw packing requires an opaque material, an assigned and realized AO texture
with linear color interpretation, and `aoUVSet() == 1` (the second UV set, also
called UV2 or `texCoord1`). The enum setter validates the mode; packing validates
these resource/property combinations. Generic UV selectors are limited to 0/1
and scalar channel selectors to 0–3 at packing time.
See [Baked indirect maps](../draw/BakedIndirectMaps.md) for precision, rendering
and export contracts, and [LightmapSettings](../bake/LightmapSettings.md) for the
RGB8 default and exposure conversion.

## Change tracking

`revision()` tracks changes to authored state independently of texture realization.
Texture setters mark the associated `*TextureUpdated()` flag false.
`setUpdated()` marks all texture flags true; `isUpdated()` requires all six.
Copy construction is defaulted; assignment tracks copied attribute changes.

## See also

- [Base API](index.md)
- [API reference](reference.md)
