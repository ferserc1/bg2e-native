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
preserves direct light and emission, bypasses live indirect AO/GI for fragments
whose material has a baked mode, and applies GI only to diffuse IBL/sheen.
Independent reflections remain
a separate display policy; callers should disable live AO/GI for a fully baked
scene to avoid paying for unused passes. Custom G-buffer/deferred layers must
match the updated output format. Producers must ensure finite nonnegative
multipliers within the half-float display range [0,65504]. Bake's RGBA32F result
and RGB32F CPU output are not reduced to this precision.

## Material flags and image data

Baked interpretation is authored once per material through
`setIndirectTextureMode(...)`. Packing derives the baked/GI flags; rasterization
stores those flags for the G-buffer fragments produced by that material. The
lighting pass reads them to bypass live AO/GI and choose the multiplier's scope.
There is no per-texel baked marker to author in a PNG, and alpha is not a mode or
coverage mask. The entire material uses the chosen interpretation, while UV2
selects the RGB multiplier at each surface point. In baked modes `aoChannel()`
is retained as material state but does not select the sampled components.

## Components, precision and portable output

The interpretation mode does not select a pixel format. Both baked AO and baked
GI support RGB8; HDR is optional. `LightmapSettings::cpuFormat()` defaults to
`RGB8` intentionally for browser/WebGL use with ordinary PNG images rather than
requiring HDR texture loading. The output is linear data and must not undergo
sRGB decoding when sampled as an indirect multiplier.

| Stage | Representation |
|-------|----------------|
| CPU `base::Image` | Byte or float components, with an independent component count. |
| CPU bake output | Tightly packed RGB8 (3 bytes/pixel) or RGB32F (12 bytes/pixel), no alpha. |
| Ordinary `draw::Texture` upload from CPU pixels | RGBA8 UNORM for bytes; RGBA32F for floats. |
| GPU bake composition result | RGBA32F, independent of the CPU export choice. |
| Raster G-buffer baked multiplier | RGBA16F, independent of the source file format. |
| Persisted image | RGB8 PNG by default; RGB32F output is encoded as Radiance RGBE HDR with quantization. |

Scalar AO means sampling one selected component, not necessarily one stored byte.
RGB means three sampled components, not necessarily floating-point storage.
A GPU float intermediate does not require distributing an HDR file.

For RGB8 GI, each component is converted as:

```text
byte = floor(clamp(multiplier * 2^exposureEV, 0, 1) * 255 + 0.5)
```

At exposure zero, multipliers 1, 2 and 4 all become 255. Negative exposure reduces
the values before clipping, but changes the resulting multiplier: there is no
automatic inverse exposure compensation during material sampling. Exposure is
baked into the byte output. AO byte conversion uses the same clamp/round without
exposure; RGB32F preserves finite nonnegative multipliers and ignores exposure.
The writer does not apply exposure a second time.

## Serialization

MaterialSerializer preserves `indirectTextureMode` as `ao`, `bakedAO` or
`bakedGI`; missing metadata leaves the receiver's existing mode unchanged
(ordinary scalar AO for a newly constructed material). Baked texture files load with a linear color interpretation. This metadata is not a texture
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
