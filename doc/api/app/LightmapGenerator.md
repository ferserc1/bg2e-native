# Standalone lightmap generator

`lightmap_generator` is an application executable, not a `bg2e::app` class.
Its complete command, option, output and exit-status reference is maintained in
[the application guide](../../apps/lightmap_generator/index.md).

Reusable engine APIs:

- [StandaloneBakeSceneAssembler](../render/StandaloneBakeSceneAssembler.md): load context/model/prefab inputs.
- [StandaloneBakeBatch](../render/StandaloneBakeBatch.md): preflight and execute a cancellable batch.
- [StandaloneBakerContext](../render/StandaloneBakerContext.md): direct synchronous scene/sample lifecycle.
- [LightmapSettings](../render/LightmapSettings.md): modes, defaults, precision and export settings.
- [LightmapOutputWriter](../db/LightmapOutputWriter.md): planned image/model writes.

The CLI enables UV2 generation and output overwrite by default, unlike the batch
API. Source: [CommandLine.cpp](../../../apps/lightmap_generator/src/CommandLine.cpp).

## Separate draw/bake executable

The delivered [draw_lightmap_generator](../../apps/draw_lightmap_generator.md)
uses public bake context/assembler/batch/writer APIs and the typed draw offscreen
lifecycle. It supports native RGB32F, strict required-target failures and final-
manifest whole-batch publication. These are explicit differences from the
production tool above. See [C++ tutorial](../../tutorials/draw/standalone_lightmaps.md)
and [standalone API](../bake/StandaloneBakerContext.md); no production classes are
required to obtain effect resources, though shared headers still contain
coexistence branches. Both executables remain registered separately.
