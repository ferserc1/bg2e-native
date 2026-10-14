# Drawable refactoring alternatives and compatibility tradeoffs

[Overview](index.md) · [Scene boundary](03-scene-boundary.md) · [Coexistence](12-coexistence-magnitude-and-decisions.md)

## 1. What the decision must resolve

`scene::Drawable` currently combines CPU mesh data, editable submesh attributes, uploaded production materials, Vulkan buffers, BLAS, native commands and engine identity. The target may keep a narrow geometry/buffer association in scene, but high-level graphics resources must belong to `draw`.

There are several valid migration strategies. The best choice depends on whether short-term production source compatibility or immediate scene cleanliness has priority. No strategy makes incompatible native Vulkan APIs magically compatible with Metal. All names in this document are proposed.

## 2. Option A: external draw resources with a CPU-data bridge

### Shape

Leave the production Drawable implementation unchanged initially. An isolated adapter reads CPU mesh data, submesh material definitions, visibility, local transforms and object identity, and creates independent `draw` geometry/material resources on the experimental device. Experimental passes consume extracted draw items, never `renderMaterial()` or Vulkan drawing callbacks.

The adapter belongs in a compatibility namespace/header, not in core `gpu` and ideally not in core `draw` public headers. Native resources are not shared. The source Drawable can be treated as a transitional data provider even if it happens to be loaded on production.

### Advantages

- Preserves production behavior and existing applications while enabling experimental deferred work.
- Gives new `draw` materials, IBL, bindings and renderer contracts a clean API immediately.
- Avoids rewriting every importer/component factory before the first deferred result.
- Fits existing CPU `.bg2` and OBJ parsing facilities.
- Makes it possible to compare production and experimental results using equivalent scene data.

### Costs and limits

- The existing scene header remains coupled to production during the transitional period. This does not meet the final scene boundary by itself.
- Geometry/material edits require explicit invalidation. Mutable reference accessors currently provide no reliable automatic revision increments.
- Independent device realizations consume additional memory. A bridge is not zero-copy interoperability.
- Custom Drawable subclasses need a data extraction interface or explicit unsupported status.
- glTF/scene convenience loaders still force production upload unless their CPU parsing path is separated. A bridge alone does not solve construction on a machine where only the experimental backend is initialized.

### Appropriate use

Use as a **bootstrap step**, with a deletion condition: once neutral scene loading and Drawable data exist, remove the bridge. Do not let core experimental resources store production objects merely for convenient texture/material access.

## 3. Option B: neutral Drawable data, external GPU realizations

### Shape

Make Drawable a geometry/material-description object. Put mesh upload, materials, BLAS and draw commands in device-scoped resource managers outside scene. A `draw` extractor turns `(asset identity, mesh revision, material revision, world transform, submesh)` into draw items. Production receives its own adapter/realization implementing the old rendering behavior.

### Advantages

- Strongest decoupling: CPU scene operations can function with no device.
- Supports multiple devices/renderers/views without a singleton engine pointer in scene.
- Makes legacy deletion straightforward because neutral scene data has no reverse dependency on rendering.
- Clarifies cloning and import/upload stages.

### Costs

- More intrusive changes to production Drawable methods and custom subclasses.
- Geometry cache lifetime/eviction and edit invalidation become explicit services.
- Existing callers that expect `drawable->renderMesh()` must use production adapters or be migrated.
- An unchanged `scene::Drawable` type name cannot preserve all old methods and simultaneously expose a completely renderer-free header.

### Appropriate use

Choose if the user prefers maximal decoupling and accepts a larger scene/resource redesign. This exceeds the requested minimum but is compatible with it. It should not be made mandatory simply because it is architecturally cleaner.

## 4. Option C: narrow GPU-backed Drawable geometry

### Shape

Keep CPU geometry and a geometry residency association in Drawable. The residency object contains vertex/index buffers, submesh ranges, device identity, layout and revision. It may use `gpu::MeshGeneric` or a neutral type-erased mesh-buffer interface. Loaded texture/material resources, object/frame/environment bindings, queues and effect histories live in `draw`.

A possible conceptual split is:

| Object | Owner | Contents |
|---|---|---|
| Drawable description | `scene` | CPU mesh, submesh metadata/material definitions, visibility, local transforms, traceability intent. |
| Geometry realization | Narrow Drawable association or `draw` cache | Device identity, vertex/index buffers, layout, index ranges, upload revision. |
| Material realization | `draw` | Texture resources, samplers, material values and resource bindings. |
| Tracing geometry | Prefer `draw` cache | BLAS per geometry/submesh and revision. |
| Frame instance | `draw` | World/previous transform, view identity, raster/RT/pick IDs. |

### Advantages

- Directly matches the user's accepted coupling limit.
- Keeps familiar mesh → buffers access useful to procedural/custom drawing.
- Can be simpler than an entirely external geometry cache for a single-device application.
- Avoids forcing high-level material or renderer classes into scene.

### Costs and decisions

- A direct `draw::GeometryResource` member creates a reverse namespace edge. Its header must stay narrow, or use a `gpu`/neutral interface instead.
- Supporting production-native geometry alongside experimental `gpu` geometry requires separate realizations or adapter-backed data.
- One residency pointer does not support simultaneous devices. Either state that one realization is allowed or key a registry by device identity/generation.
- Exposing mutation of GPU buffers invites stale BLAS and resource tables. Define upload/update ownership.
- Placing BLAS alongside buffers is a separate decision; it must remain geometry-only and cannot grow into scene TLAS/effect ownership.

### Appropriate use

This is the recommended **target compromise** if a GPU-backed Drawable is useful to the engine API. Prefer neutral `gpu` buffer/mesh types for the association and `draw` for its high-level management. Treat the production facade as removable compatibility code.

## 5. Option D: parallel legacy and experimental drawable/components

### Shape

Retain the current production Drawable/DrawableComponent and introduce a new neutral or experimental component/data type. The new renderer extracts only the new representation, or supports both through explicit adapters. Production apps migrate scene construction/import a subsystem at a time.

### Advantages

- Clean experimental API with minimal immediate production changes.
- Explicit type boundary instead of a large class with backend branches.
- Easier to abandon or revise experimental design while production stays stable.

### Costs

- Duplicated component/reflection/importer/UI support until convergence.
- Saved component type names require aliases/conversion; registering two incompatible schemas under the same `"Drawable"` name is unsafe.
- Selection/bounding box/mesh editing must handle both types or use a shared neutral interface.
- Shared assets and node cloning become more complex if both components attach to one node.

### Appropriate use

Prefer when compatibility wrappers around the existing class would become more complex than a deliberate split. A parallel component should have a retirement/convergence route. Do not keep two permanent rendering-specific scene systems by accident.

## 6. Comparison

| Criterion | A: bridge | B: external realization | C: narrow GPU association | D: parallel types |
|---|---|---|---|---|
| Immediate production stability | High | Medium/low | Medium | High |
| New draw API cleanliness | High if bridge isolated | High | High if association narrow | High |
| Final scene decoupling | Incomplete | Highest | Meets accepted limit | Depends on convergence |
| Early importer work | Medium | High | High | High for new path |
| Device coexistence | Independent resources | Natural with keyed caches | Requires explicit realization model | Explicit but duplicated |
| Legacy removal | Good only after bridge replacement | Best | Good with facade isolation | Good if schemas converge |
| Risk of broad scene→draw coupling | Low in bootstrap, production debt stays | Low | Medium | Medium |
| Main complexity | Edit synchronization and loading | Identity/cache/adapters | Narrow interface and device lifetime | Duplicate ecosystem support |

Effort labels are relative assessments, not calendar estimates.

## 7. Recommended transition

Start with Option A **only where it enables useful experimental progress**, and evolve toward Option C or B after the geometry/loading contracts are reviewed. In particular:

1. Introduce a neutral CPU data view and explicit scene extraction output. A bridge can implement the view for current Drawables.
2. Keep new high-level resources independent of `render`; create them on the experimental device.
3. Separate CPU parsing from upload so an experimental scene does not require booting production first.
4. Establish mesh/material revisions and stable submesh identity before caching both backends.
5. Choose B or C as the long-term scene contract. Preserve production convenience through a facade or explicit adapter.
6. Remove transitional core production includes and eventually delete the bridge with the legacy framework.

This is an architectural transition recommendation, not an approved implementation sequence. If an additive neutral data object is easy to extract at the start, skip unnecessary bridge stages.

## 8. Why some tempting approaches are weaker

### Just change the template parameter to gpu::Mesh

DrawableGeneric still has native vertex descriptors/commands and a fixed production material callback. Changing `RenderMeshT` alone cannot support Metal or a clean `draw` API.

### Add load(draw::Engine*) to the current class

This can be a compatibility convenience, but the class would then own both material systems, native callbacks, device-state branches and two BLAS families. It exceeds the allowed scene boundary unless all high-level state is moved behind separate adapters and removed from the scene core.

### Put every renderer behind an interface owned by Drawable

An interface does not make frame/material/IBL/effect responsibilities appropriate for scene. Keep the accepted association geometry-only.

### Rename scene::vk to draw::vk and keep the rest

This improves naming but leaves binding signatures, per-frame allocator contracts and native production types intact. It is not a multi-backend port or a sufficient dependency refactoring.

## 9. Required semantics whichever option is chosen

Define device lifetime, mesh/material edit invalidation, clone identity, asynchronous import publication, missing-texture fallback, visibility changes, reload behavior, unsupported vertex layouts, stable pick IDs and RT instance IDs. The raster queue and RT material table must derive from the same consistent scene snapshot even when they filter different subsets.

Compatibility may preserve old calls, but it must never silently draw production Vulkan buffers on a different experimental device or make a weak scene reference prolong the device lifetime unpredictably.
