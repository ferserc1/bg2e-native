# M09-06 — Draw production validation and zero legacy dependency checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-02–M09-05.

## Scope and deliverable

- `examples/draw/`
- `apps/draw/`
- `tests/`
- `lib/include/bg2e/all.hpp`
- `lib/include/bg2e.hpp`
- `scripts/PACKAGING.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/draw/](../../../../examples/draw), [tests/](../../../../tests), [lib/include/bg2e/all.hpp](../../../../lib/include/bg2e/all.hpp), [lib/include/bg2e.hpp](../../../../lib/include/bg2e.hpp), [scripts/PACKAGING.md](../../../../scripts/PACKAGING.md).

## Implementation work

Repeat [M07 product regressions](../milestone_07_bg2e_composer/product-validation.md)
on the canonical app after cleanup: selection/table clipping and Chain controls,
root/clone/complete save, queued HTTP cancel/reimport/stop-join, integrated multi-target
bake cancellation/previews/save and renderer-first shutdown. The lead confirmed the
selection correction before retirement; native package and broader scenario detail
were not specified and must remain accurately attributed.

1. Run the agreed core matrix: default deferred/no-RT, query shadows, advanced RT/histories/denoise, FSR/external upscaler, manipulation/MaterialModifier/clones, integrated maps and accumulated offscreen export.
2. Exercise all three applications and final scene/file/config/pixel output round-trips; validate window/headless failures, dialogs/resources and shutdown. Include model-editor Workspace parity, complete draw resource save/reload, UV2 regeneration, integrated AO/GI completion/cancel/application, best-view analysis and production model_edit preference/input regressions. M06's current execution coverage is unspecified.
3. Audit transitive umbrella includes, exported types/symbols, shader paths, build/scripts and generated standalone sources for old namespace/Engine/native callbacks.
4. Record compatibility-breaking changes honestly. Data-format readability is distinct from unsupported old application source/ABI; no forwarding layer is restored.

## Verification and exit conditions

- The surviving products meet their actual previous acceptance gates with final draw-only defaults and packaged resources.
- Static and authorized build/runtime evidence shows render/scene::vk/obsolete application support absent; unsupported test machines remain explicitly unverified.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 cannot write accurate final docs from a cleanup that left hidden old code/aliases; this check supplies its authoritative product/API list.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Current engine preservation requirements — 2026-10-21

Regression coverage delegated to the lead must include enabled and diagnostic
baseline paths from tests/draw/{tracing_tables,shadow_bindings,acceleration_reuse,
resource_pools,temporal_invalidation,transparent_regions,update_synchronization}.
Check cutoff 0/below/equal 0.02 and glTF MASK behavior, stable-frame cache hits,
actual edits causing misses, image/BLAS recreation, overflow, pending/aborted
recordings, retained outputs, source replacement and teardown. Compare output and
work counters; do not assert measured speedups from cache existence. Use existing
profiling probes under docs/profiling_and_stats for disabled/live/recording,
unsupported GPU timing and correct frame/session association, without timing waits.

Also compare typed-only final-color batches with forced conservative drain mode,
custom-pass/full-extent transparent fallbacks, preserved pixels outside regional
writes, full receiver/tracing domains, and all actual resource/lifecycle waits.
Validate bool false saves do not mark documents/batches successful; include missing
destination directories and filesystems without hard links. Functional and timing
acceptance of independent optimization/profiling workstreams remains separately
attributed; no blanket all-backend acceptance is inferred.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 standalone/final umbrella verification

Recheck M08 product-validation.md P01–P11 against final canonical binaries and
headers. Resolve lightmaps.json and every referenced artifact, independently inspect
PNG/native float pixels/material sidecars and reload real model geometry in draw.
Check >65504 controlled writer HDR separately from scene lighting/chart numerics,
strict required-target failures, generated/existing UV2, config identity/CLI-last,
source/alias protection, final-sample/later-target cancellation, previous-generation
preservation and native/relocated shader resources. No flat-file/skip fallback is
restored for old apps. Zero-legacy checks must start from all.hpp/bg2e.hpp and app,
scene, DB, UI and new CLI transitive includes, plus generated standalone projects.
Use R01–R14 diagnostic/enabled baselines and actual work counters; synthetic
inspector or continuation acceptance does not identify hardware/scenario evidence.

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

## M09-04 actual native bridges to validate

Use [native-retirement.md](native-retirement.md), native-changes.tsv and the
source-only audit_native_sources.py alongside prior ledgers. Both render trees
are removed. Native Vulkan/Metal GUI recording now lives in gpu/*/GuiRenderer.cpp;
UI owns context/preview/session coordination through a private primitive adapter.
Validate overlay and preview generation/rebind on resize/configuration changes,
Vulkan pool/minimum counts, Metal metadata/encoder ownership and partial shutdown.
Also validate gpu::StandaloneGuiWindow accept/cancel/window-close/high-DPI and
creation-failure cleanup, followed by engine startup; app no longer owns GL calls.
gpu::drawableSize owns native window pixel queries. Verify installed exports and
SDK/vendor backend linkage. Agent checks were static only; native/performance and
full product/package matrices remain lead-owned.

## M09-05 implemented product/distribution prerequisite

[Product delivery](products-distribution.md), product-changes.tsv and
probe-dispositions.tsv are the actual prerequisite. Legacy examples/GLSL and
flat shader frontends are removed; all current draw/GPU examples/apps and four
registered test targets remain. Reconfigure old build trees before compilation.
Use audit_product_sources.py alongside audit_native_sources.py and all four
delta ledgers; the updated lexical inventory excludes canonical DrawableComponent
from old Drawable matches. No global legacy source matches remain in that inventory.

Run test_02's new two-island/preview checks and test_03's explicit baked RGB
G-buffer and numerical receiver/overhang AO/GI checks, plus retained lifecycle/
chart/dilation/packing probes. Unsupported mandatory tracing profiles fail rather
than pass a silent skip. Check thresholds on Vulkan/Metal and preserve no-RT
raster/UV behavior. Canonical bake has no old direct-shadow toggle or alpha-validity
composition contract; direct shadow tests stay in draw/soft_shadows.

Validate clean packages/bundles contain draw/bake and no retired flat outputs;
existing reused bundles may retain historical files until recreated. Engine
bundling/install selects authoritative trees, preserving native SDK/vendor files.
Generate a fresh standalone project using the deferred template, and separately
update an existing one: match headers/binary, verify shader refresh, custom resource
preservation, config-specific binary versus shared shader roots, macOS Xcode
library fallback and Windows PowerShell behavior. Presence checks and shell parsing
were static only; package/updater/creator/native execution remains lead-owned.
