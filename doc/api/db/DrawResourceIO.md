# Complete draw resource IO

Include `<bg2e/db/scene.hpp>`, `<bg2e/db/scene_bg2.hpp>`,
`<bg2e/db/scene_gltf.hpp>` and `<bg2e/db/mesh_obj.hpp>`, or `<bg2e/db/all.hpp>`.
These explicit `draw::Engine&` operations are synchronous. Stop frame recording
and background submissions before calling them, and keep the Engine alive until
all loaded objects and textures have been released. The editor executes ordinary
IO in `prepareFrame`, before frame acquisition; its close callback also runs
between frames and drains the device before saving.

## Loading

- `loadSceneBg2(path, engine, progress)` returns a model Node with its draw
  drawable, identity Transform, input/output chain joints and resolved source.
- `loadDrawableObj(path, engine, onTextureLoaded)` returns a realized draw
  drawable. The common tinyobj geometry reader triangulates faces, generates
  absent normals/tangents and groups consecutive faces by shape and material.
  MTL files resolve relative to the OBJ; texture references resolve relative to
  each MTL. Diffuse/alpha, metalness, roughness (or an Ns approximation), normal,
  ambient and emission maps populate the engine's six material slots. Names,
  groups, scales and clamp addressing are retained. Separate opacity maps,
  height-to-normal conversion, texture offsets and turbulence require conversion
  before import and raise an error. A missing MTL raises an error.
- `loadGltf(path, engine, progress)` returns an owning shared Node. The CPU
  cgltf parser is shared with readGltf. Realization imports the selected
  scene's static mesh hierarchy, local matrices, two UV sets, PBR color and
  metallic/roughness maps, normal/AO/emission maps, alpha mode and sampler settings.
  Embedded images use the importer's temporary image cache. Materials follow the
  engine's scalar emission model, rather than a separate RGB emission color.
  The draw importer rejects skins/animations, morph targets, cameras/lights,
  non-triangle primitives, unrepresentable material extensions, colored emission
  factors, normal/AO strengths and UV offset/rotation. Indexed positions, normals
  and UV0 are required by the shared parser. Convert unsupported features before
  import; this is a static model importer, not a general glTF transcoder.
- `loadScene(path, engine, progress)` builds the registered typed graph and calls
  its virtual draw `loadResources`. BG2/VWGLB and OBJ Drawable references are
  realized with that context. Import a glTF hierarchy through `loadGltf` first,
  then save it as a scene. A hierarchy cannot be represented by one Drawable
  resource reference. Resource realization requires the initialized draw Engine.

Callbacks report synchronous parsing/resource work. OBJ texture counts are
estimated during scene realization and may increase as resources are discovered.
Schema-only and raw IO remain [separate operations](SceneReferences.md): neither
requires a device nor exports any referenced image.

## Saving a complete resource generation

```cpp
if (!bg2e::db::saveScene(root.get(), engine, outputDirectory / "model.json"))
{
    throw std::runtime_error("Complete scene export failed");
}
if (!bg2e::db::storeDrawableBg2(outputDirectory / "model.bg2", engine, modelNode.get()))
{
    throw std::runtime_error("Complete model export failed");
}
```

These engine-aware save functions return bool and are noexcept: false reports a
logged failure. Check the result before declaring document/batch success; a
try/catch alone cannot detect it. The example fragment assumes live root/engine/
modelNode and includes `<stdexcept>`. Destination parents are created by export.
Export requires realized drawables owned by the supplied Engine and no unresolved
pending texture updates. It snapshots live material attributes. Only explicitly
assigned textures are persisted: missing slots remain absent and the renderer
recreates its neutral placeholders. Filesystem images retain their filenames and
are copied once per filename. An already consolidated filename is reused; distinct
sources with identical names are not distinguished by this contract.
Procedural and GPU-only assigned images use synchronous readback to standard PNG
(RGBA8) or Radiance HDR (RGBA32F). Only mip zero of 2D images is exported;
other GPU formats and cubemaps fail explicitly. Authored mipmaps are not preserved.

Sampler settings, color interpretation, UV sets/scales/channels/inversion and
`IndirectTextureMode` travel in material JSON. The AO slot retains linear RGB
bakedGI/bakedAO semantics. BG2 stores independent submesh names, material names,
groups and visibility; repeated named material definitions retain separate edits.
Drawable transforms are baked into exported geometry, while scene Node transforms
remain in the graph. Tangents are regenerated on BG2 reload. Input/output joint
values accompany model resources and scene components.

Scene export calls each component's virtual `serializeReferences`, preserving
custom fields implemented by that hook. Registered schema creators and draw
`loadResources` remain authoritative when loading. Unsupported serializers fail the complete save (logged false);
raw document IO is the route for preserving unregistered representations. Custom
components with external resource types must provide their own resource export
contract. Environment source images are copied without re-encoding. Runtime
editor fixtures should set `ignoreSerialization`; ignored Drawables are not
exported. Temporary Drawable paths are exchanged and restored without allocation,
including on a serialization failure. Live texture settings are not modified.

Ignored runtime editor helpers do not block single-model export. BG2 alone
accepts one enabled, non-steady model node with an identity Node
Transform and only Drawable/Transform/chain-joint components. Use a scene document
for hierarchies, non-identity Node transforms, node flags or custom components.
The standalone model's display name follows its output filename; scenes preserve
Node and drawable display names separately from generated resource filenames.

A unique staging directory is created beside the destination. Generated model resources and images materialized from GPU-only sources receive
unique generation names. Filesystem images keep their filenames, and matching
filenames already in the destination are reused without replacement. Distinct
sources sharing a filename are not distinguished; texture-library collision
handling is outside this contract. New resources are linked into the destination
without replacement. The main model/scene file is replaced last using an atomic filesystem
rename (MoveFileEx on Windows). Failure removes this transaction's new sidecars
and staging directory, preserving an existing main file and its resources. The
filesystem must support same-filesystem atomic replacement. If native hardlink
publication is unavailable, exclusive resource reservation plus copy is used;
unique resources still cannot clobber previous generations. This is publication
atomicity, not a power-loss durability guarantee. Old generations
remain on disk: export deliberately does not delete possibly shared resources.
Keep the main file and its generated sidecars together when moving a document.

Standard images are decoded by the ordinary image loader. Proprietary texture
archives are no longer supported; existing archives require external conversion.

The [manual resource probe](../../../tests/draw/resource_io/README.md) checks
GPU-only HDR mip-zero export, placeholder exclusion, filename reuse, metadata
round-trip and rollback. Compilation, backend execution
and editor interaction validation are project-lead tasks; no execution evidence
is implied by these source contracts.

## Model editor bake integration

The draw model editor automatically applies completed bake output from a temporary
filesystem texture (linear RGB8 PNG by default, standard Radiance HDR for float
output). Temporary bake images have unique filenames. Complete save copies those
source files into the destination, preserving the
linear UV2 baked-AO/GI material metadata. A saved model does not depend on the
original temporary path. Reference-only serialization is not a replacement for
this resource materialization. See the [editor workflow](../../tutorials/draw/model_edit.md).

## Preparing imports without a graphics context

`readGltf(path)` returns an owning `GltfDocument` containing the parsed file and
loaded CPU buffers. `loadGltf(document, drawEngine, progress)` realizes the static
scene using that snapshot. Its private opaque parser storage keeps cgltf types
out of public headers. Image decoding, material/vertex conversion and resource
realization still run during the stopped high-level operation.

`readObj(path)` returns an `ObjMesh` with CPU mesh, MTL attributes, submesh names
and groups. `loadDrawableObj(model, drawEngine, callback)` copies its editable mesh
and texture descriptions before upload. `loadMeshBg2(path)` remains the existing
CPU reader; `loadSceneBg2(path, const Bg2Mesh&, drawEngine, progress)` realizes that
model, including chain joints. Existing path-taking draw overloads delegate to
these parser/realization boundaries; all realization overloads use draw.

These CPU readers can run on an owned worker with independent inputs. They do
not touch a live scene, graph factories or GPU objects. Consume their completed
results after joining/synchronizing the worker, using a stopped initialized draw
context. Prefab workers can use `readSceneDocument`; graph deserialization and
`Node::loadResources` stay with the application main thread. Texture and prefab
resource paths are resolved during realization, so external files must remain
available. Applications own job cancellation, document/destination validity,
publication rollback and renderer/bake/selection invalidation; parser results are
not a background GPU-loading or immutable filesystem snapshot API.

## Standalone batch boundary

The [bake writer](../bake/LightmapOutputWriter.md) reuses complete bool-checked
model snapshots inside private per-target directories, then publishes a complete
batch manifest last. Complete single-document DB publication by itself does not
supply that source-protection/overwrite/multi-target transaction. Generic image/file publication is documented in [FileIO](FileIO.md).
