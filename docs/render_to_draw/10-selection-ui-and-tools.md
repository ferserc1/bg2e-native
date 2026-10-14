# Selection, UI integration, and graphics tools

[Overview](index.md) · [Dependency map](02-dependency-map.md) · [Offscreen](11-offscreen-and-baking.md)

## 1. Selection is several systems, not one render effect

Sources: [`SelectionManager.hpp`](../../lib/include/bg2e/manipulation/SelectionManager.hpp), [`SelectionManager.cpp`](../../lib/src/bg2e/manipulation/SelectionManager.cpp), [`GizmoComponent.hpp`](../../lib/include/bg2e/manipulation/GizmoComponent.hpp), [`GizmoAndSelectionRenderer.hpp`](../../lib/include/bg2e/manipulation/GizmoAndSelectionRenderer.hpp).

Production SelectionManager combines selection state, input capture/manipulation, picking graphics, synchronous readback and result mapping. SelectionItem stores weak scene node/component/Drawable references plus submesh and pick kind. GizmoComponent derives from DrawableComponent and creates production GPU geometry. RendererDeferred owns the graphics overlay renderer and invokes it after final output.

Suggested split:

| Responsibility | Target owner |
|---|---|
| Selected items/nodes, additive selection, callbacks, clear-on-empty behavior | `manipulation` selection model. |
| Transform interaction and input capture | `manipulation` controller. |
| Pickable descriptions and transform handles | Neutral manipulation/scene data. |
| ID/depth rendering and readback | `draw` picking service. |
| Selection highlight and gizmo drawing | `draw` overlay passes. |
| Gizmo meshes/buffers/pipelines | `draw` graphics-tool resource cache. |
| Mapping result IDs into selection actions | Application/manipulation adapter. |

Keeping state in manipulation avoids a core renderer depending on editor input. Moving only GizmoAndSelectionRenderer is insufficient while GizmoComponent still creates legacy rendering resources.

## 2. Picking behavior to preserve

The production path renders encoded identifiers to an RGBA8 target and reads pixels. It supports drawable/submesh picks, additive selection, selected-node deduplication, node-only selection through the scene tree, transform-handle input capture, and click-versus-drag distinction.

The header still describes restrictions for node-only viewport picking. Do not infer that every light/camera/group is directly pickable just because a visual gizmo exists. The migration should preserve actual behavior first, with broader gizmo picking as an explicit improvement if desired.

The experimental service should accept a consistent extracted snapshot and unjittered camera/view data. It needs to define logical-window versus drawable-pixel coordinates, viewport offsets, DPI scaling, Y orientation, ID encoding and empty results. IDs must map to the same submesh/component generation that was rendered.

Synchronous readback is acceptable for baseline parity. Asynchronous readback is an optional optimization that introduces stale-result handling and cancellation when nodes disappear. Either path must resolve weak references safely and not select a replacement object that reused an old numeric ID.

## 3. Highlight and gizmo pass placement

Production renders overlays after FSR/Direct at final output resolution using unjittered matrices. It clears a dedicated depth target so the transform gizmo self-occludes and stays above scene content. Type gizmos and transform gizmos have distinct depth/order behavior, and the transform gizmo is flushed last.

Preserve this placement rather than scaling/jittering editor overlays through the main geometry pipeline. The new overlay pass requires generic depth/blend state and possibly a scissored depth clear or separate target. These are GPU primitives; colors, line intensity, screen-size scaling and handle priority are draw/manipulation policy.

Offscreen production omits editor overlays. The target should keep them opt-in so the same deferred renderer produces clean offscreen images without an editor.

## 4. Existing UI execution seam

[`UserInterface`](../../lib/include/bg2e/ui/UserInterface.hpp) already has production and experimental initialization/draw overloads. Private ImGui backends support Vulkan and Metal; [`DrawGraphicsExecution`](../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp) installs UI preparation/composition callbacks around the experimental retained scene target.

This avoids rebuilding the general UI lifecycle. It does not port every widget:

| Widget/family | Remaining coupling | Suggested treatment |
|---|---|---|
| MaterialEditor | Receives production MaterialBase. | Edit base attributes via neutral model; resource update notification belongs to draw adapter. |
| TextureWidgets | Production Texture/MaterialBase and backend image handling. | Neutral texture-preview handle with private Vulkan/Metal registration/lifetime. |
| RenderSettingsWindow | Concrete production RendererDeferred/preferences. | Renderer settings/capability view model plus persistence adapter. |
| UvMapPreview | Owns/uses production UV preview renderer/texture. | Consume draw tool output through neutral preview API. |
| Scene component reflection/editors | Drawable/material/component type assumptions. | Neutral CPU metadata and additive support for chosen Drawable transition. |

No public bg2e header should expose ImGui types/functions. TextureWidgets currently exposes a Vulkan descriptor handle; its experimental replacement should expose a neutral preview handle while private UI backends perform native registration. The existing opaque texture identifier in FileHistoryWidget is a useful boundary precedent. UI texture registrations must be retired after frames stop referencing them, even if the underlying scene material changed earlier.

## 5. Settings persistence

Production RenderSettingsPreferences directly mutates RendererDeferred and persists dynamic final-mode option indices. With FSR available, index 0 corresponds to NativeAA; without FSR the available list becomes Direct-only. This index is not a stable cross-device identity.

The new API should expose grouped setting values and feature status. UI chooses labels/options; persistence uses stable mode/quality identifiers. Preserve old saved preferences with a versioned conversion using the legacy list/capability context where known. Do not reinterpret old numeric indices as a newly ordered enum.

Capability fallback should alter effective runtime settings while keeping requested preferences recoverable. The existing renderer retains an initialized FSR processor while Direct is active and does not permanently clear FSR availability on a failed runtime switch; preserve the intent of deterministic mode switching and useful UI choices.

## 6. UV map preview

[`UvMapPreviewRenderer`](../../lib/include/bg2e/render/UvMapPreviewRenderer.hpp) consumes a CPU mesh, renders selected UV0/UV1 triangle fill/wireframe and atlas boundaries into an internal square image, and exposes it as a sampled production texture. It is intentionally independent of `ui` and uses synchronous rendering.

A `draw` UV preview service fits the same model: CPU geometry + generic graphics resources + image output. Preserve resolution changes, UV selection, colors and nonmutation of the mesh. The UI widget wraps the output; GPU knows only line/triangle pipelines, attachments and buffers.

## 7. Viewpoint analysis

[`ViewpointAnalyzer`](../../lib/include/bg2e/render/ViewpointAnalyzer.hpp) generates camera candidates, renders per-submesh IDs/depth at low resolution, batches commands and readback, computes coverage/border/depth statistics, and scores candidates. It is used through `utils::SceneCameraUtils` and applications.

Suggested split:

- GPU visibility/ID/depth measurement and readback in a standalone `draw` service.
- Candidate generation and scores as neutral math/data policy, with customization retained.
- Camera application and application-specific framing presets in utilities/apps.

Preserve current range sampling, cyclic endpoints, bounds fitting, per-submesh statistics, result mapping and overridden scoring behavior. A scene-wide deferred frame is unnecessary for this specialized measurement. Reuse the picking geometry/ID infrastructure where its behavior matches, but depth/statistical batch readback has additional requirements beyond single-click RGBA8 picking.

## 8. Integration risks

The biggest tool risk is identity drift: raster extraction, RT tables, picking and UI can each traverse/filter differently. Use stable scene/submesh identities and explicit maps for each pass. Temporary runtime gizmos should not enter serialization or normal RT lighting tables.

The second risk is resource lifetime: UI may retain a texture registration after a material reload or preview resize. Device-scoped handles and completion-aware retirement must include UI consumers, not only renderer resource sets.

The third is broad framework coupling: if RendererDeferred directly owns SelectionManager, scene-tree windows and preferences, a headless renderer and future renderer subtype become unnecessarily dependent on editor policy. Default delegates/adapters can preserve convenience without that dependency.

## 9. Future acceptance scenarios

Validate empty/additive selection, submesh identity, deleted nodes, DPI/viewport coordinates, scene-tree selection, click/drag behavior, transform-handle capture, overlay order and pause/resume. Check previews after reload/resize, settings across FSR-capable/incapable devices, and viewpoint results on equivalent geometry. These scenarios are not claimed to have been executed.
