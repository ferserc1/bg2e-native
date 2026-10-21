# Lightmap generator CLI

The canonical production workflow uses `bg2e::bake` with a draw Engine and GPU
primitives. Each mutable bake pass belongs to one lifecycle; Device and submitted
resource owners outlive all context, preview and output images. Use the maintained
references below for complete settings, signals, limits and error behavior.

- [lightmap_generator](../draw_lightmap_generator.md)
- [standalone_lightmaps](../../tutorials/draw/standalone_lightmaps.md)

For camera-less jobs use one live target/context, completed slots and a global
submission serial. Settings/geometry changes invalidate previous generations.
Cancelled or incomplete work must not publish successful results. CPU pixel
format, GPU precision and display conversion are separate contracts.
