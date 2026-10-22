# Lightmap output and materials

The canonical production workflow uses `bg2e::bake` with a draw Engine and GPU
primitives. Each mutable bake pass belongs to one lifecycle; Device and submitted
resource owners outlive all context, preview and output images. Use the maintained
references below for complete settings, signals, limits and error behavior.

- [LightmapOutputWriter](../../api/bake/LightmapOutputWriter.md)
- [BakedIndirectMaps](../../api/draw/BakedIndirectMaps.md)
- [LightmapOutput](../../api/bake/LightmapOutput.md)

For camera-less jobs use one live target/context, completed slots and a global
submission serial. Settings/geometry changes invalidate previous generations.
Cancelled or incomplete work must not publish successful results. CPU pixel
format, GPU precision and display conversion are separate contracts.

Editors default to linear RGB8 PNG with GI exposure applied before clamp/round;
AO and RGB32F ignore exposure. Optional HDR output uses standard Radiance RGBE,
which preserves dynamic range with quantization. No proprietary texture codec is
used. Editors assign uniquely named temporary image files to materials; complete
scene/model save consolidates those files by filename. Missing material textures
remain absent in serialized data and use neutral runtime placeholders. Assigned
procedural/GPU-only 2D textures without a file use mip-zero PNG/HDR readback;
authored mip chains are not persisted. See [complete resource IO](../../api/db/DrawResourceIO.md)
and [standard image IO](../../api/db/FileIO.md) for supported formats and limits.
