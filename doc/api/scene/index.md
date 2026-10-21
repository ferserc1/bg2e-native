# Scene graph contracts

Headers: `<bg2e/scene/Scene.hpp>`, `<bg2e/scene/Node.hpp>`, `<bg2e/scene/Component.hpp>` and `<bg2e/scene/ComponentFactoryRegistry.hpp>`. Scene parsing is graphics-free; resource realization accepts an explicit draw Engine. Production Engine overloads, scene native Drawables and scene::vk bindings are removed.

`Scene::releaseSceneRoot()` transfers the complete shared root, clears its Scene back-pointer/caches and leaves `rootNode()` null. Consumers may unwrap one synthetic single-root level; preserve multi-root containers. `Node::getComponent<T>()` prefers exact type then compatible subclass. `Node::clone()` rejects null, shared, already-owned or sliced component clones. Mutable draw geometry/material realizations and camera projections must remain independent; custom component/projection subclasses override clone with their concrete type.

Custom schema factories construct detached instances with the registered typeName. Implement graphics-free `deserialize`/`serializeReferences` and separate `loadResources(draw::Engine&, progress)`. Unknown/unregistered types and invalid nonobject entries warn/skip; registered parser/resource errors remain distinct. Typed saves omit skipped data; raw document IO preserves unknown JSON. See [schema/reference IO](../db/SceneReferences.md) and [complete resource IO](../db/DrawResourceIO.md).

`setMainEnvironment(EnvironmentComponent*)` opts into explicit selection; nullptr intentionally selects renderer defaults. `hasExplicitMainEnvironment()` distinguishes this mode, and `resetMainEnvironmentSelection()` restores automatic discovery. Weak component/node ownership and enabled membership checks prevent foreign/detached sources. Root release/replacement clears selection. Renderer extraction, generation and live settings use the same selected source. See [environment semantics](../draw/Environment.md), including Composer's fresh-load/removal policy and nonpersistent active selection.

Applications own dirty state, history/bake invalidation and document/job identities. Queue mutations at stopped boundaries and validate membership when they execute. Graph primitives do not supply Composer's camera restrictions, worker services or batch transactions. See [safe updates](../../safe_update_scene.md) and [Composer](../../apps/draw_composer.md).

DrawableComponent stores only draw::Drawable. `drawable()` returns a shared owner
(const component access returns const Drawable); `setDrawable`/`clearDrawable`
coordinate existing GPU users and reject a cross-engine replacement until cleared.
CPU source references remain separate from resource realization. `clone()` deep
copies the drawable/resources. Node::drawable excludes non-geometry helpers.
There is no execution-mode discriminator or alternate native Drawable accessor.

ComponentFactoryRegistry::registerComponent(name, defaultCreator) registers a
CPU constructor; create(data, path) parses the schema, and the draw Engine overload
then calls loadResources. Registered failures propagate; unknown types warn/skip.
`serializeReferences` and engine-aware complete DB saves replace Node's old implicit
resource-export serialize path. Component::serialize remains a graphics-free common
attribute hook; DrawableComponent's override is reference-only.

DrawableRegistry retains name uniqueness for draw resources. BoundingBox retains
transformed visible geometry filtering using geo::MeshPNUUT. ResizeViewportVisitor
retains generic extent conversion and graphics-free component callbacks, alongside
Find/Update/Input/Transform visitors. Render traversal/bindings live in draw.
