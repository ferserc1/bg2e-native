# Baked indirect maps

`base::MaterialAttributes::IndirectTextureMode` gives the six-slot draw material
an explicit interpretation for its existing AO texture slot:

- `AmbientOcclusion` (default): selected scalar AO channel, unchanged behavior.
- `BakedAmbientOcclusion`: linear RGB multiplier for all indirect IBL.
- `BakedGlobalIllumination`: linear RGB diffuse multiplier, preserving specular
  environment/reflection contributions.

Baked maps require an opaque material, a realized linear texture and
`aoUVSet() == 1` (`texCoord1` / UV2). `aoScale()` selects the UV scale. Original
scalar AO is bypassed while a baked mode is active; this representation has one
AO/indirect slot, so a separate authored AO texture and baked map do not coexist.
`packMaterial` validates the explicit mode and emits flag 32 for a baked map and
flag 64 for GI, without changing MaterialData layout or texture descriptor ABI.
Flags, texture presence and UV choice are independent; UV2 or an AO texture alone
never enables a baked map.

The raster G-buffer writes RGB into the `bakedMultiplier` output, now RGBA16F.
Its other five formats remain unchanged, for 32 aggregate color attachment bytes
per sample. Unbaked pixels/materials write neutral one. The deferred shader
preserves direct light and emission, bypasses live indirect AO/GI for marked
pixels, and applies GI only to diffuse IBL/sheen. Independent reflections remain
a separate display policy; callers should disable live AO/GI for a fully baked
scene to avoid paying for unused passes. Custom G-buffer/deferred layers must
match the updated output format. Producers must ensure finite nonnegative
multipliers within the half-float display range [0,65504]. Bake's RGBA32F result
and RGB32F CPU output are not reduced to this precision.

MaterialSerializer preserves `indirectTextureMode` as `ao`, `bakedAO` or
`bakedGI`; missing metadata keeps the ordinary scalar AO default. Baked texture
files load with a linear color interpretation. This metadata is not a texture
export API. GPU-authored textures need an explicit image export before a saved
material path can be reloaded elsewhere.

See [the integrated example](../../../examples/draw/12_lightmaps/README.md) for
completed-result adoption, stored/live material metadata and UI ownership.
Compilation and backend visual verification remain project lead checks.

## Persisted standalone resources

The [standalone bake writer](../bake/LightmapOutputWriter.md) exports actual pixels,
then clones/updates materials with linear UV2 bakedAO/bakedGI and standard image
sidecars. Radiance HDR file/model resources retain dynamic range with RGBE
quantization and can preserve values beyond the
interactive half-float display range; their file integrity does not promise the
G-buffer can display those values without its independent range checks. Original
material ownership remains unchanged. [Generic complete DB IO](../db/DrawResourceIO.md)
and whole-batch manifest publication have distinct policies.
