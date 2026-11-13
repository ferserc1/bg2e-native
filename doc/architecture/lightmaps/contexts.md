# Bake contexts

The canonical production workflow uses `bg2e::bake` with a draw Engine and GPU
primitives. Each mutable bake pass belongs to one lifecycle; Device and submitted
resource owners outlive all context, preview and output images. Use the maintained
references below for complete settings, signals, limits and error behavior.

- [BakerContext](../../api/bake/BakerContext.md)
- [IntegratedBakerContext](../../api/bake/IntegratedBakerContext.md)
- [StandaloneBakerContext](../../api/bake/StandaloneBakerContext.md)

For camera-less jobs use one live target/context, completed slots and a global
submission serial. Settings/geometry changes invalidate previous generations.
Cancelled or incomplete work must not publish successful results. These
validations apply to the scene supplied to the context, not automatically to an
editor's document revision. [Composer](../../apps/draw_composer.md#integrated-scene-baking)
supplies an independent initial-scene snapshot: document edits warn without
changing that context's sources or cancelling the bake. The generic engine
contract still requires invalidation when the context's own scene is edited.
CPU pixel format, GPU precision and display conversion are separate contracts.
