# StandaloneBakeBatch

**Header:** `<bg2e/render/StandaloneBakeBatch.hpp>`
**Namespace:** `bg2e::render`

Static synchronous coordinator. `run()` validates/preflights an assembly,
optionally generates UV2, loads all targets, prepares one scene TLAS and bakes
eligible targets sequentially.

## Types

| Type/field | Default | Contract |
|---|---|---|
| `Options::lightmapSettings` | `{}` | [LightmapSettings](LightmapSettings.md). |
| `Options::generateUv2` | `false` | Generate atlases and write `.bg2` copies; requires CPU-only targets. |
| `Options::uv2PaddingPixels` | `4` | xatlas chart spacing; resolution comes from lightmap settings. |
| `Options::overwriteOutputs` | `false` | Permit replacement of existing outputs; protected inputs remain excluded. |
| `Progress::targetIndex` | `0` | Zero-based index among eligible targets. |
| `Progress::targetCount` | `0` | Eligible targets, after atlas generation failures are removed. |
| `Progress::completedFrames` | `0` | Current target's finished samples. |
| `Progress::accumulationFrames` | `0` | Requested sample count for each target. |
| `SkippedTarget::identity`, `reason` | Empty | Target identity and validation/generation diagnostic. |
| `Result::bakedCount`, `skippedCount` | `0` | Successfully written and skipped targets. |
| `Result::cancelled` | `false` | Progress callback requested cancellation. |
| `Result::skippedTargets` | Empty | Individual skipped-target reports. |

`ProgressCallback = std::function<bool(const Progress&)>` returns true to
continue, false to cancel. `WarningCallback = std::function<void(const SkippedTarget&)>`
is called once for each skipped target.

```cpp
static Result run(Engine* engine,
    const StandaloneBakeSceneAssembler::Assembly& assembly,
    const std::filesystem::path& outputDirectory,
    db::ImageFormat imageFormat, const Options& options,
    ProgressCallback onProgress = {}, WarningCallback onWarning = {});
```

The engine must support ray tracing and the assembly must contain a root with
attached standard target Drawables. Invalid arguments/options, output preflight
failures, GPU loading, scene preparation and write failures throw. Without
generation, invalid UV2 is skipped. With generation, each atlas-generation or
post-generation validation failure is also skipped with a warning; other targets
continue. If none are eligible, return without GPU scene preparation or writes.

All target Drawables, including skipped ones, are loaded as potential occluders
when there are eligible targets. One `updateScene(0.0f)` serves the batch.
Progress is called before each target and after each synchronous sample. A false
return stops before the next sample; if the just-completed sample finished the
target, that target is still written. Earlier completed outputs remain.
No partially accumulated target is exported.

```cpp
bg2e::render::StandaloneBakeBatch::Options options;
options.generateUv2 = true;
auto assembly = assembler.assembleModel("studio.json", "chair.bg2", std::nullopt, false);
auto result = bg2e::render::StandaloneBakeBatch::run(
    engine, assembly, "out", bg2e::db::ImageFormat::PNG, options,
    [](const auto& progress) { return true; });
```

Library defaults differ from the CLI, which enables generation and overwrite.
Sources: [header](../../../lib/include/bg2e/render/StandaloneBakeBatch.hpp),
[implementation](../../../lib/src/bg2e/render/StandaloneBakeBatch.cpp).
See [batch architecture](../../architecture/lightmaps/standalone_scene_and_batch.md).
