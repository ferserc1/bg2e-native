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
drawEngine)` reuses CPU geometry parsing and imports supported MTL maps/settings.
`loadDrawableBg2(path, drawEngine)` realizes supported geometry/material resources.
[Complete draw resource IO](DrawResourceIO.md) adds static glTF import, native
GPU-only HDR/mip archives and transactional scene/model publication. It requires
an explicit draw Engine and remains separate from reference-only saving. Raw
preservation is broader than typed component construction.

Stop scene/GPU producers for realization or replacement, retain Engine/device
until resources are released, and use the matching DrawableComponent mode.
See [draw resources](../draw/Resources.md).
