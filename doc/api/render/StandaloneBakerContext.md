# Standalone Lightmap Baking

**Headers:** `<bg2e/render/StandaloneBakerContext.hpp>`,
`<bg2e/render/StandaloneBakeSceneAssembler.hpp>`, and
`<bg2e/render/StandaloneBakeBatch.hpp>`

**Namespace:** `bg2e::render`

Standalone baking uses a headless `render::Engine` and does not require a window,
camera, `MainLoop`, or active renderer. The caller owns the engine and scene;
both must outlive the assembler, batch, context, and bakers.

## Batch workflow

`StandaloneBakeSceneAssembler` loads the context JSON and attaches either one
`.bg2` model at the world origin or a complete prefab subtree. The batch API
accepts CPU-only target loading so UV2 can be generated before targets receive
GPU meshes and BLASes. Use this overload for both UV2 branches:

```cpp
render::Engine engine;
engine.init();

{
    render::StandaloneBakeSceneAssembler assembler(&engine);
    auto assembly = assembler.assemblePrefab(
        "studio.json", "sofa.json", std::nullopt, false);

    render::StandaloneBakeBatch::Options options;
    options.lightmapSettings.resolution = 512;
    options.lightmapSettings.accumulationFrames = 16;
    options.lightmapSettings.mode = render::LightmapMode::RTGI;
    options.generateUv2 = false;

    auto result = render::StandaloneBakeBatch::run(
        &engine, assembly, "out", db::ImageFormat::PNG, options);
    // Process result.skippedTargets and the baked/skipped/cancelled summary.
}

engine.cleanup();
```

The batch validates existing UV2 (or selects targets for generation), then
preflights all outputs. When generation is enabled, it creates and validates
the UV2 atlases before loading every target Drawable for the shared scene. It
then initializes one `StandaloneBakerContext` and calls `updateScene()` once.
That TLAS contains context geometry and all
model/prefab target geometry. Each eligible target gets its own baker; the
target loop does not rebuild the TLAS. Targets skipped for invalid UV2 when
generation is disabled remain in the scene as possible occluders.

`Result` contains `bakedCount`, `skippedCount`, `cancelled`, and the skipped
target identities and validation messages. The optional warning callback is
invoked once per skipped target. The progress callback receives a zero-based
eligible `targetIndex`, `targetCount`, `completedFrames`, and
`accumulationFrames`. Return `true` to continue or `false` to cancel before the
next sample. Only targets that finish all configured frames are written.

`StandaloneBakeBatch::Options::lightmapSettings` uses the defaults documented in
[Integrated Lightmap Baker](LightmapBaker.md#settings-and-output), including
RTGI, 512 resolution, 16 accumulation frames, and 8 samples per pixel. Set
`generateUv2` to write model copies as well as images; otherwise only images are
written and invalid UV2 targets are skipped.

## Manual context lifecycle

The context can also be used directly when the caller needs to control each
target baker. Assemble and GPU-load every scene Drawable before `updateScene()`.
Each explicit scene update runs standalone scene/component lifecycle and
rebuilds the context-owned TLAS; it resets live bakers' accumulation histories.

```cpp
render::Engine engine;
engine.init();
{
    render::StandaloneBakeSceneAssembler assembler(&engine);
    auto assembly = assembler.assemblePrefab("studio.json", "sofa.json");
    // Assume the prefab has at least two target Drawables.
    auto nodeA = assembly.targets.at(0).node;
    auto nodeB = assembly.targets.at(1).node;

    render::LightmapSettings settings;
    settings.resolution = 512;
    auto context = std::make_shared<render::StandaloneBakerContext>(&engine, assembly.scene);
    context->initialize({512, 512});
    context->updateScene(0.0f);

    auto bakerA = context->createBaker(nodeA, settings);
    auto bakerB = context->createBaker(nodeB, settings);
    for (uint32_t frame = 0; frame < settings.accumulationFrames; ++frame) {
        bakerA->update(); // one synchronous submit and wait
        bakerB->update();
    }
    auto pixelsA = bakerA->readPixels();

    bakerA.reset();
    bakerB.reset();
    context->cleanup();
}
engine.cleanup();
```

Each successful standalone sample advances `Engine::currentFrame()`, allowing
the shared `LightmapBaker::readPixels()` submission guard to verify completion.
`update()` never drives scene/component lifecycle or rebuilds the TLAS. Use
`updateScene(deltaSeconds)` when scene inputs change, then start a new
accumulation sequence. `StandaloneBakerContext::cleanup()` waits for active GPU
work; the engine must remain alive until context/baker cleanup has completed.

## UV2 and output branches

For UV2 generation, use CPU-only assembly:

```cpp
auto assembly = assembler.assembleModel(
    "studio.json", "chair.bg2", std::nullopt, false);
options.generateUv2 = true;
```

The batch applies `geo::GenerateUv2AtlasModifier` to each complete target mesh,
validates the generated atlas, and only then loads target GPU resources and
builds the one TLAS. It writes an image and a new `.bg2` copy per target. The
copy assigns that target's generated image to the existing AO texture property
of all submesh materials, with UV set 1 and unit scale. It does not save a new
prefab or context JSON and does not overwrite source files.

When `generateUv2` is false, the batch uses the public
`geo::UvAtlasValidator`; it never infers UV2 presence from `texCoord1` values.
Invalid targets produce one warning and no output file. The output writer
preflights filenames, input aliases and existing files before the first bake.
See [`db::LightmapOutputWriter`](../db/LightmapOutputWriter.md) for naming,
transaction, and image conversion details.

The standalone context builds IBL resources from the scene environment. If the
scene has no environment component, it uses the engine's procedural sky-dome
environment. RTAO uses a fixed 0.1 m radius; RTGI uses the configured bounce
count and maximum ray distance.

## See also

- [Standalone lightmap generator CLI](../app/LightmapGenerator.md)
- [`db::ImageFormat`](../db/ImageFormat.md)
- [`geo::UvAtlasValidator`](../geo/UvAtlasValidator.md)
- [`geo::GenerateUv2AtlasModifier`](../geo/GenerateUv2AtlasModifier.md)
- [Lightmap baking and scene assembly](../../lightmap_baking.md)
