# Scene boundary: what belongs in scene and what should leave

[Overview](index.md) · [Drawable alternatives](04-drawable-alternatives.md) · [Materials](09-materials-environment-and-shaders.md)

## 1. Boundary to preserve

The requested limit is a minimally coupled `scene`, with `Drawable` allowed to relate meshes to their GPU buffers. High-level rendering resources belong in `draw`; `scene::vk` does not belong in the target scene API. Full renderer independence is desirable but not mandatory.

This is a useful compromise: scene objects can retain geometry residency without owning shader bindings, renderer materials, IBL resources, effect histories or frame synchronization. The distinction is **geometry association versus rendering policy**, not merely CPU versus GPU memory.

## 2. Drawable currently has at least five roles

Sources: [`Drawable.hpp`](../../lib/include/bg2e/scene/Drawable.hpp), [`Drawable.cpp`](../../lib/src/bg2e/scene/Drawable.cpp), [`Mesh.hpp`](../../lib/include/bg2e/scene/Mesh.hpp).

### 2.1 Asset and editable object data

Drawable owns a CPU mesh, whole-mesh transform/name, per-submesh material attributes, transforms, names, groups and visibility. It exposes geometry queries, submesh metadata and modifiers. These are legitimate scene/asset concerns. The default geometry is `geo::MeshPNUUT` with position, normal, two UV sets and tangents; alternative vertex-layout typedefs also exist.

The per-submesh CPU material definition can remain in `scene` through `base::MaterialAttributes`. A `draw::MaterialResource` containing uploaded textures and resource sets should not.

### 2.2 Device residency and upload

Drawable stores a `render::Engine*`, a templated production Vulkan render mesh and loaded material resources. `load()` builds mesh, materials and ray tracing meshes; `reload()` rebuilds GPU representations. `isLoaded()` tests the production render mesh pointer. A geometry modifier currently requires the drawable to be loaded and triggers a reload.

In the target, residency should be associated with a device and mesh revision. A single bool cannot describe “loaded in production Vulkan, experimental Vulkan, and Metal.” The accepted buffer association needs either one explicitly selected device realization or a clearly keyed set of realizations. A CPU geometry modifier should not inherently require a renderer; changing that behavior is a deliberate refactoring improvement to review separately.

### 2.3 High-level materials

`materials()`, `renderMaterial()` and `updateMaterials()` expose/manage `render::MaterialBase`, which owns six loaded textures. This exceeds the allowed geometry/buffer relationship. `draw` should realize material definitions and synchronize texture/sampler resources. A draw list can then pair geometry residency with a material resource without making scene own that resource.

### 2.4 Command recording

`DrawableBase::DrawFunction` returns Vulkan descriptor sets from a production material pointer. `draw()` and `drawSubmesh()` accept native command buffers, pipeline layouts and bind points. Static vertex description functions return native Vulkan structures. The template parameter `RenderMeshT` does not make these interfaces backend-neutral because the callback/material/native types remain fixed.

A scene Drawable can expose geometry/submesh ranges. A draw pass should bind the pipeline, vertex/index buffers, frame/object/material resources and record indexed drawing. Keep the old methods in a temporary production facade if compatibility is prioritized.

### 2.5 Acceleration structures

Drawable owns one production BLAS wrapper per submesh. The RT-enable query combines user intent with the currently loaded engine's hardware support. Both choices complicate coexistence: loading on a different device changes capability, and merely disabling a pass need not mean destroying geometry resources.

**Proposed.** Keep user traceability intent as scene data if useful. Put BLAS residency in `draw`'s tracing geometry cache or a narrowly scoped geometry realization alongside buffers; do not put TLAS, material tables, effect settings or tracing passes in scene. Expose requested participation separately from effective support.

## 3. DrawableComponent and graph API

[`DrawableComponent`](../../lib/include/bg2e/scene/DrawableComponent.hpp) owns `DrawableBase`, downcasts to the standard Drawable, forwards native drawing, clones the resource, and serializes via `.bg2` files. The implications are broader than renaming one header:

- `Node::drawable()` and component reflection/editors expect this component and its current type identity.
- Selection records retain weak references to both component and Drawable, plus submesh index.
- Importers return production-loaded Drawables.
- Bounding-box code and mesh writers should keep access to CPU geometry without graphics execution.
- Custom Drawable subclasses and alternate mesh types may implement the old virtual drawing interface.

A new component referencing neutral geometry data avoids GPU callbacks. Changing the existing component directly can break application subclasses and serialized factory behavior. An additive component or facade is safer initially, provided the extractor supports it explicitly and serialization aliases are defined.

## 4. Clone semantics require an explicit decision

**Observed.** Drawable clone duplicates CPU vectors, per-submesh attribute containers, transforms/names and flags; if the source is loaded, it calls `load(_engine)` to build independent production GPU resources. `Component::clone()` documents independence for resource-owning components. Material attribute copies still contain shared pointers to base textures, so the implementation's container/value copying should not be interpreted as deep duplication of every texture source.

**Proposed default:** preserve independent mutable geometry and material definitions on scene clone. Immutable texture source/image data may be shared under a documented contract. GPU material/mesh resources are created for the destination device on demand. If production callers depend on immediate GPU readiness and distinct GPU objects, the legacy clone wrapper must preserve that behavior.

Resource sharing in the new framework is a policy decision, not an invisible optimization. A geometry cache keyed only by asset pointer could accidentally make clones share editable buffers or stale BLAS. Use stable identity plus revisions and define whether clones receive a new identity.

## 5. scene::vk: extract all five binding families

Sources are under [`scene/vk`](../../lib/include/bg2e/scene/vk/).

| Binding | Current responsibilities | Target counterpart |
|---|---|---|
| `FrameDataBinding` | View/projection uniform data, descriptor layouts and per-frame allocation. | `draw` view/frame bindings with explicit data input. |
| `ObjectDataBinding` | Production material packing, model transform, six texture samplers and per-object descriptors. | `draw` object/material bindings; renderer-private packing where appropriate. |
| `EnvironmentDataBinding` | References production EnvironmentResources and IBL samplers/maps. | `draw` environment bindings independent of graph component. |
| `LightDataBinding` | Forward-era light uniform representation. | Retain only in production compatibility; neutral light data reused as needed. |
| `DeferredLightDataBinding` | Deferred light buffer and shader resources. | `draw` deferred light resources generated from a light snapshot. |

All five use the production `PipelineDataBinding`/descriptor infrastructure. Their conceptual responsibilities belong in `draw`, but their implementations cannot simply be moved and reused with `gpu::ResourceSet` unchanged. The low-level creation/update of resource sets belongs in `gpu`; deciding how scene camera/material/light/environment data populates them belongs in `draw`.

Production aliases or forwarding headers may keep `scene::vk` available during transition. The **new implementation** must not live in `scene::vk`; the target is a removable compatibility shell. A source alias is viable only when signatures/types remain compatible. A Vulkan-only binding cannot alias a different experimental class and preserve native descriptors automatically.

## 6. Visitors

| Visitor | Observed coupling | Suggested location/contract |
|---|---|---|
| `DrawVisitor` | Native commands, pipeline and production callback. | Production compatibility; new recording/extraction in `draw`. |
| `RenderQueueVisitor` | Production queue/material/GPU mesh; standard Drawable downcast. | `draw::SceneExtractor` or collector yielding neutral geometry/material data and world transforms. |
| `ResizeViewportVisitor` | Includes production engine and accepts `VkExtent2D`; also has neutral `math::Viewport` overload. | Keep graph viewport notification in scene with neutral extent; native overload compatibility outside core. |
| `UpdateVisitor`, `InputVisitor`, `TransformVisitor` | Graph behavior and transforms. | Keep in scene; renderer may call them through an application adapter. |
| Camera/node find visitors | Graph lookup. | Keep in scene. |
| `vulkan::rt::CollectRayTracingInstancesVisitor` | Traverses graph, filters visibility/transparency, extracts materials and BLAS. | High-level `draw` extraction/trace snapshot; primitive instance upload in `gpu`. |

The current templated RenderQueueVisitor actually downcasts to the standard `scene::Drawable` in its implementation. Explicit instantiations for multiple Drawable types are not evidence that arbitrary layouts reach the current deferred renderer. A new extractor should state supported geometry formats and report unsupported ones rather than silently rely on a downcast.

## 7. Deserialization is the second major scene problem

`Component::deserialize`, `deserializeWithProgress`, `Node::deserialize`, `Scene::deserialize`, and `ComponentFactoryRegistry::Creator` accept `render::Engine&`. Derived camera/light/transform/controller/environment classes inherit that dependency. `DrawableComponent` uses it to import/upload graphics objects. This spreads renderer execution into construction of otherwise neutral scene data.

Two reasonable approaches are:

1. **CPU-first deserialization.** Parse components and resources into scene/asset data; separately realize geometry and materials for a production or experimental device. This gives the cleanest long-term boundary and allows offline inspection.
2. **Neutral loading context.** Supply path resolution, image/mesh loading, progress and optional residency services through a context. Keep synchronous “load ready-to-render scene” convenience functions in adapters. The context must not just hide a `variant<render::Engine*, draw::Engine*>` that every component branches over.

A combined approach is appropriate: a neutral context for parsing and resource resolution, then an explicit upload stage. Keep old deserialization overloads routed through a production adapter while adapting the registry. User-defined component overrides require fallback behavior and clear deprecation; replacing virtual methods in one step is an API/ABI break.

## 8. Other scene objects

`Scene`'s root/camera/environment/light references and graph lifecycle events can stay. It need not own a renderer. `EnvironmentComponent` already mostly describes an environment image path/hash; keep it declarative. `SkyDomeTextureGenerator` generates CPU pixels and depends on base texture/color, so it is misplaced semantically but **not** a Vulkan wrapper. A move to base/procedural texture utilities is optional; a forwarding alias can preserve source use.

`scene::Mesh` can remain a CPU geometry alias. `scene::RenderMesh` should become a legacy alias or disappear; it directly names a Vulkan implementation. `DrawableRegistry` is a name/ownership registry today, not a GPU cache; do not turn it into a device resource manager as part of the migration.

Camera/light/transform descriptions, bounding boxes, chain/controller behavior and reflection schemas should remain scene/neutral data. Avoid broad refactoring of unrelated scene behavior merely because its declaration currently mentions a production deserialization parameter.

## 9. Boundary acceptance checks

The eventual review should verify that:

- A scene can be constructed, inspected, serialized and cloned without initializing a renderer, subject to a deliberately preserved legacy convenience path.
- Scene public headers expose no Vulkan commands/descriptors or loaded renderer materials outside quarantined compatibility headers.
- The accepted Drawable geometry realization references only mesh buffers/device identity and, if approved, geometry acceleration residency.
- Scene traversal returns consistent transforms, visibility, material definitions and submesh identities for raster, selection and RT extraction.
- `scene::vk` has no new implementation; any remaining names serve only production compatibility.
- No `gpu` primitive depends on scene or draw while achieving these goals.

These are proposed migration criteria, not checks claimed to pass in the current repository.
