# M09-03 — Shared API cleanup delivered

[Step](step_03_shared-api-cleanup.md) · [Progress](../progress.md) ·
[Actual source delta](shared-api-changes.tsv) · [Original manifest](retirement-manifest.md)

## Baseline and acceptance

2026-10-21, HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50`, with inherited
uncommitted M09-01/02 work preserved. The lead's next-step instruction accepts
M09-02 under the continuation convention; compilation/checks assumed successful,
backend/scenario details unspecified. This delivery implements M09-03 and awaits
lead validation. No build, shader compiler, C++ fixture/application, GPU operation,
package operation or performance capture was run.

The delta records **296 source/header files: 107 modified, 187 removed, two added**.
Of the removals, 153 are production rendering headers/sources with incoming
dependencies on retired shared APIs. They move forward as required dependency
closure; 23 independent render files, including native wrappers and blue-noise
data, remain M09-04 work. Neither native confinement nor whole render retirement
is claimed here. No CMake, script/package implementation or shader file changes
are made by this step.

## Delivered public and ownership contracts

| Family | Actual final shared contract / preservation |
|---|---|
| DrawableComponent | One draw::Drawable owner. drawable() has mutable/const shared return types; old drawableBase/drawDrawable/native callbacks/modes/storage removed. setDrawable/clearDrawable retain synchronization and cross-Engine replacement rejection. CPU source paths/resource names remain separate from loading. clone retains deep independent ownership. |
| Scene graph | Remove render Engine deserialization/load overloads, native scene Drawable/Mesh/DrawVisitor/RenderQueueVisitor/SkyDomeTextureGenerator and scene::vk. Keep Node root/subclass/clone/schema helpers, independent projections, chains/joints, helper exclusion and selected environment semantics. ResizeViewportVisitor remains graphics-free with generic extent conversion. |
| Factory | registerComponent(name, defaultCreator) retains CPU construction, type/owner validation, unknown warning/skip and registered parser/resource failure propagation. create(data,path) and draw Engine realization stages remain. No production Creator/deserializer callback. Registration macros and custom probes use the same final API. |
| Serialization | Keep raw document IO, explicit saveSceneReferences and bool-noexcept engine-aware complete saveScene/storeDrawableBg2. Retire Node's implicit resource-export serialize and old DB save/store wrappers that depend on native scene objects. Component::serialize remains a common attribute hook; DrawableComponent::serialize delegates to reference-only schema. No implicit complete GPU export without an Engine. |
| DB data | CPU BG2/VWGLB/OBJ/glTF parsing/material policy, OBJ/MTL source discovery, generic file/image/archive IO and draw loading remain. Complete native resource/mip/layer snapshots and checked publication behavior are unchanged. Retire specialized db::LightmapOutputWriter; bake owns batch output without an alias. |
| Registry and bounds | DrawableRegistry retains naming/unique-registration behavior for draw resources. BoundingBox uses geo::MeshPNUUT, world/draw/local-submesh transforms, visibility and helper filtering. Preserve ordinary CPU Find/Update/Input/Transform/resize visitors. |
| Utilities | Keep weak Engine/device-validated draw TextureCache, independent CPU/live MaterialModifier patch semantics and texture revisions. Remove production overloads/maps and render CameraCandidate helper; utils candidate/viewpoint analysis remains. Uv2SafeReload::regenerateDraw keeps stopped-generation/weak cancellation and independent replacement; remove old production regenerate implementation. |
| Selection | SelectionItem::mesh/lockMesh()/meshPtr() and selectedMesh() address draw resources only. Preserve weak membership/resource/submesh/pick-ID checks, topology invalidation, pruning/notifications, additive/empty-hit behavior and transform capture. CPU init remains allocation-free; picking requires a live stopped device. Remove mode enum/native pick visitor; keep PickKind in its own public header. |
| Gizmos and overlays | Keep per-instance draw meshes, transforms/handles/scales/colors, transient clones, helper exclusion and safe resource synchronization/teardown. transformDrawable() now returns draw resources; remove production global GPU cache/native pipelines. Generic OverlayPass recording retains its frame/Engine/context guards and visitor filtering. |
| Widgets and UI | Retain DrawMaterialEditor/DrawTextureWidgets, preview registry, bake previews, settings facade, bool SelectableList and profiling. Adapt SubmeshSelector/DrawableEditor to canonical weak draw selections. Remove native MaterialEditor/TextureWidgets/UvMapPreview/RenderSettingsWindow pairs. UserInterface/Delegate accepts draw only and generic prepared command/frame pairs; remove production UI lifetime and native public callbacks. |
| Device selector | GPUSelectionDialog::run(devices) accepts caller-supplied shared gpu::PhysicalDeviceProperties: empty rejects, one returns directly, multiple present the UI and cancel returns null. Remove hidden native Vulkan enumeration/instance/surface creation. It does not change a running Engine's physical device. Remaining standalone OpenGL UI initialization is a native M09-04 seam. |
| Umbrellas and consumers | Root all.hpp exports bake/all.hpp and no render namespace; scene/DB/manipulation/UI exports omit retired headers. Apps, bake services, renderer traversal, reflection-facing headers, utilities and supported probes use canonical accessors. No forwarding types or application compatibility shim. |

The live draw::Drawable::drawMesh() and RenderQueue::Item::drawMesh GPU APIs are
preserved: they are distinct from the retired SelectionItem dual fields. Current
renderer/pass configuration, shader ABIs/capacity, material alpha policy/revisions,
completion/TLAS reuse, persistent native arrays, temporary leases/pools, temporal
signatures, processing regions and CPU/GPU profiling remain protected. Weak cache
ownership is not a global editable material catalog or ResourceManager.

## Registered and manual probes

The existing test_02_uv_surface and test_03_integrated_baker_context targets now
reuse public draw/bake probes through the new private retirement_harness.hpp.
Their main functions use canonical windowless application ownership, zero camera
frames, explicit optional backend/shader-root arguments and failure exit status.
No CMake changes are required; the obsolete test_02 include-search path is left
for M09-05 build cleanup. Original source is available at the entry HEAD, not
kept as a live compatibility implementation.

The UV target runs existing CPU/GPU UV geometry, transform, integer coverage,
retained-output and independent reload probes. The integrated target also invokes
existing pixel conversion, chart/dilation AO output, GI composition and RTAO/RTGI
sample/completion/reset/source-generation probes on a loaded UV target and generated
environment. Mandatory unavailable profiles fail, not skip successfully. These are
adapted harnesses, not claims of native execution or identical fixture-specific
numerical coverage. M09-05/06 must compare remaining original fixture-specific
material/range checks and collect real backend evidence.

The historical tests/manipulation/dual_mode folder now contains the draw-only CPU
selection fixture. It keeps weak membership, replacement/topology/ID, serialization,
clone and missing-device picking checks; old mode-specific assertions are removed.
Custom schema/assembly and resource IO probes use the final constructor/accessor
API. No C++ probe was compiled or run.

## Documentation and remaining work

Updated 25 maintained pages and added two: TextureCache reference and shared API
ownership architecture. Coverage includes scene/factories, draw resources, DB
reference/output ownership, selection/gizmos, MaterialModifier/camera/cache helpers,
UV2 reload/device selector, generic UI/material/texture/UV/settings guides and
updated manipulation/lightmap snippets. Stable retired-widget guide URLs point to
actual replacements. Existing standalone native HDR/publication documentation is
preserved. Two test guides describe current source and evidence limits.

M09-04 must remove the 23 remaining render files after relocating authoritative
blue-noise data, preserving SMAA data/licenses and native SDK/runtime support.
Move retained ImGui Vulkan/Metal native work and SDL sizing behind GPU bridges
without gpu dependencies on ui/draw/scene. Include the standalone device-selector
OpenGL setup as a remaining native seam. Current generic UI state/preview ownership
is already draw-only and must not be reverted to production shutdown closures.

M09-05 handles remaining obsolete example sources, template/package/resource rules
and explicit probe coverage comparison. M09-06 validates actual products and
R01–R14/P01–P11 matrices. M09-07 records the actual cumulative review; M09-08/M10
finish global production/documentation reconciliation. Historical plan TSVs and
M09-02 delta remain unchanged. The inventory generator now recognizes removals
from both actual deltas before refreshing; it was not rerun to overwrite history.

## Static verification

- Checked all library project-include endpoints after dependent renderer retirement. This includes the old MaterialBase cache
  calls and headers whose removed out-of-line implementations were still consumed
  by other compiled legacy translation units; retaining only their declarations
  would leave unresolved dependencies.
- No shared public declaration outside gpu/render mentions render/scene::vk/native
  Vulkan/Metal/ImGui types; supported source/probes have no retired mode/accessor/
  production Drawable references. BlueNoise's private authoritative data include
  remains intentionally valid until M09-04.
- Compared 273 GPU/base/draw implementation/header files byte-for-byte with the
  step-entry snapshot, excluding the explicitly adapted draw RenderQueueVisitor.
  No shader changes. Compared 40 CPU parser, graph ownership and complete-save
  function bodies modulo canonical accessor renaming/whitespace.
- Checked 109 modified/new C/C++ files for full GPL, project includes and lexical
  delimiters; reviewed namespace/control braces, function separation, method groups
  and required private fixture includes. Two do/while terminators are intentional,
  not unbraced loop bodies. No third-party license rewrite.
- Checked retained manipulation implementation member uses against headers,
  surviving registration endpoints, documented signatures and new harness ownership.
- Checked source delta path/action/hash records, inventory-generator syntax, local
  documentation paths/fences and git diff --check. These are static checks, not a
  C++ parser/compiler, native output or timing result.

## Project lead handoff

Reconfigure/build the surviving products and registered tests using the normal
platform setup. Check both Vulkan and Metal where applicable, including no-RT
raster behavior and explicit failure of required bake profiles without support.
Exercise CPU schema/raw IO, custom constructor failures/clones, root transfer,
chains and reference/complete native-resource saves with checked failure results.
Run UV/bake/selection probes and both editors: replacement pruning, multi-selection,
gizmo transforms, visibility/bounds, material patches/textures/previews, UV2 reload,
baking/reset, environment, settings/Performance and orderly teardown. Recheck
standalone batch output/interrupt/protection and exact camera CLI separately.
Retain backend/scenario evidence accurately; none is invented by this delivery.

M09-03 awaits lead validation. The linked successor is M09-04 only.
