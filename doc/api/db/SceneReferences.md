# Scene representation and draw resource loading

Headers: `<bg2e/db/scene.hpp>`, `<bg2e/db/mesh_obj.hpp>`,
`<bg2e/db/mesh_bg2.hpp>`, `<bg2e/scene/Component.hpp>`.

`readSceneDocument`/`writeSceneDocument` preserve raw JSON, including unknown
fields/components, without constructing a graph or opening referenced assets.
`loadScene(path)` builds typed CPU attributes and unresolved references;
`saveSceneReferences(root, path)` serializes references without GPU export.

Its signature is `bool saveSceneReferences(scene::Node* root,
const std::filesystem::path& filePath, scene::SceneProgressCallback onProgress = nullptr) noexcept`.
Check false before reporting success. It writes a private staged representation
relative to the final target directory and replaces the document atomically last;
serialization/callback/write failure leaves an existing document unchanged. It
copies no assets. This transactional graph save is separate from the lower-level
raw `writeSceneDocument`, which writes the supplied representation directly.
Unregistered component types warn and are skipped; loading continues with later
components and child nodes. The draw-engine factory also returns null for such
types without attempting resource loading. Typed saves do not preserve skipped
component JSON; use raw document IO for a lossless unknown-data round trip.
Missing schema constructors and malformed/untyped entries also warn/skip.
Registered constructor contract violations, parser and resource failures remain
errors rather than silently loading production resources. Components implement graphics-free `deserialize(data, path)`
and `serializeReferences(path)`; `loadResources(draw::Engine&, progress)` is the
separate realization stage.

Text and font components have explicit recovery boundaries: missing fonts,
malformed text/layout parameters and text backend failures warn/use defaults or
transparent output without aborting the rest of a scene. Graphics-free parsing
constructs all declarations/paragraphs first. Node::loadResources prepares the
owning Scene's complete font registry before any text, regardless of declaration
node order; Composer's incremental loader likewise retains a private resource
Scene. Detached font-bearing subtrees need an owning Scene before realization.
See [registry](../scene/Scene.md#text-font-registry).

Text embeds its paragraph and conventional material as JSON objects. Generated
albedo source references are never read/exported; sampling-only metadata remains.
Nonignored non-albedo procedural images require complete saving rather than empty
filesystem references. Null Text/TextFonts serializers fail the graph save rather
than silently dropping authored data. See [editable resource packaging](DrawResourceIO.md#editable-text-and-font-assets).

`loadScene(path, drawEngine)` realizes supported references synchronously using
that context. Production Engine overloads are removed. `loadDrawableObj(path,
drawEngine)` reuses CPU geometry parsing and imports supported MTL maps/settings.
`loadDrawableBg2(path, drawEngine)` realizes supported geometry/material resources.
[Complete draw resource IO](DrawResourceIO.md) adds static glTF import, standard
image copying, assigned GPU-only mip-zero PNG/HDR export and transactional
scene/model publication. Runtime placeholders are excluded. It requires
an explicit draw Engine and remains separate from reference-only saving. Raw
preservation is broader than typed component construction.

Stop scene/GPU producers for realization or replacement, retain Engine/device
until resources are released, and use DrawableComponent::drawable() for its sole draw resource.
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
See [Composer document IO](../../apps/draw_composer.md#documents-and-imports)
and [manual checks](../../../tests/draw/composer_io/README.md).
