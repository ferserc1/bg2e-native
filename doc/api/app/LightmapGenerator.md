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
