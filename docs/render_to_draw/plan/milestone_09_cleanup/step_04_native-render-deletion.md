# M09-04 — Delete render implementation and confine native graphics to GPU

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01–M09-03.

## Scope and deliverable

- `lib/include/bg2e/render/`
- `lib/src/bg2e/render/`
- `lib/src/bg2e/ui/ImGuiVulkanBackend.cpp`
- `lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp`
- `lib/include/bg2e/gpu/vk/`
- `lib/include/bg2e/gpu/metal/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/](../../../../lib/include/bg2e/render), [lib/src/bg2e/render/](../../../../lib/src/bg2e/render), [lib/src/bg2e/ui/ImGuiVulkanBackend.cpp](../../../../lib/src/bg2e/ui/ImGuiVulkanBackend.cpp), [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp), [lib/include/bg2e/gpu/vk/](../../../../lib/include/bg2e/gpu/vk), [lib/include/bg2e/gpu/metal/](../../../../lib/include/bg2e/gpu/metal).

## Implementation work

1. Delete the production render tree and its specialized ViewpointAnalyzer/BlueNoise/bake/forward implementations after confirming replacements; no hidden archived runtime code or forwarding namespace remains.
2. Audit remaining native Vulkan/VMA/SDL-Vulkan commands and wrappers outside GPU. Move necessary native UI, SDK and drawable-size/window graphics operations into GPU backend implementation/bridges; high-level coordination stays in its namespace.
3. Keep reflection/JSON/material/scene/gpu implementations independent of the deleted tree. Do not introduce GPU dependencies on ui/scene/draw to move native code: backend bridges accept primitive/native data or callbacks.
4. Separate actual native API implementation from explicit ShaderBinding.vulkan indices, backend type/handler casts and GLSL syntax, which remain legitimate. Native ImGui headers/types still do not enter public engine headers.

## Verification and exit conditions

- Supported C/C++ headers/sources have no render/legacy bindings references and no new native Vulkan implementation outside GPU.
- Both GPU backends, FSR bridge and UI composition function with their final interfaces; a Vulkan scan does not incorrectly delete required GPU SDK/MoltenVK assets.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-05 removes obsolete shader/build/package/test paths; M10 documents backend-native seams and the final public extension surface.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M04-07 reviewed implementation requirement

Keep the bake namespace implementation and its public draw/gpu dependencies; it contains no production rendering algorithm wrapper. Remove legacy render bake/output/preview classes only after M08 parity. The CPU UV diagnostic is an accepted implementation refinement, not a native rendering bridge that should be moved into gpu.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## Current engine preservation requirements — 2026-10-21

Keep native Vulkan/Metal persistent descriptor/argument-array caching and complete
Metal indirect-resource residency, layout and encoder stride ownership. Preserve
Vulkan timing query/timestamp capabilities and Metal encoder-envelope/submission
timing, exact RecordingCompletion state, image bindingGeneration and BLAS build
identity. Native graphics confinement is not permission to delete GPU profiling,
cache helpers or backend-owned completion safety. Do not introduce GPU dependencies
on draw/ui to retain telemetry: transport remains in base/gpu.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 actual native seam audit

Before deleting the 178-file current render implementation tree, resolve its
incoming users from M09-01/03. Inspect both ImGuiVulkanBackend.cpp AND
ImGuiMetalBackend.cpp, ui/detail/ImGuiBackend.hpp, UserInterface public draw
callbacks and app/GPUSelectionDialog. The Metal adapter still implements the
production Vk callback signature, so removing Vulkan includes only from the
Vulkan backend is insufficient. DrawGraphicsExecution also contains retained
SDL_Vulkan_GetDrawableSize coordination. Move only required native implementation
behind GPU primitive/native bridges without adding GPU-to-UI/scene/draw dependencies;
keep UI wrappers and imgui header privacy. Do not delete gpu/vk, Vulkan SDK/runtime,
current v3/v5 tables/pools/residency or legitimate binding indices/backend casts.

## M09-01 data/native preservation order

Apply [the per-file manifest](retirement-manifest.md) after incoming migrations.
Move authoritative render/blue_noise_data.h to utils/blue_noise_data.h, preserve
exact bytes/hash/metadata and update utils/BlueNoise.cpp before deleting its old
parent. Keep vendored SMAA tables/licenses. ImGui Vulkan pools/commands and Metal
preparation/handle work must move behind gpu implementation bridges; preserve UI
coordination without introducing gpu dependencies on ui/draw/scene. Also resolve
GPUSelectionDialog's render::vulkan properties and DrawGraphicsExecution's native
SDL drawable-size call. Native SDK assets and optimized gpu/draw/bake helpers are
KEEP, not obsolete render. Recheck graph/reference evidence after source changes.

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
