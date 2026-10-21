# Scene representation and draw resource loading

Headers: `<bg2e/db/scene.hpp>`, `<bg2e/db/mesh_obj.hpp>`,
`<bg2e/db/mesh_bg2.hpp>`, `<bg2e/scene/Component.hpp>`.

`readSceneDocument`/`writeSceneDocument` preserve raw JSON, including unknown
fields/components, without constructing a graph or opening referenced assets.
`loadScene(path)` builds typed CPU attributes and unresolved references;
`saveSceneReferences(root, path)` serializes references without GPU export.
Unregistered component types warn and are skipped; loading continues with later
components and child nodes. The draw-engine factory also returns null for such
types without attempting resource loading. Typed saves do not preserve skipped
component JSON; use raw document IO for a lossless unknown-data round trip.
Missing schema constructors and malformed/untyped entries also warn/skip.
Registered constructor contract violations, parser and resource failures remain
errors rather than silently loading production resources. Components implement graphics-free `deserialize(data, path)`
and `serializeReferences(path)`; `loadResources(draw::Engine&, progress)` is the
separate realization stage.

`loadScene(path, drawEngine)` realizes supported references synchronously using
that context. Production Engine overloads remain distinct. `loadDrawableObj(path,
drawEngine)` reuses CPU geometry parsing and imports supported MTL maps/settings.
`loadDrawableBg2(path, drawEngine)` realizes supported geometry/material resources.
[Complete draw resource IO](DrawResourceIO.md) adds static glTF import, native
GPU-only HDR/mip archives and transactional scene/model publication. It requires
an explicit draw Engine and remains separate from reference-only saving. Raw
preservation is broader than typed component construction.

Stop scene/GPU producers for realization or replacement, retain Engine/device
until resources are released, and use the matching DrawableComponent mode.
See [draw resources](../draw/Resources.md).

## Detached scene roots and custom component copies

`scene::Scene::releaseSceneRoot()` returns the entire root as a shared pointer,
clears its Scene back-pointer and the old Scene's camera/environment/light caches,
and leaves `rootNode()` null. Adopt the result into another Scene/container before
using Scene update or camera APIs again. Single-root consumers may detach exactly
one synthetic wrapper level; multiple-root files retain the complete container.

`Node::getComponent<T>()` prefers an exact type, then an ordered compatible
subclass. Schema constructors must produce detached objects with the registered
`typeName`; the draw route still invokes graphics-free deserialize followed by
`loadResources(draw::Engine&)`, never a production creator. `Node::clone()` rejects
null/shared/owned/sliced component copies and preserves common priority/serialization
flags. User subclasses must override inherited slicing clones and implement their
reference/resource hooks.

Camera component clones now independently copy their projections through virtual
`math::Projection::clone()`. Perspective/optical projections implement it; custom
projection subclasses must return their own concrete type. Perspective schema IO
retains FOV/near/far. Unsupported serialized base Camera projection types fail
explicitly; a custom Camera component needs its own parser for custom formats.
See the [Composer IO handoff](../../../docs/render_to_draw/plan/milestone_07_bg2e_composer/scene-io-cloning.md)
and [manual checks](../../../tests/draw/composer_io/README.md).
