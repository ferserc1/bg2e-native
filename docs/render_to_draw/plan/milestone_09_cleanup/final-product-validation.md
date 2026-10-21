# M09-06 — Final product source validation and native handoff

[Progress](../progress.md) · [Step](step_06_final-product-validation.md) ·
[Product catalog](final-products.tsv) · [Validation matrix](validation-matrix.tsv) ·
[Machine-readable audit](final-source-audit.json)

## Authority and result

M09-05 is accepted by project lead continuation instruction. Under the agreed
convention the lead compiled and performed their checks without finding failures;
backend, device, scenario, artifact, package and timing details were not specified.
That acceptance is retained without inventing per-scenario results or requesting
reconfirmation. HEAD remains `1889bdb12ade85d820eaab4a7365760fa9c34f50` with
inherited uncommitted M09-01–05 implementation. This delivery changes only planning,
source-audit tooling and evidence in this milestone directory and pending plans.
No production source, test fixture, CMake, shader, template or script is changed.

**Source gate:** the canonical products and public include graph contain no
retired renderer dependency in the audited source set. Four retirement ledgers
reconcile to 592 unique final paths, including 440 absent paths and 152 surviving
changed/added paths with matching final hashes. Source contracts and native
regression ownership are recorded below. No known source-gate failure remains.
**Native gate:** delegated to the project lead; no independently observed final
build, backend run, output round-trip, installed package or binary-symbol result
is claimed. M09-06 awaits their acceptance; M09-07 is the next separate step.

## Executed source checks

| Evidence | Observed result and limit |
|---|---|
| [audit_final_sources.py](audit_final_sources.py) / [JSON](final-source-audit.json) | 1,210 runtime/build/template/native-profiling source files scanned without retired type/namespace matches; deterministic source-set digest identifies the actual uncommitted implementation. This scope adds profiling probes to the prior product scan. |
| [Umbrella closure](umbrella-closure.tsv) | Eleven entry roots: bg2e.hpp, bg2e/all.hpp and app/scene/DB/UI/draw/bake/manipulation/utils/GPU umbrellas. Their union is 255 project headers, with no missing bg2e include or native Vulkan/Metal/ImGui header token in the external include boundary. All conditional branches are traversed lexically; system/SDK/GLM/SDL headers are not expanded. |
| [Public API evidence](public-api-evidence.tsv) | All 317 public headers have SHA-256, umbrella reachability, lexical type/alias names and BG2E_API marker text. The 62 headers outside the umbrella union remain explicit APIs/backend headers; absence from an umbrella is not retirement. These are declaration-location clues, not parsed qualified types or actual exported binary symbols. |
| [Final products](final-products.tsv) | 33 unique registered targets: three apps, 13 draw examples, 13 GPU examples and four registered checks. Each literal registration has a real CMake/source directory and C++ source digest. Manual probes are not implicitly registered. |
| Standalone template projection | Literal PROJECT_NAME substitution for m09_validation_app plus the copied main.cpp passes retired-source and project-header closure checks (255 headers). Recorded hashes are for projected UTF-8 text. Neither creator is executed; no configured standalone tree, binary copy, SDK/runtime lookup or successful launch is implied. |
| Chronological ledger reconciliation | M09-02/03/04/05 before/after chains and final filesystem state agree. M09-02's `absent` sentinel is normalized, without rewriting historical evidence. Removed files stay absent; final surviving hashes match. |
| Existing native/product audits | 707 supported C/C++ files pass native-layer/include/public-header checks; 1,205 product/build/script files pass retirement checks; 292 paired native stage outputs are collision-free and 290 quoted include edges resolve. M09-04 exact noise hash and M09-05 product delta remain valid. |
| Updated textual inventory | Current tracked/nonignored untracked sources resolve their includes with no unrecorded missing M08 baseline paths and zero legacy matches. Original retirement snapshots are not overwritten; current generation uses a temporary output directory. |
| Preservation/format checks | Step-entry hashes for all 1,327 existing files under engine/app/example/test/script/CMake/shader/profiling scopes remain unchanged. No C/C++/shader style change or runtime behavior change is introduced. Changed Markdown links, deterministic audit output, matrix source paths and git diff --check checked. |

The audit does not load stale development libraries or inspect existing bin/
artifacts as if they represented current sources. External libraries/vendor code,
preprocessor-specific availability, link resolution, actual symbols and native
resource deployment require final rebuilt binaries. Empty historical directories
or OS metadata do not constitute retained applications; supported registration
and source paths are the catalog above. Historical plans and maintained documents
still awaiting M09-08 are not available-API evidence or part of the zero-source claim.

Reproduce the nonexecuting source work with:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_final_sources.py --output-dir /tmp/bg2e-m09-06-final
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_product_sources.py
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_native_sources.py
python3 docs/render_to_draw/plan/milestone_09_cleanup/inventory_sources.py --output-dir /tmp/bg2e-m09-06-inventory
```

No CMake configuration, compilation, shader compiler, executable/test, package,
standalone creator/updater or artifact inspector was executed. The M08 inspector's
historical synthetic test result is not rerun or promoted into real product proof.

## Source-backed canonical API map

| Responsibility | Current interface and source anchor | Ownership and compatibility boundary |
|---|---|---|
| Window application | Application/MainLoop in lib/include/bg2e/app; private DrawGraphicsExecution in lib/src/bg2e/app/detail | Application::renderDelegate/setRenderDelegate use shared draw::RenderLoopDelegate. MainLoop accepts draw::EngineConfig; no execution-mode opt-in or production native execution adapter. |
| Headless execution | app/OffscreenApplication.hpp, app/DrawOffscreenApplicationDelegate.hpp; draw/DefaultOffscreenApplicationDelegate.hpp | OffscreenApplication owns draw Engine; delegates borrow Engine/OffscreenSurface and generic FrameContext. Draw-prefixed config/delegate names remain actual canonical names, not aliases. Exact completed samples/final-once behavior remains in the draw default delegate. |
| Renderer/extensions | draw/Renderer.hpp, RendererDeferred.hpp, DefaultRenderLoopDelegate.hpp, pipeline/RenderLayer.hpp, Upscaler.hpp | Runtime renderer ownership is exclusive in the delegate; injected compatible layers/effects/upscaler/settings retain shared ownership. Stopped lifecycle, generation checks and compatible formats/interfaces remain required. No private copy of the renderer is added to tools. |
| Live settings | draw/SettingsCatalog.hpp, SettingsCodec.hpp, RenderSettingsPreferences.hpp; ui/GraphicsSettingsWindow.hpp | Instance-backed reflected settings, generation-safe queued mutation, strict type/identity/import policy and headless CLI-last overrides. No compatibility conversion for removed application renderer APIs/config modes is promised. |
| Scene/resource ownership | scene/DrawableComponent.hpp, draw/Drawable.hpp; manipulation/SelectionManager.hpp and GizmoComponent.hpp | drawable() exposes shared draw resources (const reader supported); clone owns independent CPU/GPU/material resources. Selection mesh/lockMesh/meshPtr/selectedMesh and transformDrawable address draw only. Serialized Drawable component identity stays distinct from removed scene::Drawable implementation. |
| Neutral parsing and complete save | db/scene.hpp, mesh_bg2.hpp, draw_resource_io.cpp, texture_archive.hpp | CPU parser/root transfer remains neutral; complete save/store take draw Engine and return bool noexcept. Parent creation, snapshot/no-clobber/rollback and hard-link fallback survive. Every caller must respect false; format readability needs real file/native reload evidence. |
| Native UI/window integration | gpu/GuiRenderer.hpp, StandaloneGuiWindow.hpp, WindowGraphics.hpp; ui/UserInterface.hpp and private adapters | UI owns context/platform/preview session; GPU owns Vulkan/Metal recording, pools/metadata/texture registration and native chooser presentation. Caller stops users, drains and shuts down before destroying context/device. No public ImGui types or GPU dependency on draw/UI/scene. |
| Integrated and standalone bake | bake/IntegratedBakerContext.hpp, StandaloneBakerContext.hpp, StandaloneBakeSceneAssembler.hpp, StandaloneBakeBatch.hpp, LightmapOutputWriter.hpp | One active target/context, frozen validated jobs and exact completion; detached private resources, chart-aware composition/dilation and generation manifest commit. Pixel, model-copy and cancellation semantics retain their current explicit contracts. |
| Optimizations and profiling | draw cache/pool/signature/region APIs, base CPU/GPU telemetry, gpu completion/timing and ui/Performance.hpp | Preserve current v3/v5 tables/residency, exact completion/BLAS identities, retained leases and independent telemetry. Native timing granularities and unresolved/unsupported status remain distinct; no measured speedup inferred. |

## Canonical product gate and scenario attribution

[validation-matrix.tsv](validation-matrix.tsv) contains 33 source-backed rows:
V01–V17 core/products/native UI/deployment, O01–O05 diagnostic/profiling baselines,
and P01–P11 mapped from the M08 matrix to final products. Every source anchor
exists. Each row records **agent_status=source_reviewed** and
**lead_scenario_coverage=not_specified**, with execution owned by the project lead.
It is a runnable-scenario handoff, not 33 reported passing tests. Existing
continuation acceptance remains sufficient under the protocol; no new mandatory
logs, hardware list or approval process is introduced.

The matrix covers the deferred no-RT/optional-feature paths, query shadows,
advanced RT/history/denoise, optional FSR/external upscaler, manipulation/modifiers/
clones, integrated maps and accumulated offscreen output. Model Edit checks include
Workspace, complete resource save/reload, UV2/best-view/bake/cancel/application,
preferences/input and safe failure. Composer checks repeat selection/table/Chain,
root/clone/save, HTTP queued cancel/reimport/stop/join, multi-target bake/previews
and renderer-before-stage shutdown. Source inspection confirms those cleanup
orders; real race, dialog, rendering and I/O behavior remains scenario-specific.

P01–P11 uses the final draw_lightmap_generator rather than restoring the removed
CLI. Resolve every committed lightmaps.json artifact, independently inspect
asymmetric PNG/native RGB32F and material sidecars, then reload real model geometry
in draw. Separate controlled writer HDR >65504 from lighting and chart assertions.
Include strict required-target/config errors, generated/existing UV2 and model
copies, original source hashes/aliases, custom dependencies and filesystem cases,
final-sample/later-target cancellation and preservation of the previous manifest
**and all referenced outputs**. The independent inspector does not prove geometry,
lighting/chart correctness, source preservation or cancellation; non-PNG/non-native
encodings require another decoder.

R01–R14 remain operative through V02–V13/P01–P11 and O01–O05: alpha thresholds and
MASK semantics, revision/native-generation cache hits/misses, exact capacity and
neutral overflow, completion/BLAS/pending/aborted ownership, temporary leases,
history invalidation, conservative transparent-region fallback and pixels outside
regional writes, typed-only drain elision versus forced drain, and bool-false saves.
Profiler disabled/live/recording modes retain original frame/session associations,
unsupported/dropped/aborted/unresolved status and asynchronous polling. Compare
functional images plus work counters; independent optimization/profiling acceptance
and overhead measurements are not certified by migration continuation.

Final deployment covers supported Linux Ninja/macOS Xcode/Windows VS2022 builds,
three installed apps, all examples/checks, actual SDK/Metal/vendor resources,
paired trees and specialization templates, shader-only refresh and relocation.
Verify rebuilt symbol exports with platform tools and fresh artifacts without
old flat stages. Reused outputs are not automatically purged. Generate and launch
a real standalone project from matching rebuilt headers/library, then exercise
update/config-path/shader/custom-resource/failure behavior. The source template
projection cannot substitute for any of these checks.

## Deliberately breaking changes

- Removed bg2e::render and scene::vk/scene native Drawable/Mesh/visitors, old apps,
  forward/MSAA implementations and native callback/Engine overloads. No source,
  binary, forwarding-header or namespace compatibility layer remains.
- Canonical shared accessors/factories and draw-only component/storage rules replace
  old coexistence tags and modes. Explicit baked indirect mode and typed UV validity
  replace old implicit AO-as-baked/attachment/alpha assumptions.
- Draw config identity/strict settings and standalone complete-generation manifest
  layout are current contracts; old application/config/output-layout compatibility
  is not inferred from compatible authored mesh/scene/image formats.
- CPU data readers, root ownership and supported resource snapshots remain. That
  preservation is distinct from promising every historical file/custom component
  can be exported or every old application binary can load the rebuilt library.

## Pending plan impact

| Owner | Actual handoff |
|---|---|
| M09-07 technical review | Use final-products.tsv, public-api-evidence.tsv, umbrella-closure.tsv, four retirement deltas, validation matrix and source digest. Compare final API/data/ownership/native seams and deliberate breaks with pending documentation. Do not label unspecified native rows or a declaration catalog as binary/runtime proof. |
| M09-08 documentation | Author cumulative production draw guidance from current source/catalog; remove stale usage paths and experimental/coexistence promises. Keep historical reports separate, and document exact canonical names and validation limits. |
| M10 inventory/API/distribution/final audit | Audit the accepted M09-08 docs against these concrete products/headers and source hashes. Optional backend/SDK behavior and unreported native/counter scenarios retain explicit limits rather than being silently marked complete. |

M09-06 delivers the source checks and owned validation handoff required by the
execution protocol. Actual native outcomes remain attributed to the lead; no
additional implementation step, renderer feature or build-system edit is introduced.


## Lead-reported build failure and correction

The lead's subsequent macOS arm64 Clang build failed in db/scene.cpp because
saveSceneReferences called countSceneNodes after M09-03 removed that neutral
helper. The suggested countJsonNodes and pointer-conversion diagnostic are
cascading errors, not a JSON conversion requirement. This report supersedes the
original no-known-source-failure/unchanged-runtime statements for this correction.

Restored the existing recursive Node counter before its call: null returns zero,
a node contributes one, and each child's subtree is counted. This matches
Node::serializeReferences traversal and preserves progress totals without restoring
any renderer dependency. No public interface, CMake or serialization format changed.
validation-corrections.tsv extends the historical hash chain; audit_final_sources.py
reads it after the four retirement ledgers. Current generated audit evidence is
refreshed for the corrected source. The helper body was compared with HEAD's
original implementation and its declaration precedes the call. Source audits and
git diff --check pass. No compilation or runtime test was run by the agent.
M09-06 remains awaiting project-lead validation of this correction; M09-07 remains
the successor. Original source-only acceptance claims are not compilation proof.


### Metal GUI declarations correction

The lead next reported four undeclared ImGui_ImplMetal functions in the relocated
gpu/metal/GuiRenderer.cpp. The vendored header gates its Metal-cpp declarations
behind IMGUI_IMPL_METAL_CPP; the old source-specific definition in deps.cmake
still applied only to ui/ImGuiMetalBackend.cpp. The new translation unit therefore
saw no C++ declarations. It now opts in locally, guarded by #ifndef, before including
imgui_impl_metal.h and inside the existing macOS guard. The vendored Objective-C++
implementation already receives the same definition and ARC options. No CMake,
public header, vendor file or rendering behavior is changed. Vulkan's header
provides its declarations without an equivalent opt-in; no matching fix is needed.

validation-corrections.tsv extends the native hash delta; audit_native_sources.py
now checks corrected hashes through that chain rather than requiring historical
intermediate bytes. Final source evidence is refreshed. Macro ordering, the four
header declarations, vendored C++ implementations/build flags, dependency direction
and whitespace were inspected. No preprocessing/compilation/native run occurred;
both reported corrections remain pending lead validation under M09-06.


### VMA implementation/link correction

The next lead-reported macOS arm64 failure reached shared-library linking and
reported unresolved vma allocator/buffer/image/map/flush/statistics functions.
The single VMA_IMPLEMENTATION translation unit had lived in the retired
render/vulkan/common.cpp; GPU declarations/users survived but the implementation
was deleted with that wrapper. Restored the exact implementation/configuration
block (including platform diagnostics and optional allocation logging) at global
scope in existing gpu/vk/Device.cpp, after its includes and before GPU namespaces.
The SDK header places its implementation outside the declaration include guard,
so this explicit second inclusion with VMA_IMPLEMENTATION emits the definitions.
No retired namespace/getMappedData wrapper is restored. Using an existing compiled
translation unit avoids a new globbed source/reconfiguration requirement; no CMake,
public header or vendor file is modified.

validation-corrections.tsv records the Device.cpp delta; the final audit now
includes this newly modified path. The native audit additionally requires exactly
one source-level VMA_IMPLEMENTATION owner in gpu/vk and its following implementation
include. Compared the moved block and SDK header guards directly, checked global
scope, unchanged Device method bodies, existing source registration, dependency
boundaries and whitespace. No compiler/linker/native execution was performed by
the agent. The lead must validate the complete build; a link failure is not an
accepted whole-build result. M09-06 remains awaiting validation before M09-07.
