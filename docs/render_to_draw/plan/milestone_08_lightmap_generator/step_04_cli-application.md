# M08-04 — Build apps/draw/lightmap_generator and configuration flow

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01–M08-03.

## Scope and deliverable

- `apps/lightmap_generator/src/CommandLine.hpp`
- `apps/lightmap_generator/src/CommandLine.cpp`
- `apps/lightmap_generator/src/main.cpp`
- `apps/draw/lightmap_generator/`
- `apps/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/lightmap_generator/src/CommandLine.hpp](../../../../apps/lightmap_generator/src/CommandLine.hpp), [apps/lightmap_generator/src/CommandLine.cpp](../../../../apps/lightmap_generator/src/CommandLine.cpp), [apps/lightmap_generator/src/main.cpp](../../../../apps/lightmap_generator/src/main.cpp), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt).

## Implementation work

1. Inventory current CLI arguments/configuration/jobs/output codes and create the new target with equivalent useful behavior over draw/bake.
2. Use experimental OffscreenApplication/context initialization and standalone scheduler; do not instantiate a camera renderer just to access effect/material/environment resources.
3. Support explicit backend and configuration-file import for active bake/effect options using the settings codec where compatible. Render-only unavailable properties follow declared diagnostics; bake job settings own their values, not the stateless preferences facade.
4. Preserve output/preflight/overwrite/UV2 controls and packaged assets/shaders. Keep the original CLI working until cleanup and expose deterministic error/exit behavior.

## Verification and exit conditions

- Representative model/prefab/context batches run headlessly and produce expected maps/model copies at requested sample settings.
- Invalid args/config, unavailable RT, output errors and canceled/failed jobs have clear non-success status and clean shutdown.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-05 validates full CLI parity. M09 determines final installed executable naming from surviving targets without restoring old API support.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M03-10 reviewed implementation requirement

Import stable live effect settings through the codec at a stopped boundary with explicit required-capability checks. UI-only lightPose/document targets need a declared mapping or reject/skip diagnostic policy. Keep CLI dimensions/sample overrides last; no implicit preferences or RendererDeferred dependency for UV tracing.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Deploy both paired shader trees, draw and bake, through the engine shader aggregate and RuntimeShaders installation. Check bake/uv_surface/surface.vert.metallib (Metal) and its SPIR-V peer in installed/package resources; test an incremental shader-only rebuild as well as a fresh build. Keep required bake support distinct from no-RT raster fallback; the interactive example's exception UI is not CLI exit/error policy.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## Current engine preservation requirements — 2026-10-21

R11–R13: preserve existing source instrumentation and the bounded asynchronous
CPU/GPU telemetry services; headless jobs need no Performance window. Do not add
rendering-thread timing waits or file flushes. Check bool-noexcept DB export
results and propagate deterministic output failure. Settings imports remain
conservative; the interactive typed final-color optimization does not establish
headless bake resource safety. Preserve current native v3 effect deployment.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-01 delivered execution contract

[Standalone context](standalone-context.md) now borrows an initialized headless draw
Engine, with explicit initialize/prepareScene and one private serial slot. Reuse
that scheduler; do not initialize a second Engine or borrow a viewport renderer.
prepareScene takes a loaded retained root and optional EnvironmentResources;
otherwise it generates the selected authored/default environment through public
tools. The caller performs graph updates and UV2 realization first. Release the
previous baker before createBaker; stale scene jobs cannot reset into a new scene.
Each update completes exactly one synchronous sample; readPixels is available only
for current completed validated output. Context cleanup precedes Engine cleanup;
returned GPU outputs/targets must also be released inside the Engine lifetime.
No standalone assembly, batch writer or CLI is supplied by M08-01.

## M08-02 delivered assembly handoff

[Actual assembly](scene-assembly.md) supplies bake::StandaloneBakeSceneAssembler,
Assembly.scene/targetRoot, normalized context/input paths, enabled target nodes,
resolved inputSourcePath, stable sanitized hierarchy/sibling outputIdentity and
UV2 validation/generated-atlas metadata. Preserve context sources without selecting
them as targets. Use complete multi-root containers and camera-less prefab inputs.
No SceneAssembler implementation alias exists; use the actual delivered class.

Optional false target loading returns CPU geometry/materials and defers target
resource hooks; context sources are already loaded. Realize the target tree and
perform any required component updates before freezing prepareScene. Retain the
Assembly Scene alongside its root while using scene caches/selection. Do not apply
maps to the source scene between targets without source-generation invalidation.

Assembly.sourcePaths contains canonical known standard input dependencies, not a
whole-batch protection proof. Supplement original OBJ MTL and custom external
resource discovery/protection, or reject unverifiable output collisions before
writing. Lexical file paths remain authoritative for relative resource resolution;
canonical paths are protected-source identities. Complete output preflight and
publication remain M08-03; assembly writes no product output.

## M08-03 delivered batch/output handoff

Consume [StandaloneBakeBatch and LightmapOutputWriter](batch-output.md), exposed
by bake/all.hpp. Translate UV2 generation options into assembler options, then
pass the retained Assembly to Batch::run. Do not repeat resource hooks on loaded
targets, apply maps to source materials or rebuild a separate output transaction.
Use Result.cancelled/bakedCount and exceptions for deterministic CLI status.

The output entry is lightmaps.json, atomically published last and pointing to a
complete unique generation; report its path rather than assuming flat per-target
filenames. Overwrite permits replacing that manifest while retaining old complete
generations. RGB32F is exact native RGBA32F .bg2tex (draw loader required), not an
8-bit image or production-loader HDR interchange format. Explicit JPEG is lossy.
Custom components need complete file dependency declarations; undeclared inputs
reject before baking. Preserve the documented cooperative lock/crash-recovery
and no-power-loss-durability boundaries in CLI diagnostics/documentation.

## M08-04 delivered implementation

[CLI handoff](cli-application.md) records apps/draw/lightmap_generator and the
new draw_lightmap_generator target. The original CLI is preserved. Its actual
configuration ID/type, CLI-last precedence, supported effect mappings/diagnostics,
1x1 no-camera offscreen context, signal/exit policy, manifest output and inherited
paired shader/package rules supersede assumptions about flat output files or
camera-frame scheduling. See the app README and tests/bake/cli for lead checks.
No compilation/runtime/deployment evidence is claimed.
