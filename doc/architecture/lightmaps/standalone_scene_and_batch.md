# Standalone scene assembly and batches

The canonical production workflow uses `bg2e::bake` with a draw Engine and GPU
primitives. Each mutable bake pass belongs to one lifecycle; Device and submitted
resource owners outlive all context, preview and output images. Use the maintained
references below for complete settings, signals, limits and error behavior.

- [StandaloneBakeSceneAssembler](../../api/bake/StandaloneBakeSceneAssembler.md)
- [StandaloneBakeBatch](../../api/bake/StandaloneBakeBatch.md)
- [draw_standalone_lightmaps](../draw_standalone_lightmaps.md)

For camera-less jobs use one live target/context, completed slots and a global
submission serial. Settings/geometry changes invalidate previous generations.
Cancelled or incomplete work must not publish successful results. CPU pixel
format, GPU precision and display conversion are separate contracts.
