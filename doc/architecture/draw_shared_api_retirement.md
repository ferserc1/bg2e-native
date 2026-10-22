# Shared scene, data and editor APIs after draw canonicalization

The application lifecycle uses draw exclusively. Shared scene, DB, utilities,
manipulation and UI APIs now also accept draw resources only; the remaining
independent render/native implementation is scheduled for separate retirement.
There is no forwarding namespace, old Drawable shim or native scene callback.

## Scene representation and resource ownership

DrawableComponent::drawable() returns its shared draw::Drawable; const component
access returns a const Drawable. setDrawable and clearDrawable coordinate existing
GPU users, and replacing a realized object across Engines requires clearing first.
CPU source references can be parsed without an Engine. loadResources(draw::Engine&)
realizes references explicitly. clone duplicates mutable geometry/material/GPU
owners; Engine/device must outlive every clone and retained owner. Helpers override
isSceneGeometry so Node::drawable excludes editor gizmos.

[Scene](../api/scene/index.md) retains root release, ordered/subclass-compatible
component lookup, rejection of shared/sliced/owned clones, independent camera
projections, chains/joints, explicit active environment and ordinary visitors.
DrawableRegistry retains unique draw-resource names. BoundingBox consumes CPU geo
geometry and includes draw/local submesh transforms and visibility. ResizeViewportVisitor
uses generic extents and graphics-free component callbacks.

ComponentFactoryRegistry::registerComponent(name, defaultCreator) accepts a CPU
constructor. Schema parsing and loadResources are separate; unknown types warn and
skip, while registered constructor/parser/resource failures propagate. Custom
components implement those hooks and return independent clones of their actual type.

## CPU and complete resource IO

[DB representation IO](../api/db/SceneReferences.md) retains raw JSON preservation,
CPU BG2/VWGLB/OBJ/glTF parsing, OBJ/MTL dependency paths and draw realization. Explicit
reference saves are distinct from complete bool-noexcept draw scene/model saves.
Always check false before publishing success or clearing dirty state. Complete saves copy assigned image files by filename and export assigned
GPU-only 2D mip zero as PNG/HDR. Placeholders are excluded and authored mip chains
are not stored. Main-file-last publication and rollback remain supported. Node's old implicit resource-export serialize path
and production specialized DB lightmap writer are removed. Component::serialize
remains a common attribute hook; DrawableComponent::serialize is reference-only.

[bake::LightmapOutputWriter](../api/bake/LightmapOutputWriter.md) retains its separate
whole-batch final-manifest transaction. RGB32F CPU values export as standard
Radiance RGBE HDR with quantization, not exact float-bit preservation. Generic DB
file/image IO remains independent. The root bg2e/all.hpp now exports bake;
explicit bake/all.hpp remains supported.

## Editing, selection and preview lifetime

[Selection](../api/manipulation/reference.md) uses one draw resource domain.
SelectionItem::mesh/lockMesh()/meshPtr() and selectedMesh() validate weak node,
component, drawable, submesh and pick-ID identity. Replacement or topology changes
invalidate stale entries. CPU selection allocates no GPU picking state; picking
requires a stopped live device. PickKind is a neutral enum, separate from the
removed native PickSelectionVisitor.

Gizmo transform math, visibility and input capture remain. transformDrawable()
returns an instance-owned draw realization. cleanupDrawResources and UI preview
unregistration run before Engine/device teardown. OverlayPass owns generic draw
recording; no native Vulkan pipelines remain in the manipulation public API.

Use DrawMaterialEditor/DrawTextureWidgets for queued patches, TexturePreview for
UI-session registrations, and BakeLightmapPreview/UvMapPreviewRenderer for UV/bake
views. SubmeshSelector and DrawableEditor operate on draw geometry. The old native
material/texture/settings/UV widgets are removed. [UserInterface](../api/ui/UserInterface.md)
accepts draw::Engine and prepared generic command/frame pairs; public headers expose
no ImGui or native graphics types. Backend-native implementation confinement remains
the next cleanup step, not a promise already established here.

MaterialModifier retains separate CPU definition/live-attribute patches and texture
revision/loading behavior. TextureCache retains weak Engine/device-validated draw
realizations. ViewpointAnalyzer and camera utilities use utils::CameraCandidate.
Uv2SafeReload::regenerateDraw retains weak delegate/node/renderer generation guards
and publishes an independent replacement at a stopped boundary.

GPUSelectionDialog accepts a caller-supplied vector of gpu::PhysicalDeviceProperties.
It no longer creates an implicit Vulkan instance/surface. It returns one candidate
directly, presents multiple candidates, returns null on cancellation and rejects an
empty list. The UI does not change a running Engine's selected physical device.

## Verification boundaries

Current shader interfaces, alpha/material policy, persistent native arrays, exact
recording completion, acceleration reuse, pools, temporal signatures, transparent
regions and CPU/GPU profiling are preserved. Public API cleanup is not native
numeric, performance or relocated-package evidence. Compilation/runtime validation
is owned by the project lead; the migration report records static checks separately.
