# UV accumulation

The canonical production workflow uses `bg2e::bake` with a draw Engine and GPU
primitives. Each mutable bake pass belongs to one lifecycle; Device and submitted
resource owners outlive all context, preview and output images. Use the maintained
references below for complete settings, signals, limits and error behavior.

- [LightmapOutput](../../api/bake/LightmapOutput.md)
- [draw_integrated_lightmaps](../draw_integrated_lightmaps.md)

For camera-less jobs use one live target/context, completed slots and a global
submission serial. Settings/geometry changes invalidate previous generations.
Cancelled or incomplete work must not publish successful results. CPU pixel
format, GPU precision and display conversion are separate contracts.
