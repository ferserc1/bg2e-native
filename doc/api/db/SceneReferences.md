# Scene representation and draw resource loading

Headers: `<bg2e/db/scene.hpp>`, `<bg2e/db/mesh_obj.hpp>`,
`<bg2e/db/mesh_bg2.hpp>`, `<bg2e/scene/Component.hpp>`.

`readSceneDocument`/`writeSceneDocument` preserve raw JSON, including unknown
fields/components, without constructing a graph or opening referenced assets.
`loadScene(path)` builds typed CPU attributes and unresolved references;
`saveSceneReferences(root, path)` serializes references without GPU export.
Unsupported component schema hooks fail explicitly rather than silently loading
production resources. Components implement graphics-free `deserialize(data, path)`
and `serializeReferences(path)`; `loadResources(draw::Engine&, progress)` is the
separate realization stage.

`loadScene(path, drawEngine)` realizes supported references synchronously using
that context. Production Engine overloads remain distinct. `loadDrawableObj(path,
drawEngine)` reuses CPU geometry parsing but does not load MTL resources.
`loadDrawableBg2(path, drawEngine)` realizes supported geometry/material resources.
Complete glTF/resource-save/GPU-export/editor import workflows have not migrated
to draw. Raw preservation is broader than typed component construction.

Stop scene/GPU producers for realization or replacement, retain Engine/device
until resources are released, and use the matching DrawableComponent mode.
See [draw resources](../draw/Resources.md).
