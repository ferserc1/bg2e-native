# M08-04 — Draw lightmap generator CLI handoff

[Owning step](step_04_cli-application.md) · [Progress](../progress.md) ·
[User guide](../../../../apps/draw/lightmap_generator/README.md) ·
[Batch contract](batch-output.md)

## Implemented product

`apps/draw/lightmap_generator` provides the new `draw_lightmap_generator` target
and private CommandLine parser/main. The original production app and its parser,
Engine, writer and target are unchanged. This step's explicit CMake scope is the
new target CMakeLists plus its apps/add_subdirectory registration. Shared build,
shader and packaging implementations are reused without changes.

The parser inventories the old model/prefab commands and context/input/output,
format, resolution, frames, mode, generate-uv2, overwrite, samples-per-pixel,
gi-bounces and max-distance controls. Useful defaults are preserved, including
true generation/overwrite and model copies following UV generation. Inputs now
include OBJ/VWGLB as supported by the assembler. New controls cover backend,
strict settings configuration, unknown-settings policy, CPU format, exposure,
AO radius, ray bias, dilation, UV2 padding, explicit model copies, deployed shader
root and GPU validation. All values support separate or equals syntax. Duplicate,
unknown, missing, malformed/nonfinite/out-of-range and incompatible explicit
mode/format options fail before GPU initialization. Help is a standalone action.

Config import uses the existing draw SettingsCodec on a catalog bound to the
**actual** batch LightmapSettings object: renderer bg2e.bake.standalone.v1,
component bake, type bg2e.bake.lightmapSettings.v1. The example file uses the
codec's typed decimal strings. Explicit CLI overrides are applied after import,
regardless of option order; final coupled settings then validate. No Preferences
values are read. Unsupported editor/camera/effect properties reject by default
or receive syntax-validated skip warnings; schema/version/renderer/known-type
mismatches always reject. Native display effect settings without public bake
mappings are not silently imported. Config paths and explicit shader roots become
protected inputs; no custom component resolver is registered by this executable.

## Headless lifecycle and results

The DrawOffscreenApplicationDelegate initializes a 1×1 depthless context surface,
uses the explicit/default Engine backend and requests no camera frames.
OffscreenApplication's finish boundary runs the standalone job; its own render
callback is deliberately unreachable. The assembly receives optional UV2 options
using final resolution/padding, then StandaloneBakeBatch handles deferred resource
loading, full preflight and serial completed samples. Actual UV surfaces use the
bake resolution, not the context surface extent. No RendererDeferred, render API
baker or camera-frame accumulator is instantiated.

All Assembly/scene/baker/native owners are local to finish and released before
OffscreenApplication drains/cleans the Engine. The delegate retains only CPU
options/result paths. SIGINT/SIGTERM handlers store a sig_atomic_t request;
cooperative callbacks/current checks cancel without new publication. No signal
handler performs IO, GPU work or cleanup. An interrupt after commit does not undo
a completed generation. Signal handlers restore their predecessors on exit.
Progress prints one line per target without explicit per-sample flushes or waits.

Success prints target count and the final lightmaps.json path after successful
native cleanup. Exit 0 is help/complete success; 2 is argument/config/path failure;
1 covers initialization, assembly, unsupported bake capability, job/output/cleanup
failure; 130/143 report cooperative SIGINT/SIGTERM cancellation. Invalid required
targets fail the batch rather than being counted as skipped successes. Cleanup
failure can return 1 after a complete manifest has already been published.

## Deployment and preservation

The new bundle_app target automatically reuses paired draw/bake shader compilation
and shader-only resource refresh. Distribution target collection traverses apps
subdirectories, so the new target participates without editing package scripts.
RuntimeShaders installation remains unchanged. macOS has a distinct bundle name
and identifier, preserving coexistence with the original CLI. Lead checks must
inspect the active packaged/relocated shader root, especially bake UV surface
Metal/SPIR-V assets; source registration is not native deployment evidence.

Generated atlases require model copies; existing UV2 can request copies separately.
The output layout and publication guarantees are exactly M08-03's final-manifest
contract, including exact HDR native float maps, immutable prior generations,
complete independent model sidecars, protected inputs, cooperative locks and
explicit crash/durability limits. No source materials/native algorithm/settings
default/shader is changed. No parallel target jobs or extra profiling waits are
introduced.

## Verification and successor

Static checks cover source includes/GPL/control-flow/lexical structure, parser
coverage of old useful options, example typed schema, settings-before-GPU and
CLI-last ordering, offscreen no-frame lifecycle, resource/package source rules,
local links and whitespace. No configure/build/shader compilation/app/probe/GPU
execution was performed. [CPU probe and native matrix](../../../../tests/bake/cli/README.md)
are supplied for lead validation. M08-03 is accepted by continuation; native
backend/scenario coverage is unspecified. M08-04 awaits lead validation; M08-05
owns the full product validation matrix next.
