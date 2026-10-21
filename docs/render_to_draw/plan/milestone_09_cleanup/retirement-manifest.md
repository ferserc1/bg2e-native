# M09-01 — Retirement manifest and dependency decisions

[Step](step_01_retirement-manifest.md) · [Progress](../progress.md) ·
[Per-file manifest](retirement-manifest.tsv) · [Include edges](include-edges.tsv) ·
[Reference evidence](reference-evidence.tsv) · [M08 comparison](baseline-comparison.tsv) ·
[Generator](inventory_sources.py)

## Baseline and status

Delivered 2026-10-21 at HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50`,
starting from a clean checkout. The lead's next-step instruction accepts M08-07
and its cumulative documentation closure. Compilation/checks are assumed successful
under the continuation convention; backend/scenario coverage is unspecified.
This delivery inventories and assigns retirement work only. No engine, shader,
CMake, application, asset or packaging implementation is changed or removed.
Draw promotion remains future M09-02–06 work.

## Reproducible coverage and limits

Run `python3 docs/render_to_draw/plan/milestone_09_cleanup/inventory_sources.py`.
The standard-library script reads tracked files in lib/include, lib/src, apps,
examples, tests, cmake, scripts and shaders, plus root CMakeLists.txt. It writes
only the four adjacent TSV artifacts. It does not overwrite M08's historical
inventory. File suffixes and regular expressions are explicit in the script.
Untracked future implementation files must be added to coverage before retiring
anything; generated build/bin trees, author assets and third-party dependencies
are outside deletion coverage. Documentation is mapped below, not scheduled for
source deletion.

| Artifact | Result |
|---|---|
| Per-file actions | 1620 files: 407 remove, 391 migrate, 821 keep, 1 relocate_data. |
| M08 reconciliation | All 1338 old candidates covered; 282 additional files, principally neutral source and probes, now explicitly assigned. Old totals are not deletion totals. |
| Direct references | 364 files contain expanded legacy tokens, including aliases, scene Drawable/Mesh/visitors, mode tags, execution factory and shared data names. Comments/strings included. |
| Text evidence | 6365 rows identify path, line, kind and text for legacy/native/registration signals. |
| Includes | 4329 resolved lexical project include edges with consumer, line, target and original token; reverse incoming-file counts in each manifest row. |

The source SHA256 in each manifest row allows the implementing step to detect
source drift. `baseline-comparison.tsv` assigns a new action and owning step to
every old candidate and newly covered file. KEEP is preservation of the file's
responsibility, not a claim that its transitive includes are already render-free.
MIGRATE preserves useful behavior while replacing production dependencies;
REMOVE is conditional on migrating incoming consumers and preserving required
assertions/data. None of the TSVs is an executable deletion list.

Include resolution uses the source directory, public include root, and the owning
GLSL/Metal tree root, matching paired shader root separation. For example,
draw/gbuffer/probe.frag.glsl uses draw/lib/pbr.glsl, not the identically named
legacy GLSL helper. Macro-generated includes, conditional preprocessing, dynamic
shader paths, unqualified C++ types and callbacks are not a semantic graph.
The family decisions below cover those non-include relationships. Future owners
must repeat source/reference/build-resource checks after each migration; this
snapshot cannot prove zero legacy linkage or native runtime correctness.

## Incoming consumers, replacements and execution order

| Source / incoming family | Decision and retained responsibility | Owner / gate |
|---|---|---|
| render headers/sources, including vulkan, deferred, forward, MSAA and old bake | Remove after shared users migrate. draw Engine/RendererDeferred/passes, gpu primitives and bake integrated/standalone APIs are replacements. No old forwarding namespace/headers. | M09-04 after M09-02/03. |
| render/blue_noise_data.h → utils/BlueNoise.cpp private include | Relocate exact authoritative data to lib/src/bg2e/utils/blue_noise_data.h before deleting render; update generator usage/comment and diagnostics invocation examples. Preserve dimensions/ranks and bytes, not a regenerated sample. | M09-04, script/docs consumers M09-05/08. |
| scene/vk → scene/all.hpp and old bake fixture | Remove native scene binding headers/sources; draw DataBinding/ShaderData and gpu resource sets retain current packing/revisions. Port old fixture assertions. | M09-03/05. |
| scene Drawable/Mesh, DrawVisitor, RenderQueueVisitor, SkyDomeTextureGenerator → scene umbrella, DB, manipulation, BoundingBox, widgets, tests | Remove obsolete native scene types; use draw Drawable/Mesh/RenderQueueVisitor and public environment generation. Preserve scene hierarchy, enabled state, bounds and CPU serialization. | M09-03; migrate every listed incoming edge first. |
| DrawableRegistry → scene umbrella and serializers | Migrate naming/registration responsibility and CPU unique-name behavior, replacing production DrawableBase; do not blindly remove it because its header includes Drawable.hpp. | M09-03, recheck actual consumers. |
| Node/Scene/Component/factories, all component subclasses, reflection and chains | Remove render overloads, dual storage and mode adapters. Keep parsed CPU hooks, exact/subclass clone validation, independent projections/GPU clones, root release, unknown-component handling, chains/joints and helper exclusion. | M09-03. |
| ResizeViewportVisitor, Find/Update/Input/Transform visitors, BoundingBox | Keep graphics-neutral behavior; remove legacy Drawable calls where present. No blanket visitor deletion. | M09-03. |
| Application/MainLoop, private GraphicsExecution factories and RenderGraphicsExecution.cpp | Remove production delegate slot/branch and implementation; ordinary run selects DrawGraphicsExecution. Keep backend override, renderer switching, stopped queues, joined async imports, narrow typed update synchronization and timing association. | M09-02. |
| OffscreenApplication/OffscreenExecution → draw camera CLI and standalone bake CLI | Remove ProductionRender/ExperimentalDraw selection and production callbacks. Keep draw windowless lifecycle, exact completed camera samples/final resolve and standalone no-camera finish/batch as separate workloads. | M09-02, all owners release before Engine teardown. |
| GPUSelectionDialog, UserInterfaceDelegate/UserInterface, ImGui backend interface | Remove production overloads. Device selection must retain useful selection via gpu instance/device capabilities rather than render::vulkan properties. Keep wrapper UI and draw initialization; no imgui public-header leakage. | M09-02/03; native implementation M09-04. |
| ImGuiVulkanBackend/ImGuiMetalBackend and DrawGraphicsExecution SDL drawable-size call | Retained native descriptor pools, commands, resize/device synchronization, Metal preparation/texture handles belong behind gpu implementation bridges. UI owns coordination/callbacks, never a new gpu dependency on ui/draw/scene. Delete production native branches only. | M09-04; Vulkan and Metal UI/preview/resize/teardown checks M09-06. |
| DB mesh_bg2/mesh_obj/scene_gltf/scene/scene_bg2 → scene, importers, editors, bake assembly | Delete render realization overloads only. Keep CPU readers, OBJ/MTL dependency discovery, draw realization, complete bool-noexcept save, material/chains JSON, generic archive/image/file publication and native GPU resource snapshotting. | M09-03; data-format readability gate. |
| DB LightmapOutputWriter → db/all.hpp and legacy CLI | Remove specialized old writer pair/export. bake LightmapOutputWriter owns final-manifest complete generations and typed native RGB32F; generic DB IO survives. | M09-03 after old app retirement. |
| TextureCache/MaterialModifier/ViewpointAnalyzer/SceneCameraUtils/Uv2SafeReload | Remove production maps/overloads. Retain weak draw cache ownership, patch semantics, safe reload, selection identities and view analysis with stopped-GPU lifetime. | M09-03. |
| manipulation selection/gizmos/picking and UI MaterialEditor/TextureWidgets/SubmeshSelector/UvMapPreview/RenderSettingsWindow | Retire production resource variants; keep draw editor/texture widgets, overlay/picking, bake UV preview, current settings facade and bool SelectableList. Shared files retain CPU wrappers and independent material ownership. Drain/unregister previews and instance gizmos before UI/Engine release. | M09-03. |
| root all.hpp, bg2e.hpp, scene/db/app/ui umbrellas | Remove legacy exports, add bake/all.hpp to canonical root export. Check every direct incoming header and supported product, not only qualified render tokens. | M09-03/06. |
| apps/model_edit, apps/bg2e_composer, apps/lightmap_generator and registrations | Remove old source/targets; apps/draw equivalents become supported apps. Preserve draw Composer joined CPU import queue/HTTP service, save error handling, environment selection and Performance UI. | M09-02/05. |
| old examples/debug-app and 01–18 → examples/CMakeLists.txt | Retire old demos; surviving examples/gpu/01–13 and examples/draw/01–13 cover primitives, UI, rendering, manipulation, lightmaps and camera CLI. No replacement forward/MSAA renderer is required. | M09-05. |
| test_01_key_and_mouse | Port event/input assertions from old RenderLoopDelegate to draw lifecycle rather than removing useful input coverage. | M09-05. |
| test_02_uv_surface and test_03_integrated_baker_context | Port UV geometry, RGB baked material, sample/reset/completion/readback assertions to draw/bake probes or targets before retiring old native harnesses. Existing probe filenames alone do not establish assertion coverage. | M09-05. |
| test_04_uv2_atlas | Keep CPU atlas assertions and target; it depends on geo, not old renderer. | M09-05. |
| tests/draw, tests/gpu, tests/bake, tests/ui, tests/manipulation, tests/utils and optimization/profiling probes | Keep assertions and evidence provenance; update final delegates/accessors/includes, remove only dual-mode/production portions of probes. | M09-05/06. |
| shaders/src/glsl and ShaderLib/RuntimeShaders references | Remove legacy stages/helpers after consumers migrate. Preserve separate draw/bake GLSL+Metal trees, stage-relative paths, shared includes, runtime install and paired rebuild/deployment. | M09-05. |
| vendored iryku_smaa AreaTex.h/SearchTex.h, Vulkan SDK/VMA/SDL/Metal/ImGui/NFD | Keep dependencies/native runtime assets and original licenses. Native graphics backend support is not obsolete render implementation. Author assets and stored model/scene files remain untouched. | M09-04/05/06. |
| root/lib/apps/examples/tests CMake, cmake/utils/standalone_utils/package_apps, shader helpers | Migrate real source/target/install/resource consumers and remove only obsolete registration. Keep platform generators, required SDK configuration, native dependencies and recursive app packaging. No CMake edits in this step. | M09-02/05. |
| scripts/create_standalone_project.*, templates/main.cpp and *.in, package_* and update_libraries.* | Regenerate template against canonical draw delegates/defaults; retain independent external project/header/library/resource copying. Read generated-project source, installation and package rules together. | M09-05/06. |

Each individual file has an action/owner/replacement in the TSV; resolved incoming
includes are indexed separately. Files without incoming lexical includes can still
be globbed translation units, runtime stages or build/template consumers. Their
removal remains subject to family/build/resource gates above.

## Final naming decisions for implementation

These are selected M09 destinations, not APIs claimed available in M09-01.

| Area | Selected final contract | Migration owner |
|---|---|---|
| Namespaces | draw for high-level graphics, gpu for native primitives, bake for baking, db for neutral IO; no render alias. | M09-03/04 |
| DrawableComponent | `drawable()` returns draw Drawable, preserving appropriate mutable/const use; remove old scene Drawable accessor/storage and transitional `drawDrawable()`/mode tags. Update reflection, all supported callers and probes together. | M09-03 |
| Application delegate | `renderDelegate()`/`setRenderDelegate()` use draw::RenderLoopDelegate only; remove alternate `drawDelegate()` storage/accessor. | M09-02 |
| MainLoop | ordinary `run(Application*)` uses draw defaults; explicit draw::EngineConfig overload stays. No experimental selection flag. | M09-02 |
| Offscreen | Retain actual DrawOffscreenApplicationDelegate/DrawOffscreenApplicationConfig names as the sole typed path; remove obsolete production delegate/config, execution-mode enum and associated header/export once unused. Draw prefix describes its contract, not experimental opt-in. | M09-02 |
| Apps/targets | Keep apps/draw source locations and current draw_model_edit, draw_bg2e_composer, draw_lightmap_generator and draw_offscreen_application executable names. Retire old targets without forwarding aliases. | M09-02/05 |
| Widgets | Keep actual DrawMaterialEditor/DrawTextureWidgets and bake preview names; remove production variants. A blanket cosmetic rename adds no product capability. | M09-03 |
| Stored identities | Keep current draw app/preference/import-server identities and SettingsCodec schema bg2e.draw.settings/version 1; standalone renderer bg2e.bake.standalone.v1 and bake/type bg2e.bake.lightmapSettings.v1. | M09-02/05 |
| Umbrella | root bg2e/all.hpp exports draw/gpu/bake and retained modules; no render/scene::vk export. Explicit bake/all.hpp remains usable. | M09-03 |

No old source API/ABI, previous executable target or render-native callback
compatibility is promised. This is separate from stored data: BG2/VWGLB/OBJ/glTF,
scene/prefab component schema, chains, materials, native texture archives and typed
settings remain readable under actual supported loaders. Preserve alpha default
zero/effective threshold 0.02, glTF MASK handling, linear RGB UV2 baked AO/GI,
source-preserving snapshots, native float precision, cancellation and authoritative
lightmaps.json publication. Old flat-output/skip-success behavior is not restored.

## Product and optimization gates

M01–M08 have lead continuation acceptance, now including M08 documentation.
No required new-product implementation gap is established by this inventory.
Remaining migration work is explicitly owned above; native numerical, deployment,
performance and process-fault coverage remains unspecified, not silently proved.
Use [M08 P01–P11](../milestone_08_lightmap_generator/product-validation.md) and
[R01–R14](../code-reconciliation-2026-10-21.md) during M09-06 validation.

KEEP/adapt includes MaterialCache and value/binding/classification revisions;
TemporalSignatureCache, TracingPersistentResources, TracingTableLookup and Metal
ArgumentEncoderState; gpu RecordingCompletion, binding/build generations,
uploaded bounds and persistent array/residency owners; TemporaryResourcePool,
logical leases and ProcessingRegion. Preserve device-clamped capacity up to 16384,
current tracing v3/query shadows v5 ABIs/reserves, completion-based TLAS reuse,
conservative transparent bounds and typed update drain elision. Preserve base
CpuProfile/GpuProfile, gpu SubmissionTiming/native asynchronous completion and
UI Performance/detail PerformanceData/Composer menu and diagnostic probes.
These are current implementations, not retirement compatibility layers.

Lead checks following implementation steps cover windowed Vulkan/Metal UI,
backend selection, no-RT/optional feature fallbacks, actual products and settings,
scene/model/native resource reload, both headless workloads, cancellation/output
protection, generated standalone projects and relocated packages. M09-01 itself
needs review of disposition/order/naming and preservation boundaries; it changes
no runtime behavior and supplies no new native test result.

## Maintained documentation ownership

Use [M08's 31-page coverage ledger](../milestone_08_lightmap_generator/documentation.md)
as the existing source-to-document baseline. M09-02/03 must update affected live
snippets when changing app delegates/DrawableComponent accessors; M09-08 owns
cumulative production/retirement reconciliation and M10 audits it. Concrete affected
families are doc/api/app, scene, draw, gpu, bake, db, ui, manipulation, utils,
doc/tutorials, doc/apps, doc/architecture, doc/index.md, README.md and
scripts/PACKAGING.md. Preserve authored standalone API/tutorial/CLI/publication
coverage; do not replace it with speculative final APIs. Retained historical
planning/reviews remain evidence, not supported legacy usage guidance.

## Verification and handoff

Agent checks: read-only current declaration/implementation/build/template/resource
inspection; refresh versus M08; path existence, unique dispositions, source hashes,
resolved include endpoints and referenced owning steps; owning shader include-root
resolution; Python syntax, Markdown links/fences and whitespace. No C++ source was
changed, so no new C++ cosmetic/license audit is claimed. No configure, compilation,
shader compiler, engine fixture/app, package or GPU operation ran.

M09-01 awaits lead validation. The linked successor is M09-02 only. M09-07 will
record the actual final API and measured/reported evidence after implementation;
this manifest does not pre-populate that successful review. Refresh changed source
rows and dependencies during subsequent steps rather than treating hashes/counts
as immutable final deletion proof.
