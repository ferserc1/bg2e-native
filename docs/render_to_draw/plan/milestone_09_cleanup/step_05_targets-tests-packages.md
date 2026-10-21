# M09-05 — Retire legacy shaders/examples and repair tests/distribution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01–M09-04.

## Scope and deliverable

- `CMakeLists.txt`
- `lib/CMakeLists.txt`
- `apps/CMakeLists.txt`
- `examples/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/test_02_uv_surface/src/main.cpp`
- `tests/test_03_integrated_baker_context/src/main.cpp`
- `cmake/`
- `scripts/`
- `shaders/src/glsl/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [CMakeLists.txt](../../../../CMakeLists.txt), [lib/CMakeLists.txt](../../../../lib/CMakeLists.txt), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt), [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt), [tests/CMakeLists.txt](../../../../tests/CMakeLists.txt), [tests/test_02_uv_surface/src/main.cpp](../../../../tests/test_02_uv_surface/src/main.cpp), [tests/test_03_integrated_baker_context/src/main.cpp](../../../../tests/test_03_integrated_baker_context/src/main.cpp), [cmake/](../../../../cmake), [scripts/](../../../../scripts), [shaders/src/glsl/](../../../../shaders/src/glsl).

## Implementation work

Include [M07's actual app/probe inventory](../milestone_07_bg2e_composer/review.md):
tests/draw/composer_io, composer_import, composer_baking and tests/ui/selectable_list.
Retain/migrate their final API/source references and native draw/bake resource
deployment with the canonical Composer target. Audit legacy/draw server identity
and preferences when retiring targets; do not infer installed launch from recursive
package target collection alone.

1. Remove old application/example/render shader targets that require the deleted API, or migrate a specifically useful demonstration/check to draw/bake. No old application source support remains.
2. Adapt existing UV-surface/integrated-context/atlas executable checks and any supported tests to final public APIs; do not leave failing unbuilt targets to hide dependencies.
3. Update engine shader library install/bundle paths, optional FSR/native backend assets, packaging scripts and standalone-project templates to new headers/default entry points.
4. Delete legacy-only shaders/helpers after checking shared use; retain/relocate shared GLSL/Metal libraries into paired pipeline layout without output collisions. Preserve Vulkan SDK/Metal deployment still needed by GPU.

## Verification and exit conditions

- Configure/build/install/package checks, when authorized, contain no missing/dead render targets, headers or resources.
- A generated standalone project and installed new products load final shader libraries; adapted tests/examples do not include legacy APIs.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 build/tutorial/distribution pages must match final target/source/runtime paths, including retained GPU backend libraries.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-16 reviewed baseline

Preserve the real draw_deferred_renderer target and later accepted products; disposition each manual M01 probe explicitly rather than claiming an automatic test suite. Keep paired shaders/src/draw pipeline paths, gpu::vk::FidelityFXBridge optional SDK linkage and recursive standalone copies. Remove legacy-only shaders/targets only after the per-file manifest confirms no surviving consumer. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M03-10 reviewed implementation requirement

Retain native paired draw tracing/effect/SMAA shaders and official lookup dependency in installed products. Reconcile manual fixture headers with surviving supported harnesses; do not delete useful no-RT/scalar/partial-profile coverage with legacy targets. Verify example-specific GI/reflection defaults and explicit settings-window registration after canonical entrypoint changes.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.

M06 registers `draw_model_edit` through `bundle_app_sdl`; package collection is
recursive and includes registered executable targets, runtime dependencies,
assets and paired shaders. M06 inspected this source path but did not execute a
native package. M09 must re-check final target/resource names and run authorized
native install/package/standalone checks after obsolete targets are removed. See
the [M06 product validation](../milestone_06_model_edit/product-validation.md).


## M05 headless export target and regression coverage

Inventory `draw_offscreen_application`, its paired draw shader/assets packaging,
and `tests/draw/offscreen` separately from the legacy production offscreen
examples. M05-05 validates one PNG through exact decode round-trip and no-clobber
publication, but the lead's independent asymmetric-image/backend inspection is
not recorded yet. M09-05 must migrate the target and useful fixture to the final
canonical API, retain production regressions through M08, then remove obsolete
production offscreen binaries/resources only when the accepted retirement manifest
and parity evidence permit it. Do not claim that one camera PNG verifies GPU-only
texture or bake batch export.

## M04-07 reviewed implementation requirement

Preserve both engine native shader trees and their aggregate dependency, unique per-tree targets/Metal intermediates, incremental bundle copies and RuntimeShaders install paths. Exercise draw_lightmaps after legacy shader deletion. M04 previously omitted bake from compilation/copy; the accepted correction must survive cleanup and standalone/package regeneration.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
## Tracing specialization/template retention

The [M07 capacity correction](../milestone_07_bg2e_composer/tracing-capacity-16384.md)
uses deployed `_256` effect files as current v3 templates for exact counts up to 16384.
Do not delete them as obsolete small-capacity shaders or require per-capacity
filenames. Preserve Vulkan specialization ID0, Metal record arrays/residency and
the separate primitive probe's small templates; validate rebuilt native deployment.

## M07-08 source reconciliation

At HEAD `78943d5820e69363c942d71136f479a0df87128e`, current source supersedes the
historical v2 table handoff: AO/GI/reflection use native v3 with Vulkan scene set1,
persistent material arrays set2 (bindings5/6), specialization ID0 and unchanged
Metal record buffers8/9. Direct query shadows use `draw.query.softShadows.v5`,
Vulkan invocation set0, scene set1 and persistent alpha set2 (bindings16/24),
Metal records5/6 and reserves (2,1,10). AO/GI/reflection reserves are respectively
(4,2,13), (5,3,15), (6,3,16). Exact device-clamped capacity up to 16384, neutral
pair overflow and object-local ownership remain. Consume current maintained
[tracing reference](../../../../doc/api/draw/TracingSceneResources.md) and source,
not historical ABI labels. This is static reconciliation, not a new native test.

## Current engine preservation requirements — 2026-10-21

Keep current tracing v3/query-shadow v5 paired native shader libraries and `_256`
templates; source constant/layout capacity specializes exact counts. Preserve the
focused optimization fixture families and docs/profiling_and_stats probes as
keep/adapt candidates through canonical target changes. Preserve code required by
Performance, CPU/GPU transport, recording and native optional timing. Do not prune
private engine optimization sources because they were absent from the initial
retirement inventory.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 concrete registration/template/probe inventory

Use the accepted per-file manifest to reconcile both app groups, legacy examples
01..18/debug-app, surviving draw/gpu examples and four currently registered test
subdirectories. Inspect test_01_key_and_mouse/test_04_uv2_atlas individually;
CPU/input checks are not obsolete graphics code. Port useful UV/integrated checks
and remove production-only negative branches in tests/draw/composer_io/schema.cpp
without losing schema/clone/resource behavior.

Retain tests/bake/{standalone,assembly,batch,cli,product}; update native probe
accessors and CLI include/target paths to the final API. Preserve the independent
Python inspector's encoded data semantics and scope limits. Replace the actual
production default in scripts/templates/main.cpp; check create/update-library
scripts and copied include/shader trees, not only package executable collection.
Keep the neutral blue-noise generator while fixing its old output-path example.
Shared GLSL includes, paired draw/bake refresh, RuntimeShaders, optional SDK assets
and native loader formats must remain valid after legacy stages are removed.

## M09-01 concrete test/build/template dispositions

[The manifest](retirement-manifest.md) assigns each actual source/build/resource
file. Keep test_04_uv2_atlas and its CPU assertions/target. Port test_01 input
assertions and test_02/test_03 UV/RGB material/sample/reset/readback assertions
before retiring old native harnesses; existing replacement filenames alone are
not coverage proof. Retire legacy debug/01–18 examples and old app targets; keep
all delivered examples/draw, examples/gpu and apps/draw products with current
draw_* target names. Update real standalone templates/main.cpp and registrations,
package/install/runtime paths and blue-noise generator usage. Paired shader include
roots are independent: draw/gbuffer/probe.frag.glsl resolves draw/lib/pbr.glsl,
not the identically named retired GLSL helper. Preserve paired stages/runtime
resources, SDK/Metal/Vulkan deployment and original SMAA data/licenses.

## M09-02 implemented prerequisite

[Entrypoint delivery](entrypoints.md) and [actual hash delta](entrypoint-changes.tsv)
record changes to the M09-01 snapshot. Old apps, RenderGraphicsExecution,
OffscreenExecution and production DefaultOffscreenApplicationDelegate pair/export
are already removed; do not recreate them or assume old manifest paths still exist.
Application::renderDelegate now returns draw, and MainLoop/offscreen use draw only.
Old example registrations are retired but their sources remain M09-05 work.
The key/mouse display is already ported and registered; retain its neutral input
behavior. Shared scene/DB/UI/accessors/native bridges, templates and full distribution
validation remain owned by their pending steps. Preserve current source and evidence.

## M09-03 actual prerequisite and delta

Use [the shared API delivery](shared-api.md) and [per-file changes](shared-api-changes.tsv)
alongside the historical manifest and M09-02 delta. Shared render Engine overloads,
scene native resources/bindings/visitors, DB specialized writer and obsolete widgets
are already retired. 153 dependent production renderer headers/sources are also
removed to close incoming dependencies; 23 render files remain, including native
wrappers and authoritative blue-noise data. Do not restore old interfaces to make
remaining source compile.

DrawableComponent::drawable(), SelectionItem::mesh/lockMesh()/meshPtr(),
SelectionManager::selectedMesh() and GizmoComponent::transformDrawable() now address
draw types only. Factory registration accepts name + defaultCreator. GPUSelectionDialog
accepts caller-provided gpu properties; its standalone OpenGL setup remains a native
confinement seam. Generic UI/preview cleanup is draw-only; retain native Vulkan/Metal
work pending proper GPU bridge relocation, without higher-level GPU dependencies.

Registered UV/integrated test targets now call existing public bake probes through
retirement_harness.hpp; old native main bodies are replaced without CMake changes.
M09-05 audits original fixture-specific numerical/material coverage and removes
the stale test_02 private render include path. M09-06 owns actual execution and
full no-RT/optional/deployment/performance matrices. Shared documentation already
updates 27 maintained pages; preserve current accessors and standalone IO coverage.

## M09-04 implemented native/data prerequisite

[Native retirement](native-retirement.md) and [source delta](native-changes.tsv)
record deletion of the final 23 render files. There is no render implementation
to restore for an obsolete target. Keep gpu::GuiRenderer (Vulkan/Metal),
gpu::StandaloneGuiWindow and gpu::drawableSize and their new globbed GPU sources.
UI files are thin primitive-only adapters; the standalone chooser's OpenGL
backend remains required. Preserve vendor renderer linkage, native SDK/MoltenVK
assets, and byte-identical utils/blue_noise_data.h plus SMAA tables/licenses.
Generator usage now names utils; do not reintroduce the old output path.
The inventory supports --output-dir and all three delta ledgers; preserve historical
TSVs and include untracked new files when refreshing evidence. Full obsolete
example/template/shader/package/test cleanup still belongs to this step.
