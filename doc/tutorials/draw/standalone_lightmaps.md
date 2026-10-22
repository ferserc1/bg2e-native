# Bake a model or prefab without a camera

The production `bg2e::bake` standalone path runs completed UV AO/GI samples on
an initialized windowless draw Engine. Both standalone and integrated jobs use these public contracts.
This tutorial uses delivered APIs; it creates no viewport renderer or camera
history. For a ready executable see [lightmap_generator](../../apps/draw_lightmap_generator.md).

## Choose the owner

Use `<bg2e/bake/all.hpp>` for explicit bake includes; the root umbrella also
exports draw and bake. A caller
owns an initialized [offscreen draw Engine](../../api/draw/Engine.md#headless-initialization)
and keeps it/Device alive through all native owners and cleanup. Stop frame,
background/resource and scene producers. The CLI uses a typed
[OffscreenApplication delegate](../../api/app/OffscreenApplication.md), a depthless
1×1 context surface and its finish hook; it records no camera frames. UV surfaces
use the job resolution, independently of context surface extent.

## Prepare and run a protected batch

The following is a function body/example for a caller with a live stopped
windowless Engine. Context/input/output paths are supplied by that caller.
No original file is modified; generated UV2 needs output model copies.

```cpp
#include <bg2e/bake/all.hpp>
#include <bg2e/draw/Engine.hpp>
#include <filesystem>
#include <stdexcept>

void bakeModel(bg2e::draw::Engine& engine,
    const std::filesystem::path& contextFile,
    const std::filesystem::path& modelFile,
    const std::filesystem::path& outputDirectory)
{
    bg2e::bake::StandaloneBakeBatch::Options options;
    options.settings.setResolution(512);
    options.settings.setMode(bg2e::bake::LightmapMode::RTGI);
    options.settings.setAccumulationFrames(16);
    options.settings.setSamplesPerPixel(8);
    options.settings.setCpuFormat(bg2e::bake::LightmapPixelFormat::RGB32F);
    options.settings.validate();
    options.writeModelCopies = true;
    options.overwriteOutputs = false;

    bg2e::bake::StandaloneBakeSceneAssembler assembler(engine);
    auto assembly = assembler.assembleModel(contextFile, modelFile,
        bg2e::geo::Uv2AtlasOptions{options.settings.resolution(), 4}, false);
    auto result = bg2e::bake::StandaloneBakeBatch::run(engine, assembly,
        outputDirectory, options,
        [](const bg2e::bake::StandaloneBakeBatch::Progress& /*progress*/)
        {
            // CPU-only observation/cancellation; do not submit GPU work here.
            return true; // Replace with a read-only job-validity check to cancel.
        });
    if (result.cancelled)
    {
        throw std::runtime_error("Batch cancelled; no new output was published");
    }
    // Consume result.manifestPath and its relative artifact paths.
    // All Assembly/native owners leave scope before the caller cleans Engine.
}
```

Use assemblePrefab with the same atlas/loading options for a complete single-
node or multi-root prefab. Context is scene JSON with geometry/lights/environment;
its geometry contributes but is never selected as an output target. Retain
Assembly.scene alongside targetRoot. Existing UV2 uses std::nullopt instead of
atlas options; writeModelCopies may independently be true or false. Invalid
required targets fail; overlaps follow Warn rather than automatic skipping.

Deferred `false` loading stages UV2 before GPU realization. Batch loads prepared
targets when needed and avoids repeating hooks on an already loaded assembly.
CPU/uploaded geometry and Engine identities must agree. Retained custom components
need complete dependency declarations through options.resolveDependencies; no
resolver is inferred from custom code. Add nonstandard input files to
options.protectedInputs. The shipped CLI has no custom component registration.

## Direct samples without file export

For an already loaded current root/target, the lower-level sequence is:

```cpp
bg2e::bake::StandaloneBakerContext context(engine);
context.initialize();
context.prepareScene(root); // Retained loaded root; scene owners also stay alive.
bg2e::bake::LightmapBakerConfiguration job;
job.target = target;
job.settings->setAccumulationFrames(2);
auto baker = context.createBaker(job);
while (baker->progress().state != bg2e::bake::BakeState::Complete)
{
    baker->update(); // One synchronous completed UV sample, not a camera frame.
}
auto pixels = baker->readPixels(); // Owned CPU value; RGB8 by default.
baker->cleanup();
baker.reset(); // Required before another createBaker.
context.cleanup();
```

`engine`, `root` and `target` here are caller-provided, live and already prepared;
this fragment is not a standalone main. Frozen settings, native support and
strict numeric/count validation govern completion. Changes to source geometry,
transforms/material/environment retire results; prepare a new scene/baker.
Reset accumulation only within the same current scene. Keep returned GPU result
owners inside Engine lifetime; CPU pixels can survive cleanup.

## Inspect actual output

Successful batch commit publishes `output/lightmaps.json` pointing to one complete
unique generation. Read its paths; do not assume flat filenames or infer success
from a target progress callback. RGB32F is exact native RGBA32F `.bg2tex`, not a
general interchange HDR image. The draw resource loader reads this native archive. RGB8 GI applies exposure once
before clamp/round; AO and RGB32F ignore exposure. No gamma or receiver albedo is
applied again. Raw GI precision is validated separately; a file encoder cannot
recover invalid/saturated raw input.

Model copies retain generated atlas and actual linear UV2 baked AO/GI resources,
including native sidecars, but omit node placement/descendants. Reattach them at
the intended scene placement; they are not full prefab exports. Sources and
previous complete generations remain protected. An explicit overwrite only
switches the manifest; old generation garbage collection is manual.

Use the [independent artifact inspector](../../../tests/bake/product/README.md)
for PNG/native pixel and material-section checks, and native draw model reload
for geometry/scene validation. Choose asymmetric known fixture pixels for
orientation and actual chart-mask edge values for dilation; arbitrary lighting
values are not a universal golden image. Synthetic inspector tests are not GPU
or package evidence. Build/native checks remain lead-owned.

Sources/call sites: [CLI](../../../apps/lightmap_generator/src/main.cpp),
[batch](../../../lib/src/bg2e/bake/StandaloneBakeBatch.cpp),
[context](../../../lib/src/bg2e/bake/StandaloneBakerContext.cpp),
[output contract](../../api/bake/LightmapOutputWriter.md).
